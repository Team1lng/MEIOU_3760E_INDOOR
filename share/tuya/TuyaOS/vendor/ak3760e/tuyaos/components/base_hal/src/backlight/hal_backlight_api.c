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
#include "hal_backlight.h"
#include "hal_backlight_core.h"

int hal_backlight_set_value(int brightness)
{
    if (NULL == hal_if_backlight.set_value)
        return hal_backlight_set_value_common(brightness);

    return hal_if_backlight.set_value(brightness);
}


int hal_backlight_get_value(void)
{
    if (NULL == hal_if_backlight.get_value)
        return hal_backlight_get_value_common();

    return hal_if_backlight.get_value();
}
