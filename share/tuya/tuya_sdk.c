#include "tuya_sdk.h"
#include "tuya_ipc_event.h"
#include "tuya_ipc_sdk_init.h"
#include "tuya_ipc_upgrade.h"
#include "tuya_cloud_com_defs.h"
#include "tuya_ipc_media_adapter.h"
#include "tuya_ipc_video_msg.h"
#include "tuya_ring_buffer.h"
#include "tkl_media.h"
#include "tuya_svc_netmgr_linkage.h"
#include "tuya_ipc_p2p.h"
#include "Amy_log.h"
#include "Amy_sdk.h"
#include "tuya_ipc_media.h"
#include "tuya_audio_recv.h"
#include "tuya_dp_handle.h"
#include "cloud_httpc.h"
#include "tuya_ipc_media_stream.h"
#include "tuya_devos_utils.h"
#include <sys/stat.h>
#include <unistd.h>
#define TUYA_STORAGE_PATH "/home/tuya/"

static void *ringbuffer_video_main_handler = NULL;
static void *ringbuffer_video_sub_handler = NULL;
static void *ringbuffer_audio_main_handler = NULL;
static char qrcode_shorturl[128] = {0};
/* 是否允许同步涂鸦云时间，默认开启 */
static int g_time_sync_enable = 0;
static Amy_mutex_t g_time_sync_mutex = PTHREAD_MUTEX_INITIALIZER;

int tuya_ipc_call_init(void);
int tuya_stream_storage_init(void);
int tuya_media_stream_init(void);

void ipc_status_change_cb(TUYA_IPC_STATUS_GROUP_E changed_group, const TUYA_IPC_STATUS_E status[TUYA_IPC_STATUS_GROUP_MAX]);

// 不定义原生dp
static void raw_dp_receive_cb(const TY_RECV_RAW_DP_S *dps)
{
    AMY_LOG_INFO("receive raw dp:%d\n", dps->dpid);
}

static void dp_receive_cb(const TY_RECV_OBJ_DP_S *dps)
{
    tuya_handle_dp_objs(dps);
}

static void dp_query_cb(const TY_DP_QUERY_S *dps)
{
    for (int i = 0; i < dps->cnt; i++)
    {
        AMY_LOG_INFO("query dp:%d\n", dps->dpid[i]);
        int value = 1;
        //  respone_dp_value( dps->dpid[i], 1);
        tuya_ipc_dp_report(NULL, dps->dpid[i], PROP_VALUE, &value, 1);
    }
}

extern int ipc_ugprade_cb(const FW_UG_S *fw);

static void ipc_rest_cb(GW_RESET_TYPE_E type)
{
    AMY_LOG_INFO("reset ipc success,please restart the ipc:%d\n", type);
    // 清除涂鸦缓存，使设备恢复未绑定状态，重新配网
    system("rm -rf /home/tuya/tuya*");
    sync();
    system("reboot");
}

static void ipc_restart_cb(void)
{
    AMY_LOG_INFO("sdk internal restart request. please restart the ipc\n");
    sync();
    system("reboot");
}

static void ipc_qrcode_cb(char *shorturl)
{
    AMY_LOG_INFO("<<<--------\n");
    if (shorturl)
    {
        AMY_LOG_INFO("<<<--------\n");
        char *p_str = strstr(shorturl, "https");
        if (p_str != NULL)
        {
            AMY_LOG_INFO("<<<--------\n");
            strncpy(qrcode_shorturl, p_str, strlen(p_str) - 2);
        }
        AMY_LOG_INFO("sdk qrcode:%s\n", shorturl);
    }
    return;
}

