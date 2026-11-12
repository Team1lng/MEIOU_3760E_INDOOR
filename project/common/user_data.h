#ifndef _USER_DATA_H_
#define _USER_DATA_H_

#include "stdbool.h"
#include <stdint.h>
#include <time.h>
#include "sat_ipcamera.h"

#define DOOR_DEVICE_MAX 2
#define VOLUME_DOOR_RING_TOTAL 3

// 管理员信息
#define ADMIN_PASSWORD_LEN 6
#define ADMIN_DEFAULT_PASSWORD "888888"

typedef struct
{
    char password[ADMIN_PASSWORD_LEN + 1];
} admin_info_t;

bool admin_password_validate(const char *pwd);

// 网络认证信息
typedef struct
{
    char number[128];
    char password[10];
} network_auth_info;

// 设备属性信息
typedef struct
{
    int device_type;
    int building;
    int room;
} device_attr_info;

typedef enum
{
    RECORD_MODE_OFF = 0,
    RECORD_MODE_PHOTO,
    RECORD_MODE_VIDEO,
    RECORD_MODE_PHOTO_VIDEO,
    RECORD_MODE_MAX
} record_mode_type_def;

typedef enum
{
    DETECT_LEVEL_OFF = 0, /* 关 */
    DETECT_LEVEL_LOW,     /* 低 */
    DETECT_LEVEL_MID,     /* 中 */
    DETECT_LEVEL_HIGH,    /* 高 */

    DETECT_LEVEL_MAX
} detect_level_type_def;

typedef enum
{
    RINGTONE_TYPE_1 = 1,
    RINGTONE_TYPE_2,
    RINGTONE_TYPE_3,
    RINGTONE_TYPE_4,
    RINGTONE_TYPE_5,
    RINGTONE_TYPE_6,
    RINGTONE_TYPE_MAX
} ringtone_type_def;

typedef enum
{
    RING_DURATION_20S = 0,
    RING_DURATION_30S,
    RING_DURATION_60S,
    RING_DURATION_MAX
} ring_duration_type_def;

typedef enum
{
    LOCK_SELECT_GATE = 0,
    LOCK_SELECT_GARAGE,
    LOCK_SELECT_BOTH,
    LOCK_SELECT_MAX
} lock_select_type_def;

typedef struct
{
    uint16_t start_time;
    uint16_t end_time;
} time_period_config_type_def;

typedef struct
{
    detect_level_type_def detect_level;
    uint32_t alarm_interval; /* 移动侦测时长(秒): 10~20 步进1, 30~300 步进10 */
    record_mode_type_def record_mode;
    time_period_config_type_def time_period[3];
} human_detect_config_type_def;

typedef struct
{
    int start_h;
    int start_m;
    int end_h;
    int end_m;
    ringtone_type_def type;
    ring_duration_type_def duration;
    uint8_t volume;
    uint8_t mode;   /* 0=Standard(内置), 1=Customized(SD 自定义) */
    uint8_t select; /* Customized 模式下选第几个音乐文件(1..100)；Standard 下不使用 */
} ringtone_config_type_def;

typedef struct
{
    char card_id[16];
    bool valid;
} access_card_info_type_def;

typedef struct
{
    uint8_t doorbell_ring_volume;
    record_mode_type_def global_record_mode;
    uint16_t gate_lock_close_time;
    uint16_t garage_lock_close_time;

    lock_select_type_def access_lock_select;
    uint16_t access_card_count;
    access_card_info_type_def access_cards[64];

    human_detect_config_type_def human_detect;
    ringtone_config_type_def ringtone[3];
    uint8_t indoor_ringback_volume;
    bool message_sw;
    uint8_t message_time;        /* 留言录制时长，单位秒 */
    uint8_t record_time;         /* 自动录像定时关闭时长，单位秒 */
    uint8_t indoor_talk_volume;  /* 室内机通话音量 1-10 */
    uint8_t outdoor_talk_volume; /* 门口机通话音量 1-10 */
    uint8_t display_brightness;  /* 背光亮度 0-100，per-door */
} door_config_type_def;

typedef struct
{
    network_auth_info auth;
    device_attr_info attr;
} device_core_info;

typedef struct
{
    door_config_type_def door_cfg;
    struct ipcamera_info camera_info;
    bool device_valid;
} device_single_cfg;

typedef enum
{
    LANG_ENGLISH = 0,
    LANG_CHINESE,
    LANG_MAX
} lang_type_def;

typedef enum
{
    SCREEN_SAVER_DISABLE = 0,
    SCREEN_SAVER_ENABLE,
    SCREEN_SAVER_MAX
} screen_saver_type_def;

