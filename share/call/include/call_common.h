/**
 * @file        call_common.h
 * @brief       通话模块公共类型定义
 * @details     包含事件结构体、枚举等公共数据结构
 * @ingroup     call_common
 */

#ifndef __CALL_COMMON_H__
#define __CALL_COMMON_H__

#include "device_types.h"

/**
 * @brief 通话事件回调属性结构体
 */
typedef struct
{
  int call_event;
  int reason;
  device_info_attr device_info;
} call_cb_attr;

/**
 * @brief 通话结束原因枚举
 */
typedef enum
{
  REASON_NONE = 0,             /**< 无原因（初始状态） */
  REASON_HANGUP_SELF,          /**< 自己主动挂断 */
  REASON_HANGUP,               /**< 对方主动挂断 */
  REASON_BUSY,                 /**< 对方忙线 */
  REASON_NO_RESPONSE,          /**< 无响应超时 */
  REASON_DEVICE_OFFLINE,       /**< 设备离线 */
  REASON_MAX_DURATION_TIMEOUT, /**< 最大通话时长超时 */
  REASON_NO_INTERACT_TIMEOUT   /**< 无操作超时 */
} call_end_reason;

/**
 * @brief 会话状态枚举
 * 覆盖会话全生命周期的状态管理
 */
typedef enum
{
  CALL_IDLE,
  CALL_INCOMING,
  CALL_OUTGOING_PROCESS,
  CALL_OUTGOING_RINGING,
  CALL_CONNECTED,
  CALL_END,
} call_state;

#endif