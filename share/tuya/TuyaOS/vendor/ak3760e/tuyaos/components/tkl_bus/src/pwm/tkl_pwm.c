/*
 * Copyright (C) 2021 Ty Inc.
 */

#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include "sys_log.h"
#include "tkl_file.h"
#include "tkl_pwm.h"

#ifndef TAG 
#define TAG "tkl_pwm"
#endif

#ifndef MAX_PATH
#define MAX_PATH 256
#endif

#define PWM_CHIP_PATH           "/sys/class/pwm/pwmchip%d"
#define PWM_CHANNEL_PATH        "/sys/class/pwm/pwmchip%d/pwm%d"
#define PWM_EXPORT_PATH         "/sys/class/pwm/pwmchip%d/export"

#define PWM_PERIOD_PATH         "/sys/class/pwm/pwmchip%d/pwm%d/period"
#define PWM_DUTY_CYCLE_PATH     "/sys/class/pwm/pwmchip%d/pwm%d/duty_cycle"
#define PWM_POLARITY_PATH       "/sys/class/pwm/pwmchip%d/pwm%d/polarity"
#define PWM_ENABLE_PATH         "/sys/class/pwm/pwmchip%d/pwm%d/enable"

#define PWM_POLARITY_NORMAL     "normal"
#define PWM_POLARITY_INVERSED   "inversed"
#define PWM_ENABLE_STR          "1"
#define PWM_DISABLE_STR         "0"

#define TKL_PWM_MAX_CHIP_NUM        512
#define TKL_PWM_MAX_CHANNEL_NUM     512

#define PWM_CHIP_VALID(dev)     (((dev) >= 0) && ((dev) < TKL_PWM_MAX_CHIP_NUM))
#define PWM_CHANNEL_VALID(chn)  (((chn) >= 0) && ((chn) < TKL_PWM_MAX_CHANNEL_NUM))

#define TKL_PWM_POL_NORMAL      0
#define TKL_PWM_POL_INVERSED    1

#define TKL_PWM_NUM_GET(__PWMNAME)      (__PWMNAME & 0x1F)
#define TKL_PWM_PORT_GET(__PWMNAME)     (__PWMNAME >> 5)
#define TKL_PWM_NAME_GET(__PWM, __CH) ((__PWM << 5) + (__CH & 0x1F))

/* pwm channel attribute */
typedef struct {
    int freq;           /* frequency, in Hz */
    int duty_cycle;     /* range [0, 10000], e.g. 1234 means 12.34% */
    int polarity;       /* normal or inversed */
    int reserved[5];
}PWM_CHAN_ATTR_T;

static int __pwm_chip_is_exists(int dev)
{
    char path[MAX_PATH] = {0};

    if (!PWM_CHIP_VALID(dev)) {
        LOGE(TAG, "invalid pwm device %d\n", dev);
        return 0;
    }

    snprintf(path,MAX_PATH,PWM_CHIP_PATH, dev);

    return tkl_file_exists(path);
}

static int __pwm_number_is_valid(int dev, int chn)
{
    if (!PWM_CHIP_VALID(dev)) {
        LOGE(TAG, "invalid pwm device %d\n", dev);
        return 0;
    }

    if (!PWM_CHANNEL_VALID(chn)) {
        LOGE(TAG, "invalid pwm channel %d\n", chn);
        return 0;
    }

    return 1;
}

static int __pwm_channel_is_exists(int dev, int chn)
{
    char path[MAX_PATH] = {0};

    if (!__pwm_number_is_valid(dev, chn)) {
        return 0;
    }

    snprintf(path,MAX_PATH,PWM_CHANNEL_PATH, dev, chn);

    return tkl_file_exists(path);
}

static int __tkl_pwm_config(int dev, int chn, PWM_CHAN_ATTR_T *attr)
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

    if (attr == NULL) {
        LOGE(TAG,"invalid param (attr is null)\n");
        return -1;
    }

    if (!__pwm_chip_is_exists(dev)) {
        LOGE(TAG,"pwm device %d is not exists\n", dev);
        return -1;
    }

    // if channel not exists, try to export it
    if (!__pwm_channel_is_exists(dev, chn)) {
        snprintf(export_path,MAX_PATH, PWM_EXPORT_PATH, dev);
        snprintf(chn_num_str,32, "%d", chn);

        /* export channel */
        ret = tkl_file_write(export_path, chn_num_str, strlen(chn_num_str));
        if (ret < 0) {
            LOGE(TAG, "Failed to export pwm channel(%d)\n", chn);
            return -1;
        }
    }

    // set frequency
    snprintf(period_path,MAX_PATH,PWM_PERIOD_PATH, dev, chn);
    if (attr->freq != 0) {
        period_ns = 1000000000 / attr->freq;
        snprintf(period_str,32,"%d", period_ns);
        ret = tkl_file_write(period_path, period_str, strlen(period_str));
        if (ret < 0) {
            LOGE(TAG, "Failed to set freq\n");
            return -1;
        }
    } else {
        period_ns = 0;
    }

    // set duty-cycle
    snprintf(duty_cycle_path,MAX_PATH,PWM_DUTY_CYCLE_PATH,dev,chn);
    duty_cycle_ns = (attr->duty_cycle * period_ns)/10000;
    snprintf(duty_cycle_str,32,"%d", duty_cycle_ns);
    ret = tkl_file_write(duty_cycle_path, duty_cycle_str, strlen(duty_cycle_str));
    if (ret < 0) {
        LOGE(TAG, "Failed to set duty_cycle\n");
        return -1;
    }

    // set polarity
    snprintf(polarity_path,MAX_PATH,PWM_POLARITY_PATH, dev, chn);
    if (attr->polarity == TKL_PWM_POL_NORMAL) {
        strcpy(polarity_str, PWM_POLARITY_NORMAL);
    } else {
        strcpy(polarity_str, PWM_POLARITY_INVERSED);
    }
    ret = tkl_file_write(polarity_path, polarity_str, strlen(polarity_str));
    if (ret < 0) {
        LOGE(TAG, "Failed to set polarity\n");
        return -1;
    }

    return 0;
}

