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

int hal_video_snr_init(int id, hal_video_snr_attr_t *attr)
{
	if (hal_if_video.snr.init != NULL)
		return hal_if_video.snr.init(id, attr);

	return -EPERM;
}

int hal_video_snr_deinit(int id)
{
	if (hal_if_video.snr.deinit != NULL)
		return hal_if_video.snr.deinit(id);

	return -EPERM;
}

int hal_video_snr_enable(int id)
{
	if (hal_if_video.snr.enable != NULL)
		return hal_if_video.snr.enable(id);

	return -EPERM;
}

int hal_video_snr_disable(int id)
{
	if (hal_if_video.snr.disable != NULL)
		return hal_if_video.snr.disable(id);

	return -EPERM;
}

int hal_video_snr_set_fps(int id, int fps)
{
	if (hal_if_video.snr.set_fps != NULL)
		return hal_if_video.snr.set_fps(id, fps);

	return -EPERM;
}

int hal_video_snr_get_fps(int id, int *fps)
{
	if (hal_if_video.snr.get_fps != NULL)
		return hal_if_video.snr.get_fps(id, fps);

	return -EPERM;
}

int hal_video_snr_get_res_num(int id, int *num)
{
	if (hal_if_video.snr.get_res_num != NULL)
		return hal_if_video.snr.get_res_num(id, num);

	return -EPERM;
}

int hal_video_snr_get_one_res(int id, int index, hal_video_res_t *res)
{
	if (hal_if_video.snr.get_one_res != NULL)
		return hal_if_video.snr.get_one_res(id, index, res);

	return -EPERM;
}

int hal_video_snr_set_res(int id, int index)
{
	if (hal_if_video.snr.set_res != NULL)
		return hal_if_video.snr.set_res(id, index);

	return -EPERM;
}

int hal_video_snr_get_cur_res(int id, int *index, hal_video_res_t *res)
{
	if (hal_if_video.snr.get_cur_res != NULL)
		return hal_if_video.snr.get_cur_res(id, index, res);

	return -EPERM;
}

int hal_video_snr_set_mirror(int id, hal_video_mirror_e mirror)
{
	if (hal_if_video.snr.set_mirror != NULL)
		return hal_if_video.snr.set_mirror(id, mirror);

	return -EPERM;
}

int hal_video_snr_get_mirror(int id, hal_video_mirror_e *mirror)
{
	if (hal_if_video.snr.get_mirror != NULL)
		return hal_if_video.snr.get_mirror(id, mirror);

	return -EPERM;
}
