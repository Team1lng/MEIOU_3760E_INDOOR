/**
 * @file        call_core.h
 * @brief       通话系统核心公共接口头文件
 * @details     声明通话系统核心数据类型、全局配置宏、公共接口，作为系统与外部的交互契约
 * @ingroup     call_core
 */

#ifndef _CALL_CORE_H_
#define _CALL_CORE_H_

#include "string.h"
#include "stdbool.h"
#include "stdlib.h"
#include "stdint.h"
#include "time.h"
#include "call_common.h"
#include "call_log.h"
#include "call_record.h"
#include "call_comm_interface.h"
#include "call_event_hook.h"
#include "call_api.h"
/**
 * @brief: 通话系统全局配置参数
 */

#define CALL_SESSION_MAX 10
#define DEVICE_MAX 18

/**
 * @brief 通话事件类型枚举
 * 用于事件分发机制
 */
typedef enum
{
    CALL_EVENT_ID_CALL_ACTIVE_INVITE = 0x01,
    CALL_EVENT_ID_CALL_ACTIVE_ACCEPT,
    CALL_EVENT_ID_CALL_ACTIVE_END,
    CALL_EVENT_ID_CALL_SWITCH_CALL,
    CALL_EVENT_ID_CALL_PASSIVE_INVITE,
    CALL_EVENT_ID_CALL_PASSIVE_RINGING,
    CALL_EVENT_ID_CALL_PASSIVE_BUSY,
    CALL_EVENT_ID_CALL_PASSIVE_ACCEPT,
    CALL_EVENT_ID_CALL_PASSIVE_END,
    CALL_EVENT_ID_CALL_PASSIVE_HEARTBEAT_REQUEST,
    CALL_EVENT_ID_CALL_PASSIVE_HEARTBEAT_RESPONSE,
} call_event_type;

typedef struct _call_msg
{
    long msg_type;
    call_cb_attr attr;
} call_msg;

/**
 * @brief 设备在线状态枚举
 */
typedef enum
{
    DEVICE_STATUS_OFFLINE,
    DEVICE_STATUS_ONLINE,
    DEVICE_STATUS_UNKNOWN
} device_status;

/**
 * @brief 设备在线状态管理结构体
 */
typedef struct
{
    device_status status;
    uint32_t check_count;
} device_onlineInfo;

/**
 * @brief 会话上下文结构体
 * 存储单个会话的全量业务信息
 */
typedef struct
{
    call_end_reason reason;
    uint8_t call_state;
    call_log_type log_type;
    uint32_t call_flag;
    device_info_attr device_info;
    uint64_t session_id;
    uint64_t start_time;
    uint64_t end_time;
    uint64_t start_connected_time;
    uint64_t last_interact_time;
    device_onlineInfo online_info[DEVICE_MAX];
    call_record_info_attr record_info_ctx;
} call_session_context;

/**
 * @brief 全局核心上下文结构体
 * 存储系统全局配置和核心资源
 */
typedef struct
{
    call_comm_interface_t comm_interface;
    call_state_cb on_state_changed;
    device_info_attr local_info;
    uint8_t answer_auto;
    uint8_t max_sessions;
    uint32_t max_call_duration_ms;   ///< 最大通话时长(ms)
    uint32_t no_interact_timeout_ms; ///< 无操作超时(ms)
    uint32_t session_id;
    int q;
    uint8_t run;
    uint8_t call_nb;
    call_session_context call_session_ctx[CALL_SESSION_MAX];
    call_session_context *currrent_call;
} call_context;

extern call_context call_global_ctx;

/**
 * @brief 更新会话状态
 * @param call_session_ctx 会话上下文
 * @param new_state 目标状态
 * @note 自动处理状态联动，会话终止时自动记录
 */
void update_current_call_state(call_session_context *call_session_ctx, call_state new_state);

/**
 * @brief 获取空闲会话
 * @return 空闲会话指针，失败返回NULL
 */
call_session_context *new_call_session_context(void);

/**
 * @brief 根据设备信息获取活跃会话
 * @param info 设备信息
 * @return 匹配会话指针
 */
call_session_context *get_call_session(device_info_attr *info);

/**
 * @brief 获取任意活跃会话
 * @return 首个活跃会话指针
 */
call_session_context *call_session_context_get(void);

/**
 * @brief 重置会话上下文
 * @param session_ctx 会话指针
 */
void call_session_contex_reset(call_session_context *session_ctx);

