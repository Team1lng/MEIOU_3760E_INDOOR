/*
 * Copyright (C) 2021 Tuya Inc.
 */

#ifndef __HAL_POWER_CORE_H__
#define __HAL_POWER_CORE_H__

#include "hal_power.h"

/* HAL interface defination */
typedef struct {
    int (*set_power_state)(const char *mode);
    int (*get_power_state)(char *mode);
    int (*get_wakeup_src)(unsigned int *value)
} hal_if_power_t;

extern hal_if_power_t hal_if_power;

int hal_set_power_state_common(const char *mode);
int hal_get_power_state_common(char *mode);
int hal_get_wakeup_src_common(unsigned int *value);
#endif