static int __tkl_pwm_enable(int dev, int chn, bool enable)
{
    char export_path[MAX_PATH] = {0};
    char chn_num_str[32] = {0};
    char enable_path[MAX_PATH] = {0};
    char enable_str[32] = {0};
    int ret;

    if (!__pwm_chip_is_exists(dev)) {
        LOGE(TAG, "pwm device %d is not exists\n", dev);
        return -1;
    }

    // if channel not exists, try to export it
    if (!__pwm_channel_is_exists(dev, chn)) {
        snprintf(export_path,MAX_PATH,PWM_EXPORT_PATH, dev);
        snprintf(chn_num_str,32,"%d",chn);

        /* export channel */
        ret = tkl_file_write(export_path, chn_num_str, strlen(chn_num_str));
        if (ret < 0) {
            LOGE(TAG, "Failed to export pwm channel(%d)\n", chn);
            return -1;
        }
    }

    snprintf(enable_path,MAX_PATH,PWM_ENABLE_PATH,dev, chn);
    if (enable) {
        strncpy(enable_str,PWM_ENABLE_STR,32);
    } else {
        strncpy(enable_str,PWM_DISABLE_STR,32);
    }
    ret = tkl_file_write(enable_path, enable_str, strlen(enable_str));
    if (ret < 0) {
	LOGE(TAG,"Fail to enable(%s) %s\n",enable_str,enable_path);
        return -1;
    }

    return 0;
}

OPERATE_RET tkl_pwm_info_get_common(UINT32_T port, TUYA_PWM_BASE_CFG_T *info)
{
    // TODO
    return OPRT_NOT_SUPPORTED;
}

OPERATE_RET tkl_pwm_duty_set(TUYA_PWM_NUM_E ch_id, UINT32_T duty)
{
    return OPRT_NOT_SUPPORTED;
}

OPERATE_RET tkl_pwm_frequency_set(TUYA_PWM_NUM_E ch_id, UINT32_T frequency)
{
    return OPRT_NOT_SUPPORTED;
}

OPERATE_RET tkl_pwm_polarity_set(TUYA_PWM_NUM_E ch_id, TUYA_PWM_POLARITY_E polarity)
{
    return OPRT_NOT_SUPPORTED;
}


OPERATE_RET tkl_pwm_init(TUYA_PWM_NUM_E port, CONST TUYA_PWM_BASE_CFG_T *cfg)
{
    return OPRT_OK;
}

OPERATE_RET tkl_pwm_deinit(TUYA_PWM_NUM_E port)
{
    return OPRT_OK;
}

OPERATE_RET tkl_pwm_start(TUYA_PWM_NUM_E port)
{
    int dev = TKL_PWM_PORT_GET((UINT32_T)port);
    int chn = TKL_PWM_NUM_GET((UINT32_T)port);

    if (__tkl_pwm_enable(dev,chn,1) != 0) {
        return OPRT_COM_ERROR;
    }

    return OPRT_OK;
}

OPERATE_RET tkl_pwm_multichannel_start(TUYA_PWM_NUM_E *ch_id, UINT8_T num)
{
    return OPRT_NOT_SUPPORTED;
}

OPERATE_RET tkl_pwm_multichannel_stop(TUYA_PWM_NUM_E *ch_id, UINT8_T num)
{
    return OPRT_NOT_SUPPORTED;
}

OPERATE_RET tkl_pwm_stop(TUYA_PWM_NUM_E port)
{
    int dev = TKL_PWM_PORT_GET((UINT32_T)port);
    int chn = TKL_PWM_NUM_GET((UINT32_T)port);

    if (__tkl_pwm_enable(dev, chn,0) != 0) {
        return OPRT_COM_ERROR;
    }

    return OPRT_OK;
}


OPERATE_RET tkl_pwm_info_set(TUYA_PWM_NUM_E port, CONST TUYA_PWM_BASE_CFG_T *info)
{
    int dev = TKL_PWM_PORT_GET((UINT32_T)port);
    int chn = TKL_PWM_NUM_GET((UINT32_T)port);

    PWM_CHAN_ATTR_T attr = {0};

    if (info == NULL) {
        return OPRT_INVALID_PARM;
    }

    attr.freq = info->frequency;

  /*
    duty:range[0,10000] ,exp : 100 == 1/100
    duty_cycle:range [0, 100*10000], e.g. 123456 means 12.3456% 
  */

  //attr.duty_cycle = info->duty / 100;
    attr.duty_cycle = info->duty*100;


    if (info->polarity == TUYA_PWM_POSITIVE) {
        attr.polarity = TKL_PWM_POL_NORMAL;
    } else {
        attr.polarity = TKL_PWM_POL_INVERSED;
    }

    if (__tkl_pwm_config(dev, chn, &attr) != 0) {
        return OPRT_COM_ERROR;
    }

    return OPRT_OK;
}

OPERATE_RET tkl_pwm_info_get(TUYA_PWM_NUM_E port,TUYA_PWM_BASE_CFG_T *info)
{
    return OPRT_NOT_SUPPORTED;
}
