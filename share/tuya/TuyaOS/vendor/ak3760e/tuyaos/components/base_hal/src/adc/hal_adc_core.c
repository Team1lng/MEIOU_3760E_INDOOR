/*
 * Copyright (C) 2021 Tuya Inc.
 */

#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/reboot.h>
#include <pthread.h>
#include "bcmnvram.h"
#include "hal_log.h"
#include "hal_misc.h"
#include "hal_types.h"
#include "hal_file.h"
#include "hal_adc.h"
#include "hal_adc_core.h"

/*
 * For generic instance, set all the members to NULL
 */
hal_if_adc_t hal_if_adc = {0};

#define HAL_ADC_MAX_DEV_NUM     32
#define HAL_ADC_MAX_CHN_NUM     256

#define ADC_VOLTAGE_RAW     "/sys/bus/iio/devices/iio:device%d/in_voltage%d_raw"
#define ADC_ENABLE_PATH     "/sys/bus/iio/devices/iio:device%d/enable"
#define ADC_ENABLE_STR      "1"
#define ADC_DISABLE_STR     "0"

static int __hal_adc_enable(int dev, bool enable)
{
    char enable_path[MAX_PATH] = {0};
    char enable_str[32] = {0};
    int ret;

    sprintf(enable_path, ADC_ENABLE_PATH, dev);
    if (enable) {
        strcpy(enable_str, ADC_ENABLE_STR);
    } else {
        strcpy(enable_str, ADC_DISABLE_STR);
    }
    ret = hal_file_write(enable_path, enable_str, strlen(enable_str));
    if (ret < 0) {
        return -1;
    }

    return 0;
}

int hal_adc_init_common(int dev)
{
    return __hal_adc_enable(dev, TRUE);
}

int hal_adc_deinit_common(int dev)
{
    return __hal_adc_enable(dev, FALSE);
}

int hal_adc_get_value_common(int dev, int chn, int *value)
{
    char path[MAX_PATH] = {0};
    char buf[32] = {0};
    int ret;

    if (NULL == value) {
        return -1;
    }
    if ((dev < 0) || (dev >= HAL_ADC_MAX_DEV_NUM) ||
        (chn < 0) || (chn >= HAL_ADC_MAX_CHN_NUM)) {
        return -1;
    }

    sprintf(path, ADC_VOLTAGE_RAW, dev, chn);

    ret = hal_file_read(path, buf, sizeof(buf) - 1);
    if (ret < 0) {
        hal_log(HAL_LOG_ERR, "Failed to read %s\n", path);
        return -1;
    }

    *value = atoi(buf);
    return 0;
}

/* This is a platform specific interface implemented by base_hal_custom */
int hal_adc_get_resolution_common(int dev, int chn, hal_adc_res_t *res)
{
    return -1;
}
