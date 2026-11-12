/**
 * @file        call_api.c
 * @brief       通话系统对外应用接口层
 * @details     提供初始化、拨号、接听、挂断、状态查询等对外 API
 * @ingroup     call_api
 */

#include "call_core.h"

/**
 * @brief 会话系统初始化（应用层统一入口）
 * @param[in] config 通话系统配置（不可为NULL）
 * @return 0=成功，非0=失败
 */
int sat_call_init(call_config_t *config)
{
    int ret = call_core_init(config);
    if (ret != 0)
    {
        CALL_LOG_ERROR(CALL_MODULE_API, "call init failed\r\n");
        return ret;
    }
    CALL_LOG_INFO(CALL_MODULE_API, "call init success\r\n");
    return 0;
}

/**
 * @brief 会话系统反初始化
 * @return 0=成功，非0=失败
 */
int sat_call_uninit(void)
{
    return call_core_uninit();
}

/**
 * @brief 动态更新本地设备信息（设备号/房号变更且不重启时调用）
 * @param[in] device_info 新的设备信息（不可为NULL）
 */
void sat_call_update_local_info(const device_info_attr *device_info)
{
    if (device_info == NULL)
        return;
    core_set_local_info(device_info);
    CALL_LOG_INFO(CALL_MODULE_API, "local device info updated (building=%d, room=%d, type=%d)",
                  device_info->building, device_info->room, device_info->device_type);
}

/**
 * @brief 主动接听指定设备
 * @param[in] device_info 目标设备信息
 */
void sat_call_answer(device_info_attr *device_info)
{
    core_call_answer(device_info);
}

/**
 * @brief 主动接听当前活跃会话
 * @return 成功/失败
 */
bool sat_call_answer_current(void)
{
    return core_call_answer_current();
}

/**
 * @brief 挂断当前活跃会话
 * @return 成功/失败
 */
bool sat_call_hangup_current(void)
{
    CALL_LOG_INFO(CALL_MODULE_API, "sat_call_hangup_current\r\n");
    return core_call_hangup_current();
}

/**
 * @brief 挂断指定设备会话
 * @param[in] device_info 目标设备
 * @return 成功/失败
 */
bool sat_call_hangup_specified(device_info_attr *device_info)
{
    CALL_LOG_INFO(CALL_MODULE_API, "sat_call_hangup_specified\r\n");
    return core_call_hangup_specified(device_info);
}

/**
 * @brief 挂断所有活跃会话
 * @return 成功/失败
 */
bool sat_call_hangup_all(void)
{
    CALL_LOG_INFO(CALL_MODULE_API, "sat_call_hangup_all\r\n");
    return core_call_hangup_all();
}

/**
 * @brief 主动拨号
 * @param[in] device_info 目标设备
 */
void sat_call_dial(device_info_attr *device_info)
{
    core_call_dial(device_info);
}

/**
 * @brief 接收外部呼叫邀请v
 * @param[in] device_info 呼叫方设备
 */
void sat_call_receive_invite(device_info_attr *device_info)
{
    core_call_receive_invite(device_info);
}

/**
 * @brief 接收振铃通知
 * @param[in] device_info 设备信息
 */
void sat_call_receive_ringing(device_info_attr *device_info)
{
    core_call_receive_ringing(device_info);
}

/**
 * @brief 接收对方接听
 * @param[in] device_info 对方设备
 */
void sat_call_receive_accept(device_info_attr *device_info)
{
    core_call_receive_accept(device_info);
}

/**
 * @brief 接收对方占线
 * @param[in] device_info 对方设备
 */
void sat_call_receive_busy(device_info_attr *device_info)
{
    core_call_receive_busy(device_info);
}

/**
 * @brief 接收会话结束
 * @param[in] device_info 对方设备
 */
void sat_call_receive_end(device_info_attr *device_info)
{
    core_call_receive_end(device_info);
}

/**
 * @brief 接收心跳查询
 * @param[in] device_info 发起方设备
 */
void sat_call_receive_heartbeat_request(device_info_attr *device_info)
{
    core_call_receive_heartbeat_request(device_info);
}

/**
 * @brief 接收心跳回复
 * @param[in] device_info 回复方设备
 */
void sat_call_receive_heartbeat_response(device_info_attr *device_info)
{
    core_call_receive_heartbeat_response(device_info);
}

/**
 * @brief 获取当前通话设备信息
 * @param[out] device_info 设备信息
 * @return 成功/失败
 */
bool sat_call_get_current_call(device_info_attr *device_info)
{
    return core_get_current_call_device(device_info);
}

/**
 * @brief 获取所有等待中通话
 * @param[out] device_info_array 设备信息数组
 * @param[in] max_count 最大数量
 * @param[out] actual_count 实际数量
 * @return 成功/失败
 */
bool sat_get_all_pending_calls(device_info_attr *device_info_array, int max_count, int *actual_count)
{
    return core_get_all_pending_calls(device_info_array, max_count, actual_count);
}

/**
 * @brief 切换通话
 * @param[in] device_info 目标设备
 * @return 成功/失败
 */
void sat_call_switch_call(device_info_attr *device_info)
{
    CALL_LOG_INFO(CALL_MODULE_API, "sat_call_switch_call\r\n");
    core_switch_call(device_info);
}

/**
 * @brief 获取当前活跃通话数量
 * @return 数量
 */
uint8_t sat_call_active_call_count(void)
{
    return core_get_call_count();
}