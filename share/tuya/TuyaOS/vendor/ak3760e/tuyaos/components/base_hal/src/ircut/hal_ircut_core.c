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
#include "hal_ircut.h"
#include "hal_ircut_core.h"

/*
 * For generic instance, set all the members to NULL
 */
hal_if_ircut_t hal_if_ircut = { 0 };


int hal_ircut_switch_common(char *name, hal_ircut_cmd_e cmd)
{
    char *ircut_name = name;
    int ircut_pin1;
    int ircut_pin2;
    int value_pin1;
    int value_pin2;
    hal_gpio_attr_t attr = {0};
    int ret;

    if (NULL == ircut_name) {
        ircut_name = ITEM_IRCUT;
    }

    // get ircut cfg
    ret = hal_cfg_get_item_int(ircut_name, KEY_IRCUT_PIN1, &ircut_pin1);
    if (ret != 0) {
        hal_log(HAL_LOG_ERR, "Failed to get ircut_pin1 of %s\n", ircut_name);
        return -1;
    }
    ret = hal_cfg_get_item_int(ircut_name, KEY_IRCUT_PIN2, &ircut_pin2);
    if (ret != 0) {
        hal_log(HAL_LOG_ERR, "Failed to get ircut_pin2 of %s\n", ircut_name);
        return -1;
    }
    hal_log(HAL_LOG_DEBUG, "%s: pin1=%d, pin2=%d\n",
            ircut_name, ircut_pin1, ircut_pin2);

    if (cmd == IRCUT_CMD_ON) {
        value_pin1 = GPIO_HIGH;
        value_pin2 = GPIO_LOW;
    } else if (cmd == IRCUT_CMD_OFF) {
        value_pin1 = GPIO_LOW;
        value_pin2 = GPIO_HIGH;
    } else if (cmd == IRCUT_CMD_SLEEP) {
        value_pin1 = GPIO_LOW;
        value_pin2 = GPIO_LOW;
    } else {
        hal_log(HAL_LOG_ERR, "%s: null cmd\n", ircut_name);
        return 0;
    }

    // set gpio

    attr.direction = GPIO_OUT;
    attr.value = value_pin1;
    ret = hal_gpio_config(ircut_pin1, &attr);
    attr.direction = GPIO_OUT;
    attr.value = value_pin2;
    ret |= hal_gpio_config(ircut_pin2, &attr);
    if (ret != 0) {
        hal_log(HAL_LOG_ERR, "Failed to config gpio of %s\n", ircut_name);
        return -1;
    }

    ret = hal_gpio_set_value(ircut_pin1, value_pin1);
    ret |= hal_gpio_set_value(ircut_pin2, value_pin2);
    if (ret != 0) {
        hal_log(HAL_LOG_ERR, "Failed to set gpio of %s\n", ircut_name);
        return -1;
    }

    return 0;
}
