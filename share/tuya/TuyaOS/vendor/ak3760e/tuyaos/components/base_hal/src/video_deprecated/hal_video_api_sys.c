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
#include "hal_video_core.h"

int hal_video_sys_init(void)
{
	if (hal_if_video.sys.init != NULL)
		return hal_if_video.sys.init();

	return 0;
}

int hal_video_sys_deinit(void)
{
	if (hal_if_video.sys.deinit != NULL)
		return hal_if_video.sys.deinit();

	return 0;
}