static int tuya_ipc_app_start(const char *pid, const char *uuid, const char *key)
{
    if (access(TUYA_STORAGE_PATH, F_OK) != 0)
    {
        mkdir(TUYA_STORAGE_PATH, 0644);
    }
    // 1. init sdk
    TUYA_IPC_ENV_VAR_T env;
    memset(&env, 0, sizeof(TUYA_IPC_ENV_VAR_T));
    // 缓存文件路径
    strncpy(env.storage_path, TUYA_STORAGE_PATH, IPC_STORAGE_PATH_LEN);
    // pid
    strncpy(env.product_key, pid, IPC_PRODUCT_KEY_LEN);
    // uuid
    strncpy(env.uuid, uuid, IPC_UUID_LEN);
    // key
    strncpy(env.auth_key, key, IPC_AUTH_KEY_LEN);
    // version
    strncpy(env.dev_sw_version, SOFT_VERSION, IPC_SW_VER_LEN);
    // sernumber
    strncpy(env.dev_serial_num, "tuya_ipc", IPC_SERIAL_NUM_LEN);

    // raw dp cmd callback function, which hanles cmd from TUYA cloud and APP
    // 来自云和app端 raw dp命令进行处理
    env.dev_raw_dp_cb = raw_dp_receive_cb;
    // 来自云和app端的dp命令进行处理
    env.dev_obj_dp_cb = dp_receive_cb;
    // dp命令查询
    env.dev_dp_query_cb = dp_query_cb;
    // 状态改变
    env.status_changed_cb = ipc_status_change_cb;
    // 升级
    env.upgrade_cb_info.upgrade_cb = ipc_ugprade_cb;
    // 复位数据
    env.gw_rst_cb = ipc_rest_cb;
    // 重启
    env.gw_restart_cb = ipc_restart_cb;
    // 二维码获取短链
    env.qrcode_active_cb = ipc_qrcode_cb;
    env.dev_type = NORMAL_POWER_DEV;
    env.link_type = TUYA_IPC_LINK_WIRE;
    env.ip_mode_type = TUYA_IPC_IPV4_ONLY;
    if (tuya_ipc_init_sdk(&env))
    {
        AMY_LOG_ERROR("init sdk error\n");
        return -1;
    }
    return 0;
}

static void ipc_snashot_cb(int device, int channel, char *snap_addr, int *snap_size)
{
    AMY_LOG_ERROR("ipc snashort device:%d,chn:%d,addr:%#x,maxsize:%d\n", device, channel, snap_addr, *snap_size);
}

static void ipc_recv_audio_cb(int device, int chn, const MEDIA_AUDIO_FRAME_T *frame)
{
    tuya_recv_audio_frame_write(frame->p_audio_buf, frame->buf_len);
    // AMY_LOG_ERROR("recv audio ,size:%d,codec:%d,sample:%d,databits:%d,chn:%d\n", frame->buf_len, frame->audio_codec, frame->audio_sample, frame->audio_databits, frame->audio_channel);
}
static void ipc_recv_video_cb(int device, int chn, const MEDIA_VIDEO_FRAME_T *frame)
{
    AMY_LOG_ERROR("recv video ,size:%d,codec:%d,frame type:%d\n", frame->buf_len, frame->video_codec, frame->video_frame_type);
}

