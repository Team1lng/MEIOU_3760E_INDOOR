#include "monitor.h"
#include "Amy_log.h"
#include "Amy_sdk.h"
#include "Amy_tfcard.h"
#include "gpio.h"
#include "video_input.h"
#include "live/testRTSPClient.h"
#include "Amy_media_mgr.h"

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include "user_data.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "Amy_lvgl.h"
#include "tuya/tuya_sdk.h"
typedef struct
{
    Amy_mutex_t mutex;
    rtsp_client_context *rtsp_context;
    unsigned long cur_chn;
    int snapshort_run;
    unsigned long snaphsort_mode;
    bool snapshort_save_media;
    int record_run;
    unsigned long record_mode;
} monitor_context;

/** 与 user_data.private_cfg.talk_volume 一致：1~10 */
#define MONITOR_TALK_VOLUME_MIN 1
#define MONITOR_TALK_VOLUME_MAX 10

static monitor_context mon_ctx = {
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .cur_chn = MEDIA_CHANNEL_ALL,
    .rtsp_context = NULL,
    .snaphsort_mode = 0,
    .snapshort_run = 0,
    .record_mode = 0,
    .record_run = 0,
};

// 根据设备类型获取监控通道
int door_monitor_channel_get_by_device_type(int device_type)
{
    if (device_type == DEVICE_TYPE_DOOR_ID1)
    {
        return MON_CHANNEL_DOOR1;
    }
    else if (device_type == DEVICE_TYPE_DOOR_ID2)
    {
        return MON_CHANNEL_DOOR2;
    }
    else
    {
        return MON_CHANNEL_UNKNOW;
    }
}

// 获取第一个门有效监控通道
int door_monitor_first_valid_channel_get()
{
    int channels[] = {MON_CHANNEL_DOOR1, MON_CHANNEL_DOOR2};
    int channel_count = sizeof(channels) / sizeof(channels[0]);

    for (int i = 0; i < channel_count; i++)
    {
        int curr_channel = channels[i];
        if ((user_data_get()->public_cfg.net_door_device[curr_channel].rtsp[0].rtsp_url[0] != '\0') &&
            user_data_get()->public_cfg.door_device[curr_channel].device_valid)
        {
            return curr_channel; // 记录第一个有效通道
        }
    }
    return MON_CHANNEL_UNKNOW;
}

// 获取第一个CCTV有效监控通道
int cctv_monitor_first_valid_channel_get()
{
    int channels[] = {MON_CHANNEL_CCTV1, MON_CHANNEL_CCTV2};
    int channel_count = sizeof(channels) / sizeof(channels[0]);

    // 3. 遍历通道，找到第一个URL有效的通道
    for (int i = 0; i < channel_count; i++)
    {
        int channel_index = channels[i] - MON_CHANNEL_CCTV1;
        if ((user_data_get()->public_cfg.net_cctv_device[channel_index].rtsp[0].rtsp_url[0] != '\0'))
        {
            return channel_index + MON_CHANNEL_CCTV1; // 记录第一个有效通道
        }
    }
    return MON_CHANNEL_UNKNOW;
}

// 获取第一个有效监控通道（door1 优先，其次 door2，再 CCTV1/CCTV2）
int monitor_first_valid_channel_get(void)
{
    int channel = door_monitor_first_valid_channel_get();
    if (channel != MON_CHANNEL_UNKNOW)
        return channel;
    return cctv_monitor_first_valid_channel_get();
}

// 判断指定监控通道是否有效:return int: 1-通道有效；0-通道无效；-1-参数非法/数据异常
int monitor_channel_is_valid(int channel)
{
    // 1. 前置校验：获取用户数据指针，避免空指针访问
    user_data_info *p_user_data = user_data_get();
    if (p_user_data == NULL)
    {
        return -1;
    }

    switch (channel)
    {
    case MON_CHANNEL_DOOR1:
    case MON_CHANNEL_DOOR2:
        if (p_user_data->public_cfg.net_door_device[channel].rtsp[0].rtsp_url[0] != '\0' &&
            p_user_data->public_cfg.door_device[channel].device_valid)
        {
            return 1; // 通道有效
        }
        break;

    case MON_CHANNEL_CCTV1:
    case MON_CHANNEL_CCTV2:
    {
        int cctv_index = channel - MON_CHANNEL_CCTV1;
        if (p_user_data->public_cfg.net_cctv_device[cctv_index].rtsp[0].rtsp_url[0] != '\0')
        {
            return 1; // 通道有效
        }
    }
    break;
    default:
        return -1; // 参数非法
    }

    return 0;
}

