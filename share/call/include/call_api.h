/**
 * @file        call_api.h
 * @brief       通话系统对外应用接口头文件
 * @details     定义应用层与通话核心模块交互契约，隐藏内部实现
 * @ingroup     call_api
 */

#ifndef __CALL_API_H__
#define __CALL_API_H__

#include "call_common.h"
#include "call_event_hook.h"
#include "call_comm_interface.h"

#define CALL_DEFAULT_MAX_DURATION_MS 300000       /**< 默认最大通话时长(5min) */
#define CALL_DEFAULT_NO_INTERACT_TIMEOUT_MS 60000 /**< 无操作超时基准(60s)，框架配置 0 关闭后由应用层页面计时使用 */

/**
 * @brief 通话系统配置结构体
 * @details 初始化时一次性传入所有配置，替代分散的初始化调用
 */
typedef struct
{
  device_info_attr local_info;          /**< 本地设备信息 */
  bool auto_answer;                     /**< 自动接听开关 */
  uint8_t max_sessions;                 /**< 最大会话个数（<= CALL_SESSION_MAX） */
  uint32_t max_call_duration_ms;        /**< 最大通话时长（毫秒，0=默认值） */
  uint32_t no_interact_timeout_ms;      /**< 无操作超时时间（毫秒，0=关闭该检测） */
  bool record_enable;                   /**< 通话记录使能 */
  const char *record_path;              /**< 通话记录保存路径（record_enable 为 true 时有效） */
  call_state_cb on_state_changed;       /**< 通话状态回调函数指针 */
  call_comm_interface_t comm_interface; /**< 通信接口实现 */
} call_config_t;

/**
 * @brief 会话系统核心初始化（统一入口）
 * @param[in] config 通话系统配置（不可为NULL）
 * @return 0=成功，非0=失败
 */
int sat_call_init(call_config_t *config);

/**
 * @brief 会话系统反初始化
 * @return 0=成功，非0=失败
 */
int sat_call_uninit(void);

/**
 * @brief 动态更新本地设备信息（设备号/房号变更且不重启时调用）
 * @param[in] device_info 新的设备信息（不可为NULL）
 */
void sat_call_update_local_info(const device_info_attr *device_info);

/**
 * @brief 主动接听指定设备
 * @param[in] device_info 目标设备信息
 */
void sat_call_answer(device_info_attr *device_info);

/**
 * @brief 主动接听当前活跃会话
 * @return 成功/失败
 */
bool sat_call_answer_current(void);

/**
 * @brief 切换通话
 * @param[in] device_info 目标设备
 * @return 成功/失败
 */
void sat_call_switch_call(device_info_attr *device_info);

/**
 * @brief 主动拨号
 * @param[in] device_info 目标设备
 */
void sat_call_dial(device_info_attr *device_info);

/**
 * @brief 接收外部呼叫邀请
 * @param[in] device_info 呼叫方设备
 */
void sat_call_receive_invite(device_info_attr *device_info);

/**
 * @brief 接收振铃通知
 * @param[in] device_info 设备信息
 */
void sat_call_receive_ringing(device_info_attr *device_info);

/**
 * @brief 接收对方接听
 * @param[in] device_info 对方设备
 */
void sat_call_receive_accept(device_info_attr *device_info);

/**
 * @brief 接收对方占线
 * @param[in] device_info 对方设备
 */
void sat_call_receive_busy(device_info_attr *device_info);

/**
 * @brief 接收会话结束
 * @param[in] device_info 对方设备
 */
void sat_call_receive_end(device_info_attr *device_info);

/**
 * @brief 接收心跳查询
 * @param[in] device_info 发起方设备
 */
void sat_call_receive_heartbeat_request(device_info_attr *device_info);

/**
 * @brief 接收心跳回复
 * @param[in] device_info 回复方设备
 */
void sat_call_receive_heartbeat_response(device_info_attr *device_info);

/**
 * @brief 获取当前通话设备信息
 * @param[out] device_info 设备信息
 * @return 成功/失败
 */
bool sat_call_get_current_call(device_info_attr *device_info);

/**
 * @brief 获取所有等待中通话
 * @param[out] device_info_array 设备信息数组
 * @param[in] max_count 最大数量
 * @param[out] actual_count 实际数量
 * @return 成功/失败
 */
bool sat_get_all_pending_calls(device_info_attr *device_info_array, int max_count, int *actual_count);

/**
 * @brief 挂断当前活跃会话
 * @return 成功/失败
 */
bool sat_call_hangup_current(void);

/**
 * @brief 挂断指定设备会话
 * @param[in] device_info 目标设备
 * @return 成功/失败
 */
bool sat_call_hangup_specified(device_info_attr *device_info);

/**
 * @brief 挂断所有活跃会话
 * @return 成功/失败
 */
bool sat_call_hangup_all(void);

#endif // __CALL_API_H__