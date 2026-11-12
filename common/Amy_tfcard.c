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

#define DEV_PATH "/dev"
#define TFCARD_DEV "mmcblk0"
#define TFCARD_PART "mmcblk0p1"
/** 最多尝试挂载的分区个数（mmcblk0p1 ~ pN） */
#define TFCARD_PART_INDEX_MAX 3
#define TFCARD_MEDIA_PATH TFCARD_MOUNT_PATH "/media/"
typedef struct tfcared_event
{
    unsigned int event;
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
    int skip_detection_cnt;
    int capacity_check_cnt; // 容量检测计数（控制刷新频率）
    /** 当前挂载使用的分区节点名，如 mmcblk0p2（多分区卡兼容） */
    char active_part[40];
} tfcard_handle;

static tfcard_handle tfcard_context;
static tf_card_status_e tfcard_status_update(tfcard_handle *ctx);

typedef struct
{
    int found;
    time_t oldest_mtime;
    char oldest_path[PATH_MAX];
} tfcard_oldest_file_ctx_t;

static tfcard_oldest_file_ctx_t g_oldest_file_ctx;

static int tfcard_status_change(tfcard_handle *ctx, tf_card_status_e new_status)
{
    ctx->status = new_status;
    return Amy_tfcard_event_put();
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

static int tfcard_capacity_refresh(tfcard_handle *ctx)
{
    if (ctx == NULL)
    {
        return -1;
    }
    if (!(ctx->status & SD_STATE_INSERT))
    {
        return -1;
    }
    tf_card_status_e new_status = tfcard_status_update(ctx);
    if (new_status != ctx->status)
    {
        tfcard_status_change(ctx, new_status);
    }
    return 0;
}

int tfcard_capacity_get(unsigned long *total_kb, unsigned long *used_kb, unsigned long *free_kb)
{
    if (total_kb == NULL || used_kb == NULL)
    {
        return -1;
    }

    Amy_mutex_lock(&tfcard_context.mutex);
    // 若TF卡未插入，返回0容量
    if (tfcard_context.status == SD_STATE_UNPLUG)
    {
        *total_kb = 0;
        *used_kb = 0;
        if (free_kb != NULL)
        {
            *free_kb = 0;
        }
        Amy_mutex_unlock(&tfcard_context.mutex);
        return -1;
    }

    // 容量改为按需刷新，不做周期刷新
    tfcard_capacity_refresh(&tfcard_context);

    // 赋值输出
    *total_kb = tfcard_context.total_kbbytes;
    *used_kb = tfcard_context.used_kbbytes;
    if (free_kb != NULL)
    {
        *free_kb = tfcard_context.free_kbbytes;
    }
    Amy_mutex_unlock(&tfcard_context.mutex);
    return 0;
}

static int is_substring(const void *user_data, char *output, int line)
{
    if (strstr(output, user_data) != NULL)
    {
        return 0;
    }
    return -1;
}

static void tfcard_part_idx_to_name(unsigned idx1based, char *out, size_t out_sz)
{
    if (out == NULL || out_sz == 0)
    {
        return;
    }
    if (idx1based == 3)
    {
        snprintf(out, out_sz, "%s", TFCARD_DEV);
    }
    else
    {
        snprintf(out, out_sz, "%sp%u", TFCARD_DEV, idx1based);
    }
}

static int tfcard_partition_node_exists(const char *part_name)
{
    if (part_name == NULL || part_name[0] == '\0')
    {
        return 0;
    }
    char node[64];
    snprintf(node, sizeof(node), DEV_PATH "/%s", part_name);
    return (access(node, F_OK) == 0) ? 1 : 0;
}

/**
 * @brief 选择用于 df / 状态判断的分区节点：优先已挂载记录的 active_part，否则 p1→p3 第一个存在的节点
 */
static int tfcard_resolve_blockdev(tfcard_handle *ctx, char *out, size_t out_sz)
{
    if (ctx == NULL || out == NULL || out_sz == 0)
    {
        return -1;
    }
    if (ctx->active_part[0] != '\0' && tfcard_partition_node_exists(ctx->active_part))
    {
        snprintf(out, out_sz, "%s", ctx->active_part);
        return 0;
    }
    for (unsigned i = 0; i <= TFCARD_PART_INDEX_MAX; i++)
    {
        char name[48];
        tfcard_part_idx_to_name(i, name, sizeof(name));
        if (tfcard_partition_node_exists(name))
        {
            snprintf(out, out_sz, "%s", name);
            return 0;
        }
    }
    return -1;
}

static int tfcard_format_finish(void)
{
    return Amy_tfcard_format_finish_event_put();
}

static int df_cmd_umount_filter(const void *user_data, char *string, int line)
{
    /// dev/sdb1      vfat   7662592 4265472   3397120  56% /tmp/tf
    if (line == 0)
    {
        return -1;
    }

    if (is_substring(DEV_PATH "/" TFCARD_DEV, string, 0))
    {
        return -1;
    }
    const char *path = string_spaces_after_char(string, 5);
    if (!path)
    {
        AMY_LOG_ERROR("mount error path failed(%s\n", string);
        return -1;
    }

    char cmd[64] = {0};
    memset(cmd, 0, sizeof(cmd));
    snprintf(cmd, sizeof(cmd) - 1, "umount %s", path);
    cmd_execution_param_t param;
    memset(&param, 0, sizeof(cmd_execution_param_t));
    param.cmd = cmd;
    return system_cmd_execution(&param);
}

static int tfcard_dev_umount(tfcard_handle *ctx)
{
    char cmd[32] = {0};
    snprintf(cmd, sizeof(cmd) - 1, "df");

    cmd_execution_param_t param;
    memset(&param, 0, sizeof(cmd_execution_param_t));
    param.cb = df_cmd_umount_filter;
    param.cmd = cmd;
    param.isread = 1;
    param.userdata = ctx;
    return system_cmd_execution(&param);
}

static int tfcard_dev_mount(tfcard_handle *ctx)
{
    if (access(TFCARD_MOUNT_PATH, F_OK))
    {
        mkdir(TFCARD_MOUNT_PATH, 0644);
    }

    if (access(TFCARD_MOUNT_PATH, F_OK))
    {
        AMY_LOG_ERROR("mkdir %s failed\n", TFCARD_MOUNT_PATH);
        return -1;
    }

    cmd_execution_param_t param;
    memset(&param, 0, sizeof(cmd_execution_param_t));
    for (unsigned i = 1; i <= TFCARD_PART_INDEX_MAX; i++)
    {
        char part[48];
        tfcard_part_idx_to_name(i, part, sizeof(part));
        if (!tfcard_partition_node_exists(part))
        {
            continue;
        }
        char cmd[160] = {0};
        snprintf(cmd, sizeof(cmd) - 1, "mount -t vfat " DEV_PATH "/%s " TFCARD_MOUNT_PATH, part);
        param.cmd = cmd;
        if (system_cmd_execution(&param) == 0)
        {
            snprintf(ctx->active_part, sizeof(ctx->active_part), "%s", part);
            AMY_LOG_INFO("tfcard mounted vfat on %s\n", part);

            const char *sub_dirs[] = {"media", "music", "picture"};
            for (unsigned d = 0; d < sizeof(sub_dirs) / sizeof(sub_dirs[0]); d++)
            {
                char sub_path[64];
                snprintf(sub_path, sizeof(sub_path), "%s/%s", TFCARD_MOUNT_PATH, sub_dirs[d]);
                if (access(sub_path, F_OK) != 0)
                {
                    mkdir(sub_path, 0777);
                }
            }

            return 0;
        }
        AMY_LOG_WARNING("tfcard mount vfat failed on %s, try next partition\n", part);
    }
    AMY_LOG_ERROR("tfcard mount vfat failed (tried up to %u partitions)\n", (unsigned)TFCARD_PART_INDEX_MAX);
    return -1;
}

static void tfcard_status_reset(tfcard_handle *ctx)
{
    tfcard_dev_umount(ctx);
    ctx->status = SD_STATE_UNPLUG;
    ctx->skip_detection_cnt = 0;
    ctx->inserted = 0;
    ctx->capacity_check_cnt = 0; // 新增：初始化容量检测计数
    strncpy(ctx->active_part, TFCARD_PART, sizeof(ctx->active_part) - 1);
    ctx->active_part[sizeof(ctx->active_part) - 1] = '\0';
}

static int tfcard_mount_adjust_cb(const void *user_data, char *string, int line)
{
    char cmd[128] = {0};
    if (line == 0)
    {
        return -1;
    }
    tfcard_handle *ctx = (tfcard_handle *)user_data;

    // 判断挂在路径是否一致
    if (is_substring(TFCARD_MOUNT_PATH, string, line))
    {
        // 不一致，卸载 /dev/sdb1      vfat   7662592 4265472   3397120  56% /tmp/tf
        const char *path = string_spaces_after_char(string, 6);
        if (!path)
        {
            AMY_LOG_ERROR("get mount error path failed(%s\n", string);
            return -1;
        }

        memset(cmd, 0, sizeof(cmd));
        snprintf(cmd, sizeof(cmd) - 1, "umount %s", path);
        AMY_LOG_WARNING("%s\n", cmd);

        cmd_execution_param_t param;
        memset(&param, 0, sizeof(cmd_execution_param_t));
        param.cmd = cmd;
        system_cmd_execution(&param);

        return -1;
    }
    /// dev/mmcblk0p1       vfat        30855440        16  30855424   0% /mnt/tf
    ctx->free_kbbytes = ctx->total_kbbytes = ctx->used_kbbytes = 0;
    char *s = string_spaces_after_char(string, 2);
    if (s)
    {
        char *token = strtok(s, " ");
        if (token)
        {
            ctx->total_kbbytes = strtol(token, NULL, 10);
            token = strtok(NULL, " ");
            if (token)
            {
                ctx->used_kbbytes = strtol(token, NULL, 10);
            }
        }
        ctx->free_kbbytes = ctx->total_kbbytes - ctx->used_kbbytes;
        AMY_LOG_INFO("tf memory: free:%lu kb,used:%lu kb,total:%lukb\n", ctx->free_kbbytes, ctx->used_kbbytes, ctx->total_kbbytes);
    }
    return 0;
}

static tf_card_status_e tfcard_status_update(tfcard_handle *ctx)
{
    cmd_execution_param_t param;
    memset(&param, 0, sizeof(cmd_execution_param_t));

    // 查找设备存在
    char cmd[128] = {0};
    char node[64];
    snprintf(node, sizeof(node), DEV_PATH "/" TFCARD_DEV);
    if (access(node, F_OK) != 0)
    {
        AMY_LOG_INFO("tfcard unplug\n");
        return SD_STATE_UNPLUG;
    }

    // 查找分区节点：支持 mmcblk0p1 ~ pN（最多 3 个），兼容仅有 p2/p3 或未初始化多分区场景
    char block_part[48];
    if (tfcard_resolve_blockdev(ctx, block_part, sizeof(block_part)) != 0)
    {
        AMY_LOG_INFO("tfcard error: no mmcblk0p1~p%u\n", (unsigned)TFCARD_PART_INDEX_MAX);
        return SD_STATE_ERROR;
    }

    // 查找存储块是否挂载
    memset(cmd, 0, sizeof(cmd)); // df -T /dev/mmcblk0pN
    snprintf(cmd, sizeof(cmd) - 1, "df -T " DEV_PATH "/%s", block_part);
    param.cb = tfcard_mount_adjust_cb;
    param.cmd = cmd;
    param.isread = 1;
    param.userdata = ctx;
    if (system_cmd_execution(&param))
    {
        AMY_LOG_INFO("tfcard insert\n");
        return SD_STATE_INSERT;
    }

    if (ctx->free_kbbytes < 1048576)
    {
        AMY_LOG_INFO("tfcard memory full!\n");
        return SD_STATE_FULL;
    }
    AMY_LOG_INFO("tfcard mount suncess!!\n");
    return SD_STATE_MOUNT;
}

static void tfcard_detection(tfcard_handle *ctx)
{
    int inserted = 0;
    char node[64];
    snprintf(node, sizeof(node), DEV_PATH "/" TFCARD_DEV);
    if (access(node, F_OK) == 0)
    {
        inserted = 1;
    }

    if (ctx->inserted != inserted)
    {
        ctx->skip_detection_cnt = 10;
        ctx->inserted = inserted;
        // 拔卡时重置所有容量字段为0
        ctx->free_kbbytes = 0;
        ctx->used_kbbytes = 0;
        ctx->total_kbbytes = 0;
        ctx->capacity_check_cnt = 0;
        return;
    }

    if (ctx->skip_detection_cnt)
    {
        ctx->skip_detection_cnt--;
        return;
    }

    //    AMY_LOG_INFO("status:%d inserted:%d\n", ctx->status, inserted);
    // tf 设备不存在
    if ((!inserted) && (ctx->status != SD_STATE_UNPLUG))
    {
        if (tfcard_dev_umount(ctx) == 0)
        {
            tfcard_status_change(ctx, SD_STATE_UNPLUG);
        }
    } // 挂载
    else if ((inserted) && ((ctx->status == SD_STATE_UNPLUG) /*|| (ctx->status == SD_STATE_INSERT)*/))
    {
        if (tfcard_dev_mount(ctx) == 0)
        {
            tfcard_status_change(ctx, tfcard_status_update(ctx));
        }
    }
    // 已挂载状态下不做周期容量刷新，改为按需查询（见 tfcard_capacity_get）
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
static int tfcared_format(tfcard_handle *ctx)
{
    char cmd[256] = {0};
    cmd_execution_param_t param;
    memset(&param, 0, sizeof(cmd_execution_param_t));

    // 清空旧分区表并创建主分区1（脚本见 tfcard_create_parttion_cb）
    snprintf(cmd, sizeof(cmd) - 1, "fdisk " DEV_PATH "/" TFCARD_DEV);
    param.cmd = cmd;
    param.isread = 0;
    param.cb = tfcard_create_parttion_cb;
    if (system_cmd_execution(&param))
    {
        AMY_LOG_ERROR("tfcard careate parrtion\n");
        return -1;
    }
    // 格式化分区fat32
    memset(cmd, 0, sizeof(cmd));
    snprintf(cmd, sizeof(cmd) - 1, "mkdosfs -F 32 -I " DEV_PATH "/" TFCARD_PART " -n Mete");
    param.cmd = cmd;
    param.isread = 0;
    param.cb = NULL;
    if (system_cmd_execution(&param))
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
    tfcard_status_reset(ctx);

    tfcared_event *ev;
    while (ctx->run)
    {
        Amy_mutex_lock(&ctx->mutex);
        tfcard_detection(ctx);
        Amy_mutex_unlock(&ctx->mutex);
        if (Amy_queues_get(ctx->q, (void **)&ev, NULL, NULL) == 0)
        {
            if ((ev->event == TFCARD_EVENT_FORMAT) && (ev->event != SD_STATE_UNPLUG))
            {
                AMY_LOG_INFO("tfcard foramt...\n");
                tfcard_status_change(ctx, SD_STATE_FORMAT);
                tfcard_status_reset(ctx);
                Amy_msleep(100);
                tfcared_format(ctx);
                tfcard_format_finish();
            }
            Amy_mempool_free(ctx->mp, ev);
        }
        Amy_msleep(10);
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
    return tfcard_context.status;
}

int Amy_tfcard_inserted(void)
{
    return tfcard_context.inserted;
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
    tfcared_event *ev = Amy_mempool_alloc(tfcard_context.mp, sizeof(tfcared_event));
    if (!ev)
    {
        AMY_LOG_ERROR("mp allo failed\n");
        return -1;
    }
    ev->event = TFCARD_EVENT_FORMAT;
    if (Amy_queues_put(tfcard_context.q, ev))
    {
        Amy_mempool_free(tfcard_context.mp, ev);
        return -1;
    }
    return 0;
}