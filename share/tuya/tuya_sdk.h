#ifndef _TUYA_SDK_H_
#define _TUYA_SDK_H_
#include "stdbool.h"

/**
 * @brief 涂鸦云端 MQTT / 链路综合状态（由 tuya_status_change 根据 IPC 状态组推导）
 * @note 与「仅在线/离线」二元判断区分：离线时区分本地网络链路是否已建立
 */
typedef enum
{
    TUYA_MQTT_LINK_UNKNOWN = 0,      /**< 尚未收到有效状态 */
    TUYA_MQTT_LINK_OFFLINE_NET_FAIL, /**< 云端离线且无可用本地链路（WiFi 未连且有线未连） */
    TUYA_MQTT_LINK_OFFLINE_NET_OK,   /**< 云端离线但本地链路已通（STA 已连或有线已连），外网/云端待恢复 */
    TUYA_MQTT_LINK_ONLINE,           /**< 云端 MQTT 在线 */
} tuya_mqtt_link_state_e;
/**
 * @brief 天气数据结构体
 */
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
} tuya_api_weather;

int tuya_sdk_init(const char *pid, const char *uuid, const char *key);

/** @return tuya_mqtt_link_state_e */
int tuya_mqtt_link_state_get(void);

/** @return 1 当且仅当 TUYA_MQTT_LINK_ONLINE */
#define tuya_mqtt_is_online() (tuya_mqtt_link_state_get() == TUYA_MQTT_LINK_ONLINE)

int tuya_video_main_ringbuffer_put(unsigned char *data, int size, unsigned long long pts, int keyframe);

int tuya_video_sub_ringbuffer_put(unsigned char *data, int size, unsigned long long pts, int keyframe);

int tuya_audio_main_ringbuffer_put(unsigned char *data, int size, unsigned long long pts);

bool tuya_api_get_weather(tuya_api_weather *weather);

bool tuya_api_get_qrcode(char *qrcode, int buf_len);

int tuya_api_client_num(void);

bool tuya_api_time_sync(void);

bool tuya_api_call_event(int channel, const char *jpg, int size);

bool tuya_api_human_alarm_event(int channel, const char *jpg, int size);

/* tuya_sdk.h 或对应头文件中添加 */
void tuya_api_set_time_sync_enable(int enable); /* 0=禁用, 1=启用 */

int tuya_api_get_time_sync_enable(void);

int tuya_media_stream_pause(void);

/**
 * @brief 主动上报子设备版本到涂鸦云
 * @param tp 子设备通道号（DEV_ATTACH_MOD_1 等）
 * @param ver 版本字符串
 */
void tuya_subdev_version_report(int tp, const char *ver);
#endif
