/*
 * Copyright (C) 2021 Tuya Inc.
 */

#ifndef __HAL_BACKLIGHT_CORE_H__
#define __HAL_BACKLIGHT_CORE_H__

#include "hal_backlight.h"

/* HAL interface defination */
typedef struct {
    int (*set_value)(int brightness);
    int (*get_value)(void);
} hal_if_backlight_t;

extern hal_if_backlight_t hal_if_backlight;

int hal_backlight_set_value_common(int brightness);
int hal_backlight_get_value_common(void);
#endif
