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
#include "hal_power.h"
#include "hal_power_core.h"


int hal_set_power_state(const char *mode)
{
    if (NULL == hal_if_power.set_power_state)
        return hal_set_power_state_common(mode);

    return hal_if_power.set_power_state(mode);
}

int hal_get_power_state(char *mode)
{
    if (NULL == hal_if_power.get_power_state)
        return hal_get_power_state_common(mode);

    return hal_if_power.get_power_state(mode);
}

int hal_get_wakeup_src(unsigned int *value)
{
   if (NULL == hal_if_power.get_wakeup_src)
	return hal_get_wakeup_src_common(value);

   return hal_if_power.get_wakeup_src(value);
}
