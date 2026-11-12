/*
 * Copyright (C) 2021 Tuya Inc.
 */

#ifndef HAL_VIDEO_SENSOR_H
#define HAL_VIDEO_SENSOR_H

#include "hal_video_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hal_video_snr_attr_s {
	bool hdr_en;
	hal_video_res_t main_res;
	hal_video_res_t sub_res;
	char *cfg_file;
/* This structure is packed to 64 bytes for extension */
} __attribute__((packed, aligned(64))) hal_video_snr_attr_t;

/** Initialize a sensor device.
 * @param id sensor ID.
 * @param attr sensor attributes.
 * @returns 0 on success, error code on failure.
 */
int hal_video_snr_init(int id, hal_video_snr_attr_t *attr);

/** Deinitialize a sensor device.
 * @param id sensor ID.
 * @returns 0 on success, error code on failure.
 */
int hal_video_snr_deinit(int id);

/** Enable a sensor device.
 * @param id sensor ID.
 * @returns 0 on success, error code on failure.
 */
int hal_video_snr_enable(int id);

/** Disable a sensor device.
 * @param id sensor ID.
 * @returns 0 on success, error code on failure.
 */
int hal_video_snr_disable(int id);

/** Set frame rate.
 * @param id sensor ID.
 * @param fps frame rate.
 * @returns 0 on success, error code on failure.
 */
int hal_video_snr_set_fps(int id, int fps);

/** Get frame rate.
 * @param id sensor ID.
 * @param fps frame rate.
 * @returns 0 on success, error code on failure.
 */
int hal_video_snr_get_fps(int id, int *fps);

/** Get the number of resolution.
 * @param id sensor ID.
 * @param num the number of resolution.
 * @returns 0 on success, error code on failure.
 */
int hal_video_snr_get_res_num(int id, int *num);

/** Get one resolution.
 * @param id sensor ID.
 * @param index the index of resolution.
 * @param res resolution.
 * @returns 0 on success, error code on failure.
 */
int hal_video_snr_get_one_res(int id, int index, hal_video_res_t *res);

/** Get current resolution.
 * @param id sensor ID.
 * @param index the index of resolution.
 * @param res resolution.
 * @returns 0 on success, error code on failure.
 */
int hal_video_snr_get_cur_res(int id, int *index, hal_video_res_t *res);

/** Set resolution.
 * @param id sensor ID.
 * @param index the index of resolution.
 * @returns 0 on success, error code on failure.
 */
int hal_video_snr_set_res(int id, int index);

/** Set mirror type.
 * @param id sensor ID.
 * @param mirror mirror type.
 * @returns 0 on success, error code on failure.
 */
int hal_video_snr_set_mirror(int id, hal_video_mirror_e mirror);

/** Get mirror type.
 * @param id sensor ID.
 * @param mirror mirror type.
 * @returns 0 on success, error code on failure.
 */
int hal_video_snr_get_mirror(int id, hal_video_mirror_e *mirror);

#ifdef __cplusplus
}
#endif

#endif
