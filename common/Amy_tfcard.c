#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "Amy_tfcard.h"
#include "Amy_log.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mount.h>
#include <sys/statvfs.h>
#include <sys/sysmacros.h>

#define DEV_PATH "/dev"
#define TFCARD_DEV "mmcblk0"
#define TFCARD_PART "mmcblk0p1"
#define TFCARD_POLL_MS 100
#define TFCARD_RETRY_TICKS 10
#define TFCARD_MEDIA_PATH TFCARD_MOUNT_PATH "/media/"
typedef struct tfcared_event
{
    unsigned int event;
    dev_t card_device;
    ino_t card_inode;
} tfcared_event;

typedef struct tfcard_handle
{
    int run;
    Amy_thread_t tid;
    Amy_mutex_t mutex;
    int status;

    mp_pool *mp;
    queues_conetext *q;

    unsigned long total_kbbytes;
    unsigned long free_kbbytes;
    unsigned long used_kbbytes;

    int inserted;
    /** 当前挂载使用的分区节点名，如 mmcblk0p2（多分区卡兼容） */
    char active_part[40];
    dev_t mount_device;
    dev_t card_device;
    ino_t card_inode;
    unsigned int generation;
    unsigned int event_sequence;
    unsigned int pending_event;
    int event_queued;
    int cleanup_pending;
    unsigned int io_users;
    int retry_ticks;
    int refresh_capacity;
    int format_stage; /* 0: none, 1: cleanup/format, 2: verify new mount */
    dev_t format_device;
    ino_t format_inode;
    int format_requested;
    int format_event_queued;
    int format_result_pending;
    int format_result;
} tfcard_handle;

static tfcard_handle tfcard_context;
static tf_card_status_e tfcard_status_update(tfcard_handle *ctx);
static int tfcared_format(tfcard_handle *ctx);

static void tfcard_event_retry(tfcard_handle *ctx)
{
    unsigned int sequence = 0;
    Amy_mutex_lock(&ctx->mutex);
    if (ctx->pending_event && !ctx->event_queued)
    {
        sequence = ctx->pending_event;
        ctx->event_queued = 1;
    }
    Amy_mutex_unlock(&ctx->mutex);
    if (sequence && Amy_tfcard_state_event_put(sequence) != 0)
    {
        Amy_mutex_lock(&ctx->mutex);
        if (ctx->pending_event == sequence)
            ctx->event_queued = 0;
        Amy_mutex_unlock(&ctx->mutex);
    }
}


typedef struct
{
    int found;
    time_t oldest_mtime;
    char oldest_path[PATH_MAX];
} tfcard_oldest_file_ctx_t;

static tfcard_oldest_file_ctx_t g_oldest_file_ctx;

static int tfcard_status_change(tfcard_handle *ctx, tf_card_status_e new_status)
{
    Amy_mutex_lock(&ctx->mutex);
    ctx->status = new_status;
    if (++ctx->event_sequence == 0)
        ++ctx->event_sequence;
    ctx->pending_event = ctx->event_sequence;
    ctx->event_queued = 0;
    Amy_mutex_unlock(&ctx->mutex);
    tfcard_event_retry(ctx);
    return 0;
}

static bool tfcard_is_media_file(const char *path)
{
    if (path == NULL)
    {
        return false;
    }
    const char *dot = strrchr(path, '.');
    if (dot == NULL)
    {
        return false;
    }
    return (strcasecmp(dot, ".jpg") == 0 ||
            strcasecmp(dot, ".jpeg") == 0 ||
            strcasecmp(dot, ".png") == 0 ||
            strcasecmp(dot, ".bmp") == 0 ||
            strcasecmp(dot, ".avi") == 0 ||
            strcasecmp(dot, ".mp4") == 0 ||
            strcasecmp(dot, ".h264") == 0 ||
            strcasecmp(dot, ".aac") == 0 ||
            strcasecmp(dot, ".wav") == 0);
}