// 在当前通道同类型内获取下一个有效通道（door 之间或 CCTV 之间切换）
int monitor_next_valid_channel_same_type_get(int cur_channel)
{
    if (is_channel_ipc_camera(cur_channel) == 0)
    {
        int other = (cur_channel == MON_CHANNEL_DOOR1) ? MON_CHANNEL_DOOR2 : MON_CHANNEL_DOOR1;
        if (monitor_channel_is_valid(other) == 1)
            return other;
    }
    else if (is_channel_ipc_camera(cur_channel) == 1)
    {
        int other = (cur_channel == MON_CHANNEL_CCTV1) ? MON_CHANNEL_CCTV2 : MON_CHANNEL_CCTV1;
        if (monitor_channel_is_valid(other) == 1)
            return other;
    }
    return MON_CHANNEL_UNKNOW;
}

// 获取当前通道的下一个有效通道（循环模式）
// 入参：当前通道 cur_channel
// 返回值：下一个有效通道，无任何有效通道 或 只有一个有效通道 返回 MON_CHANNEL_UNKNOW
int monitor_next_valid_channel_get(int cur_channel)
{
    // 定义所有支持的监控通道（按你业务顺序排列）
    const int all_channels[] = {
        MON_CHANNEL_DOOR1,
        MON_CHANNEL_DOOR2,
        MON_CHANNEL_CCTV1,
        MON_CHANNEL_CCTV2};
    int total_channels = sizeof(all_channels) / sizeof(all_channels[0]);

    // 1. 先统计总有效通道数量
    int valid_count = 0;
    for (int i = 0; i < total_channels; i++)
    {
        if (monitor_channel_is_valid(all_channels[i]) == 1)
        {
            valid_count++;
        }
    }

    // 2. 如果有效通道数 <= 1 → 直接返回 UNKNOW
    if (valid_count <= 1)
    {
        return MON_CHANNEL_UNKNOW;
    }

    // 3. 找到当前通道索引
    int cur_index = -1;
    for (int i = 0; i < total_channels; i++)
    {
        if (all_channels[i] == cur_channel)
        {
            cur_index = i;
            break;
        }
    }

    // 当前通道无效 → 从第0个开始
    if (cur_index == -1)
    {
        cur_index = 0;
    }

    // 4. 从下一个位置循环查找
    for (int i = 1; i <= total_channels; i++)
    {
        int next_index = (cur_index + i) % total_channels;
        int next_chn = all_channels[next_index];

        if (monitor_channel_is_valid(next_chn) == 1)
        {
            return next_chn;
        }
    }

    // 理论上不会走到这里
    return MON_CHANNEL_UNKNOW;
}

