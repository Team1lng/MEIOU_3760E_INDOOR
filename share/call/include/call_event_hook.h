/**
 * @file        call_event_hook.h
 * @brief       通话状态变化回调接口头文件
 * @ingroup     call_event_hook
 */

#ifndef CALL_EVENT_HOOK_H
#define CALL_EVENT_HOOK_H
#include "call_common.h"

/**
 * @brief 通话事件回调属性结构体
 */
typedef struct
{
  int reason;
  device_info_attr device_info;
  call_state state;
  call_state prev_state; // 上一状态，用于区分通话结束前是否已接通
} call_state_cb_attr;

typedef void (*call_state_cb)(call_state_cb_attr *state_attr);

#endif // CALL_EVENT_HOOK_H