/*
 * Copyright (C) 2021 Tuya Inc.
 */

#ifndef HAL_AUDIO_COMMON_H
#define HAL_AUDIO_COMMON_H

#include "hal_types.h"

#define HAL_AUD_MAX_CHN_NUM	16

/* audio device ID */
typedef enum {
	/*
	 * 0-255: output
	 */

	/* 0-15: I2S OUT */
	HAL_AUD_DEV_I2S_OUT0 = 0,
	HAL_AUD_DEV_I2S_OUT1,
	HAL_AUD_DEV_I2S_OUT2,
	HAL_AUD_DEV_I2S_OUT3,

	/* 16-31: Line OUT */
	HAL_AUD_DEV_LINE_OUT0 = 16,
	HAL_AUD_DEV_LINE_OUT1,
	HAL_AUD_DEV_LINE_OUT2,
	HAL_AUD_DEV_LINE_OUT3,

	/*
	 * 256-511: input
	 */

	/* 256-271: I2S IN */
	HAL_AUD_DEV_I2S_IN0 = 256,
	HAL_AUD_DEV_I2S_IN1,
	HAL_AUD_DEV_I2S_IN2,
	HAL_AUD_DEV_I2S_IN3,

	/* 272-287: Line IN */
	HAL_AUD_DEV_LINE_IN0 = 272,
	HAL_AUD_DEV_LINE_IN1,
	HAL_AUD_DEV_LINE_IN2,
	HAL_AUD_DEV_LINE_IN3,
} hal_aud_dev_e;

/* audio sample format */
typedef enum {
	/* Signed, 8-bit */
	HAL_PCM_FORMAT_S8 = 1,
	/* Signed 16-bit, little endian */
	HAL_PCM_FORMAT_S16_LE = 0,
	/* Signed, 16-bit, big endian */
	HAL_PCM_FORMAT_S16_BE = 2,
	/* Signed, 24-bit (32-bit in memory), little endian */
	HAL_PCM_FORMAT_S24_LE,
	/* Signed, 24-bit (32-bit in memory), big endian */
	HAL_PCM_FORMAT_S24_BE,
	/* Signed, 24-bit, little endian */
	HAL_PCM_FORMAT_S24_3LE,
	/* Signed, 24-bit, big endian */
	HAL_PCM_FORMAT_S24_3BE,
	/* Signed, 32-bit, little endian */
	HAL_PCM_FORMAT_S32_LE,
	/* Signed, 32-bit, big endian */
	HAL_PCM_FORMAT_S32_BE,
	/* Max of the enumeration list */
	HAL_PCM_FORMAT_MAX
} hal_pcm_format_e;

/* audio sample rate */
typedef enum {
	HAL_PCM_SAMPLE_RATE_8000 = 8000,
	HAL_PCM_SAMPLE_RATE_11025 = 11025,
	HAL_PCM_SAMPLE_RATE_12000 = 12000,
	HAL_PCM_SAMPLE_RATE_16000 = 16000,
	HAL_PCM_SAMPLE_RATE_22050 = 22050,
	HAL_PCM_SAMPLE_RATE_24000 = 24000,
	HAL_PCM_SAMPLE_RATE_32000 = 32000,
	HAL_PCM_SAMPLE_RATE_44100 = 44100,
	HAL_PCM_SAMPLE_RATE_48000 = 48000,
	HAL_PCM_SAMPLE_RATE_96000 = 96000,
} hal_pcm_sample_rate_e;

typedef enum {
	HAL_PCM_BIT_WIDTH_16,	/* 16-bit width */
	HAL_PCM_BIT_WIDTH_24,	/* 24-bit width */
	HAL_PCM_BIT_WIDTH_MAX,
} hal_pcm_bit_width_e;

typedef enum {
	HAL_AUD_SOUND_MODE_MONO,	/* mono */
	HAL_AUD_SOUND_MODE_STEREO,	/* stereo */
	HAL_AUD_SOUND_MODE_QUEUE,	/* all data in one channel */
	/* Max of the enumeration list */
	HAL_AUD_SOUND_MODE_MAX
} hal_aud_sound_mode_e;

