#include "call_event_hook.h"
#include "call_event_indicate.h"
#include "Amy_lvgl.h"

/**
 * @brief 通话状态变化回调
 * @param cb_attr 状态变化属性
 */
void sat_state_changed(call_state_cb_attr *cb_attr)
{
  device_info_attr device_info = cb_attr->device_info;
  call_end_reason reason = cb_attr->reason;
  call_state state = cb_attr->state;
  call_cb_attr attr;
  AMY_LOG_DEBUG("state is %d\r\n", state);
  switch (state)
  {
  case CALL_INCOMING:
    attr.call_event = AMY_CALL_EVENT_ID_CALL_INCOMING;
    attr.device_info = device_info;
    attr.reason = reason;
    sat_call_event_put(&attr);
    break;

  case CALL_OUTGOING_PROCESS:
    attr.call_event = AMY_CALL_EVENT_ID_CALL_OUTGOING_PROCESS;
    attr.device_info = device_info;
    attr.reason = reason;
    sat_call_event_put(&attr);
    break;

  case CALL_OUTGOING_RINGING:
    attr.call_event = AMY_CALL_EVENT_ID_CALL_OUTGOING_RINGING;
    attr.device_info = device_info;
    attr.reason = reason;
    sat_call_event_put(&attr);
    break;

  case CALL_CONNECTED:
    attr.call_event = AMY_CALL_EVENT_ID_CALL_CONNECTED;
    attr.device_info = device_info;
    attr.reason = reason;
    sat_call_event_put(&attr);
    break;

  case CALL_END:
    attr.call_event = AMY_CALL_EVENT_ID_CALL_END;
    attr.device_info = device_info;
    attr.reason = reason;
    sat_call_event_put(&attr);
    break;

  default:
    break;
  }
}
