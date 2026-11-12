#ifndef _PAGE_COMMON_CALL_H_
#define _PAGE_COMMON_CALL_H_
#include "device_types.h"
void common_call_pending_call_container_create(void);
void common_call_extern_call_callback(void *arg);
void common_call_incoming_handle(device_info_attr device_info);
void common_call_session_complete_handle(void);
void common_call_door_no_one_home_broadcast(int channel);
#endif // _PAGE_COMMON_CALL_H_