static int tfcard_scan_oldest_media_file(const char *dir_path)
{
    DIR *dir = opendir(dir_path);
    if (dir == NULL)
    {
        return -1;
    }

    struct dirent *ent = NULL;
    while ((ent = readdir(dir)) != NULL)
    {
        if ((strcmp(ent->d_name, ".") == 0) || (strcmp(ent->d_name, "..") == 0))
        {
            continue;
        }

        char full_path[PATH_MAX] = {0};
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, ent->d_name);

        struct stat st;
        if (stat(full_path, &st) != 0)
        {
            continue;
        }

        if (S_ISDIR(st.st_mode))
        {
            (void)tfcard_scan_oldest_media_file(full_path);
            continue;
        }

        if (!S_ISREG(st.st_mode))
        {
            continue;
        }

        if (!tfcard_is_media_file(full_path))
        {
            continue;
        }

        if (!g_oldest_file_ctx.found || st.st_mtime < g_oldest_file_ctx.oldest_mtime)
        {
            g_oldest_file_ctx.found = 1;
            g_oldest_file_ctx.oldest_mtime = st.st_mtime;
            strncpy(g_oldest_file_ctx.oldest_path, full_path, sizeof(g_oldest_file_ctx.oldest_path) - 1);
            g_oldest_file_ctx.oldest_path[sizeof(g_oldest_file_ctx.oldest_path) - 1] = '\0';
        }
    }

    closedir(dir);
    return 0;
}

static int tfcard_delete_oldest_media_file_once(void)
{
    memset(&g_oldest_file_ctx, 0, sizeof(g_oldest_file_ctx));
    if (access(TFCARD_MEDIA_PATH, F_OK) != 0)
    {
        return -1;
    }
    (void)tfcard_scan_oldest_media_file(TFCARD_MEDIA_PATH);
    if (!g_oldest_file_ctx.found)
    {
        return -1;
    }
    AMY_LOG_WARNING("tfcard low space, delete oldest file: %s", g_oldest_file_ctx.oldest_path);
    if (unlink(g_oldest_file_ctx.oldest_path) != 0)
    {
        AMY_LOG_ERROR("unlink oldest file failed(%s), errno=%d", g_oldest_file_ctx.oldest_path, errno);
        return -1;
    }
    return 0;
}


/* Only the worker mounts/unmounts. UI notifications and in-flight writers must
 * finish before an old mount is removed or a replacement card is accepted. */
typedef struct
{
    dev_t device;
    int read_only;
    int owned_fat;
} tfcard_mount_info;

static const char *const tfcard_parts[] = {
    TFCARD_PART, "mmcblk0p2", TFCARD_DEV
};

static int tfcard_node_stat(const char *part, struct stat *node, ino_t *instance)
{
    char path[96];
    struct stat sysfs;
    snprintf(path, sizeof(path), DEV_PATH "/%s", part);
    if (stat(path, node) != 0 || !S_ISBLK(node->st_mode))
        return -1;
    snprintf(path, sizeof(path), "/sys/dev/block/%u:%u",
             major(node->st_rdev), minor(node->st_rdev));
    if (stat(path, &sysfs) != 0)
        return -1;
    if (instance)
        *instance = sysfs.st_ino;
    return 0;
}

static int tfcard_source_owned(const char *source)
{
    char path[64];
    for (unsigned int i = 0; i < sizeof(tfcard_parts) / sizeof(tfcard_parts[0]); i++)
    {
        snprintf(path, sizeof(path), DEV_PATH "/%s", tfcard_parts[i]);
        if (strcmp(source, path) == 0)
            return 1;
    }
    return 0;
}

static int tfcard_option_ro(const char *options)
{
    return strncmp(options, "ro", 2) == 0 &&
           (options[2] == ',' || options[2] == '\0');
}

/* Return 1 for the visible mount, 0 for no mount, -1 for an unreadable or
 * inconsistent mount table. Inspecting proc metadata never reads the card. */
