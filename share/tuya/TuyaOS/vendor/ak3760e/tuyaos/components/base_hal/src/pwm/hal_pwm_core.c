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
#include <pthread.h>
#include "bcmnvram.h"
#include "hal_log.h"
#include "hal_file.h"
#include "hal_misc.h"
#include "hal_pwm.h"
#include "hal_pwm_core.h"

/*
 * For generic instance, set all the members to NULL
 */
hal_if_pwm_t hal_if_pwm = { 0 };


#define HAL_PWM_MAX_CHIP_NUM        512
#define HAL_PWM_MAX_CHANNEL_NUM     512

#define PWM_CHIP_PATH           "/sys/class/pwm/pwmchip%d"
#define PWM_CHANNEL_PATH        "/sys/class/pwm/pwmchip%d/pwm%d"
#define PWM_EXPORT_PATH         "/sys/class/pwm/pwmchip%d/export"

/*
 * period     - The total period of the PWM signal, in nanoseconds
 * duty_cycle - The active time of the PWM signal, in nanoseconds
 */
#define PWM_PERIOD_PATH         "/sys/class/pwm/pwmchip%d/pwm%d/period"
#define PWM_DUTY_CYCLE_PATH     "/sys/class/pwm/pwmchip%d/pwm%d/duty_cycle"
#define PWM_POLARITY_PATH       "/sys/class/pwm/pwmchip%d/pwm%d/polarity"
#define PWM_ENABLE_PATH         "/sys/class/pwm/pwmchip%d/pwm%d/enable"

#define PWM_POLARITY_NORMAL     "normal"
#define PWM_POLARITY_INVERSED   "inversed"
#define PWM_ENABLE_STR          "1"
#define PWM_DISABLE_STR         "0"

#define PWM_CHIP_VALID(dev)     (((dev) >= 0) && ((dev) < HAL_PWM_MAX_CHIP_NUM))
#define PWM_CHANNEL_VALID(chn)  (((chn) >= 0) && ((chn) < HAL_PWM_MAX_CHANNEL_NUM))

static bool pwm_chip_exists(int dev)
{
    char path[MAX_PATH] = {0};

    if (!PWM_CHIP_VALID(dev)) {
        hal_log(HAL_LOG_ERR, "invalid pwm device %d\n", dev);
        return FALSE;
    }

    sprintf(path, PWM_CHIP_PATH, dev);
    return hal_file_exists(path);
}

static bool pwm_number_valid(int dev, int chn)
{
    if (!PWM_CHIP_VALID(dev)) {
        hal_log(HAL_LOG_ERR, "invalid pwm device %d\n", dev);
        return FALSE;
    }
    if (!PWM_CHANNEL_VALID(chn)) {
        hal_log(HAL_LOG_ERR, "invalid pwm channel %d\n", chn);
        return FALSE;
    }
    return TRUE;
}

static bool pwm_channel_exists(int dev, int chn)
{
    char path[MAX_PATH] = {0};

    if (!pwm_number_valid(dev, chn)) {
        return FALSE;
    }

    sprintf(path, PWM_CHANNEL_PATH, dev, chn);
    return hal_file_exists(path);
}

int hal_pwm_config_common(int dev, int chn, hal_pwm_chn_attr_t *attr)
{
    char export_path[MAX_PATH] = {0};
    char period_path[MAX_PATH] = {0};
    char duty_cycle_path[MAX_PATH] = {0};
    char polarity_path[MAX_PATH] = {0};
    char chn_num_str[32] = {0};
    char period_str[32] = {0};
    char duty_cycle_str[32] = {0};
    char polarity_str[32] = {0};
    int period_ns;
    int duty_cycle_ns;
    int ret;

    if (NULL == attr) {
        hal_log(HAL_LOG_ERR, "invalid param (attr is null)\n");
        return -1;
    }

    if (!pwm_chip_exists(dev)) {
        hal_log(HAL_LOG_ERR, "pwm device %d NOT exists\n", dev);
        return -1;
    }

    // if channel not exists, try to export it
    if (!pwm_channel_exists(dev, chn)) {
        sprintf(export_path, PWM_EXPORT_PATH, dev);
        sprintf(chn_num_str, "%d", chn);

        /* export channel */
        ret = hal_file_write(export_path, chn_num_str, strlen(chn_num_str));
        if (ret < 0) {
            hal_log(HAL_LOG_ERR, "Failed to export pwm channel(%d)\n", chn);
            return -1;
        }
    }

    // set frequency
    sprintf(period_path, PWM_PERIOD_PATH, dev, chn);
    if (attr->freq != 0) {
        period_ns = 1000000000 / attr->freq;
        sprintf(period_str, "%d", period_ns);
        ret = hal_file_write(period_path, period_str, strlen(period_str));
        if (ret < 0) {
            hal_log(HAL_LOG_ERR, "Failed to set freq\n");
            return -1;
        }
    } else {
        period_ns = 0;
    }

    // set duty-cycle
    sprintf(duty_cycle_path, PWM_DUTY_CYCLE_PATH, dev, chn);
    duty_cycle_ns = (attr->duty_cycle * period_ns) / 10000;
    sprintf(duty_cycle_str, "%d", duty_cycle_ns);
    ret = hal_file_write(duty_cycle_path, duty_cycle_str, strlen(duty_cycle_str));
    if (ret < 0) {
        hal_log(HAL_LOG_ERR, "Failed to set duty_cycle\n");
        return -1;
    }

    // set polarity
    sprintf(polarity_path, PWM_POLARITY_PATH, dev, chn);
    if (attr->polarity == HAL_PWM_POL_NORMAL) {
        strcpy(polarity_str, PWM_POLARITY_NORMAL);
    } else {
        strcpy(polarity_str, PWM_POLARITY_INVERSED);
    }
    ret = hal_file_write(polarity_path, polarity_str, strlen(polarity_str));
    if (ret < 0) {
        hal_log(HAL_LOG_ERR, "Failed to set polarity\n");
        return -1;
    }

    return 0;
}

static int __hal_pwm_enable(int dev, int chn, bool enable)
{
    char export_path[MAX_PATH] = {0};
    char chn_num_str[32] = {0};
    char enable_path[MAX_PATH] = {0};
    char enable_str[32] = {0};
    int ret;

    if (!pwm_chip_exists(dev)) {
        hal_log(HAL_LOG_ERR, "pwm device %d NOT exists\n", dev);
        return -1;
    }

    // if channel not exists, try to export it
    if (!pwm_channel_exists(dev, chn)) {
        sprintf(export_path, PWM_EXPORT_PATH, dev);
        sprintf(chn_num_str, "%d", chn);

        /* export channel */
        ret = hal_file_write(export_path, chn_num_str, strlen(chn_num_str));
        if (ret < 0) {
            hal_log(HAL_LOG_ERR, "Failed to export pwm channel(%d)\n", chn);
            return -1;
        }
    }

    sprintf(enable_path, PWM_ENABLE_PATH, dev, chn);
    if (enable) {
        strcpy(enable_str, PWM_ENABLE_STR);
    } else {
        strcpy(enable_str, PWM_DISABLE_STR);
    }
    ret = hal_file_write(enable_path, enable_str, strlen(enable_str));
    if (ret < 0) {
        return -1;
    }

    return 0;
}

int hal_pwm_enable_common(int dev, int chn)
{
    return __hal_pwm_enable(dev, chn, TRUE);
}

int hal_pwm_disable_common(int dev, int chn)
{
    __hal_pwm_enable(dev, chn, FALSE);
    return 0;
}
