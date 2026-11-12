#include "Amy_log.h"
#include "Amy_sdk.h"
#include "intercom.h"
#include "live/testRTSPClient.h"
#include "sat_user_common.h"

#define INTERCOM_RTSP_WIDTH 0    // （音频可设0）
#define INTERCOM_RTSP_HEIGHT 0   // （音频可设0）
#define INTERCOM_URL_BUF_LEN 128 // RTSP URL缓冲区长度

// 默认音量值（根据你的SDK实际范围调整）
#define INTERCOM_DEFAULT_VOLUME 80

typedef struct
{
    Amy_mutex_t mutex;                 // 互斥锁（保护上下文线程安全）
    rtsp_client_context *rtsp_context; // RTSP客户端上下文
    device_info_attr device_info;      // 当前通话的目标设备信息
    int is_audio_open;                 // 音频通道是否已打开（状态标记）
    int volume_cache;                  // 音量缓存（未打开设备时也生效）
} intercom_context;

// 静态上下文初始化（增加状态标记）
static intercom_context intercom_ctx = {
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .rtsp_context = NULL,
    .is_audio_open = 0,
    .volume_cache = INTERCOM_DEFAULT_VOLUME // 初始默认音量
};

/**
 * @brief 生成内线通话的RTSP音频URL
 * @param device_info 目标设备信息
 * @param out_url 输出URL的缓冲区
 * @param out_len 输出缓冲区长度
 * @return 0-成功，-1-失败
 */
static int intercom_url_generate(device_info_attr device_info, char *out_url, int out_len)
{
    // 入参合法性校验
    if (out_url == NULL || out_len < INTERCOM_URL_BUF_LEN)
    {
        AMY_LOG_ERROR("intercom URL buffer is invalid (len:%d)", out_len);
        return -1;
    }

    char ip[32] = {0};
    // 生成目标设备IP（校验生成结果）
    if (generate_ip(device_info.building, device_info.room, device_info.device_type, ip) != 0)
    {
        AMY_LOG_ERROR("generate intercom IP failed (building:%d, room:%d, type:%d)",
                      device_info.building, device_info.room, device_info.device_type);
        return -1;
    }

    // 格式化RTSP URL（安全的snprintf，避免缓冲区溢出）
    snprintf(out_url, out_len, "rtsp://%s:8554/audio", ip);
    AMY_LOG_INFO("intercom RTSP URL generated: %s", out_url);
    return 0;
}

static int intercom_rtsp_close_internal(void)
{
    int ret = 0;
    if (Amy_mutex_lock(&intercom_ctx.mutex) != 0)
    {
        AMY_LOG_ERROR("intercom mutex lock failed when close RTSP");
        return -1;
    }

    if (intercom_ctx.rtsp_context != NULL)
    {
        live555_rtsp_client_close(intercom_ctx.rtsp_context);
        intercom_ctx.rtsp_context = NULL;
        intercom_ctx.is_audio_open = 0; // 重置状态
        AMY_LOG_INFO("intercom RTSP audio closed successfully");
    }

    Amy_mutex_unlock(&intercom_ctx.mutex);
    return ret;
}

/**
 * @brief 内部函数：将缓存的音量同步到RTSP设备
 */
static void intercom_volume_apply(void)
{
    if (intercom_ctx.rtsp_context != NULL)
    {
        live555_rtsp_request_volume_set(intercom_ctx.rtsp_context, intercom_ctx.volume_cache);
        AMY_LOG_INFO("intercom volume applied to device: %d", intercom_ctx.volume_cache);
    }
}

