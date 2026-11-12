/*
 * Copyright (C) 2021 Tuya Inc.
 */

#ifndef __HAL_RTC_CORE_H__
#define __HAL_RTC_CORE_H__

#include "hal_rtc.h"

/* HAL interface defination */
typedef struct {
    int (*init)(void);
    int (*deinit)(void);
    int (*set_time)(hal_rtc_time_t *time);
    int (*get_time)(hal_rtc_time_t *time);
} hal_if_rtc_t;

extern hal_if_rtc_t hal_if_rtc;

int hal_rtc_init_common(void);
int hal_rtc_deinit_common(void);
int hal_rtc_set_time_common(hal_rtc_time_t *time);
int hal_rtc_get_time_common(hal_rtc_time_t *time);

#endif
