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
#include <linux/rtc.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include "bcmnvram.h"
#include "hal_log.h"
#include "hal_misc.h"
#include "hal_types.h"
#include "hal_file.h"
#include "hal_board.h"
#include "hal_config.h"
#include "hal_rtc.h"
#include "hal_rtc_core.h"

/*
 * For generic instance, set all the members to NULL
 */
hal_if_rtc_t hal_if_rtc = {0};


#define RTC_DEFAULT_DEVICE  "/dev/rtc0"

static int g_rtc_fd = -1;

int hal_rtc_init_common(void)
{
    char dev_name[MAX_PATH] = {0};
    int ret;

    // get RTC device attributes
    ret = hal_cfg_get_item_string(ITEM_RTC, KEY_RTC_DEV, dev_name, sizeof(dev_name));
    if (ret != 0) {
        strncpy(dev_name, RTC_DEFAULT_DEVICE, strlen(RTC_DEFAULT_DEVICE) + 1);
    }

    g_rtc_fd = open(dev_name, O_RDWR);
    if (g_rtc_fd < 0) {
        hal_log(HAL_LOG_ERR, "Failed to open %s\n", dev_name);
        return -1;
    }

    return 0;
}

int hal_rtc_deinit_common(void)
{
    if (g_rtc_fd != -1) {
        close(g_rtc_fd);
        g_rtc_fd = -1;
    }
    return 0;
}

int hal_rtc_set_time_common(hal_rtc_time_t *time)
{
    struct rtc_time newtime = {0};
    int ret;

    if (NULL == time) {
        return -1;
    }

    newtime.tm_year = time->year;
    newtime.tm_mon  = time->month;
    newtime.tm_mday = time->day;
    newtime.tm_hour = time->hour;
    newtime.tm_min  = time->minute;
    newtime.tm_sec  = time->second;
    newtime.tm_year -= 1900;
    newtime.tm_mon -= 1;

    ret = ioctl(g_rtc_fd, RTC_SET_TIME, &newtime);
    if (ret < 0) {
        hal_log(HAL_LOG_ERR, "RTC_SET_TIME failed\n");
        return -1;
    }

    return 0;
}

int hal_rtc_get_time_common(hal_rtc_time_t *time)
{
    struct rtc_time curtime;
    int ret;

    if (NULL == time) {
        return -1;
    }

	ret = ioctl(g_rtc_fd, RTC_RD_TIME, &curtime);
    if (ret < 0) {
        hal_log(HAL_LOG_ERR, "RTC_RD_TIME failed\n");
        return -1;
    }

    curtime.tm_year += 1900;
    curtime.tm_mon += 1;

    time->year = curtime.tm_year;
    time->month = curtime.tm_mon;
    time->day = curtime.tm_mday;
    time->hour = curtime.tm_hour;
    time->minute = curtime.tm_min;
    time->second = curtime.tm_sec;
    return 0;
}