static void ipc_recv_file_cb(int device, int chn, const MEDIA_FILE_DATA_T *file_data)
{
    AMY_LOG_INFO("recv file data:%s\n", file_data);
}
static int tuya_ipc_media_init(void)
{
    TUYA_IPC_MEDIA_ADAPTER_VAR_T var;
    memset(&var, 0, sizeof(TUYA_IPC_MEDIA_ADAPTER_VAR_T));
    var.get_snapshot_cb = ipc_snashot_cb;
    var.on_recv_audio_cb = ipc_recv_audio_cb;
    var.on_recv_video_cb = ipc_recv_video_cb;
    var.on_recv_file_cb = ipc_recv_file_cb;
    var.available_media_memory = 2;
    tuya_ipc_media_adapter_init(&var);

    // 设置媒体信息
    DEVICE_MEDIA_INFO_T media_info;
    memset(&media_info, 0, sizeof(DEVICE_MEDIA_INFO_T));
    media_info.av_encode_info.audio_codec[E_IPC_STREAM_AUDIO_MAIN] = TUYA_CODEC_AUDIO_G711U;
    media_info.av_encode_info.audio_sample[E_IPC_STREAM_AUDIO_MAIN] = TUYA_AUDIO_SAMPLE_16K;
    media_info.av_encode_info.audio_databits[E_IPC_STREAM_AUDIO_MAIN] = TUYA_AUDIO_DATABITS_16;
    media_info.av_encode_info.audio_channel[E_IPC_STREAM_AUDIO_MAIN] = TUYA_AUDIO_CHANNEL_MONO;

    media_info.audio_decode_info.enable = 1;
    media_info.audio_decode_info.audio_codec = TUYA_CODEC_AUDIO_G711U;
    media_info.audio_decode_info.audio_sample = TUYA_AUDIO_SAMPLE_16K;
    media_info.audio_decode_info.audio_databits = TUYA_AUDIO_DATABITS_16;
    media_info.audio_decode_info.audio_channel = TUYA_AUDIO_CHANNEL_MONO;
    media_info.max_pic_len = 100;
#if 1 // 上报视频解码能力，用于双向对讲
    TUYA_DECODER_T v_codec;
    memset(&v_codec, 0, sizeof(TUYA_DECODER_T));
    v_codec.codec_id = TUYA_CODEC_VIDEO_H264;
    v_codec.decoder_desc.v_decoder.height = 320;
    v_codec.decoder_desc.v_decoder.width = 240;
    v_codec.decoder_desc.v_decoder.profile = VIDEO_AVC_PROFILE_BASE_LINE;
    media_info.decoder_cnt = 1;
    media_info.decoders = &v_codec;
#endif
    tuya_ipc_media_adapter_set_media_info(0, 0, media_info);
    return 0;
}

static int tuya_ipc_ringbuffer_init(void)
{
    RING_BUFFER_INIT_PARAM_T param;

    // 视频主码流
    memset(&param, 0, sizeof(RING_BUFFER_INIT_PARAM_T));
    param.bitrate = TUYA_VIDEO_BITRATE_2M;
    param.fps = 30;
    param.max_buffer_seconds = 0;
    param.request_key_frame_cb = NULL;
    tuya_ipc_ring_buffer_init(0, 0, E_IPC_STREAM_VIDEO_MAIN, &param);
#if 0
    //次码流
    memset(&param, 0, sizeof(RING_BUFFER_INIT_PARAM_T));
    param.bitrate = TUYA_VIDEO_BITRATE_512K;
    param.fps = 30;
    param.max_buffer_seconds = 0;
    param.request_key_frame_cb = NULL;
    AMY_LOG_ERROR("main video bitrate %d,fps:%d\n", param.bitrate, param.fps);
    tuya_ipc_ring_buffer_init(0, 0, E_IPC_STREAM_VIDEO_SUB, &param);
#endif
    // 音频流
    memset(&param, 0, sizeof(RING_BUFFER_INIT_PARAM_T));
    param.bitrate = TUYA_AUDIO_SAMPLE_8K * TUYA_AUDIO_DATABITS_16 / 1024;
    param.fps = 25;
    param.max_buffer_seconds = 0;
    param.request_key_frame_cb = NULL;
    tuya_ipc_ring_buffer_init(0, 0, E_IPC_STREAM_AUDIO_MAIN, &param);

    tuya_ipc_ring_buffer_adapter_register_media_source();
    return 0;
}

static tuya_api_weather g_weather_data;
static Amy_mutex_t g_weather_mutex = PTHREAD_MUTEX_INITIALIZER;
static int g_weather_fail_count = 0;

/**
 * @brief 内部天气获取函数
 * @param nm 输出参数，存储获取到的天气数据
 * @return 获取成功返回true，失败返回false
 */
