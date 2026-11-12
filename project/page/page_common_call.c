/**
 * @file page_common_call.c
 * @brief 公共通话模块处理逻辑
 * @details 包含待处理通话列表创建、通话事件回调、通话状态提示、会话收尾等通用通话逻辑
 * @author
 * @date
 */
#include "page_common_call.h"
#include "page_common.h"
#include "page_tabel.h"
#include "call_api.h"
#include "lv_common.h"
#include "sat_user_common.h"
#include "page_live.h"
#include "page_call.h"
#include "live/testRTSPClient.h"
#include "sat_ipcamera.h"
#include "tuya/tuya_sdk.h"
#include "../monitor.h"
#include "user_data.h"
#include "language.h"

/************************** 旧资源宏映射 **************************/
#define ICON_TALK_PNG_OFFSET ICON_LIVE_TALK_PNG_OFFSET
#define ICON_TALK_PNG_SIZE ICON_LIVE_TALK_PNG_SIZE
#define ICON_CALL_ARROW_PNG_OFFSET ICON_LIVE_ADJUST_PNG_OFFSET
#define ICON_CALL_ARROW_PNG_SIZE ICON_LIVE_ADJUST_PNG_SIZE
// static void common_call_incoming_handle(device_info_attr device_info);
static void common_call_outgoing_ringing_handle(void);
static void common_call_outgoing_process_handle(void);
static void common_call_connected_handle(void);
static void common_call_end_handle(void);
void common_call_session_complete_handle(void);
static void common_call_away_mode_no_one_home_broadcast(device_info_attr device_info);

/**
 * @brief 通知门口机播报“无人在家请留言”（统一播报入口）
 */
void common_call_door_no_one_home_broadcast(int channel)
{
  if (user_data_get()->private_cfg.device.attr.device_type != DEVICE_TYPE_INDOOR_ID1)
  {
    return;
  }

  if (channel != MON_CHANNEL_DOOR1 && channel != MON_CHANNEL_DOOR2)
  {
    return;
  }

  struct ipcamera_info *door_dev = &user_data_get()->public_cfg.net_door_device[channel];
  if (door_dev->ipaddr[0] == '\0')
  {
    return;
  }

  sat_ipcamera_device_no_one_home_broadcast(door_dev->ipaddr,
                                            door_dev->port,
                                            door_dev->username,
                                            door_dev->password,
                                            (int)language_id_get(),
                                            1500, false);
}

/**
 * @brief 离家模式下通知门口机播放“无人在家请留言”
 */
static void common_call_away_mode_no_one_home_broadcast(device_info_attr device_info)
{
  if (user_data_get()->public_cfg.global_func.scene_mode != SCENE_MODE_AWAY)
  {
    return;
  }

  if ((device_info.device_type != DEVICE_TYPE_DOOR_ID1) &&
      (device_info.device_type != DEVICE_TYPE_DOOR_ID2))
  {
    return;
  }

  int door_channel = door_monitor_channel_get_by_device_type(device_info.device_type);
  if (door_channel == MON_CHANNEL_UNKNOW)
  {
    return;
  }

  common_call_door_no_one_home_broadcast(door_channel);
}

/**
 * @brief 处理来电事件
 * @param device_info 来电设备信息
 * @details 1. 初始化监控通道和状态，跳转到监控页面
 *          2. 切换到当前来电会话
 *          3. 根据设备类型判断：室内设备跳转到对讲页面，其他设备跳转到监控页面
 */
void common_call_incoming_handle(device_info_attr device_info)
{
  common_call_away_mode_no_one_home_broadcast(device_info);

  // 初始化监控通道和来电状态，默认跳转到监控页面
  int channel = door_monitor_channel_get_by_device_type(device_info.device_type);
  // 根据设备类型跳转对应页面
  if (device_info.device_type >= DEVICE_TYPE_INDOOR_MIN && device_info.device_type <= DEVICE_TYPE_INDOOR_MAX)
  {
    // 室内设备：设置对讲来电状态，跳转到对讲页面
    intercom_status_set(INTERCOM_INCOMING);
    intercom_device_info_set(&device_info);
    Amy_lv_page_goto(lv_page(call), LV_SCR_LOAD_ANIM_NONE, );
  }
  else
  {
    // 非室内设备：重新初始化监控通道，跳转到监控页面
    channel = door_monitor_channel_get_by_device_type(device_info.device_type);
    monitor_channel_set(channel);
    monitor_status_set(MONITOR_INCOMING_CONNECTING);
    monitor_device_info_set(&device_info);
    Amy_lv_page_goto(lv_page(live), LV_SCR_LOAD_ANIM_NONE, );
  }

  AMY_LOG_INFO("来电事件处理完成\n");
}

