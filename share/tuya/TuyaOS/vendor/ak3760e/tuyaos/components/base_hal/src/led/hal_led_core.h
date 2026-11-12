/*
 * Copyright (C) 2021 Tuya Inc.
 */

#ifndef __HAL_LED_CORE_H__
#define __HAL_LED_CORE_H__

#include "hal_led.h"

/* HAL interface defination */
typedef struct {
    int (*led_control)(char *name, hal_led_attr_t *attr);
    int (*switch_ctrl)(char *name, int on);
    int (*pwm_control)(char *name, hal_led_pwm_attr_t *attr);
    int (*brightness_set)(char *name, int brightness);
} hal_if_led_t;

extern hal_if_led_t hal_if_led;


#define LED_BRIGHTNESS_PATH     "/sys/class/leds/%s/brightness"
#define LED_ON_STR              "1"
#define LED_OFF_STR             "0"

int hal_led_switch_common(char *name, int on);
int hal_led_pwm_control_common(char *name, hal_led_pwm_attr_t *attr);
int hal_led_control_common(char *name, hal_led_attr_t *attr);
int hal_led_brightness_set_common(char *name, int brightness);

#endif