static bool _tuya_api_weather_get(tuya_api_weather *nm)
{
    if (nm == NULL)
    {
        AMY_LOG_ERROR("weather param is NULL");
        return false;
    }

    if (!tuya_mqtt_is_online())
    {
        AMY_LOG_ERROR("device is offline, can't get weather");
        return false;
    }

    // 构造天气查询参数
    const char *weather_param =
        "{                              \
        \"codes\":[                 \
            \"w.date.1\",           \
            \"w.currdate\",         \
            \"w.conditionNum\",     \
            \"w.temp\",             \
            \"w.humidity\",         \
            \"w.pressure\",         \
            \"w.pm10\",             \
            \"w.pm25\",             \
            \"w.aqi\",              \
            \"w.thigh\",            \
            \"w.tlow\",             \
            \"c.city\"              \
        ]                           \
    }";

    cJSON *result = NULL;
    OPERATE_RET ret = tuya_http_gw_ipc_custom_msg("tuya.device.public.data.get", "1.0", weather_param, &result);
    if (ret != OPRT_OK || result == NULL)
    {
        AMY_LOG_ERROR("call weather api failed, ret:%d", ret);
        return false;
    }

    // 解析天气数据
    cJSON *data_json = cJSON_GetArrayItem(result, 0);
    if (data_json == NULL)
    {
        AMY_LOG_ERROR("weather data json parse failed");
        cJSON_Delete(result);
        return false;
    }

    // 初始化天气结构体
    memset(nm, 0, sizeof(tuya_api_weather));
    nm->is_valid = 0;

    // 解析各字段（增加空指针检查，提升健壮性）
    cJSON *temp_json = cJSON_GetObjectItem(data_json, "w.temp");
    if (temp_json != NULL && cJSON_IsNumber(temp_json))
    {
        nm->temp = temp_json->valueint;
    }

    cJSON *condition_json = cJSON_GetObjectItem(data_json, "w.conditionNum");
    if (condition_json != NULL && cJSON_IsString(condition_json))
    {
        sscanf(condition_json->valuestring, "%d", &(nm->condition));
    }

    cJSON *humidity_json = cJSON_GetObjectItem(data_json, "w.humidity");
    if (humidity_json != NULL && cJSON_IsNumber(humidity_json))
    {
        nm->humidity = humidity_json->valueint;
    }

    cJSON *pressure_json = cJSON_GetObjectItem(data_json, "w.pressure");
    if (pressure_json != NULL && cJSON_IsNumber(pressure_json))
    {
        nm->pressure = pressure_json->valueint;
    }

    cJSON *pm10_json = cJSON_GetObjectItem(data_json, "w.pm10");
    if (pm10_json != NULL && cJSON_IsNumber(pm10_json))
    {
        nm->pm10 = pm10_json->valueint;
    }

    cJSON *pm25_json = cJSON_GetObjectItem(data_json, "w.pm25");
    if (pm25_json != NULL && cJSON_IsNumber(pm25_json))
    {
        nm->pm25 = pm25_json->valueint;
    }

    cJSON *aqi_json = cJSON_GetObjectItem(data_json, "w.aqi");
    if (aqi_json != NULL && cJSON_IsNumber(aqi_json))
    {
        nm->aqi = aqi_json->valueint;
    }

    cJSON *thigh_json = cJSON_GetObjectItem(data_json, "w.thigh.0");
    if (thigh_json != NULL && cJSON_IsNumber(thigh_json))
    {
        nm->thigh = thigh_json->valueint;
    }

    cJSON *tlow_json = cJSON_GetObjectItem(data_json, "w.tlow.0");
    if (tlow_json != NULL && cJSON_IsNumber(tlow_json))
    {
        nm->tlow = tlow_json->valueint;
    }
    nm->is_valid = 1;

    // 释放JSON内存
    cJSON_Delete(result);
    return true;
}

