/*
 * Copyright (C) 2021 Tuya Inc.
 */

#ifndef __HAL_GPIO_CORE_H__
#define __HAL_GPIO_CORE_H__

#include "hal_gpio.h"
#include "hal_file.h"

/* HAL interface defination */
typedef struct {
    int (*config)(int num, hal_gpio_attr_t *attr);
    int (*set_value)(int num, int value);
    int (*get_value)(int num, int *value);
    int (*wait_for_irq)(int num, int timeout);
} hal_if_gpio_t;

extern hal_if_gpio_t hal_if_gpio;


#define GPIO_EXPORT_PATH        "/sys/class/gpio/export"
#define GPIO_PATH               "/sys/class/gpio/gpio%d"
#define GPIO_DIRECTION_PATH     "/sys/class/gpio/gpio%d/direction"
#define GPIO_VALUE_PATH         "/sys/class/gpio/gpio%d/value"
#define GPIO_EDGE_PATH          "/sys/class/gpio/gpio%d/edge"

#define GPIO_DIRECTION_IN       "in"
/* Writing as "out" defaults to initializing the value as low */
#define GPIO_DIRECTION_OUT      "out"
/* Writing as "high" to configure the GPIO as an output with high value */
#define GPIO_DIRECTION_OUT_HIGH "high"

#define GPIO_VALUE_STR_HIGH     "1"
#define GPIO_VALUE_STR_LOW      "0"

#define GPIO_EDGE_STR_RISING    "rising"
#define GPIO_EDGE_STR_FALLING   "falling"
#define GPIO_EDGE_STR_BOTH      "both"

int hal_gpio_config_common(int num, hal_gpio_attr_t *attr);
int hal_gpio_set_value_common(int num, int value);
int hal_gpio_get_value_common(int num, int *value);
int hal_gpio_wait_for_irq_common(int num, int timeout);

#endif
