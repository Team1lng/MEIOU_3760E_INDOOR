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
#include "hal_gpio.h"
#include "hal_gpio_core.h"

/*
 * For generic instance, set all the members to NULL
 */
hal_if_gpio_t hal_if_gpio = { 0 };


int hal_gpio_config_common(int num, hal_gpio_attr_t *attr)
{
    char gpio_path[MAX_PATH] = {0};
    char direction_path[MAX_PATH] = {0};
    char value_path[MAX_PATH] = {0};
    char edge_path[MAX_PATH] = {0};
    char gpio_num[32] = {0};
    int ret = 0;

    if (NULL == attr) {
        hal_log(HAL_LOG_ERR, "invalid gpio attr\n");
        return -1;
    }

    sprintf(gpio_num, "%d", num);
    sprintf(gpio_path, GPIO_PATH, num);
    sprintf(direction_path, GPIO_DIRECTION_PATH, num);
    sprintf(value_path, GPIO_VALUE_PATH, num);

    /* export gpio */
    if (! hal_file_exists(gpio_path)) {
        ret = hal_file_write(GPIO_EXPORT_PATH, gpio_num, strlen(gpio_num));
        if (ret < 0) {
            hal_log(HAL_LOG_ERR, "Failed to export gpio(%d)\n", num);
            return -1;
        }
    }

    /* set gpio direction */
    if (hal_file_exists(direction_path)) {

        if (attr->direction == GPIO_OUT) {
            if (attr->value == GPIO_HIGH) {
                ret = hal_file_write(direction_path, GPIO_DIRECTION_OUT_HIGH,
                                    strlen(GPIO_DIRECTION_OUT_HIGH));
            } else {
                ret = hal_file_write(direction_path, GPIO_DIRECTION_OUT,
                                    strlen(GPIO_DIRECTION_OUT));
            }
        } else {
            ret = hal_file_write(direction_path, GPIO_DIRECTION_IN,
                                strlen(GPIO_DIRECTION_IN));
        }

        if (ret < 0) {
            hal_log(HAL_LOG_ERR, "Failed to set gpio(%d) direction\n", num);
            return -1;
        }
    } else {
        hal_log(HAL_LOG_WARNING,
                "kernel doesn't support changing the direction of gpio(%d)\n", num);
    }

    /* set gpio interrupt */
    if (attr->irq_en && (attr->direction == GPIO_IN)) {
        sprintf(edge_path, GPIO_EDGE_PATH, num);

        if (attr->irq_type == GPIO_IRQ_EDGE_RISING) {
            ret = hal_file_write(edge_path, GPIO_EDGE_STR_RISING,
                                 strlen(GPIO_EDGE_STR_RISING));
        } else if (attr->irq_type == GPIO_IRQ_EDGE_FALLING) {
            ret = hal_file_write(edge_path, GPIO_EDGE_STR_FALLING,
                                 strlen(GPIO_EDGE_STR_FALLING));
        } else if (attr->irq_type == GPIO_IRQ_EDGE_BOTH) {
            ret = hal_file_write(edge_path, GPIO_EDGE_STR_BOTH,
                                 strlen(GPIO_EDGE_STR_BOTH));
        } else {
            hal_log(HAL_LOG_ERR, "invalid irq type\n");
            return -1;
        }

        if (ret < 0) {
            hal_log(HAL_LOG_ERR, "Failed to set irq type %d\n", attr->irq_type);
            return -1;
        }
    }

    return 0;
}

int hal_gpio_set_value_common(int num, int value)
{
    char value_path[MAX_PATH] = {0};
    int ret;

    sprintf(value_path, GPIO_VALUE_PATH, num);

    if (value == GPIO_HIGH) {
        ret = hal_file_write(value_path,
                             GPIO_VALUE_STR_HIGH, strlen(GPIO_VALUE_STR_HIGH));
    } else {
        ret = hal_file_write(value_path,
                             GPIO_VALUE_STR_LOW, strlen(GPIO_VALUE_STR_LOW));
    }

    if (ret < 0) {
        hal_log(HAL_LOG_ERR, "Failed to set gpio(%d) level\n", num);
        return -1;
    }

    return 0;
}

int hal_gpio_get_value_common(int num, int *value)
{
    char buf[32] = {0};
    char value_path[MAX_PATH] = {0};

    sprintf(value_path, GPIO_VALUE_PATH, num);

    if (hal_file_read(value_path, buf, sizeof(buf)) <= 0) {
        hal_log(HAL_LOG_ERR, "Failed to get gpio(%d) level\n", num);
        return -1;
    }

    if (strncmp(buf, GPIO_VALUE_STR_LOW, strlen(GPIO_VALUE_STR_LOW)) == 0) {
        *value = GPIO_LOW;
    } else {
        *value = GPIO_HIGH;
    }

    return 0;
}

int hal_gpio_wait_for_irq_common(int num, int timeout)
{
    char value_path[MAX_PATH] = {0};
    int fd = -1;
    struct pollfd gpiofd;
    char buf[32] = {0};
    int ret = 0;

    sprintf(value_path, GPIO_VALUE_PATH, num);

    fd = open(value_path, O_RDONLY);
    if (fd < 0) {
        hal_log(HAL_LOG_ERR, "Failed to open %s\n", value_path);
        return -1;
    }

    ret = read(fd, buf, sizeof(buf));
    if (ret < 0) {
        hal_log(HAL_LOG_ERR, "Failed to read %s\n", value_path);
        close(fd);
        return -1;
    }

    gpiofd.fd = fd;
    gpiofd.events = POLLPRI;
    gpiofd.revents = 0;
    ret = poll(&gpiofd, 1, timeout);
    close(fd);
    if (ret > 0) {
        if (gpiofd.revents & POLLPRI) {
            return 0;
        } else {
            hal_log(HAL_LOG_ERR, "gpio: unexpected event\n");
            hal_log(HAL_LOG_ERR, "event cnt: %d, revents: 0x%X\n",
                    ret, gpiofd.revents);
            return -1;
        }
    } else if (ret == 0) {
        return -ETIMEDOUT;
    } else {
        hal_log(HAL_LOG_ERR, "gpio: failed to waiting for irq\n");
        return -1;
    }

    return 0;
}


/*
 * legacy API, deprecated
 */

// TODO: delete legacy API

int hal_get_gpio_syspath(char *name, char *value)
{
	char key[NVRAM_MAX_PARAM_LEN] = {0};
	char def_value[NVRAM_MAX_VALUE_LEN] = {0};

	if (!name || !value)
		return -1;

	snprintf(key, NVRAM_MAX_PARAM_LEN, "hal_%s_gpio_syspath", name);
	snprintf(def_value, NVRAM_MAX_VALUE_LEN, "/sys/class/gpio/%s/value", name);

	return hal_get_prop(key, value, def_value);
}

static int gpio_write(char *path, int value)
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

static int gpio_read(char *path)
{
	char value_str[3] = {0};
	int fd;

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		hal_log(HAL_LOG_ERR, "%s: open path %s failed errno:%d\n",
				__func__, path, -errno);
		return -1;
	}

	if (read(fd, value_str, 3) < 0) {
		hal_log(HAL_LOG_ERR, "%s: Failed to read value errno:%d\n",
				__func__, -errno);
		return -1;
	}

	close(fd);

	return (atoi(value_str));
}

int hal_set_gpio(char *name, int value)
{
	int ret = -1;
	char syspath[NVRAM_MAX_VALUE_LEN] = {0};

	ret = hal_get_gpio_syspath(name, syspath);
	if (ret < 0)
		return -1;

	return gpio_write(syspath, value);
}