/**
 * @brief 初始化通话系统
 * @param config 通话系统配置
 * @return 0成功，-1失败
 */
int call_core_init(call_config_t *config);

/**
 * @brief 反初始化通话系统
 * @return 0成功
 */
int call_core_uninit(void);

/**
 * @brief 获取当前活跃会话数
 * @return 活跃数量
 */
uint8_t call_core_active_session_count(void);

/**
 * @brief 获取当前活跃通话设备信息
 * @param device_info 输出设备信息
 * @return 成功/失败
 */
bool core_get_current_call_device(device_info_attr *device_info);

/**
 * @brief 获取当前会话对象
 * @return 会话指针
 */
call_session_context *core_get_current_session(void);

/**
 * @brief 获取本地设备信息
 * @return 设备信息结构体
 */
device_info_attr core_get_local_info(void);

/**
 * @brief 动态更新本地设备信息（设备号/房号变更且不重启时调用）
 * @param[in] info 新的设备信息（不可为NULL）
 */
void core_set_local_info(const device_info_attr *info);

/**
 * @brief 获取门口机设备信息
 * @param idx 索引
 * @return 设备信息指针
 */
struct ipcamera_info *core_get_door_device(uint8_t idx);

/**
 * @brief 获取自动接听状态
 * @return 1开启 0关闭
 */
uint8_t core_get_answer_auto(void);

/**
 * @brief 设置自动接听
 * @param auto_answer 开关
 */
void core_set_auto_answer(bool auto_answer);

/**
 * @brief 获取系统运行状态
 * @return 1运行 0停止
 */
uint8_t core_get_run(void);

/**
 * @brief 获取活跃通话总数
 * @return 数量
 */
uint8_t core_get_call_count(void);

/**
 * @brief 获取会话池基地址
 * @return 会话数组指针
 */
call_session_context *core_get_clls(void);

/**
 * @brief 获取当前会话
 * @return 会话指针
 */
call_session_context *core_get_current_call(void);

/**
 * @brief 设置当前会话
 * @param ctx 目标会话
 */
void core_set_current_call(call_session_context *ctx);

/**
 * @brief 获取除当前外所有活跃通话
 * @param public_info_buf 输出缓冲区
 * @param max_count 最大条数
 * @param actual_count 实际条数
 * @return 成功/失败
 */
bool core_get_all_pending_calls(device_info_attr *public_info_buf, int max_count, int *actual_count);

/**
 * @brief 主动接听指定设备
 * @param[in] device_info 目标设备信息
 */
void core_call_answer(device_info_attr *device_info);

/**
 * @brief 主动接听当前活跃会话
 * @return 成功/失败
 */
bool core_call_answer_current(void);
/**
 * @brief 挂断当前活跃会话
 * @return 成功/失败
 */
bool core_call_hangup_current(void);

/**
 * @brief 挂断指定设备会话
 * @param[in] device_info 目标设备
 * @return 成功/失败
 */
bool core_call_hangup_specified(device_info_attr *device_info);

/**
 * @brief 挂断所有活跃会话
 * @return 成功/失败
 */
bool core_call_hangup_all(void);

/**
 * @brief 切换通话
 * @param device_info 目标设备
 * @return 成功/失败
 */
void core_switch_call(device_info_attr *device_info);

/**
 * @brief 主动拨号
 * @param[in] device_info 目标设备
 */
void core_call_dial(device_info_attr *device_info);

/**
 * @brief 接收外部呼叫邀请
 * @param[in] device_info 呼叫方设备
 */
void core_call_receive_invite(device_info_attr *device_info);

/**
 * @brief 接收振铃通知
 * @param[in] device_info 设备信息
 */
void core_call_receive_ringing(device_info_attr *device_info);

/**
 * @brief 接收对方接听
 * @param[in] device_info 对方设备
 */
void core_call_receive_accept(device_info_attr *device_info);

/**
 * @brief 接收对方占线
 * @param[in] device_info 对方设备
 */
void core_call_receive_busy(device_info_attr *device_info);

/**
 * @brief 接收会话结束
 * @param[in] device_info 对方设备
 */
void core_call_receive_end(device_info_attr *device_info);

/**
 * @brief 接收心跳查询
 * @param[in] device_info 发起方设备
 */
void core_call_receive_heartbeat_request(device_info_attr *device_info);

/**
 * @brief 接收心跳回复
 * @param[in] device_info 回复方设备
 */
void core_call_receive_heartbeat_response(device_info_attr *device_info);

#endif // _CALL_CORE_H_