int intercom_audio_open(void)
{
    if (intercom_rtsp_close_internal() != 0)
    {
        AMY_LOG_WARNING("close old intercom RTSP failed, continue to open new");
    }

    char rtsp_url[INTERCOM_URL_BUF_LEN] = {0};
    if (intercom_url_generate(intercom_ctx.device_info, rtsp_url, sizeof(rtsp_url)) != 0)
    {
        AMY_LOG_ERROR("intercom URL generate failed");
        return -1;
    }

    if (Amy_mutex_lock(&intercom_ctx.mutex) != 0)
    {
        AMY_LOG_ERROR("intercom mutex lock failed when open audio");
        return -1;
    }
    intercom_ctx.rtsp_context = live555_rtsp_client_start(
        rtsp_url, "br0", 0, 0, INTERCOM_RTSP_WIDTH, INTERCOM_RTSP_HEIGHT);
    if (intercom_ctx.rtsp_context == NULL)
    {
        AMY_LOG_ERROR("intercom RTSP client start failed (url:%s)", rtsp_url);
        Amy_mutex_unlock(&intercom_ctx.mutex);
        return -1;
    }

    intercom_ctx.is_audio_open = 1;

    intercom_volume_apply();

    open_server_speaker();
    open_client_speaker();
    AMY_LOG_INFO("intercom RTSP audio open success (device: %d-%d-%d)",
                 intercom_ctx.device_info.building, intercom_ctx.device_info.room, intercom_ctx.device_info.device_type);

    Amy_mutex_unlock(&intercom_ctx.mutex);
    return 0;
}

int intercom_audio_close(void)
{
    return intercom_rtsp_close_internal();
}

/**
 * @brief 设置当前内线通话的设备信息
 * @param info 要设置的设备信息
 * @return 0-成功，-1-参数错误/加锁失败
 */
int intercom_device_info_set(const device_info_attr *info)
{
    // 1. 参数检查
    if (info == NULL)
    {
        AMY_LOG_ERROR("intercom set device info: info is NULL");
        return -1;
    }

    // 2. 互斥锁保护
    if (Amy_mutex_lock(&intercom_ctx.mutex) != 0)
    {
        AMY_LOG_ERROR("intercom mutex lock failed when set device info");
        return -1;
    }

    // 3. 直接赋值（框架统一维护）
    intercom_ctx.device_info = *info;

    Amy_mutex_unlock(&intercom_ctx.mutex);
    return 0;
}

/**
 * @brief 获取当前内线通话的设备信息
 * @param out_info 输出设备信息的缓冲区
 * @return 0-成功（已打开），-1-失败（未打开/参数错误）
 */
int intercom_get_current_device(device_info_attr *out_info)
{
    if (out_info == NULL)
    {
        AMY_LOG_ERROR("intercom get device info: out_info is NULL");
        return -1;
    }

    if (Amy_mutex_lock(&intercom_ctx.mutex) != 0)
    {
        AMY_LOG_ERROR("intercom mutex lock failed when get device info");
        return -1;
    }

    *out_info = intercom_ctx.device_info;
    Amy_mutex_unlock(&intercom_ctx.mutex);
    return 0;
}

/**
 * @brief 设置通话音量（未打开设备也生效，缓存参数）
 * @param volume 音量值（范围按你的SDK定义）
 * @return 0-成功，-1-参数错误/加锁失败
 */
int intercom_volume_set(int volume)
{
    // 音量范围校验（根据实际需求修改）
    if (volume < 0 || volume > 100)
    {
        AMY_LOG_ERROR("intercom volume invalid: %d (range 0~100)", volume);
        return -1;
    }

    if (Amy_mutex_lock(&intercom_ctx.mutex) != 0)
    {
        AMY_LOG_ERROR("intercom mutex lock failed when set volume");
        return -1;
    }

    intercom_ctx.volume_cache = volume;
    AMY_LOG_INFO("intercom volume set cache: %d", volume);

    intercom_volume_apply();

    Amy_mutex_unlock(&intercom_ctx.mutex);
    return 0;
}

/**
 * @brief 可选：获取当前缓存的音量
 */
int intercom_volume_get(void)
{
    int volume;
    if (Amy_mutex_lock(&intercom_ctx.mutex) != 0)
    {
        AMY_LOG_ERROR("intercom mutex lock failed when get volume");
        return -1;
    }

    volume = intercom_ctx.volume_cache;
    Amy_mutex_unlock(&intercom_ctx.mutex);
    return volume;
}