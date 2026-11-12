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
#include "hal_motor.h"
#include "hal_motor_core.h"

int hal_motor_init(int dev, hal_motor_attr_t *attr)
{
	if (NULL == hal_if_motor.init)
		return hal_motor_init_common(dev, attr);

	return hal_if_motor.init(dev, attr);
}

int hal_motor_deinit(int dev)
{
	if (NULL == hal_if_motor.deinit)
		return hal_motor_deinit_common(dev);

	return hal_if_motor.deinit(dev);
}

int hal_motor_get_attr(int dev, hal_motor_attr_t *attr)
{
	if (NULL == hal_if_motor.get_attr)
		return hal_motor_get_attr_common(dev, attr);

	return hal_if_motor.get_attr(dev, attr);
}

int hal_motor_op(int dev, hal_motor_cmd_t *cmd)
{
	if (NULL == hal_if_motor.op)
		return hal_motor_op_common(dev, cmd);

	return hal_if_motor.op(dev, cmd);
}

int hal_motor_get_state(int dev, hal_motor_stat_t *stat)
{
	if (NULL == hal_if_motor.get_state)
		return hal_motor_get_state_common(dev, stat);

	return hal_if_motor.get_state(dev, stat);
}