typedef struct
{
    lang_type_def lang;
    uint32_t date;
    uint32_t time;
    bool auto_sync_network_time;
    uint8_t display_brightness;
    screen_saver_type_def screen_saver_en;
} system_setting_cfg;

typedef enum
{
    SCENE_MODE_HOME = 0,
    SCENE_MODE_AWAY,
    SCENE_MODE_SLEEP,
    SCENE_MODE_MAX
} scene_mode_type_def;

/* 硬件已移除有线网卡 (eth2)。置 1 可恢复 Cable 配网模式及有线链路相关代码，
 * 也可用 -DNET_SUPPORT_CABLE=1 在编译时覆盖。 */
#ifndef NET_SUPPORT_CABLE
#define NET_SUPPORT_CABLE 0
#endif

typedef enum
{
    PAIRING_MODE_WLAN = 0,
    PAIRING_MODE_WIRED,
    PAIRING_MODE_TOTAL
} pairing_mode_t;

typedef enum
{
    IP_ALLOC_STATIC = 0,
    IP_ALLOC_DHCP,
    IP_ALLOC_TOTAL
} ip_allocation_mode_t;

typedef struct
{
    bool reserved;
    scene_mode_type_def scene_mode;
} global_func_cfg;

typedef struct
{
    char pid[64];
    char uid[64];
    char key[64];
    char qr_info[64];
} tuya_conf_info_t;

typedef struct
{
    int temp;      // 当前温度
    int condition; // 天气状况编码
    int humidity;  // 湿度
    int pressure;  // 气压
    int pm10;      // PM10
    int pm25;      // PM2.5
    int aqi;       // 空气质量指数
    int thigh;     // 当日最高温
    int tlow;      // 当日最低温
    int is_valid;  // 数据是否有效（0：无效，1：有效）
} user_weather_t;

// 场景模式配置（数码相框 + 背景音乐）
typedef struct
{
    bool digital_photo_frame_sw;
    int digital_photo_sw_time; // 1-30 秒
    bool bg_music_sw;
    int bg_music_vol; // 1-100
} user_scene_info;

// ======================== 合并后的唯一配置结构体 ========================

// 公共配置 —— 主机 ↔ 分机 同步
typedef struct
{
    // 系统全局
    struct tm time;
    bool auto_sync_network_time;
    pairing_mode_t pairing_mode;
    ip_allocation_mode_t allocation_mode;

    // 全局功能
    global_func_cfg global_func;

    // 天气配置
    user_weather_t weather_info;

    // 氛围灯
    bool atmosphere;

    // 门口机配置
    device_single_cfg door_device[DOOR_DEVICE_MAX];

    // 网络摄像头
    struct ipcamera_info net_door_device[DOOR_DEVICE_MAX];
    struct ipcamera_info net_cctv_device[DOOR_DEVICE_MAX];

    // CCTV 通道背光亮度 0-100
    uint8_t cctv_display_brightness[DOOR_DEVICE_MAX];

} sys_public_cfg_t;

// 私有配置 —— 每个设备自己的，不参与同步
typedef struct
{
    // 管理员
    admin_info_t admin;
    // 设备唯一身份
    device_core_info device;
    // 涂鸦
    tuya_conf_info_t tuya_info;

    // 本地显示设置
    lang_type_def lang;
    uint8_t display_brightness;
    screen_saver_type_def screen_saver_en;
    bool mute_enable;

    uint8_t date_format;           /* 0=YY-MM-DD, 1=MM-DD-YY, 2=DD-MM-YY */
    bool keytone_enable;           /* 按键音开关 */
    bool detection_preview_enable; /* 移动侦测预览开关 */
    uint8_t unlock_delay;          /* 开锁延时 1-10 秒 */
    bool unlock_voice_enable;      /* 门口机开锁是否播报开锁铃声 */

    user_scene_info scene;
} sys_private_cfg_t;

typedef struct
{
    sys_public_cfg_t public_cfg;   // 公共：会同步
    sys_private_cfg_t private_cfg; // 私有：不同步
} user_data_info;

bool user_data_save(void);
bool user_data_init(void);
user_data_info *user_data_get(void);
void user_data_reset(void);
void scene_mode_switch_to(scene_mode_type_def new_mode);

/* 判断指定时刻是否落在门口机任一铃声时段内（跨零点时段 end<=start 按跨天处理） */
bool ringtone_schedule_active(const door_config_type_def *cfg, int hour, int min);

/* 返回命中时段索引 0/1/2，未命中返回 -1 */
int ringtone_schedule_active_index(const door_config_type_def *cfg, int hour, int min);

#endif