static int tfcard_mount_read(tfcard_mount_info *info)
{
    FILE *fp = fopen("/proc/self/mountinfo", "r");
    if (!fp)
        return -1;
    char line[1024];
    int found = 0;
    memset(info, 0, sizeof(*info));
    while (fgets(line, sizeof(line), fp))
    {
        unsigned int maj, min;
        char root[256], target[256], options[128];
        char type[32], source[128], super_options[128];
        if (!strchr(line, '\n') && !feof(fp))
        {
            found = -1;
            break;
        }
        if (sscanf(line, "%*u %*u %u:%u %255s %255s %127s",
                   &maj, &min, root, target, options) != 5)
            continue;
        if (strcmp(target, TFCARD_MOUNT_PATH) != 0)
            continue;
        char *separator = strstr(line, " - ");
        if (!separator || sscanf(separator + 3, "%31s %127s %127s",
                                  type, source, super_options) != 3)
        {
            found = -1;
            break;
        }
        /* Later entries cover earlier mounts at the same path. */
        info->device = makedev(maj, min);
        info->read_only = tfcard_option_ro(options) || tfcard_option_ro(super_options);
        info->owned_fat = strcmp(root, "/") == 0 && strcmp(type, "vfat") == 0 &&
                          tfcard_source_owned(source);
        found = 1;
    }
    if (ferror(fp))
        found = -1;
    fclose(fp);
    if (found == 1)
    {
        struct stat visible;
        if (stat(TFCARD_MOUNT_PATH, &visible) != 0 || visible.st_dev != info->device)
            return -1;
    }
    return found;
}

static int tfcard_matching_part(const tfcard_mount_info *info, char *part, size_t size)
{
    struct stat node;
    for (unsigned int i = 0; i < sizeof(tfcard_parts) / sizeof(tfcard_parts[0]); i++)
    {
        if (tfcard_node_stat(tfcard_parts[i], &node, NULL) == 0 && node.st_rdev == info->device)
        {
            snprintf(part, size, "%s", tfcard_parts[i]);
            return 1;
        }
    }
    return 0;
}

static int tfcard_capacity_read(dev_t device, unsigned long *total,
                                unsigned long *used, unsigned long *available)
{
    struct statvfs fs;
    tfcard_mount_info info;
    if (tfcard_mount_read(&info) != 1 || !info.owned_fat || info.read_only ||
        info.device != device || statvfs(TFCARD_MOUNT_PATH, &fs) != 0 ||
        (fs.f_flag & ST_RDONLY))
        return -1;
    unsigned long long unit = fs.f_frsize ? fs.f_frsize : fs.f_bsize;
    unsigned long long total_kb = (unsigned long long)fs.f_blocks * unit / 1024;
    unsigned long long free_kb = (unsigned long long)fs.f_bavail * unit / 1024;
    if (!total_kb || total_kb > ULONG_MAX || free_kb > total_kb)
        return -1;
    *total = (unsigned long)total_kb;
    *available = (unsigned long)free_kb;
    *used = *total - *available;
    return 0;
}

int tfcard_capacity_get(unsigned long *total_kb, unsigned long *used_kb, unsigned long *free_kb)
{
    if (!total_kb || !used_kb)
        return -1;
    *total_kb = *used_kb = 0;
    if (free_kb)
        *free_kb = 0;
    tfcard_handle *ctx = &tfcard_context;
    Amy_mutex_lock(&ctx->mutex);
    dev_t device = ctx->mount_device;
    unsigned int generation = ctx->generation;
    int ready = ctx->status == SD_STATE_MOUNT || ctx->status == SD_STATE_FULL;
    if (ready)
        ctx->refresh_capacity = 1;
    Amy_mutex_unlock(&ctx->mutex);
    unsigned long total, used, available;
    if (!ready || tfcard_capacity_read(device, &total, &used, &available) != 0)
        return -1;
    Amy_mutex_lock(&ctx->mutex);
    if (ctx->generation != generation || ctx->mount_device != device || ctx->cleanup_pending)
    {
        Amy_mutex_unlock(&ctx->mutex);
        return -1;
    }
    ctx->total_kbbytes = *total_kb = total;
    ctx->used_kbbytes = *used_kb = used;
    ctx->free_kbbytes = available;
    ctx->refresh_capacity = 1;
    if (free_kb)
        *free_kb = available;
    Amy_mutex_unlock(&ctx->mutex);
    return 0;
}

