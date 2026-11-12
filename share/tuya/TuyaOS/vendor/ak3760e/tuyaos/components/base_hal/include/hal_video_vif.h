/*
 * Copyright (C) 2021 Tuya Inc.
 */

#ifndef HAL_VIDEO_VIF_H
#define HAL_VIDEO_VIF_H

#include "hal_video_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hal_video_vif_attr_s {
	unsigned int snr_id;	/* the ID of sensor connected to VIF */
	int reserved[2];
	hal_video_res_t res;
	bool hdr_en;
/* This structure is packed to 64 bytes for extension */
} __attribute__((packed, aligned(64))) hal_video_vif_attr_t;

typedef struct hal_video_vif_info_s {
	int pixfmt;
	int fps;
	hal_video_res_t res;
	hal_video_mirror_e mirror;
	hal_video_rotate_e rotate;
/* This structure is packed to 64 bytes for extension */
} __attribute__((packed, aligned(64))) hal_video_vif_info_t;

/** Initialize VIF.
 * @param dev device ID.
 * @param chn channel ID.
 * @param port port ID.
 * @param attr VIF attributes.
 * @returns 0 on success, error code on failure.
 */
int hal_video_vif_init(int dev, int chn, int port, hal_video_vif_attr_t *attr);

/** Deinitialize VIF.
 * @param dev device ID.
 * @param chn channel ID.
 * @param port port ID.
 * @returns 0 on success, error code on failure.
 */
int hal_video_vif_deinit(int dev, int chn, int port);

/** Enable VIF.
 * @param dev device ID.
 * @param chn channel ID.
 * @param port port ID.
 * @returns 0 on success, error code on failure.
 */
int hal_video_vif_enable(int dev, int chn, int port);

/** Disable VIF.
 * @param dev device ID.
 * @param chn channel ID.
 * @param port port ID.
 * @returns 0 on success, error code on failure.
 */
int hal_video_vif_disable(int dev, int chn, int port);

/** Get pixel format.
 * @param dev device ID.
 * @param chn channel ID.
 * @param port port ID.
 * @param info vif info.
 * @returns 0 on success, error code on failure.
 */
int hal_video_vif_get_info(int dev, int chn, int port, hal_video_vif_info_t *info);

#ifdef __cplusplus
}
#endif

#endif
