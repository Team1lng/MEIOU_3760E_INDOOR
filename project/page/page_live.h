#ifndef _PAGE_LIVE_H
#define _PAGE_LIVE_H
/* =========================== 枚举定义 =========================== */

/* =========================== 枚举定义 =========================== */
typedef enum
{
  MONITOR_INCOMING_CONNECTING, // 来电·连接中（等待接听）
  MONITOR_INCOMING_TALKING,    // 来电·通话中
  MONITOR_OUTGOING_CONNECTING, // 去电·连接中（等待接通）
  MONITOR_OUTGOING_TALKING,    // 去电·通话中
  MONITOR_SESSION_CLOSED,      // 会话已结束，页面未退出
  MONITOR_UNKNOW,
} monitor_state;

void monitor_ctx_set_tuya(bool tuya);

void monitor_status_set(monitor_state flag);

void monitor_device_info_set(const device_info_attr *info);

#endif // _PAGE_LIVE_H