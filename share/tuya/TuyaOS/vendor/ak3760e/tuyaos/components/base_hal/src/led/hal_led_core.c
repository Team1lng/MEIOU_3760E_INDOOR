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
#include "hal_pwm.h"
#include "hal_led.h"
#include "hal_led_core.h"

/*
 * For generic instance, set all the members to NULL
 */
hal_if_led_t hal_if_led = { 0 };


static int led_switch_sysfs(char *name, int on)
{
    char path[MAX_PATH] = {0};
    int ret;

    sprintf(path, LED_BRIGHTNESS_PATH, name);

    if (on) {
        ret = hal_file_write(path, LED_ON_STR, strlen(LED_ON_STR));
    } else {
        ret = hal_file_write(path, LED_OFF_STR, strlen(LED_OFF_STR));
    }

    if (ret < 0) {
        hal_log(HAL_LOG_ERR, "Failed to set led %s\n", name);
        return -1;
    }
    return 0;
}

static int led_switch_gpio(char *name, int on)
{
    int gpio_num;
    int high_active;
    int value;
    hal_gpio_attr_t attr = {0};
    int ret;

    // get led cfg
    ret = hal_cfg_get_item_int(name, KEY_LED_GPIO_NUM, &gpio_num);
    if (ret != 0) {
        hal_log(HAL_LOG_ERR, "Failed to get gpio_num of %s\n", name);
        return -1;
    }
    ret = hal_cfg_get_item_int(name, KEY_LED_HI_ACTIVE, &high_active);
    if (ret != 0) {
        hal_log(HAL_LOG_ERR, "Failed to get high_active of %s\n", name);
        return -1;
    }
    hal_log(HAL_LOG_DEBUG, "%s: gpio=%d, high_active=%d\n",
            name, gpio_num, high_active);

    if (on) {
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

int hal_led_switch_common(char *name, int on)
{
    char path[MAX_PATH] = {0};

    if (NULL == name) {
        hal_log(HAL_LOG_ERR, "invalid led name(null)\n");
        return -1;
    }

    sprintf(path, LED_BRIGHTNESS_PATH, name);

    if (hal_file_exists(path)) {
        return led_switch_sysfs(name, on);
    } else {
        return led_switch_gpio(name, on);
    }
    return 0;
}

int hal_led_pwm_control_common(char *name, hal_led_pwm_attr_t *attr)
{
    hal_pwm_chn_attr_t pwm_attr = {0};
    int pwm_dev;
    int pwm_chn;
    int polarity_inversed;
    int ret;

    if ((NULL == name) || (NULL == attr)) {
        hal_log(HAL_LOG_ERR, "invalid led name or attr\n");
        return -1;
    }

    // get led cfg
    ret = hal_cfg_get_item_int(name, KEY_LED_PWM_DEV, &pwm_dev);
    if (ret != 0) {
        hal_log(HAL_LOG_ERR, "Failed to get pwm_dev of %s\n", name);
        return -1;
    }
    ret = hal_cfg_get_item_int(name, KEY_LED_PWM_CHN, &pwm_chn);
    if (ret != 0) {
        hal_log(HAL_LOG_ERR, "Failed to get pwm_chn of %s\n", name);
        return -1;
    }
    ret = hal_cfg_get_item_int(name, KEY_LED_PWM_POL_INV, &polarity_inversed);
    if (ret != 0) {
        hal_log(HAL_LOG_ERR, "Failed to get polarity_inversed of %s\n", name);
        return -1;
    }

    // disable pwm
    hal_pwm_disable(pwm_dev, pwm_chn);

    // configure pwm channel
    pwm_attr.freq = attr->freq;
    pwm_attr.duty_cycle = attr->duty_cycle;
    pwm_attr.polarity = HAL_PWM_POL_NORMAL;
    if (polarity_inversed) {
        pwm_attr.polarity = HAL_PWM_POL_INVERSED;
    }
    ret = hal_pwm_config(pwm_dev, pwm_chn, &pwm_attr);
    if (ret != 0) {
        hal_log(HAL_LOG_ERR, "Failed to config pwm for %s\n", name);
        return -1;
    }

    // enable pwm
    ret = hal_pwm_enable(pwm_dev, pwm_chn);
    if (ret != 0) {
        hal_log(HAL_LOG_ERR, "Failed to enable %s\n", name);
        return -1;
    }

    return 0;
}

int hal_led_control_common(char *name, hal_led_attr_t *attr)
{
    int controlled_by_pwm = 0;
    int ret;

    if ((NULL == name) || (NULL == attr)) {
        hal_log(HAL_LOG_ERR, "invalid name or led_attr\n");
        return -1;
    }

    ret = hal_cfg_get_item_int(name, KEY_LED_CONTROLLED_BY_PWM, &controlled_by_pwm);
    if (ret != 0) {
        controlled_by_pwm = 0;
    }

    if (controlled_by_pwm) {
        hal_led_pwm_attr_t pwm_attr;
        pwm_attr.freq = attr->freq;
        pwm_attr.duty_cycle = attr->duty_cycle;
        return hal_led_pwm_control_common(name, &pwm_attr);
    } else {
        int led_state;
        if (attr->duty_cycle != 0) {
            led_state = LED_ON;
        } else {
            led_state = LED_OFF;
        }
        return hal_led_switch_common(name, led_state);
    }

    return 0;
}

int hal_led_brightness_set_common(char *name, int brightness)
{
    int ret;
    char path[MAX_PATH] = {0};
    unsigned char bright[8] = {0};

    sprintf(path, LED_BRIGHTNESS_PATH, name);
    sprintf(bright, "%d", brightness);
    ret = hal_file_write(path, bright, strlen(bright));
    if (ret < 0) {
        hal_log(HAL_LOG_ERR, "Failed to set led %s\n", name);
        return -1;
    }

    return 0;
}

/*
 * legacy API, deprecated
 */
// TODO: delete legacy API

static int led_write(char *path, int value)
{
	static const char values_str[] = "01";
	int fd;

	fd = open(path, O_WRONLY);
	if (fd < 0) {
		hal_log(HAL_LOG_ERR, "%s: open path %s failed errno:%d\n",
				__func__, path, -errno);
		return -1;
	}

	if (write(fd, &values_str[value == 0 ? 0 : 1], 1) < 0) {
		hal_log(HAL_LOG_ERR, "%s: Failed to write value errno:%d\n",
				__func__, -errno);
		return -1;
	}

	close(fd);
	return 0;
}

int hal_get_led_syspath(char *name, char *value)
{
	char key[NVRAM_MAX_PARAM_LEN] = {0};
	char def_value[NVRAM_MAX_VALUE_LEN] = {0};

	if (!name)
		return -1;

	snprintf(key, NVRAM_MAX_PARAM_LEN, "hal_%s_led_syspath", name);
	snprintf(def_value, NVRAM_MAX_VALUE_LEN, "/sys/class/leds/%s/brightness",
			name);

	return hal_get_prop(key, value, def_value);
}

int hal_set_led(char *name, int value)
{
	int ret = -1;
	char syspath[NVRAM_MAX_VALUE_LEN] = {0};

	ret = hal_get_led_syspath(name, syspath);
	if (ret < 0)
		return -1;

	return led_write(syspath, value);
}