static tf_card_status_e tfcard_status_update(tfcard_handle *ctx)
{
    unsigned long total, used, available;
    if (tfcard_capacity_read(ctx->mount_device, &total, &used, &available) != 0)
        return SD_STATE_ERROR;
    Amy_mutex_lock(&ctx->mutex);
    ctx->total_kbbytes = total;
    ctx->used_kbbytes = used;
    ctx->free_kbbytes = available;
    ctx->refresh_capacity = 0;
    Amy_mutex_unlock(&ctx->mutex);
    return available < 1048576 ? SD_STATE_FULL : SD_STATE_MOUNT;
}

static void tfcard_begin_cleanup(tfcard_handle *ctx)
{
    Amy_mutex_lock(&ctx->mutex);
    if (ctx->cleanup_pending)
    {
        Amy_mutex_unlock(&ctx->mutex);
        return;
    }
    ctx->cleanup_pending = 1;
    ctx->mount_device = 0;
    ctx->card_device = 0;
    ctx->card_inode = 0;
    if (++ctx->generation == 0)
        ++ctx->generation;
    ctx->free_kbbytes = ctx->used_kbbytes = ctx->total_kbbytes = 0;
    Amy_mutex_unlock(&ctx->mutex);
    tfcard_status_change(ctx, SD_STATE_UNPLUG);
}

static int tfcard_cleanup_ready(tfcard_handle *ctx)
{
    Amy_mutex_lock(&ctx->mutex);
    int ready = ctx->pending_event == 0 && ctx->io_users == 0;
    Amy_mutex_unlock(&ctx->mutex);
    return ready;
}

static void tfcard_format_complete(tfcard_handle *ctx, int result)
{
    Amy_mutex_lock(&ctx->mutex);
    ctx->format_stage = 0;
    Amy_mutex_unlock(&ctx->mutex);
    ctx->format_result = result;
    ctx->format_result_pending = 1;
}

static int tfcard_make_directories(void)
{
    static const char *const dirs[] = {"media", "music", "picture"};
    char path[64];
    for (unsigned int i = 0; i < sizeof(dirs) / sizeof(dirs[0]); i++)
    {
        struct stat st;
        snprintf(path, sizeof(path), TFCARD_MOUNT_PATH "/%s", dirs[i]);
        if (stat(path, &st) == 0)
        {
            if (!S_ISDIR(st.st_mode))
                return -1;
        }
        else if (errno != ENOENT || mkdir(path, 0777) != 0)
            return -1;
    }
    return 0;
}

static int tfcard_dev_mount(tfcard_handle *ctx)
{
    struct stat node;
    if (stat(TFCARD_MOUNT_PATH, &node) != 0)
    {
        if (errno != ENOENT || mkdir(TFCARD_MOUNT_PATH, 0755) != 0)
            return -1;
    }
    else if (!S_ISDIR(node.st_mode))
        return -1;
    for (unsigned int i = 0; i < sizeof(tfcard_parts) / sizeof(tfcard_parts[0]); i++)
    {
        if (tfcard_node_stat(tfcard_parts[i], &node, NULL) != 0)
            continue;
        char source[64];
        snprintf(source, sizeof(source), DEV_PATH "/%s", tfcard_parts[i]);
        if (mount(source, TFCARD_MOUNT_PATH, "vfat", 0, NULL) == 0)
        {
            snprintf(ctx->active_part, sizeof(ctx->active_part), "%s", tfcard_parts[i]);
            return 0; /* The next probe verifies the resulting mount. */
        }
    }
    return -1;
}