static char *monitor_url_channel_url_get(int channel)
{
    if (channel != MON_CHANNEL_DOOR1 && channel != MON_CHANNEL_DOOR2 && channel != MON_CHANNEL_CCTV1 && channel != MON_CHANNEL_CCTV2)
        return NULL;
    static char rtsp_uri[128] = {0};
    memset(rtsp_uri, 0, sizeof(rtsp_uri)); // 清空缓冲区

    int channel_index = channel - MON_CHANNEL_CCTV1 >= 0 ? channel - MON_CHANNEL_CCTV1 : channel;
    if ((channel == MON_CHANNEL_DOOR1 || channel == MON_CHANNEL_DOOR2) &&
        user_data_get()->public_cfg.net_door_device[channel_index].rtsp[0].rtsp_url_type == RTSP_URL_TYPE_BASE)
    {
        char *url = user_data_get()->public_cfg.net_door_device[channel_index].rtsp[0].rtsp_url;
        char *user = user_data_get()->public_cfg.net_door_device[channel_index].username;
        char *pwd = user_data_get()->public_cfg.net_door_device[channel_index].password;

        char *sep = strstr(url ? url : "", "//");
        if (sep)
            snprintf(rtsp_uri, sizeof(rtsp_uri), "rtsp://%s:%s@%s", user, pwd, sep + 2);
    }
    else if ((channel == MON_CHANNEL_CCTV1 || channel == MON_CHANNEL_CCTV2) &&
             user_data_get()->public_cfg.net_cctv_device[channel_index].rtsp[0].rtsp_url_type == RTSP_URL_TYPE_BASE)
    {
        char *url = user_data_get()->public_cfg.net_cctv_device[channel_index].rtsp[0].rtsp_url;
        char *user = user_data_get()->public_cfg.net_cctv_device[channel_index].username;
        char *pwd = user_data_get()->public_cfg.net_cctv_device[channel_index].password;

        char *sep = strstr(url ? url : "", "//");
        if (sep)
            snprintf(rtsp_uri, sizeof(rtsp_uri), "rtsp://%s:%s@%s", user, pwd, sep + 2);
    }
    else
    {
        char *src_url = (channel == MON_CHANNEL_DOOR1 || channel == MON_CHANNEL_DOOR2) ? user_data_get()->public_cfg.net_door_device[channel_index].rtsp->rtsp_url : user_data_get()->public_cfg.net_cctv_device[channel_index].rtsp->rtsp_url;
        strncpy(rtsp_uri, src_url ? src_url : "", sizeof(rtsp_uri) - 1);
        rtsp_uri[sizeof(rtsp_uri) - 1] = '\0';
    }
    printf("rtsp_uri is %s\r\n", rtsp_uri);
    return rtsp_uri;
}
int monitor_video_close(void)
{
    Amy_mutex_lock(&mon_ctx.mutex);
    if (mon_ctx.rtsp_context)
    {
        live555_rtsp_client_close(mon_ctx.rtsp_context);
        mon_ctx.rtsp_context = NULL;
        mon_ctx.record_run = false;
        mon_ctx.snapshort_run = false;
        // mon_ctx.cur_chn = MON_CHANNEL_UNKNOW;
    }
    Amy_mutex_unlock(&mon_ctx.mutex);
    return 0;
}

int monitor_video_open(void)
{
    if (mon_ctx.rtsp_context)
    {
        unsigned long temp_channel = mon_ctx.cur_chn;
        monitor_video_close();
        mon_ctx.cur_chn = temp_channel;
    }
    Amy_mutex_lock(&mon_ctx.mutex);
    // door 通道走 br0，cctv 通道走 wlan0（与 monitor_url_channel_url_get 的取源逻辑一致）
    const char *ifname = (mon_ctx.cur_chn == MON_CHANNEL_DOOR1 || mon_ctx.cur_chn == MON_CHANNEL_DOOR2) ? "br0" : "wlan0";
    mon_ctx.rtsp_context = live555_rtsp_client_start(monitor_url_channel_url_get(mon_ctx.cur_chn), ifname, 0, 0, 1024, 600);
    Amy_mutex_unlock(&mon_ctx.mutex);
    return 0;
}

int monitor_talk_volume_set(int volume)
{
    if (volume < MONITOR_TALK_VOLUME_MIN || volume > MONITOR_TALK_VOLUME_MAX)
    {
        AMY_LOG_ERROR("monitor talk volume invalid: %d (range %d~%d)", volume,
                      MONITOR_TALK_VOLUME_MIN, MONITOR_TALK_VOLUME_MAX);
        return -1;
    }
    Amy_mutex_lock(&mon_ctx.mutex);
    if (mon_ctx.rtsp_context != NULL)
    {
        if (live555_rtsp_request_volume_set(mon_ctx.rtsp_context, volume * 10) != 0)
        {
            AMY_LOG_WARNING("monitor RTSP volume set failed (vol=%d)", volume);
            Amy_mutex_unlock(&mon_ctx.mutex);
            return -1;
        }
        AMY_LOG_INFO("monitor RTSP talk volume applied: %d", volume);
    }
    Amy_mutex_unlock(&mon_ctx.mutex);
    return 0;
}