/* audio frame */
typedef struct hal_aud_frame_s {
	/* 'private_data' is only used by HAL. User codes should ignore it. */
	void *private_data;

	unsigned int seq;	/* audio frame sequence number */
	unsigned long long timestamp;	/* timestamp, in us */
	void *vir_addr[HAL_AUD_MAX_CHN_NUM];
	unsigned int len;	/* data lenth per channel in frame */
	hal_pcm_format_e format;/* sample format */
	hal_aud_sound_mode_e sound_mode;/* momo or stereo mode */

	/* Only use for audio input */
	void *ori_vir_addr[HAL_AUD_MAX_CHN_NUM];/* original pcm data from audio input channel */
	unsigned int ori_len;	/* length of audio input original pcm data */
} hal_aud_frame_s;


/*
 * VQE (Voice Quality Enhancement)
 */

/* HPF */
typedef enum {
	HAL_AUD_HPF_MODE_DEFAULT,
} hal_aud_hpf_mode_e;

typedef struct hal_aud_hpf_attr_s {
	hal_aud_hpf_mode_e mode;

} __attribute__((packed, aligned(128))) hal_aud_hpf_attr_t;

/* ANR */

typedef enum {
	HAL_AUD_ANR_MODE_DEFAULT,
} hal_aud_anr_mode_e;

typedef struct hal_aud_anr_attr_s {
	hal_aud_anr_mode_e mode;

} __attribute__((packed, aligned(128))) hal_aud_anr_attr_t;

/* AGC */

typedef enum {
	HAL_AUD_AGC_MODE_DEFAULT,
} hal_aud_agc_mode_e;

typedef struct hal_aud_agc_attr_s {
	hal_aud_agc_mode_e mode;

} __attribute__((packed, aligned(128))) hal_aud_agc_attr_t;

/* EQ */
typedef enum {
	HAL_AUD_EQ_MODE_DEFAULT,
} hal_aud_eq_mode_e;

typedef struct hal_aud_eq_attr_s {
	hal_aud_eq_mode_e mode;

} __attribute__((packed, aligned(128))) hal_aud_eq_attr_t;

/* AEC */

typedef enum {
	HAL_AUD_AEC_MODE_DEFAULT,
} hal_aud_aec_mode_e;

typedef struct hal_aud_aec_attr_s {
	hal_aud_aec_mode_e mode;
	int ao_dev; /* audio output device for AEC */
	int ao_chn; /* audio output channel for AEC */
	int gain;

} __attribute__((packed, aligned(128))) hal_aud_aec_attr_t;

typedef struct hal_aud_vqe_attr_s {
	/* HPF */
	bool hpf_enable;
	hal_aud_hpf_attr_t hpf_attr;
	/* ANR */
	bool anr_enable;
	hal_aud_anr_attr_t anr_attr;
	/* AGC */
	bool agc_enable;
	hal_aud_agc_attr_t agc_attr;
	/* EQ */
	bool eq_enable;
	hal_aud_eq_attr_t eq_attr;
	/* AEC */
	bool aec_enable;
	hal_aud_aec_attr_t aec_attr;
} __attribute__((packed, aligned(4))) hal_aud_vqe_attr_t;

/*
 * ADEC, AENC
 */

typedef enum {
	HAL_AUD_ENC_TYPE_G711A,
	HAL_AUD_ENC_TYPE_G711U,
	HAL_AUD_ENC_TYPE_G726,
} hal_aud_enc_type_e;

typedef enum {
	HAL_AUD_G726_MODE_16,
	HAL_AUD_G726_MODE_24,
	HAL_AUD_G726_MODE_32,
	HAL_AUD_G726_MODE_40,
	HAL_AUD_G726_MODE_INVALID,
} hal_aud_g726_mode_e;


#endif
