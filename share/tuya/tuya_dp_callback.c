#include "tuya_dp_define.h"
#include "tuya_dp_callback.h"
#include "tuya_ipc_api.h"
#include "Amy_lvgl.h"
#include "tuya_sdk.h"
#include "Amy_tfcard.h"
#include "user_data.h"

static void respone_dp_value(BYTE_T dp_id, INT_T val)
{
    tuya_ipc_dp_report(NULL, dp_id, PROP_VALUE, &val, 1);
}

static void respone_dp_bool(BYTE_T dp_id, BOOL_T true_false)
{
    tuya_ipc_dp_report(NULL, dp_id, PROP_BOOL, &true_false, 1);
}

static void respone_dp_str(BYTE_T dp_id, CHAR_T *p_val_str)
{
    tuya_ipc_dp_report(NULL, dp_id, PROP_STR, p_val_str, 1);
}

// ===================== 自定义功能DP点上报函数 =====================

// 上传通道格式："{\"cmd\":1,\"cc\":1,\"chs\":[{\"id\":1,\"n\":\"door1\"},{\"id\":2,\"n\":\"door2\"}]}"
#define TUYA_CHANNEL_CMD_START "{\"cmd\":1,\"cc\":%d,\"chs\":["
#define TUYA_CHANNEL_CMD_VALID "{\"id\":%d,\"n\":\"%s\"},"
#define TUYA_CHANNEL_CMD_END "]}"
#define DP_STR_BUF_SIZE 512   // 主缓冲区大小
#define TEMP_STR_BUF_SIZE 128 // 临时缓冲区大小
#define DP_STR_MAX_LEN 255    // 最终上报字符串最大长度（对应注释要求）
#define INVALID_CH_ID -1      // 无效通道ID标识
/**
 * @brief 上报有效通道状态（TUYA_DP_SWITCH_CHANNEL）
 * @param curr_ch: 当前激活的通道ID（有效值需为非负整数）
 * @param info: 通道信息数组（tuya_ch_info_t类型），不可为NULL
 *        - ch: 通道ID（非负整数）
 *        - name: 通道名称（非空字符串）
 * @param total: 通道信息数组的总长度，有效值：1~(DP_STR_BUF_SIZE/32)（避免缓冲区溢出）
 * @return int: 0-上报成功；-1-参数无效；-2-云端 MQTT 未在线（tuya_mqtt_link_state_get != ONLINE）；-3-字符串拼接失败；-4-上报失败
 * @note 1. 拼接的JSON字符串缓冲区为512字节，最终上报字符串长度≤255字节；
 *       2. 函数内部做全参数校验+长度校验，杜绝缓冲区溢出；
 *       3. 自动过滤空名称/无效ID的通道，不影响整体上报逻辑
 */
int tuya_channel_valid_report(int curr_ch, tuya_ch_info_t *info, int total)
{
    if (info == NULL || total <= 0 || total > (DP_STR_BUF_SIZE / 32) || curr_ch < 0)
    {
        return -1;
    }

    if (!tuya_mqtt_is_online())
    {
        return -2;
    }

    char dp_str[DP_STR_BUF_SIZE] = {0};
    char temp_str[TEMP_STR_BUF_SIZE] = {0};
    int ret = 0;
    int curr_len = 0; // 记录当前dp_str已使用长度，避免溢出

    ret = snprintf(dp_str, DP_STR_BUF_SIZE, TUYA_CHANNEL_CMD_START, curr_ch);
    if (ret < 0 || ret >= DP_STR_BUF_SIZE)
    {
        return -3;
    }
    curr_len = ret;

    for (int i = 0; i < total; i++)
    {
        if (info[i].ch < 0 || info[i].name == NULL || strlen(info[i].name) == 0)
        {
            continue;
        }

        ret = snprintf(temp_str, TEMP_STR_BUF_SIZE, TUYA_CHANNEL_CMD_VALID, info[i].ch, info[i].name);
        if (ret < 0 || ret >= TEMP_STR_BUF_SIZE)
        {
            return -3;
        }

        if ((curr_len + ret) >= DP_STR_BUF_SIZE)
        {
            return -3;
        }

        strncat(dp_str, temp_str, DP_STR_BUF_SIZE - curr_len - 1);
        curr_len += ret;
    }

    if (curr_len > 0)
    {
        dp_str[curr_len - 1] = '\0';
        curr_len--;
    }
    else
    {
        return -3;
    }

    ret = snprintf(dp_str + curr_len, DP_STR_BUF_SIZE - curr_len, TUYA_CHANNEL_CMD_END);
    if (ret < 0 || (curr_len + ret) > DP_STR_MAX_LEN)
    {
        return -3;
    }

    respone_dp_str(TUYA_DP_SWITCH_CHANNEL, dp_str);

    return 0;
}

/**
 * @brief 上报在家模式状态（TUYA_DP_AT_MODE）
 * @param en: true-开启在家模式，false-关闭在家模式
 */
void tuya_dp_at_home_response(bool en)
{
    respone_dp_bool(TUYA_DP_AT_MODE, en);
}

/**
 * @brief 上报离家模式状态（TUYA_DP_AWAY_MODE）
 * @param en: true-开启离家模式，false-关闭离家模式
 */
void tuya_dp_away_mode_response(bool en)
{
    respone_dp_bool(TUYA_DP_AWAY_MODE, en);
}

/**
 * @brief 上报睡眠模式状态（TUYA_DP_SLEEP_MODE）
 * @param en: true-开启睡眠模式，false-关闭睡眠模式
 */