static void *sync_task(void *arg)
{
    cmd_execution_param_t param;
    memset(&param, 0, sizeof(cmd_execution_param_t));
    param.cmd = "sync";
    system_cmd_execution(&param);
    return NULL;
}

static void sync_run(void)
{
    Amy_thread_t tid;
    Amy_pthread_create(&tid, sync_task, NULL, 32 * 1024, 12);
    Amy_pthread_detach(tid);
}

static void monitor_local_media_delete_oldest(const char *dir)
{
    DIR *d = opendir(dir);
    if (!d)
        return;

    char oldest_path[256] = {0};
    time_t oldest_time = 0;
    struct dirent *entry;
    while ((entry = readdir(d)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;
        char fullpath[256];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, entry->d_name);
        struct stat st;
        if (stat(fullpath, &st) != 0)
            continue;
        if (!S_ISREG(st.st_mode))
            continue;
        if (oldest_time == 0 || st.st_mtime < oldest_time)
        {
            oldest_time = st.st_mtime;
            strncpy(oldest_path, fullpath, sizeof(oldest_path) - 1);
        }
    }
    closedir(d);

    if (oldest_path[0] != '\0')
    {
        unlink(oldest_path);
        sync_run();
        AMY_LOG_INFO("local media limit: removed %s\n", oldest_path);
    }
}

static int monitor_snapshort_cb(const char *data, int size)
{
    if ((!data) || (!size))
    {
        mon_ctx.snapshort_run = 0;
        return -1;
    }

    int channel = mon_ctx.cur_chn;

    if (mon_ctx.snaphsort_mode & MEDIA_MODE_AUTO)
        tuya_api_call_event(channel, data, size);
    else if (mon_ctx.snaphsort_mode & MEDIA_MODE_MOTION)
        tuya_api_human_alarm_event(channel, data, size);

    if (!mon_ctx.snapshort_save_media)
    {
        mon_ctx.snapshort_run = 0;
        return 0;
    }
    char dir[32] = {0};
    int is_local = 0;
    if (Amy_tfcard_status() == SD_STATE_MOUNT)
    {
        sprintf(dir, "%s", TFCARD_MOUNT_PATH);
    }
    else
    {
        return -1;
        sprintf(dir, "%s", LOCAL_MEDIA_DIR);
        mkdir(dir, 0777);
        is_local = 1;
    }

    Amy_media_context *media_ctx = Amy_media_open(dir, mon_ctx.snaphsort_mode | MEDIA_TYPE_PHOTO | MEDIA_ATTR_NEW | (MEDIA_CHANNEL_0 << (mon_ctx.cur_chn - MON_CHANNEL_DOOR1)), 0);
    if (!media_ctx)
    {
        mon_ctx.snapshort_run = 0;
        AMY_LOG_ERROR("media sqlite open failed\n");
        return -1;
    }

    if (is_local)
    {
        unsigned long total = 0;
        Amy_media_all_index_total_get(media_ctx, NULL, &total);
        if (total >= MEDIA_LOCAL_MAX_FILES)
        {
            char media_dir[64];
            snprintf(media_dir, sizeof(media_dir), "%s/media", LOCAL_MEDIA_DIR);
            monitor_local_media_delete_oldest(media_dir);
        }
    }

    char path[128] = {0};
    if (Amy_media_insert(media_ctx, 0, NULL, path, sizeof(path)) == 0)
    {
        int fd = open(path, O_CREAT | O_WRONLY, 0666);
        if (fd < 0)
        {
            AMY_LOG_ERROR("open %s failed\n", path);
            mon_ctx.snapshort_run = 0;
            Amy_media_close(media_ctx);
            return -1;
        }
        write(fd, data, size);
        close(fd);

        sync_run();
        AMY_LOG_INFO("snapshort %s suncess!!\n", path);
    }

    Amy_media_close(media_ctx);

    mon_ctx.snapshort_run = 0;
    return 0;
}

