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
#include "hal_audio.h"
#include "hal_audio_core.h"

int hal_aud_in_init(int dev, int chn, hal_aud_in_attr_t *attr)
{
	if (hal_if_audio.in.init != NULL)
		return hal_if_audio.in.init(dev, chn, attr);

	return -EPERM;
}

int hal_aud_in_deinit(int dev, int chn)
{
	if (hal_if_audio.in.deinit != NULL)
		return hal_if_audio.in.deinit(dev, chn);

	return -EPERM;
}

int hal_aud_in_wait_for_frame(int dev, int chn, int timeout)
{
	if (hal_if_audio.in.wait_for_frame != NULL)
		return hal_if_audio.in.wait_for_frame(dev, chn, timeout);

	return -EPERM;
}

int hal_aud_in_get_frame(int dev, int chn, hal_aud_frame_s *frame, int timeout)
{
	if (hal_if_audio.in.get_frame != NULL)
		return hal_if_audio.in.get_frame(dev, chn, frame, timeout);

	return -EPERM;
}

int hal_aud_in_release_frame(int dev, int chn, hal_aud_frame_s *frame)
{
	if (hal_if_audio.in.release_frame != NULL)
		return hal_if_audio.in.release_frame(dev, chn, frame);

	return -EPERM;
}

int hal_aud_in_set_volume(int dev, int chn, int vol)
{
	if (hal_if_audio.in.set_volume != NULL)
		return hal_if_audio.in.set_volume(dev, chn, vol);

	return -EPERM;
}

int hal_aud_in_get_volume(int dev, int chn, int *vol)
{
	if (hal_if_audio.in.get_volume != NULL)
		return hal_if_audio.in.get_volume(dev, chn, vol);

	return -EPERM;
}

int hal_aud_in_mute(int dev, int chn)
{
	if (hal_if_audio.in.mute != NULL)
		return hal_if_audio.in.mute(dev, chn);

	return -EPERM;
}

int hal_aud_in_unmute(int dev, int chn)
{
	if (hal_if_audio.in.unmute != NULL)
		return hal_if_audio.in.unmute(dev, chn);

	return -EPERM;
}
