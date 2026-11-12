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
#include <poll.h>
#include "bcmnvram.h"
#include "hal_log.h"
#include "hal_misc.h"
#include "hal_config.h"
#include "hal_gpio.h"
#include "hal_board.h"
#include "hal_gpio_key.h"
#include "hal_gpio_key_core.h"


int hal_gpio_key_get_state_common(char *name, hal_gpio_key_state_e *state)
{
    int gpio_num;
    int gpio_hi_active = 0;
    int value;
    hal_gpio_attr_t attr = {0};
    int ret;

    if (NULL == name) {
        hal_log(HAL_LOG_ERR, "invalid key name (null)\n");
        return -1;
    }

    if (NULL == state) {
        hal_log(HAL_LOG_ERR, "invalid key state (null)\n");
        return -1;
    }

    // config key gpio

    ret = hal_cfg_get_item_int(name, KEY_KEY_GPIO_NUM, &gpio_num);
    ret |= hal_cfg_get_item_int(name, KEY_KEY_HI_ACTIVE,
                                &gpio_hi_active);
    if (ret != 0) {
        hal_log(HAL_LOG_ERR, "Failed to get gpio of %s\n", name);
        return -1;
    }

    attr.direction = GPIO_IN;
    attr.irq_en = FALSE;
    ret = hal_gpio_config(gpio_num, &attr);
    if (ret != 0) {
        hal_log(HAL_LOG_ERR, "Failed to set gpio of %s\n", name);
        return -1;
    }

    // get key state

    ret = hal_gpio_get_value(gpio_num, &value);
    if (ret != 0) {
        printf("Failed to get gpio value of %s\n", name);
        return -1;
    }

    if (gpio_hi_active) {
        if (value == GPIO_HIGH) {
            *state = HAL_GPIO_KEY_DOWN;
        } else {
            *state = HAL_GPIO_KEY_UP;
        }
    } else {
        if (value == GPIO_HIGH) {
            *state = HAL_GPIO_KEY_UP;
        } else {
            *state = HAL_GPIO_KEY_DOWN;
        }
    }

    return 0;
}
