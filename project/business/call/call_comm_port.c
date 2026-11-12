/**
 * @file        call_comm_port.c
 * @brief       通话系统通信接口实现层
 * @details     提供呼叫、振铃、接听、挂断、心跳等通信接口注册与实现
 * @ingroup     call_comm
 */

#include "call_comm_port.h"
#include "call_comm_interface.h"
#include "sat_ipcamera.h"
#include "sat_user_common.h"
#include "user_data.h"
/** TCP 通信端口号 */
#define TCP_PORT 80
/** TCP 连接超时时间（毫秒） */
#define TCP_TIMEOUT 500
/** 邀请专用 TCP 超时时间 */
#define INVITE_TCP_TIMEOUT 1000

/**
 * @brief 全局通信接口实例（默认初始化为 0）
 */
call_comm_interface_t g_call_comm_interface = {0};

/**
 * @brief 呼叫邀请接口实现
 * @param[in] attr 设备信息结构体
 * @return 成功/失败
 */
bool call_invite(device_info_attr attr)
{
  char ip[32] = {0};
  if (generate_ip(attr.building, attr.room, attr.device_type, ip) != 0)
  {
    return false;
  }
  device_attr_info *info = &user_data_get()->private_cfg.device.attr;
  return sat_ipcamera_device_call_invite(info->device_type, info->building, info->room,
                                         ip, TCP_PORT, "admin", "123456789",
                                         INVITE_TCP_TIMEOUT, true);
}

/**
 * @brief 呼叫振铃接口实现
 * @param[in] attr 设备信息结构体
 */
void call_ringing(device_info_attr attr)
{
  char ip[32] = {0};
  if (generate_ip(attr.building, attr.room, attr.device_type, ip) != 0)
  {
    return;
  }
  device_attr_info *info = &user_data_get()->private_cfg.device.attr;
  sat_ipcamera_device_call_ringing(info->device_type, info->building, info->room,
                                   ip, TCP_PORT, "admin", "123456789",
                                   INVITE_TCP_TIMEOUT, true);
}

/**
 * @brief 呼叫接听接口实现
 * @param[in] attr 设备信息结构体
 */
void call_accept(device_info_attr attr)
{
  char ip[32] = {0};
  if (generate_ip(attr.building, attr.room, attr.device_type, ip) != 0)
  {
    return;
  }
  device_attr_info *info = &user_data_get()->private_cfg.device.attr;
  sat_ipcamera_device_call_accept(info->device_type, info->building, info->room,
                                  ip, TCP_PORT, "admin", "123456789",
                                  INVITE_TCP_TIMEOUT, true);
}

/**
 * @brief 呼叫忙线接口实现
 * @param[in] attr 设备信息结构体
 */
void call_busy(device_info_attr attr)
{
  char ip[32] = {0};
  if (generate_ip(attr.building, attr.room, attr.device_type, ip) != 0)
  {
    return;
  }
  device_attr_info *info = &user_data_get()->private_cfg.device.attr;
  sat_ipcamera_device_call_busy(info->device_type, info->building, info->room,
                                ip, TCP_PORT, "admin", "123456789",
                                INVITE_TCP_TIMEOUT, true);
}

/**
 * @brief 呼叫错误接口实现
 * @param[in] attr 设备信息结构体
 */
void call_error(device_info_attr attr)
{
  char ip[32] = {0};
  if (generate_ip(attr.building, attr.room, attr.device_type, ip) != 0)
  {
    return;
  }
  device_attr_info *info = &user_data_get()->private_cfg.device.attr;
  sat_ipcamera_device_call_error(info->device_type, info->building, info->room,
                                 ip, TCP_PORT, "admin", "123456789",
                                 INVITE_TCP_TIMEOUT, true);
}

/**
 * @brief 呼叫结束接口实现
 * @param[in] attr 设备信息结构体
 */
void call_end(device_info_attr attr)
{
  char ip[32] = {0};
  if (generate_ip(attr.building, attr.room, attr.device_type, ip) != 0)
  {
    return;
  }
  device_attr_info *info = &user_data_get()->private_cfg.device.attr;
  sat_ipcamera_device_call_end(info->device_type, info->building, info->room,
                               ip, TCP_PORT, "admin", "123456789",
                               INVITE_TCP_TIMEOUT, true);
}

/**
 * @brief 心跳请求接口实现
 * @param[in] attr 设备信息结构体
 */
void call_heartbeat_request(device_info_attr attr)
{
  char ip[32] = {0};
  if (generate_ip(attr.building, attr.room, attr.device_type, ip) != 0)
  {
    return;
  }
  device_attr_info *info = &user_data_get()->private_cfg.device.attr;
  sat_ipcamera_device_call_heartbeat_request(info->device_type, info->building, info->room,
                                             ip, TCP_PORT, "admin", "123456789",
                                             INVITE_TCP_TIMEOUT, true);
}

/**
 * @brief 心跳响应接口实现
 * @param[in] attr 设备信息结构体
 */
void call_heartbeat_response(device_info_attr attr)
{
  char ip[32] = {0};
  if (generate_ip(attr.building, attr.room, attr.device_type, ip) != 0)
  {
    return;
  }
  device_attr_info *info = &user_data_get()->private_cfg.device.attr;
  sat_ipcamera_device_call_heartbeat_response(info->device_type, info->building, info->room,
                                              ip, TCP_PORT, "admin", "123456789",
                                              INVITE_TCP_TIMEOUT, true);
}

/**
 * @brief 通信接口初始化（填充函数指针表）
 * @details 仅填充结构体，不再调用注册。通过 call_config_t.comm_interface 传入 sat_call_init
 * @return true 永远成功
 */
bool call_comm_init(void)
{
  g_call_comm_interface.call_invite = call_invite;
  g_call_comm_interface.call_accept = call_accept;
  g_call_comm_interface.call_ringing = call_ringing;
  g_call_comm_interface.call_busy = call_busy;
  g_call_comm_interface.call_error = call_error;
  g_call_comm_interface.call_end = call_end;
  g_call_comm_interface.call_heartbeat_request = call_heartbeat_request;
  g_call_comm_interface.call_heartbeat_response = call_heartbeat_response;

  return true;
}

void call_comm_uninit(void)
{
}