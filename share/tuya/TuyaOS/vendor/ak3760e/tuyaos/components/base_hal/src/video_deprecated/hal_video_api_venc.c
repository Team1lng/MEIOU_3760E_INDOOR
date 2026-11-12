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

int hal_video_venc_init(hal_video_venc_attr_t *attr)
{
	if (hal_if_video.venc.init != NULL)
		return hal_if_video.venc.init(attr);

	return -EPERM;
}

int hal_video_venc_deinit(void)
{
	if (hal_if_video.venc.deinit != NULL)
		return hal_if_video.venc.deinit();

	return -EPERM;
}

int hal_video_venc_create_channel(int chn, hal_video_venc_chn_attr_t *attr)
{
	if (hal_if_video.venc.create_channel != NULL)
		return hal_if_video.venc.create_channel(chn, attr);

	return -EPERM;
}

int hal_video_venc_destroy_channel(int chn)
{
	if (hal_if_video.venc.destroy_channel != NULL)
		return hal_if_video.venc.destroy_channel(chn);

	return -EPERM;
}

int hal_video_venc_start_channel(int chn)
{
	if (hal_if_video.venc.start_channel != NULL)
		return hal_if_video.venc.start_channel(chn);

	return -EPERM;
}

int hal_video_venc_stop_channel(int chn)
{
	if (hal_if_video.venc.stop_channel != NULL)
		return hal_if_video.venc.stop_channel(chn);

	return -EPERM;
}

int hal_video_venc_wait_for_stream(int chn, int timeout)
{
	if (hal_if_video.venc.wait_for_stream != NULL)
		return hal_if_video.venc.wait_for_stream(chn, timeout);

	return -EPERM;
}

int hal_video_venc_get_packet_num(int chn, int *num)
{
	if (hal_if_video.venc.get_packet_num != NULL)
		return hal_if_video.venc.get_packet_num(chn, num);

	return -EPERM;
}

int hal_video_venc_get_stream(int chn, hal_video_venc_stream_t *stream, int timeout)
{
	if (hal_if_video.venc.get_stream != NULL)
		return hal_if_video.venc.get_stream(chn, stream, timeout);

	return -EPERM;
}

int hal_video_venc_release_stream(int chn, hal_video_venc_stream_t *stream)
{
	if (hal_if_video.venc.release_stream != NULL)
		return hal_if_video.venc.release_stream(chn, stream);

	return -EPERM;
}