static void tfcard_detection(tfcard_handle *ctx)
{
    tfcard_event_retry(ctx);
    if (ctx->format_result_pending)
    {
        Amy_mutex_lock(&ctx->mutex);
        ctx->format_event_queued = 1;
        Amy_mutex_unlock(&ctx->mutex);
        if (Amy_tfcard_format_result_event_put(ctx->format_result) == 0)
            ctx->format_result_pending = 0;
        else
        {
            Amy_mutex_lock(&ctx->mutex);
            ctx->format_event_queued = 0;
            Amy_mutex_unlock(&ctx->mutex);
        }
    }
    if (ctx->retry_ticks > 0)
    {
        ctx->retry_ticks--;
        return;
    }

    struct stat card;
    ino_t card_inode = 0;
    int present = tfcard_node_stat(TFCARD_DEV, &card, &card_inode) == 0;
    Amy_mutex_lock(&ctx->mutex);
    ctx->inserted = present;
    int status = ctx->status;
    int refresh = ctx->refresh_capacity;
    dev_t accepted_device = ctx->mount_device;
    Amy_mutex_unlock(&ctx->mutex);

    if (ctx->format_stage && (!present || card.st_rdev != ctx->format_device ||
                              card_inode != ctx->format_inode))
        tfcard_format_complete(ctx, -1);

    tfcard_mount_info info;
    int found = tfcard_mount_read(&info);
    if (found < 0)
    {
        tfcard_begin_cleanup(ctx);
        ctx->retry_ticks = TFCARD_RETRY_TICKS;
        return;
    }
    char part[40];
    int current = found == 1 && present && tfcard_matching_part(&info, part, sizeof(part));
    int ready = status == SD_STATE_MOUNT || status == SD_STATE_FULL;
    if (found == 1 && current && info.owned_fat && !info.read_only &&
        !ctx->cleanup_pending && ctx->format_stage != 1 &&
        (!ctx->card_inode || (ctx->card_device == card.st_rdev && ctx->card_inode == card_inode)) &&
        (!accepted_device || accepted_device == info.device))
    {
        if (!ready || refresh || !accepted_device)
        {
            Amy_mutex_lock(&ctx->mutex);
            ctx->mount_device = info.device;
            ctx->card_device = card.st_rdev;
            ctx->card_inode = card_inode;
            if (!ctx->generation)
                ctx->generation = 1;
            Amy_mutex_unlock(&ctx->mutex);
            snprintf(ctx->active_part, sizeof(ctx->active_part), "%s", part);
            tf_card_status_e next = tfcard_status_update(ctx);
            if (next == SD_STATE_ERROR ||
                ((!ready || !accepted_device) && tfcard_make_directories() != 0))
            {
                tfcard_begin_cleanup(ctx);
                if (ctx->format_stage == 2)
                    tfcard_format_complete(ctx, -1);
                ctx->retry_ticks = TFCARD_RETRY_TICKS;
                return;
            }
            if (status != next)
                tfcard_status_change(ctx, next);
            if (ctx->format_stage == 2)
                tfcard_format_complete(ctx, 0);
        }
        return;
    }

    if (ready || found == 1 || ctx->cleanup_pending || ctx->format_stage == 1)
    {
        tfcard_begin_cleanup(ctx);
        if (!tfcard_cleanup_ready(ctx))
            return;
        if (found == 1)
        {
            /* Preserve an unrelated filesystem or an explicitly read-only
             * current card. An explicit format request may unmount our FAT. */
            if (!info.owned_fat || (current && info.read_only && !ctx->format_stage))
            {
                if (ctx->format_stage)
                    tfcard_format_complete(ctx, -1);
                ctx->retry_ticks = TFCARD_RETRY_TICKS;
                return;
            }
            if (umount(TFCARD_MOUNT_PATH) != 0)
            {
                AMY_LOG_WARNING("tfcard unmount pending: errno=%d", errno);
                ctx->retry_ticks = TFCARD_RETRY_TICKS;
            }
            return; /* Verify absence before mounting, including stacked mounts. */
        }
        Amy_mutex_lock(&ctx->mutex);
        ctx->cleanup_pending = 0;
        Amy_mutex_unlock(&ctx->mutex);
    }

    if (!present)
    {
        if (status != SD_STATE_UNPLUG)
            tfcard_status_change(ctx, SD_STATE_UNPLUG);
        return;
    }
    if (ctx->format_stage == 1)
    {
        tfcard_status_change(ctx, SD_STATE_FORMAT);
        if (tfcared_format(ctx) != 0)
        {
            tfcard_format_complete(ctx, -1);
            tfcard_status_change(ctx, SD_STATE_ERROR);
            ctx->retry_ticks = TFCARD_RETRY_TICKS;
            return;
        }
        Amy_mutex_lock(&ctx->mutex);
        ctx->format_stage = 2;
        Amy_mutex_unlock(&ctx->mutex);
    }
    if (tfcard_dev_mount(ctx) != 0)
    {
        if (Amy_tfcard_status() != SD_STATE_ERROR)
            tfcard_status_change(ctx, SD_STATE_ERROR);
        if (ctx->format_stage == 2)
            tfcard_format_complete(ctx, -1);
        ctx->retry_ticks = TFCARD_RETRY_TICKS;
    }
}