static void *tuya_sdk_thread(void *arg)
{

    // 设置日志等级
    tuya_ipc_set_log_attr(TY_IPC_LOG_LEVEL_ERR, NULL);

    // 设置媒体信息
    tuya_ipc_media_init();

    // 创建循环buffer
    tuya_ipc_ringbuffer_init();

    // 创建app音频接收缓冲队列
    tuya_recv_audio_ringbuffer_init();

    TUYA_IPC_LINK_TYPE_E link_type = PARING_MODE_WIRED;
    if (tuya_ipc_start_sdk(link_type, NULL))
    {
        AMY_LOG_ERROR("tuya ipc start sdk failed\n");
        return NULL;
    }
    if (TUYA_IPC_LINK_WIRE == link_type)
    {
        // if only use wire for paring
        tuya_svc_netmgr_linkage_set_default(LINKAGE_TYPE_WIRED);
    }
    // 这里开启tuya的移动侦测和录制功能
    //....

    while (!tuya_mqtt_is_online())
    {
        Amy_msleep(1000);
    }
    AMY_LOG_INFO("mqtt is start run\n");

    TIME_T time_utc;
    int time_zone;
    int ret = 0;
    do
    {
        ret = tuya_ipc_get_service_time_force(&time_utc, &time_zone);
        if (ret == 0 && tuya_api_get_time_sync_enable())
        {
            struct timeval tv;
            tv.tv_sec = time_utc + time_zone;
            tv.tv_usec = 0;
            settimeofday(&tv, NULL);
            system("hwclock -w");
            struct tm tm;
            Amy_time2localtime(tv.tv_sec, tm);
            AMY_LOG_INFO("sync service time:%04d-%02d-%02d %02d:%02d:%02d\n",
                         tm.tm_year, tm.tm_mon, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
        }
    } while (ret);

    tuya_media_stream_init();

    // 事件初始化
    TUYA_ALARM_BITMAP_T alarm_of_events[1];
    memset(alarm_of_events, 0, sizeof(alarm_of_events));
    tuya_ipc_event_module_init(sizeof(alarm_of_events) / sizeof(TUYA_ALARM_BITMAP_T), alarm_of_events, 1);

    // 本地存初始化
    tuya_stream_storage_init();

    // 门铃消息
    tuya_ipc_video_msg_init(MSG_BOTH, 10);

    // ai detection 模块暂时不启用
    // tuya_ipc_rule_init();

    // 云端存储初始化
    tuya_ipc_cloud_storage_init();

    // 云存储功能暂时不启用

    // tuya call处理
    tuya_ipc_call_init();

    // 重传各种状态
    // dp信息上传同步

    // 上传设备信息
    tuya_ipc_upload_skills();

    ringbuffer_video_main_handler = tuya_ipc_ring_buffer_open(0, 0, E_IPC_STREAM_VIDEO_MAIN, E_RBUF_WRITE);
    if (!ringbuffer_video_main_handler)
    {
        AMY_LOG_ERROR("open ringbuffer video main stream failed\n");
    }
#if 0
    ringbuffer_video_sub_handler = tuya_ipc_ring_buffer_open(0, 0, E_IPC_STREAM_VIDEO_SUB, E_RBUF_WRITE);
    if (!ringbuffer_video_sub_handler) {
        AMY_LOG_ERROR("open ringbuffer video sub stream failed\n");
    }
#endif
    ringbuffer_audio_main_handler = tuya_ipc_ring_buffer_open(0, 0, E_IPC_STREAM_AUDIO_MAIN, E_RBUF_WRITE);
    if (!ringbuffer_audio_main_handler)
    {
        AMY_LOG_ERROR("open ringbuffer audio main stream failed\n");
    }
    while (1)
    {
        tuya_api_weather temp_data;
        if (_tuya_api_weather_get(&temp_data) == true)
        {
            g_weather_fail_count = 0;
            Amy_mutex_lock(&g_weather_mutex);
            memcpy(&g_weather_data, &temp_data, sizeof(tuya_api_weather));
            Amy_mutex_unlock(&g_weather_mutex);
        }
        else
        {
            g_weather_fail_count++;
            if (g_weather_fail_count >= 10)
            {
                Amy_mutex_lock(&g_weather_mutex);
                g_weather_data.is_valid = 0;
                Amy_mutex_unlock(&g_weather_mutex);
            }
        }
        Amy_msleep(60000);
    }
    return NULL;
}

bool tuya_api_get_weather(tuya_api_weather *weather)
{
    if (weather == NULL)
    {
        AMY_LOG_ERROR("weather pointer is NULL");
        return false;
    }
    // MQTT离线时天气必然无法抓取，立即将实时缓存置为无效，避免返回过期数据
    if (!tuya_mqtt_is_online())
    {
        pthread_mutex_lock(&g_weather_mutex);
        g_weather_data.is_valid = 0;
        pthread_mutex_unlock(&g_weather_mutex);
    }
    // 加锁读取缓存数据（保证线程安全）
    pthread_mutex_lock(&g_weather_mutex);
    memcpy(weather, &g_weather_data, sizeof(tuya_api_weather));
    pthread_mutex_unlock(&g_weather_mutex);

    // 仅当数据有效时返回true
    return (weather->is_valid == 1) ? true : false;
}

bool tuya_api_get_qrcode(char *qrcode, int buf_len)
{
    // 1. 参数检查
    if (qrcode == NULL || buf_len == 0)
    {
        AMY_LOG_ERROR("qrcode pointer or buf_len invalid");
        return false;
    }

    // 2. 长度检查（防止越界）
    uint32_t qr_len = strlen(qrcode_shorturl);
    if (qr_len + 1 > buf_len) // +1 是给 \0 留空间
    {
        AMY_LOG_ERROR("qrcode buffer too small, need %u", qr_len + 1);
        return false;
    }

    // 3. 拷贝（包含结束符）
    memcpy(qrcode, qrcode_shorturl, qr_len + 1);

    return true;
}

int tuya_sdk_init(const char *pid, const char *uuid, const char *key)
{

    static int inited = 0;
    if (inited)
    {
        return 0;
    }
    inited = 1;

    /*event function (essential)*/
    TUYA_ALARM_BITMAP_T alarm_of_events[1];
    memset((void *)alarm_of_events, 0, sizeof(alarm_of_events));
    tuya_ipc_event_add_alarm_types(&alarm_of_events[0], E_ALARM_MOTION);

    // 初始化sdk
    if (tuya_ipc_app_start(pid, uuid, key))
    {
        AMY_LOG_ERROR("tuya sdk start failed\n");
        return -1;
    }

    Amy_thread_t tid;
    Amy_pthread_create(&tid, tuya_sdk_thread, NULL, 50 * 1024, 12);
    return 0;
}

int tuya_video_main_ringbuffer_put(unsigned char *data, int size, unsigned long long pts, int keyframe)
{
    if (!ringbuffer_video_main_handler)
    {
        return -1;
    }

    unsigned int num;
    CLIENT_CONNECT_INFO_T *pinfo;
    tuya_ipc_get_client_conn_info(&num, &pinfo);
    if (num < 1)
    {
        return -1;
    }
    return tuya_ipc_ring_buffer_append_data(ringbuffer_video_main_handler, data, size, keyframe ? TKL_VIDEO_I_FRAME : TKL_VIDEO_PB_FRAME, pts);
}

int tuya_video_sub_ringbuffer_put(unsigned char *data, int size, unsigned long long pts, int keyframe)
{
    if (!ringbuffer_video_sub_handler)
    {
        return -1;
    }
    unsigned int num;
    CLIENT_CONNECT_INFO_T *pinfo;
    tuya_ipc_get_client_conn_info(&num, &pinfo);
    if (num < 1)
    {
        return -1;
    }
    return tuya_ipc_ring_buffer_append_data(ringbuffer_video_sub_handler, data, size, keyframe ? TKL_VIDEO_I_FRAME : TKL_VIDEO_PB_FRAME, pts);
}

int tuya_audio_main_ringbuffer_put(unsigned char *data, int size, unsigned long long pts)
{
    if (!ringbuffer_audio_main_handler)
    {
        return -1;
    }
    unsigned int num;
    CLIENT_CONNECT_INFO_T *pinfo;
    tuya_ipc_get_client_conn_info(&num, &pinfo);
    if (num < 1)
    {
        return -1;
    }
    return tuya_ipc_ring_buffer_append_data(ringbuffer_audio_main_handler, data, size, TKL_AUDIO_FRAME, pts);
}

bool tuya_api_call_event(int channel, const char *jpg, int size)
{
    if (!tuya_mqtt_is_online())
    {
        return false;
    }
    tuya_ipc_door_bell_press(DOORBELL_NORMAL, jpg, size, NOTIFICATION_CONTENT_JPEG);
    return true;
}

bool tuya_api_human_alarm_event(int channel, const char *jpg, int size)
{
    if (!tuya_mqtt_is_online())
    {
        return false;
    }
    tuya_ipc_notify_alarm(jpg, size, NOTIFICATION_NAME_MOTION, true, NULL);
    return true;
}

bool tuya_api_time_sync(void)
{
    /* 检查时间同步开关 */
    if (tuya_api_get_time_sync_enable() == 0)
    {
        AMY_LOG_INFO("time sync is disabled, skip\n");
        return false;
    }

    if (tuya_mqtt_is_online() == false)
    {
        return false;
    }

    TIME_T time_utc;
    int time_zone;
    bool result = false;
    do
    {
        int ret = tuya_ipc_get_service_time_force(&time_utc, &time_zone);
        if (ret == OPRT_OK)
        {
            struct timeval tv;
            tv.tv_sec = time_utc + time_zone;
            tv.tv_usec = 0;
            settimeofday(&tv, NULL);
            system("hwclock -w");
            struct tm tm;
            Amy_time2localtime(tv.tv_sec, tm);
            result = true;
            AMY_LOG_INFO("sync service time:%04d-%02d-%02d %02d:%02d:%02d\n",
                         tm.tm_year, tm.tm_mon, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
        }
    } while (0);

    return result;
}

int tuya_api_client_num(void)
{
    if (tuya_mqtt_is_online() == false)
    {
        return 0;
    }

    return tuya_ipc_get_client_online_num();
}

void tuya_api_set_time_sync_enable(int enable)
{
    Amy_mutex_lock(&g_time_sync_mutex);
    g_time_sync_enable = (enable != 0) ? 1 : 0;
    Amy_mutex_unlock(&g_time_sync_mutex);
    AMY_LOG_INFO("tuya time sync %s\n", g_time_sync_enable ? "enabled" : "disabled");
}

int tuya_api_get_time_sync_enable(void)
{
    int val;
    Amy_mutex_lock(&g_time_sync_mutex);
    val = g_time_sync_enable;
    Amy_mutex_unlock(&g_time_sync_mutex);
    return val;
}

void tuya_subdev_version_report(int tp, const char *ver)
{
    GW_ATTACH_ATTR_T attr = {0};

    strncpy(attr.ver, ver, SW_VER_LEN);
    attr.ver[SW_VER_LEN] = '\0';
    attr.tp = (GW_PERMIT_DEV_TP_T)tp + DEV_ATTACH_MOD_1;
    tuya_iot_dev_update_attachs(1, &attr);
    AMY_LOG_INFO("tuya: report subdev ver tp=%d ver=%s\n", tp, attr.ver);
}