/**
 * @brief 处理外呼振铃事件
 * @details 外呼过程中收到振铃事件的基础处理（当前仅打日志，可扩展）
 */
static void common_call_outgoing_ringing_handle(void)
{
  AMY_LOG_INFO("收到外呼振铃事件\n");
}

/**
 * @brief 处理外呼中事件
 * @details 外呼过程中收到"呼叫中"事件的基础处理（当前仅打日志，可扩展）
 */
static void common_call_outgoing_process_handle(void)
{
  AMY_LOG_INFO("收到外呼中事件\n");
}

/**
 * @brief 处理通话接通事件
 * @details 通话接通后的基础处理（当前仅打日志，可扩展）
 */
static void common_call_connected_handle(void)
{
  AMY_LOG_INFO("收到通话接通事件\n");
}

/**
 * @brief 处理通话结束事件（事件层面）
 * @details 仅响应"通话结束"指令的基础处理（当前仅打日志，可扩展：如标记事件已接收、清理临时状态等）
 */
static void common_call_end_handle(void)
{
  AMY_LOG_INFO("收到通话结束事件\n");
}

/**
 * @brief 外部通话事件回调入口
 * @param arg 事件参数，类型为call_cb_attr*
 * @details 统一处理所有通话事件，根据事件类型分发到对应处理函数
 */
void common_call_extern_call_callback(void *arg)
{
  // 类型转换：事件参数转为通话事件属性结构体
  call_cb_attr *attr = (call_cb_attr *)arg;
  if (attr == NULL)
  {
    AMY_LOG_ERROR("通话事件参数为空\n");
    return;
  }

  int call_event = attr->call_event; // 获取事件类型
  // 根据事件类型分发处理
  switch (call_event)
  {
  case AMY_CALL_EVENT_ID_CALL_INCOMING: // 来电事件
    common_call_incoming_handle(attr->device_info);
    break;
  case AMY_CALL_EVENT_ID_CALL_OUTGOING_RINGING: // 外呼振铃
    common_call_outgoing_ringing_handle();
    break;
  case AMY_CALL_EVENT_ID_CALL_OUTGOING_PROCESS: // 外呼中
    common_call_outgoing_process_handle();
    break;
  case AMY_CALL_EVENT_ID_CALL_CONNECTED: // 通话接通
    common_call_connected_handle();
    break;
  case AMY_CALL_EVENT_ID_CALL_END: // 通话结束
    common_call_end_handle();
    break;
  default: // 未知事件
    AMY_LOG_WARNING("收到未知通话事件：%d\n", call_event);
    break;
  }
}

/**
 * @brief 完整会话结束后的最终处理（全流程收尾）
 * @details 1. 检查是否有待处理的其他会话
 *          2. 有待处理会话：切换到该会话，并跳转到对应页面（对讲/监控）
 *          3. 无待处理会话：跳转到首页
 */
void common_call_session_complete_handle(void)
{
  intercom_status_set(INTERCOM_UNKNOW);
  device_info_attr device_info = {0}; // 待处理通话设备信息
  int actual_count = 0;               // 待处理通话数量
  // 获取第一个待处理通话（最多1条）
  sat_get_all_pending_calls(&device_info, 1, &actual_count);
  if (actual_count != 0) // 存在未处理的其他会话
  {
    // 切换到待处理会话
    sat_call_switch_call(&device_info);
    // 根据设备类型跳转对应页面
    if ((device_info.device_type >= DEVICE_TYPE_INDOOR_MIN && device_info.device_type <= DEVICE_TYPE_INDOOR_MAX) != 0)
    {
      // 室内家庭设备：设置对讲来电状态，跳转到对讲页面
      intercom_status_set(INTERCOM_INCOMING);
      intercom_device_info_set(&device_info);
      Amy_lv_page_goto(lv_page(call), LV_SCR_LOAD_ANIM_NONE, );
    }
    else
    {
      // 其他设备：初始化监控通道，跳转到监控页面
      int channel = door_monitor_channel_get_by_device_type(device_info.device_type);
      monitor_channel_set(channel);
      monitor_status_set(MONITOR_INCOMING_CONNECTING);
      monitor_device_info_set(&device_info);
      common_call_away_mode_no_one_home_broadcast(device_info);
      Amy_lv_page_goto(lv_page(live), LV_SCR_LOAD_ANIM_NONE, );
    }
  }
  else // 无待处理会话，跳转到首页
  {
    close_client_speaker();
    close_server_speaker();
    tuya_media_stream_pause();
    Amy_lv_page_goto(lv_page(standby), LV_SCR_LOAD_ANIM_NONE, );
  }
}
