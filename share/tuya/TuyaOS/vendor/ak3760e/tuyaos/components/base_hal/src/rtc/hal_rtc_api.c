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
#include "hal_rtc.h"
#include "hal_rtc_core.h"

int hal_rtc_init(void)
{
    if (NULL == hal_if_rtc.init)
        return hal_rtc_init_common();

    return hal_if_rtc.init();
}

int hal_rtc_deinit(void)
{
    if (NULL == hal_if_rtc.deinit)
        return hal_rtc_deinit_common();

    return hal_if_rtc.deinit();
}

int hal_rtc_set_time(hal_rtc_time_t *time)
{
    if (NULL == hal_if_rtc.set_time)
        return hal_rtc_set_time_common(time);

    return hal_if_rtc.set_time(time);
}

int hal_rtc_get_time(hal_rtc_time_t *time)
{
    if (NULL == hal_if_rtc.get_time)
        return hal_rtc_get_time_common(time);

    return hal_if_rtc.get_time(time);
}
