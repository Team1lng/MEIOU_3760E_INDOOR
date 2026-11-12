/*
 * Copyright (C) 2021 Tuya Inc.
 */

#ifndef __HAL_PWM_H__
#define __HAL_PWM_H__

#include "hal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HAL_PWM_POL_NORMAL      0
#define HAL_PWM_POL_INVERSED    1

/* pwm channel attribute */
typedef struct {
    int freq;           /* frequency, in Hz */
    int duty_cycle;     /* range [0, 10000], e.g. 1234 means 12.34% */
    int polarity;       /* normal or inversed */
    int reserved[5];
} hal_pwm_chn_attr_t;

/** Configure a pwm channel.
 * @param dev device number.
 * @param chn channel number.
 * @param attr channel attribute.
 * @returns 0 on success, error code on failure.
 */
int hal_pwm_config(int dev, int chn, hal_pwm_chn_attr_t *attr);

/** Enable a pwm channel.
 * @param dev device number.
 * @param chn channel number.
 * @returns 0 on success, error code on failure.
 */
int hal_pwm_enable(int dev, int chn);

/** Disable a pwm channel.
 * @param dev device number.
 * @param chn channel number.
 * @returns 0 on success, error code on failure.
 */
int hal_pwm_disable(int dev, int chn);

#ifdef __cplusplus
}
#endif

#endif
