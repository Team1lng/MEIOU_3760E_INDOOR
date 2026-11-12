#ifndef __WHEEL_TIMER_H__
#define __WHEEL_TIMER_H__


#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef void* WHEEL_TIMER_HANDLE;
#define WHEEL_TIMER_INVALID_HANDLE ((void*)0)

typedef void (*wheel_timer_expire_cb)(WHEEL_TIMER_HANDLE handle, void* pri_data);

int wheel_timer_init();
int wheel_timer_deinit();

WHEEL_TIMER_HANDLE wheel_timer_add(uint64_t delay_ms, wheel_timer_expire_cb cb, void* pri_data);

int wheel_timer_remove(WHEEL_TIMER_HANDLE handle);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __WHEEL_TIMER_H__

