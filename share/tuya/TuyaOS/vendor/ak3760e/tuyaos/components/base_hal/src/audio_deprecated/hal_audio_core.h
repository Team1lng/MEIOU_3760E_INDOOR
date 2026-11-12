/*
 * Copyright (C) 2021 Tuya Inc.
 */

#ifndef __HAL_AUDIO_CORE_H
#define __HAL_AUDIO_CORE_H

#include "hal_audio.h"

/* HAL interface defination */

typedef struct {
    int (*init)(int dev, int chn, hal_aud_in_attr_t *attr);
    int (*deinit)(int dev, int chn);
    int (*wait_for_frame)(int dev, int chn, int timeout);
    int (*get_frame)(int dev, int chn, hal_aud_frame_s *frame, int timeout);
    int (*release_frame)(int dev, int chn, hal_aud_frame_s *frame);
    int (*set_volume)(int dev, int chn, int vol);
    int (*get_volume)(int dev, int chn, int *vol);
    int (*mute)(int dev, int chn);
    int (*unmute)(int dev, int chn);
} hal_if_audio_in_t;

typedef struct {
    int (*init)(int dev, int chn, hal_aud_out_attr_t *attr);
    int (*deinit)(int dev, int chn);
    int (*write_frame)(int dev, int chn, hal_aud_frame_s *frame, int timeout);
    int (*set_volume)(int dev, int chn, int vol);
    int (*get_volume)(int dev, int chn, int *vol);
    int (*mute)(int dev, int chn);
    int (*unmute)(int dev, int chn);
    int (*speaker_switch)(int num, bool is_on);
} hal_if_audio_out_t;

typedef struct {
    hal_if_audio_in_t in;
    hal_if_audio_out_t out;
} hal_if_audio_t;

extern hal_if_audio_t hal_if_audio;


int hal_aud_out_speaker_switch_common(int num, bool is_on);

#endif
