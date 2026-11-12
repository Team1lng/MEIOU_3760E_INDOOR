/*
 * Copyright (C) 2021 Tuya Inc.
 */

#ifndef __HAL_ADC_CORE_H__
#define __HAL_ADC_CORE_H__

#include "hal_adc.h"

/* HAL interface defination */
typedef struct {
    int (*init)(int dev);
    int (*deinit)(int dev);
    int (*get_value)(int dev, int chn, int *value);
    int (*get_resolution)(int dev, int chn, hal_adc_res_t *res);
} hal_if_adc_t;

extern hal_if_adc_t hal_if_adc;


int hal_adc_init_common(int dev);
int hal_adc_deinit_common(int dev);
int hal_adc_get_value_common(int dev, int chn, int *value);
int hal_adc_get_resolution_common(int dev, int chn, hal_adc_res_t *res);

#endif