static int tfcard_create_parttion_cb(const void *user_data, char *output, int line)
{
    FILE *fp = (FILE *)output;
    char cmd[128] = {0};
    // o 命令清空整个分区表，避免逐个删除分区（最后一个分区时 fdisk 自动选中会干扰脚本）
    sprintf(cmd, "o\nn\np\n1\n\n\nw\n");
    fputs(cmd, fp);
    return 0;
}

/* Pin and validate the device before a destructive command. The command opens
 * /proc/<our pid>/fd/N, so a reused /dev/mmcblk0[p1] cannot select another card.
 * CLOEXEC keeps unrelated child processes from retaining our exclusive handle.
 * O_EXCL also refuses formatting if another mount still holds this device. */
static int tfcard_format_open(tfcard_handle *ctx, const char *part)
{
    struct stat node, opened, current;
    ino_t instance, current_instance;
    if (tfcard_node_stat(part, &node, &instance) != 0)
        return -1;
    char path[64];
    snprintf(path, sizeof(path), DEV_PATH "/%s", part);
    int fd = open(path, O_RDWR | O_EXCL | O_CLOEXEC);
    if (fd < 0)
        return -1;
    if (fd < 3)
    {
        int new_fd = fcntl(fd, F_DUPFD_CLOEXEC, 3);
        close(fd);
        fd = new_fd;
        if (fd < 0)
            return -1;
    }
    if (fstat(fd, &opened) != 0 || !S_ISBLK(opened.st_mode) || opened.st_rdev != node.st_rdev ||
        tfcard_node_stat(part, &current, &current_instance) != 0 ||
        current.st_rdev != node.st_rdev || current_instance != instance ||
        tfcard_node_stat(TFCARD_DEV, &current, &current_instance) != 0 ||
        current.st_rdev != ctx->format_device || current_instance != ctx->format_inode)
    {
        close(fd);
        return -1;
    }
    return fd;
}

