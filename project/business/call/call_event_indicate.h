/**
 * @file        call_event_indicate.h
 * @brief       通话状态回调函数声明
 * @details     提供 sat_state_changed 回调，通过 call_config_t 注册到 call 框架
 * @ingroup     call_event
 */

#ifndef CALL_EVENT_INDICATE_H
#define CALL_EVENT_INDICATE_H
#include "call_event_hook.h"

/**
 * @brief 通话状态变化回调（注册到 call_config_t.on_state_changed）
 * @param cb_attr 状态变化属性
 */
void sat_state_changed(call_state_cb_attr *cb_attr);

#endif // CALL_EVENT_INDICATE_H
