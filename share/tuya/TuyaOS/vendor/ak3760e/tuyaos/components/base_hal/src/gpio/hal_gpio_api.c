/*
 * Copyright (C) 2021 Tuya Inc.
 */

#include <errno.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "hal_misc.h"
#include "hal_log.h"
#include "hal_gpio.h"
#include "hal_gpio_core.h"

int hal_gpio_config(int num, hal_gpio_attr_t *attr)
{
    if (NULL == hal_if_gpio.config)
        return hal_gpio_config_common(num, attr);

    return hal_if_gpio.config(num, attr);
}

int hal_gpio_set_value(int num, int value)
{
    if (NULL == hal_if_gpio.set_value)
        return hal_gpio_set_value_common(num, value);

    return hal_if_gpio.set_value(num, value);
}

int hal_gpio_get_value(int num, int *value)
{
    if (NULL == hal_if_gpio.get_value)
        return hal_gpio_get_value_common(num, value);

    return hal_if_gpio.get_value(num, value);
}

int hal_gpio_wait_for_irq(int num, int timeout)
{
    if (NULL == hal_if_gpio.wait_for_irq)
        return hal_gpio_wait_for_irq_common(num, timeout);

    return hal_if_gpio.wait_for_irq(num, timeout);
}
