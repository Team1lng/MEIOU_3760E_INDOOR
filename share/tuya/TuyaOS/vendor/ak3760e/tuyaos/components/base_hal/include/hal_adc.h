/*
 * Copyright (C) 2021 Tuya Inc.
 */

#ifndef __HAL_ADC_H__
#define __HAL_ADC_H__

#ifdef __cplusplus
extern "C" {
#endif

/** Initialize an ADC device.
 * @param dev device number.
 * @returns 0 on success, error code on failure.
 */
int hal_adc_init(int dev);

/** Deinitialize an ADC device.
 * @param dev device number.
 * @returns 0 on success, error code on failure.
 */
int hal_adc_deinit(int dev);

/** Read data from an ADC channel.
 * @param dev device number.
 * @param chn channel number.
 * @param value ADC value.
 * @returns 0 on success, error code on failure.
 */
int hal_adc_get_value(int dev, int chn, int *value);

/* adc resolution */
typedef struct {
    int min;    /* minimum value */
    int max;    /* maximum value */
} hal_adc_res_t;

/** Get ADC resolution.
 * @param dev device number.
 * @param chn channel number.
 * @param res ADC resolution.
 * @returns 0 on success, error code on failure.
 */
int hal_adc_get_resolution(int dev, int chn, hal_adc_res_t *res);

#ifdef __cplusplus
}
#endif

#endif