static int tfcared_format(tfcard_handle *ctx)
{
    char cmd[256] = {0};
    cmd_execution_param_t param;
    memset(&param, 0, sizeof(cmd_execution_param_t));

    int fd = tfcard_format_open(ctx, TFCARD_DEV);
    if (fd < 0)
        return -1;
    // 清空旧分区表并创建主分区1（脚本见 tfcard_create_parttion_cb）
    snprintf(cmd, sizeof(cmd) - 1, "fdisk /proc/%ld/fd/%d", (long)getpid(), fd);
    param.cmd = cmd;
    param.isread = 0;
    param.cb = tfcard_create_parttion_cb;
    int result = system_cmd_execution(&param);
    int close_result = close(fd);
    if (result != 0 || close_result != 0)
    {
        AMY_LOG_ERROR("tfcard careate parrtion\n");
        return -1;
    }
    fd = tfcard_format_open(ctx, TFCARD_PART);
    if (fd < 0)
        return -1;
    // 格式化分区fat32
    memset(cmd, 0, sizeof(cmd));
    snprintf(cmd, sizeof(cmd) - 1, "mkdosfs -F 32 -I /proc/%ld/fd/%d -n Mete", (long)getpid(), fd);
    param.cmd = cmd;
    param.isread = 0;
    param.cb = NULL;
    result = system_cmd_execution(&param);
    close_result = close(fd);
    if (result != 0 || close_result != 0)
    {
        AMY_LOG_ERROR("tfcard mkdosfs parrtion\n");
        return -1;
    }
    strncpy(ctx->active_part, TFCARD_PART, sizeof(ctx->active_part) - 1);
    ctx->active_part[sizeof(ctx->active_part) - 1] = '\0';
    return 0;
}

static void *media_file_thread(void *arg)
{
    tfcard_handle *ctx = (tfcard_handle *)arg;
    while (ctx->run)
    {
        tfcared_event *ev;
        if (Amy_queues_get(ctx->q, (void **)&ev, NULL, NULL) == 0)
        {
            if (ev->event == TFCARD_EVENT_FORMAT && !ctx->format_stage)
            {
                Amy_mutex_lock(&ctx->mutex);
                ctx->format_stage = 1;
                ctx->format_device = ev->card_device;
                ctx->format_inode = ev->card_inode;
                Amy_mutex_unlock(&ctx->mutex);
                tfcard_begin_cleanup(ctx);
            }
            Amy_mempool_free(ctx->mp, ev);
        }
        tfcard_detection(ctx);
        Amy_msleep(TFCARD_POLL_MS);
    }
    return NULL;
}

int Amy_tfcard_init(void)
{
    memset(&tfcard_context, 0, sizeof(tfcard_handle));

    Amy_mutex_init(&tfcard_context.mutex, NULL);
    tfcard_context.mp = Amy_mempool_init(sizeof(tfcared_event) * 2, sizeof(tfcared_event) * 2);
    tfcard_context.q = Amy_queues_open(2, 0, 0, NULL, 0);
    tfcard_context.run = 1;
    tfcard_context.status = SD_STATE_UNPLUG;
    Amy_pthread_create(&tfcard_context.tid, media_file_thread, &tfcard_context, 32 * 1024, 12);
    return 0;
}

int Amy_tfcard_status(void)
{
    Amy_mutex_lock(&tfcard_context.mutex);
    int status = tfcard_context.status;
    Amy_mutex_unlock(&tfcard_context.mutex);
    return status;
}

int Amy_tfcard_inserted(void)
{
    Amy_mutex_lock(&tfcard_context.mutex);
    int inserted = tfcard_context.inserted;
    Amy_mutex_unlock(&tfcard_context.mutex);
    return inserted;
}

bool Amy_tfcard_event_is_current(unsigned int sequence)
{
    Amy_mutex_lock(&tfcard_context.mutex);
    bool current = sequence && tfcard_context.pending_event == sequence;
    Amy_mutex_unlock(&tfcard_context.mutex);
    return current;
}

void Amy_tfcard_event_processed(unsigned int sequence)
{
    Amy_mutex_lock(&tfcard_context.mutex);
    if (tfcard_context.pending_event == sequence)
    {
        tfcard_context.pending_event = 0;
        tfcard_context.event_queued = 0;
    }
    Amy_mutex_unlock(&tfcard_context.mutex);
}

