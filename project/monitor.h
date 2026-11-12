#ifndef __APP_MONITOR_H__
#define __APP_MONITOR_H__
#include "Amy_media_mgr.h"
#define is_channel_ipc_camera(ch) (((ch >= 2 && ch <= 3)) ? 1 : (ch >= 0 && ch <= 1) ? 0 \
                                                                                     : -1)
#define LOCAL_MEDIA_DIR "/home/data"
#define MEDIA_LOCAL_MAX_FILES 30
typedef enum
{
    MON_CHANNEL_UNKNOW = -1,
    MON_CHANNEL_DOOR1,
    MON_CHANNEL_DOOR2,
    MON_CHANNEL_CCTV1,
    MON_CHANNEL_CCTV2,
} monitor_channel_e;

int monitor_video_open(void);

int monitor_video_close(void);

/** 设置门通话本地（室内机）通话音量（刻度 1~10，RTSP 已打开时立即生效） */
int monitor_talk_volume_set(int volume);

int monitor_video_snapshort(unsigned int mode, bool save_media);

int monitor_video_record_start(unsigned int mode);

int monitor_video_record_stop(void);

/** 获取当前录像模式：正在录像返回 record_mode(MEDIA_MODE_*)，未录像返回 -1 */
int monitor_video_record_mode_get(void);

int monitor_channel_set(unsigned long chn);

int monitor_channel_get(void);

int door_monitor_first_valid_channel_get();

int cctv_monitor_first_valid_channel_get();

int monitor_first_valid_channel_get(void);

int monitor_channel_is_valid(int channel);

int monitor_next_valid_channel_get(int cur_channel);

int monitor_next_valid_channel_same_type_get(int cur_channel);

int door_monitor_channel_get_by_device_type(int device_type);
#endif
