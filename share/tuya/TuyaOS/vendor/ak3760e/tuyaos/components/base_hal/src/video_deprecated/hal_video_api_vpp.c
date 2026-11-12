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

int hal_video_vpp_init(hal_video_vpp_attr_t *attr)
{
	if (hal_if_video.vpp.init != NULL)
		return hal_if_video.vpp.init(attr);

	return -EPERM;
}

int hal_video_vpp_deinit(void)
{
	if (hal_if_video.vpp.deinit != NULL)
		return hal_if_video.vpp.deinit();

	return -EPERM;
}

int hal_video_vpp_create_channel(int chn, hal_video_vpp_chn_attr_t *attr)
{
	if (hal_if_video.vpp.create_channel != NULL)
		return hal_if_video.vpp.create_channel(chn, attr);

	return -EPERM;
}

int hal_video_vpp_destroy_channel(int chn)
{
	if (hal_if_video.vpp.destroy_channel != NULL)
		return hal_if_video.vpp.destroy_channel(chn);

	return -EPERM;
}

int hal_video_vpp_start_channel(int chn)
{
	if (hal_if_video.vpp.start_channel != NULL)
		return hal_if_video.vpp.start_channel(chn);

	return -EPERM;
}

int hal_video_vpp_stop_channel(int chn)
{
	if (hal_if_video.vpp.stop_channel != NULL)
		return hal_if_video.vpp.stop_channel(chn);

	return -EPERM;
}
