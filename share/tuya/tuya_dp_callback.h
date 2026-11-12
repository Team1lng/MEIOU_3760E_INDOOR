#ifndef _TUYA_DP_CALLBACK_H_
#define _TUYA_DP_CALLBACK_H_
#include "stdbool.h"

// 单个通道配置（通用，适配所有通道）
typedef struct
{
    int ch;
    const char *name;
} tuya_ch_info_t;

/**
 * @brief 上报有效通道状态（TUYA_DP_SWITCH_CHANNEL）
 * @param curr_ch: 当前激活的通道ID（有效值需为非负整数）
 * @param info: 通道信息数组（tuya_ch_info_t类型），不可为NULL
 *        - ch: 通道ID（非负整数）
 *        - name: 通道名称（非空字符串）
 * @param total: 通道信息数组的总长度，有效值：1~(DP_STR_BUF_SIZE/32)（避免缓冲区溢出）
 * @return int: 0-上报成功；-1-参数无效；-2-MQTT未在线；-3-字符串拼接失败；-4-上报失败
 * @note 1. 拼接的JSON字符串缓冲区为512字节，最终上报字符串长度≤255字节；
 *       2. 函数内部做全参数校验+长度校验，杜绝缓冲区溢出；
 *       3. 自动过滤空名称/无效ID的通道，不影响整体上报逻辑
 */
int tuya_channel_valid_report(int curr_ch, tuya_ch_info_t *info, int total);

/**
 * @brief 上报在家模式状态（TUYA_DP_AT_MODE）
 * @param en: true-开启在家模式，false-关闭在家模式
 */
void tuya_dp_at_home_response(bool en);

/**
 * @brief 上报离家模式状态（TUYA_DP_AWAY_MODE）
 * @param en: true-开启离家模式，false-关闭离家模式
 */
void tuya_dp_away_mode_response(bool en);

/**
 * @brief 上报睡眠模式状态（TUYA_DP_SLEEP_MODE）
 * @param en: true-开启睡眠模式，false-关闭睡眠模式
 */
void tuya_dp_sleep_mode_response(bool en);

/**
 * @brief 上报室内机锁状态（TUYA_DP_ACCSESS_GATE2 232）
 * @param locked: true-已上锁，false-已解锁
 */
void tuya_dp_access_gate2_response(bool locked);

/**
 * @brief 上报门禁锁1状态（TUYA_DP_ACCSESS_LOCK）
 * @param locked: true-锁1已上锁，false-锁1已解锁
 */
void tuya_dp_access_lock1_response(bool locked);

/**
 * @brief 上报门禁锁2状态（TUYA_DP_ACCSESS_GATE1）
 * @param locked: true-锁2已上锁，false-锁2已解锁
 */
void tuya_dp_access_lock2_response(bool locked);

/**
 * @brief 上报支持的门禁锁列表（TUYA_DP_ACCSESS_LOCK_SUPPORT）
 * @note 固定上报三把锁，不随监控通道变化
 */
void tuya_dp_access_lock_support_response(void);

/**
 * @brief 上报SD卡状态（TUYA_DP_SD_STATUS_ONLY_GET 110）
 * @param status: 0-未知，1-正常，2-异常，3-满，4-正在格式化，5-无sd卡
 */
void tuya_dp_sd_status_response(int status);
/**
 * @brief 上报SD卡存储状态（TUYA_DP_SD_STORAGE_ONLY_GET 109）
 */
void tuya_dp_sd_storage_response(unsigned long long total, unsigned long long used, unsigned long long empty);

/**
 * @brief 上报SD卡格式命令执行（TUYA_DP_SD_FORMAT 111）
 * @param started: true-已开始格式化（或命令成功触发），false-命令失败/未执行
 */
void tuya_dp_sd_format_response(bool started);

/**
 * @brief 上报SD卡格式进度/状态（TUYA_DP_SD_FORMAT_STATUS_ONLY_GET 117）
 * @param status: -2000: formatting, -2001: abnormal, -2002: no SD card, -2003: sd error, >=0 progress percentage
 */
void tuya_dp_sd_format_status_response(int status);

/**
 * @brief 上报氛围灯开关状态（TUYA_DP_ATMOSPHERE_SWITCH 235）
 * @param en: true-开启氛围灯，false-关闭氛围灯
 */
void tuya_dp_atmosphere_switch_response(bool en);

bool tuya_all_dp_response(void);

#endif