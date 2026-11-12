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
#include "hal_led.h"
#include "hal_led_core.h"

int hal_led_switch(char *name, int on)
{
    if (NULL == hal_if_led.switch_ctrl)
        return hal_led_switch_common(name, on);

    return hal_if_led.switch_ctrl(name, on);
}

int hal_led_pwm_control(char *name, hal_led_pwm_attr_t *attr)
{
    if (NULL == hal_if_led.pwm_control)
        return hal_led_pwm_control_common(name, attr);

    return hal_if_led.pwm_control(name, attr);
}

int hal_led_control(char *name, hal_led_attr_t *attr)
{
    if (NULL == hal_if_led.led_control)
        return hal_led_control_common(name, attr);

    return hal_if_led.led_control(name, attr);
}

int hal_led_brightness_set(char *name, int brightness)
{
    if (NULL == hal_if_led.brightness_set)
        return hal_led_brightness_set_common(name, brightness);

    return hal_if_led.brightness_set(name, brightness);
}