unsigned int Amy_tfcard_generation(void)
{
    Amy_mutex_lock(&tfcard_context.mutex);
    unsigned int generation = tfcard_context.status == SD_STATE_MOUNT &&
                              !tfcard_context.cleanup_pending ? tfcard_context.generation : 0;
    Amy_mutex_unlock(&tfcard_context.mutex);
    return generation;
}

bool Amy_tfcard_io_begin(unsigned int generation)
{
    Amy_mutex_lock(&tfcard_context.mutex);
    bool ready = generation && generation == tfcard_context.generation &&
                 tfcard_context.status == SD_STATE_MOUNT && !tfcard_context.cleanup_pending &&
                 tfcard_context.io_users < UINT_MAX;
    if (ready)
        tfcard_context.io_users++;
    Amy_mutex_unlock(&tfcard_context.mutex);
    return ready;
}

void Amy_tfcard_io_end(void)
{
    Amy_mutex_lock(&tfcard_context.mutex);
    if (tfcard_context.io_users)
        tfcard_context.io_users--;
    Amy_mutex_unlock(&tfcard_context.mutex);
}

bool Amy_tfcard_format_pending(void)
{
    Amy_mutex_lock(&tfcard_context.mutex);
    bool pending = tfcard_context.format_requested || tfcard_context.format_stage != 0;
    Amy_mutex_unlock(&tfcard_context.mutex);
    return pending;
}

void Amy_tfcard_format_event_processed(void)
{
    Amy_mutex_lock(&tfcard_context.mutex);
    if (tfcard_context.format_event_queued && !tfcard_context.format_stage)
    {
        tfcard_context.format_event_queued = 0;
        tfcard_context.format_requested = 0;
    }
    Amy_mutex_unlock(&tfcard_context.mutex);
}

int Amy_tfcard_ensure_free_percent(unsigned int min_free_percent)
{
    if (min_free_percent == 0 || min_free_percent >= 100)
    {
        return -1;
    }

    int status = Amy_tfcard_status();
    if (!(status & SD_STATE_INSERT))
    {
        return -1;
    }

    for (int i = 0; i < 1024; i++)
    {
        unsigned long total_kb = 0;
        unsigned long used_kb = 0;
        unsigned long free_kb = 0;
        if (tfcard_capacity_get(&total_kb, &used_kb, &free_kb) != 0 || total_kb == 0)
        {
            return -1;
        }
        unsigned long min_free_kb = (total_kb * min_free_percent) / 100;
        if (free_kb > min_free_kb)
        {
            return 0;
        }
        if (tfcard_delete_oldest_media_file_once() != 0)
        {
            AMY_LOG_ERROR("tfcard ensure free percent failed: no deletable media file");
            return -1;
        }
    }
    return -1;
}

int Amy_tfcard_format(void)
{
    struct stat card;
    ino_t instance;
    if (tfcard_node_stat(TFCARD_DEV, &card, &instance) != 0)
        return -1;
    Amy_mutex_lock(&tfcard_context.mutex);
    if (tfcard_context.format_requested || tfcard_context.format_stage ||
        !tfcard_context.mp || !tfcard_context.q)
    {
        Amy_mutex_unlock(&tfcard_context.mutex);
        return -1;
    }
    tfcard_context.format_requested = 1;
    Amy_mutex_unlock(&tfcard_context.mutex);
    tfcared_event *ev = Amy_mempool_alloc(tfcard_context.mp, sizeof(tfcared_event));
    if (!ev)
    {
        AMY_LOG_ERROR("mp allo failed\n");
        goto failed;
    }
    ev->event = TFCARD_EVENT_FORMAT;
    ev->card_device = card.st_rdev;
    ev->card_inode = instance;
    if (Amy_queues_put(tfcard_context.q, ev))
    {
        Amy_mempool_free(tfcard_context.mp, ev);
        goto failed;
    }
    return 0;
failed:
    Amy_mutex_lock(&tfcard_context.mutex);
    tfcard_context.format_requested = 0;
    Amy_mutex_unlock(&tfcard_context.mutex);
    return -1;
}
