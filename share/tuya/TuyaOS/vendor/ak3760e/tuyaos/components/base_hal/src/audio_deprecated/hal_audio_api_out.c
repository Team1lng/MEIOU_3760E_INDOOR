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

int hal_aud_out_init(int dev, int chn, hal_aud_out_attr_t *attr)
{
	if (hal_if_audio.out.init != NULL)
		return hal_if_audio.out.init(dev, chn, attr);

	return -EPERM;
}

int hal_aud_out_deinit(int dev, int chn)
{
	if (hal_if_audio.out.deinit != NULL)
		return hal_if_audio.out.deinit(dev, chn);

	return -EPERM;
}

int hal_aud_out_write_frame(int dev, int chn, hal_aud_frame_s *frame, int timeout)
{
	if (hal_if_audio.out.write_frame != NULL)
		return hal_if_audio.out.write_frame(dev, chn, frame, timeout);

	return -EPERM;
}

int hal_aud_out_set_volume(int dev, int chn, int vol)
{
	if (hal_if_audio.out.set_volume != NULL)
		return hal_if_audio.out.set_volume(dev, chn, vol);

	return -EPERM;
}

int hal_aud_out_get_volume(int dev, int chn, int *vol)
{
	if (hal_if_audio.out.get_volume != NULL)
		return hal_if_audio.out.get_volume(dev, chn, vol);

	return -EPERM;
}

int hal_aud_out_mute(int dev, int chn)
{
	if (hal_if_audio.out.mute != NULL)
		return hal_if_audio.out.mute(dev, chn);

	return -EPERM;
}

int hal_aud_out_unmute(int dev, int chn)
{
	if (hal_if_audio.out.unmute != NULL)
		return hal_if_audio.out.unmute(dev, chn);

	return -EPERM;
}

int hal_aud_out_speaker_switch(int num, bool is_on)
{
	if (hal_if_audio.out.speaker_switch != NULL)
		return hal_if_audio.out.speaker_switch(num, is_on);

	return hal_aud_out_speaker_switch_common(num, is_on);
}
