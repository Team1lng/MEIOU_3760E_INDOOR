#ifndef __CALL_COMM_INTERFACE_H__
#define __CALL_COMM_INTERFACE_H__

#include "device_types.h"
#include "stdbool.h"
typedef struct
{
  bool (*call_invite)(device_info_attr attr);
  void (*call_accept)(device_info_attr attr);
  void (*call_end)(device_info_attr attr);
  void (*call_ringing)(device_info_attr attr);
  void (*call_busy)(device_info_attr attr);
  void (*call_error)(device_info_attr attr);
  void (*call_heartbeat_request)(device_info_attr attr); // 修正拼写
  void (*call_heartbeat_response)(device_info_attr attr);
} call_comm_interface_t;

#endif