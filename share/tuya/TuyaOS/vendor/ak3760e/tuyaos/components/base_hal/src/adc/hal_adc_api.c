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
#include "bcmnvram.h"
#include "hal_log.h"
#include "hal_misc.h"
#include "hal_adc.h"
#include "hal_adc_core.h"

int hal_adc_init(int dev)
{
    if (NULL == hal_if_adc.init)
        return hal_adc_init_common(dev);

    return hal_if_adc.init(dev);
}

int hal_adc_deinit(int dev)
{
    if (NULL == hal_if_adc.deinit)
        return hal_adc_deinit_common(dev);

    return hal_if_adc.deinit(dev);
}

int hal_adc_get_value(int dev, int chn, int *value)
{
    if (NULL == hal_if_adc.get_value)
        return hal_adc_get_value_common(dev, chn, value);

    return hal_if_adc.get_value(dev, chn, value);
}

int hal_adc_get_resolution(int dev, int chn, hal_adc_res_t *res)
{
    if (NULL == hal_if_adc.get_resolution)
        return hal_adc_get_resolution_common(dev, chn, res);

    return hal_if_adc.get_resolution(dev, chn, res);
}
