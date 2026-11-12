/*
 * Copyright (C) 2021 Tuya Inc.
 */

#ifndef __HAL_PWM_CORE_H__
#define __HAL_PWM_CORE_H__

#include "hal_pwm.h"

/* HAL interface defination */
typedef struct {
    int (*config)(int dev, int chn, hal_pwm_chn_attr_t *attr);
    int (*enable)(int dev, int chn);
    int (*disable)(int dev, int chn);
} hal_if_pwm_t;

extern hal_if_pwm_t hal_if_pwm;

#define HAL_PWM_MAX_CHIP_NUM        32
#define HAL_PWM_MAX_CHANNEL_NUM     512

int hal_pwm_config_common(int dev, int chn, hal_pwm_chn_attr_t *attr);
int hal_pwm_enable_common(int dev, int chn);
int hal_pwm_disable_common(int dev, int chn);

#endif
