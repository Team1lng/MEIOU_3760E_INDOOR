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
#include "hal_pwm.h"
#include "hal_pwm_core.h"

int hal_pwm_config(int dev, int chn, hal_pwm_chn_attr_t *attr)
{
	if (NULL == hal_if_pwm.config)
		return hal_pwm_config_common(dev, chn, attr);

	return hal_if_pwm.config(dev, chn, attr);
}

int hal_pwm_enable(int dev, int chn)
{
	if (NULL == hal_if_pwm.enable)
		return hal_pwm_enable_common(dev, chn);

	return hal_if_pwm.enable(dev, chn);
}

int hal_pwm_disable(int dev, int chn)
{
	if (NULL == hal_if_pwm.disable)
		return hal_pwm_disable_common(dev, chn);

	return hal_if_pwm.disable(dev, chn);
}
