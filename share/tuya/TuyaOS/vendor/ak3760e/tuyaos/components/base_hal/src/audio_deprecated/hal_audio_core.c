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
#include "hal_config.h"
#include "hal_gpio.h"
#include "hal_board.h"
#include "hal_audio.h"
#include "hal_audio_core.h"

/*
 * For generic instance, set all the members to NULL
 */
hal_if_audio_t hal_if_audio = { 0 };


#define SPEAKER_NAME    "speaker_%d"

int hal_aud_out_speaker_switch_common(int num, bool is_on)
{
    char name[32] = {0};
    int gpio_num;
    int high_active;
    int value;
    hal_gpio_attr_t attr = {0};
    int ret;

    if ((num < 0) || (num > 1000)) {
        hal_log(HAL_LOG_ERR, "Invalid speaker %d\n", num);
        return -1;
    }
    sprintf(name, SPEAKER_NAME, num);

    // get speaker cfg
    ret = hal_cfg_get_item_int(name, KEY_SPEAKER_GPIO_NUM, &gpio_num);
    if (ret != 0) {
        //hal_log(HAL_LOG_ERR, "Failed to get gpio_num of %s\n", name);
        /*
         * If the speaker gpio is not specified in configuration file,
         * we assume that the board has no speaker <NUM>.
         */
        return 0;
    }
    ret = hal_cfg_get_item_int(name, KEY_SPEAKER_HI_ACTIVE, &high_active);
    if (ret != 0) {
        hal_log(HAL_LOG_ERR, "Failed to get high_active of %s\n", name);
        return -1;
    }
    hal_log(HAL_LOG_DEBUG, "%s: gpio=%d, high_active=%d\n",
            name, gpio_num, high_active);

    if (is_on) {
        if (high_active)
            value = GPIO_HIGH;
        else
            value = GPIO_LOW;
    } else {
        if (high_active)
            value = GPIO_LOW;
        else
            value = GPIO_HIGH;
    }

    // set gpio
    attr.direction = GPIO_OUT;
    attr.value = value;
    ret = hal_gpio_config(gpio_num, &attr);
    if (ret < 0) {
        hal_log(HAL_LOG_ERR, "Failed to config gpio of %s\n", name);
        return -1;
    }

    return hal_gpio_set_value(gpio_num, value);
}