void tuya_dp_sleep_mode_response(bool en)
{
    respone_dp_bool(TUYA_DP_SLEEP_MODE, en);
}

/**
 * @brief 上报室内机锁状态（TUYA_DP_ACCSESS_GATE2 232）
 * @param locked: true-已上锁，false-已解锁
 */
void tuya_dp_access_gate2_response(bool locked)
{
    respone_dp_bool(TUYA_DP_ACCSESS_GATE2, locked);
}

/**
 * @brief 上报门禁锁1状态（TUYA_DP_ACCSESS_LOCK）
 * @param locked: true-锁1已上锁，false-锁1已解锁
 */
void tuya_dp_access_lock1_response(bool locked)
{
    respone_dp_bool(TUYA_DP_ACCSESS_LOCK, locked);
}

/**
 * @brief 上报门禁锁2状态（TUYA_DP_ACCSESS_GATE1）
 * @param locked: true-锁2已上锁，false-锁2已解锁
 */
void tuya_dp_access_lock2_response(bool locked)
{
    respone_dp_bool(TUYA_DP_ACCSESS_GATE1, locked);
}

/**
 * @brief 上报支持的门禁锁列表（TUYA_DP_ACCSESS_LOCK_SUPPORT）
 * @note 固定上报三把锁（锁1、锁2、室内机锁），不随监控通道变化，
 *       避免云端显示的锁数量异常；监控通道下锁1/锁2 命令由事件处理层忽略
 */
void tuya_dp_access_lock_support_response(void)
{
    char str[16];
    snprintf(str, sizeof(str), "%d,%d,%d", TUYA_DP_ACCSESS_LOCK, TUYA_DP_ACCSESS_GATE1, TUYA_DP_ACCSESS_GATE2);
    respone_dp_str(TUYA_DP_ACCSESS_LOCK_SUPPORT, str);
}

// ===================== 标准功能DP点上报函数（SD卡相关） =====================
/**
 * @brief 上报SD卡状态（TUYA_DP_SD_STATUS_ONLY_GET 110）
 * @param status: 0-未知，1-正常，2-异常，3-满，4-正在格式化，5-无sd卡
 */
void tuya_dp_sd_status_response(int status)
{
    // 校验数值范围
    if (status < 0 || status > 5)
        status = 0;
    respone_dp_value(TUYA_DP_SD_STATUS_ONLY_GET, status);
}

/**
 * @brief 上报SD卡存储状态（TUYA_DP_SD_STORAGE_ONLY_GET 109）
 * @param total: SD卡总容量（字节）
 * @param used: SD卡已使用容量（字节）
 * @param empty: SD卡剩余容量（字节）
 * @note 上报格式为"total|used|empty"，如"1073741824|536870912|536870912"
 */
void tuya_dp_sd_storage_response(unsigned long long total, unsigned long long used, unsigned long long empty)
{
    char tmp_str[100] = {0};
    snprintf(tmp_str, 100, "%llu|%llu|%llu", total, used, empty);
    respone_dp_str(TUYA_DP_SD_STORAGE_ONLY_GET, tmp_str);
}

/**
 * @brief 上报SD卡格式命令执行（TUYA_DP_SD_FORMAT 111）
 * @param started: true-已开始格式化（或命令成功触发），false-命令失败/未执行
 */
void tuya_dp_sd_format_response(bool started)
{
    respone_dp_bool(TUYA_DP_SD_FORMAT, started);
}

/**
 * @brief 上报SD卡格式进度/状态（TUYA_DP_SD_FORMAT_STATUS_ONLY_GET 117）
 * @param status: -2000: formatting, -2001: abnormal, -2002: no SD card, -2003: sd error, >=0 progress percentage
 */
void tuya_dp_sd_format_status_response(int status)
{
    respone_dp_value(TUYA_DP_SD_FORMAT_STATUS_ONLY_GET, status);
}

void tuya_dp_atmosphere_switch_response(bool en)
{
    respone_dp_bool(TUYA_DP_ATMOSPHERE_SWITCH, en);
}

bool tuya_all_dp_response(void)
{
    /* 1. 门禁锁支持列表 */
    tuya_dp_access_lock_support_response();

    /* 2. 场景模式：三个模式互斥，仅上报当前激活的一种 */
    int scene = user_data_get()->public_cfg.global_func.scene_mode;
    tuya_dp_at_home_response(scene == SCENE_MODE_HOME);
    tuya_dp_away_mode_response(scene == SCENE_MODE_AWAY);
    tuya_dp_sleep_mode_response(scene == SCENE_MODE_SLEEP);

    /* 3. SD卡状态 */
    int sd_status = Amy_tfcard_status();
    switch (sd_status)
    {
    case SD_STATE_UNPLUG:
        tuya_dp_sd_status_response(5);
        break;
    case SD_STATE_ERROR:
        tuya_dp_sd_status_response(2);
        break;
    case SD_STATE_MOUNT:
        tuya_dp_sd_status_response(1);
        break;
    case SD_STATE_FULL:
        tuya_dp_sd_status_response(3);
        break;
    case SD_STATE_FORMAT:
        tuya_dp_sd_status_response(4);
        break;
    default:
        tuya_dp_sd_status_response(0);
        break;
    }

    /* 4. SD卡存储容量 */
    unsigned long total_kb, used_kb, free_kb;
    if (tfcard_capacity_get(&total_kb, &used_kb, &free_kb) == 0)
    {
        tuya_dp_sd_storage_response(total_kb, used_kb, free_kb);
    }

    return true;
}