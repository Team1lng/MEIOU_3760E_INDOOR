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

int hal_video_vif_init(int dev, int chn, int port, hal_video_vif_attr_t *attr)
{
	if (hal_if_video.vif.init != NULL)
		return hal_if_video.vif.init(dev, chn, port, attr);

	return -EPERM;
}

int hal_video_vif_deinit(int dev, int chn, int port)
{
	if (hal_if_video.vif.deinit != NULL)
		return hal_if_video.vif.deinit(dev, chn, port);

	return -EPERM;
}

int hal_video_vif_enable(int dev, int chn, int port)
{
	if (hal_if_video.vif.enable != NULL)
		return hal_if_video.vif.enable(dev, chn, port);

	return -EPERM;
}

int hal_video_vif_disable(int dev, int chn, int port)
{
	if (hal_if_video.vif.disable != NULL)
		return hal_if_video.vif.disable(dev, chn, port);

	return -EPERM;
}

int hal_video_vif_get_info(int dev, int chn, int port, hal_video_vif_info_t *info)
{
	if (hal_if_video.vif.get_info != NULL)
		return hal_if_video.vif.get_info(dev, chn, port, info);

	return -EPERM;	
}
