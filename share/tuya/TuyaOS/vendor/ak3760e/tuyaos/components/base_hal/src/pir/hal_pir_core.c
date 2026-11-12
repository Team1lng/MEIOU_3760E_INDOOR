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
#include "hal_pir.h"
#include "hal_pir_core.h"

/*
 * For generic instance, set all the members to NULL
 */
hal_if_pir_t hal_if_pir = { 0 };


int hal_pir_enable_common(char *name, bool enable)
{
    int gpio_en;
    int gpio_en_hi_active = 0;
    int gpio_en_value;
    hal_gpio_attr_t attr = {0};
    int ret;

    /*
     * If the key 'gpio_enable' is not set in configuration file,
     * this function will do nothing.
     */
    ret = hal_cfg_get_item_int(ITEM_PIR, KEY_PIR_GPIO_EN, &gpio_en);
    if (ret != 0) {
        hal_log(HAL_LOG_NOTICE, "%s has no gpio_enable pin\n", ITEM_PIR);
        return 0;
    }

    /*
     * If key 'gpio_enable' is set in configuration file,
     * the key 'gpio_enable_high_active' should be set.
     */
    ret = hal_cfg_get_item_int(ITEM_PIR, KEY_PIR_GPIO_EN_HI_ACTIVE,
                               &gpio_en_hi_active);
    if (ret != 0) {
        hal_log(HAL_LOG_ERR, "Failed to get gpio_enable_high_active of\n", ITEM_PIR);
        return -1;
    }

    /* set gpio to enable/disable pir */

    if (enable) {
        if (gpio_en_hi_active) {
            gpio_en_value = GPIO_HIGH;
        } else {
            gpio_en_value = GPIO_LOW;
        }
    } else {
        if (gpio_en_hi_active) {
            gpio_en_value = GPIO_LOW;
        } else {
            gpio_en_value = GPIO_HIGH;
        }
    }

    attr.direction = GPIO_OUT;
    attr.value = gpio_en_value;
    ret = hal_gpio_config(gpio_en, &attr);
    if (ret != 0) {
        hal_log(HAL_LOG_ERR, "Failed to config gpio_en of %s\n", ITEM_PIR);
        return -1;
    }

    ret = hal_gpio_set_value(gpio_en, gpio_en_value);
    if (ret != 0) {
        hal_log(HAL_LOG_ERR, "Failed to set gpio_en of %s\n", ITEM_PIR);
        return -1;
    }

    return 0;
}

int hal_pir_set_sens_common(char *name, int sens)
{
    int gpio_set1;
    int gpio_set2;
    int value_set1;
    int value_set2;
    hal_gpio_attr_t attr = {0};
    int ret;

    if (sens == HAL_PIR_SENS_DEFAULT) {
        // The default sensitivity is configured by hardware
        return 0;
    }

    ret = hal_cfg_get_item_int(ITEM_PIR, KEY_PIR_GPIO_SENS_SET1, &gpio_set1);
    ret |= hal_cfg_get_item_int(ITEM_PIR, KEY_PIR_GPIO_SENS_SET2, &gpio_set2);
    if (ret != 0) {
        hal_log(HAL_LOG_ERR, "Failed to get sens_set pin of %s\n", ITEM_PIR);
        return -1;
    }

    /*
     * hardware sensitivity setting:
     *     Low    : PIR_SET1=0  PIR_SET2=0
     *     Middle : PIR_SET1=0  PIR_SET2=1
     *     High   : PIR_SET1=1  PIR_SET2=0
     */
    if (sens == HAL_PIR_SENS_LOW) {
        value_set1 = GPIO_LOW;
        value_set2 = GPIO_LOW;
    } else if (sens == HAL_PIR_SENS_MIDDLE) {
        value_set1 = GPIO_LOW;
        value_set2 = GPIO_HIGH;
    } else if (sens == HAL_PIR_SENS_HIGH) {
        value_set1 = GPIO_HIGH;
        value_set2 = GPIO_LOW;
    } else {
        hal_log(HAL_LOG_ERR, "%s: unsupported sensitivity\n", ITEM_PIR);
        return -1;
    }

    // set gpio to configure sensitivity

    attr.direction = GPIO_OUT;
    attr.value = value_set1;
    ret = hal_gpio_config(gpio_set1, &attr);
    attr.direction = GPIO_OUT;
    attr.value = value_set2;
    ret |= hal_gpio_config(gpio_set2, &attr);
    if (ret != 0) {
        hal_log(HAL_LOG_ERR, "Failed to config gpio_set of %s\n", ITEM_PIR);
        return -1;
    }

    ret = hal_gpio_set_value(gpio_set1, value_set1);
    ret |= hal_gpio_set_value(gpio_set2, value_set2);
    if (ret != 0) {
        hal_log(HAL_LOG_ERR, "Failed to set gpio_set of %s\n", ITEM_PIR);
        return -1;
    }

    return 0;
}

int hal_pir_detect_state_common(char *name, hal_pir_state_e *state, int timeout)
{
    int gpio_irq;
    int gpio_irq_hi_active = 0;
    int value;
    hal_gpio_attr_t attr = {0};
    int ret;

    if (NULL == state) {
        hal_log(HAL_LOG_ERR, "invalid pir state (null)\n");
        return -1;
    }

    // config pir irq gpio

    ret = hal_cfg_get_item_int(ITEM_PIR, KEY_PIR_GPIO_IRQ, &gpio_irq);
    ret |= hal_cfg_get_item_int(ITEM_PIR, KEY_PIR_GPIO_IRQ_HI_ACTIVE,
                                &gpio_irq_hi_active);
    if (ret != 0) {
        hal_log(HAL_LOG_ERR, "Failed to get gpio_irq pin of %s\n", ITEM_PIR);
        return -1;
    }

    attr.direction = GPIO_IN;
    attr.irq_en = TRUE;
    if (gpio_irq_hi_active) {
        attr.irq_type = GPIO_IRQ_EDGE_RISING;
    } else {
        attr.irq_type = GPIO_IRQ_EDGE_FALLING;
    }
    ret = hal_gpio_config(gpio_irq, &attr);
    if (ret != 0) {
        hal_log(HAL_LOG_ERR, "Failed to set gpio_irq pin of %s\n", ITEM_PIR);
        return -1;
    }

    // waiting for PIR interrupt
    if (timeout != 0) {
        ret = hal_gpio_wait_for_irq(gpio_irq, timeout);
        if (ret != 0) {
            return ret;
        }
    }

    // get PIR state

    ret = hal_gpio_get_value(gpio_irq, &value);
    if (ret != 0) {
        printf("Failed to get gpio_irq value\n");
        return -1;
    }

    if (gpio_irq_hi_active) {
        if (value == GPIO_HIGH) {
            *state = HAL_PIR_TRIG;
        } else {
            *state = HAL_PIR_NON_TRIG;
        }
    } else {
        if (value == GPIO_HIGH) {
            *state = HAL_PIR_NON_TRIG;
        } else {
            *state = HAL_PIR_TRIG;
        }
    }

    return 0;
}
