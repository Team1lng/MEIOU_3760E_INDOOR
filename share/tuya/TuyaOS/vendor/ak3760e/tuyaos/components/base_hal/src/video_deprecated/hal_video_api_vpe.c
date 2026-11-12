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

int hal_video_vpe_init(hal_video_vpe_attr_t *attr)
{
	if (hal_if_video.vpe.init != NULL)
		return hal_if_video.vpe.init(attr);

	return -EPERM;
}

int hal_video_vpe_deinit(void)
{
	if (hal_if_video.vpe.deinit != NULL)
		return hal_if_video.vpe.deinit();

	return -EPERM;
}

int hal_video_vpe_create_channel(int chn, int port, hal_video_vpe_chn_attr_t *attr)
{
	if (hal_if_video.vpe.create_channel != NULL)
		return hal_if_video.vpe.create_channel(chn, port, attr);

	return -EPERM;
}

int hal_video_vpe_destroy_channel(int chn, int port)
{
	if (hal_if_video.vpe.destroy_channel != NULL)
		return hal_if_video.vpe.destroy_channel(chn, port);

	return -EPERM;
}

int hal_video_vpe_start_channel(int chn, int port)
{
	if (hal_if_video.vpe.start_channel != NULL)
		return hal_if_video.vpe.start_channel(chn, port);

	return -EPERM;
}

int hal_video_vpe_stop_channel(int chn, int port)
{
	if (hal_if_video.vpe.stop_channel != NULL)
		return hal_if_video.vpe.stop_channel(chn, port);

	return -EPERM;
}

int hal_video_vpe_isp_load_para_file(int dev, char *path, unsigned int magic)
{
	if (hal_if_video.vpe.isp_load_para_file != NULL)
		return hal_if_video.vpe.isp_load_para_file(dev, path, magic);

	return -EPERM;
}
