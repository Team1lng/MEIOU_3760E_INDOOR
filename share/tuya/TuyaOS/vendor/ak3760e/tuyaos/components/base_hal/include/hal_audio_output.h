/*
 * Copyright (C) 2021 Tuya Inc.
 */

#ifndef HAL_AUDIO_OUTPUT_H
#define HAL_AUDIO_OUTPUT_H

#include "hal_audio_common.h"

typedef struct hal_adec_g711_config_s {
	hal_pcm_sample_rate_e sample_rate;
	hal_aud_sound_mode_e sound_mode;
} hal_adec_g711_config_t;

typedef struct hal_adec_g726_config_s {
	hal_pcm_sample_rate_e sample_rate;
	hal_aud_sound_mode_e sound_mode;
	hal_aud_g726_mode_e mode;
} hal_adec_g726_config_t;

typedef struct hal_adec_attr_s {
	hal_aud_enc_type_e type;
	union
	{
		hal_adec_g711_config_t g711_cfg;
		hal_adec_g726_config_t g726_cfg;
	};
} hal_adec_attr_t;

/* attributes of audio output device */
typedef struct hal_aud_out_attr_s {
	int channels;           /* channel number */
	int rate;               /* sample rate */
	hal_pcm_format_e format;/* sample format */
	int points;             /* number of sampling points per frame */

	int volume;             /* volume in db */

	int period_count;       /* The number of periods in a PCM buffer */
	int period_size;        /* The number of frames in a period */

	bool resmp_enable;      /* enable resample */
	int resmp_rate;         /* the source sample rate of resample */

	bool vqe_enable;
	hal_aud_vqe_attr_t *vqe_attr;

	bool adec_enable;
	hal_adec_attr_t *adec_attr;

/* This structure is packed to 128 bytes for extension */
} __attribute__((packed, aligned(128))) hal_aud_out_attr_t;

/** Initialize an audio output device.
 * @param dev device number.
 * @param chn channel number.
 * @param attr device attributes.
 * @returns 0 on success, error code on failure.
 */
int hal_aud_out_init(int dev, int chn, hal_aud_out_attr_t *attr);

/** Deinitialize an audio output device.
 * @param dev device number.
 * @param chn channel number.
 * @returns 0 on success, error code on failure.
 */
int hal_aud_out_deinit(int dev, int chn);

/** Write audio frames into audio device.
 * @param dev device number.
 * @param chn channel number.
 * @param frame audio frames.
 * @param timeout
 *     -1 : block indefinitely
 *     0  : non-block
 *     >0 : timeout in milliseconds
 * @returns 0 on success,
 *          -ETIMEDOUT on timeout,
 *          other error code on failure.
 */
int hal_aud_out_write_frame(int dev, int chn, hal_aud_frame_s *frame, int timeout);

/** Set audio device volume.
 * @param dev device number.
 * @param chn channel number.
 * @param vol volume (in dB).
 * @returns 0 on success, error code on failure.
 */
int hal_aud_out_set_volume(int dev, int chn, int vol);

/** Get audio device volume.
 * @param dev device number.
 * @param chn channel number.
 * @param vol volume (in dB).
 * @returns 0 on success, error code on failure.
 */
int hal_aud_out_get_volume(int dev, int chn, int *vol);

/** Mute audio device.
 * @param dev device number.
 * @param chn channel number.
 * @returns 0 on success, error code on failure.
 */
int hal_aud_out_mute(int dev, int chn);

/** Un-mute audio device.
 * @param dev device number.
 * @param chn channel number.
 * @returns 0 on success, error code on failure.
 */
int hal_aud_out_unmute(int dev, int chn);

/** Turn on/off speaker.
 * @param num speaker number.
 * @param is_on turn on or off.
 * @returns 0 on success, error code on failure.
 */
int hal_aud_out_speaker_switch(int num, bool is_on);

#endif
