/**
 * @file        call_heartbeat_internal.h
 * @brief       通话心跳检测模块内部接口
 * @details     设备在线状态维护、心跳收发处理（core 内部使用）
 * @ingroup     call_heartbeat
 */

#ifndef __CALL_HEARTBEAT_INTERNAL_H__
#define __CALL_HEARTBEAT_INTERNAL_H__

/**
 * @brief 初始化心跳模块
 * @return 成功/失败
 */
bool heartbeat_module_init(void);

/**
 * @brief 反初始化心跳模块
 * @return 成功/失败
 */
bool heartbeat_module_uninit(void);

#endif // __CALL_HEARTBEAT_INTERNAL_H__