static int monitor_record_cb(const char *file, int duration)
{
    if ((!file) || (duration < 1))
    {
        mon_ctx.record_run = 0;
        Amy_mutex_unlock(&mon_ctx.mutex);
        return -1;
    }
    if (Amy_tfcard_status() != SD_STATE_MOUNT)
    {
        AMY_LOG_ERROR("TF card is not mounted\n");
        mon_ctx.record_run = 0;
        Amy_mutex_unlock(&mon_ctx.mutex);
        return -1;
    }
    int shift = mon_ctx.cur_chn - MON_CHANNEL_DOOR1;
    unsigned long long mask = mon_ctx.record_mode | MEDIA_TYPE_VIDEO | MEDIA_ATTR_NEW | ((unsigned long long)MEDIA_CHANNEL_0 << shift);
    AMY_LOG_DEBUG("shift=%d, mask is 0x%llx\r\n", shift, mask);
    Amy_media_context *media_ctx = Amy_media_open(TFCARD_MOUNT_PATH, mon_ctx.record_mode | MEDIA_TYPE_VIDEO | MEDIA_ATTR_NEW | (MEDIA_CHANNEL_0 << (mon_ctx.cur_chn - MON_CHANNEL_DOOR1)), 0); //"/tmp/"
    if (!media_ctx)
    {
        mon_ctx.record_run = 0;
        AMY_LOG_ERROR("media sqlite open failed\n");
        Amy_mutex_unlock(&mon_ctx.mutex);
        return -1;
    }

    char path[128] = {0};
    if (Amy_media_insert(media_ctx, duration, NULL, path, sizeof(path)) == 0)
    {
        rename(file, path);

        sync_run();
        AMY_LOG_INFO("record %s suncess!!\n", path);
    }
    Amy_media_close(media_ctx);
    mon_ctx.record_run = 0;
    Amy_mutex_unlock(&mon_ctx.mutex);
    return 0;
}

int monitor_video_snapshort(unsigned int mode, bool save_media)
{
    Amy_mutex_lock(&mon_ctx.mutex);
    if (mon_ctx.snapshort_run)
    {
        Amy_mutex_unlock(&mon_ctx.mutex);
        return -1;
    }

    if (mon_ctx.rtsp_context)
    {
        if (live555_rtsp_request_snapshort_start(mon_ctx.rtsp_context, monitor_snapshort_cb))
        {
            AMY_LOG_ERROR("snapshort failed\n");
            Amy_mutex_unlock(&mon_ctx.mutex);
            return -1;
        }
        mon_ctx.snapshort_run = 1;
        mon_ctx.snaphsort_mode = mode;
        mon_ctx.snapshort_save_media = save_media;
    }
    Amy_mutex_unlock(&mon_ctx.mutex);
    return 0;
}

int monitor_video_record_start(unsigned int mode)
{
    Amy_mutex_lock(&mon_ctx.mutex);
    if (mon_ctx.record_run)
    {
        Amy_mutex_unlock(&mon_ctx.mutex);
        return -1;
    }

    if (mon_ctx.rtsp_context)
    {
        if (live555_rtsp_request_record_start(mon_ctx.rtsp_context, monitor_record_cb))
        {
            AMY_LOG_ERROR("record failed\n");
            Amy_mutex_unlock(&mon_ctx.mutex);
            return -1;
        }
        mon_ctx.record_run = 1;
        mon_ctx.record_mode = mode;
    }
    Amy_mutex_unlock(&mon_ctx.mutex);
    return 0;
}

int monitor_video_record_stop(void)
{
    // 这里不能使用互斥锁，原因是record stop是阻塞运行的。
    if (!mon_ctx.record_run)
    {
        AMY_LOG_WARNING("record is closed!!! \n");
        return -1;
    }
    if (mon_ctx.rtsp_context)
    {
        if (live555_rtsp_request_record_stop(mon_ctx.rtsp_context))
        {
            AMY_LOG_ERROR("record stop failed\n");
            mon_ctx.record_run = 0;
            return -1;
        }
    }
    return 0;
}

int monitor_video_record_mode_get(void)
{
    Amy_mutex_lock(&mon_ctx.mutex);
    int mode = mon_ctx.record_run ? (int)mon_ctx.record_mode : -1;
    Amy_mutex_unlock(&mon_ctx.mutex);
    return mode;
}

int monitor_channel_set(unsigned long chn)
{
    mon_ctx.cur_chn = chn;
    return 0;
}

int monitor_channel_get(void)
{
    return mon_ctx.cur_chn;
}
