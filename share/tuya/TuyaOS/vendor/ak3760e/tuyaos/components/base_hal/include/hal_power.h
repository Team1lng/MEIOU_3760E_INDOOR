/*
 * Copyright (C) 2021 Tuya Inc.
 */

#ifndef __HAL_POWER_H__
#define __HAL_POWER_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifndef POWER_MODE_PATH
#define POWER_MODE_PATH 		"/sys/power/state"
#endif

#ifndef WAKEUP_SRC_PATH
#define WAKEUP_SRC_PATH			"/sys/power/wakeup_src"
#endif

int hal_set_power_state(const char *mode);
int hal_get_power_state(char *mode);
int hal_get_wakeup_src(unsigned int *value);

#ifdef __cplusplus
}
#endif

#endif
