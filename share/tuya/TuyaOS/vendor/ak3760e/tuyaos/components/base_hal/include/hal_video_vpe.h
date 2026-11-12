/*
 * Copyright (C) 2021 Tuya Inc.
 */

#ifndef HAL_VIDEO_VPE_H
#define HAL_VIDEO_VPE_H

#include "hal_video_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hal_video_vpe_attr_s {

/* This structure is packed to 32 bytes for extension */
} __attribute__((packed, aligned(32))) hal_video_vpe_attr_t;

typedef struct hal_video_vpe_chn_attr_s {
	int vif_dev;	/* the device number of VIF connected to VPE */
	int vif_chn;	/* the channel number of VIF connected to VPE */
	int vif_port;	/* the port number of VIF connected to VPE */

	int pixfmt;
	hal_video_res_t res;
	hal_video_rotate_e rotate;
	hal_video_mirror_e mirror;
	int src_fps;	/* fps of input port */
	int dst_fps;	/* fps of output port */
	bool hdr_en;	/* HDR */
	bool nr_en;	/* Denoising */
	bool nr_3d_en;	/* 3D Denoising */
	bool sharpen_en;/* sharpen */
	bool es_en;	/* edge smooth */
/* This structure is packed to 256 bytes for extension */
} __attribute__((packed, aligned(256))) hal_video_vpe_chn_attr_t;

/** Initialize VPE.
 * @param attr vpe attributes.
 * @returns 0 on success, error code on failure.
 */
int hal_video_vpe_init(hal_video_vpe_attr_t *attr);

/** Deinitialize VPE.
 * @param void
 * @returns 0 on success, error code on failure.
 */
int hal_video_vpe_deinit(void);

/** Create a channel.
 * @param chn channel ID.
 * @param port port ID.
 * @param attr channel attributes.
 * @returns 0 on success, error code on failure.
 */
int hal_video_vpe_create_channel(int chn, int port, hal_video_vpe_chn_attr_t *attr);

/** Destroy a channel.
 * @param chn channel ID.
 * @param port port ID.
 * @returns 0 on success, error code on failure.
 */
int hal_video_vpe_destroy_channel(int chn, int port);

/** Start a channel.
 * @param chn channel ID.
 * @param port port ID.
 * @returns 0 on success, error code on failure.
 */
int hal_video_vpe_start_channel(int chn, int port);

/** Stop a channel.
 * @param chn channel ID.
 * @param port port ID.
 * @returns 0 on success, error code on failure.
 */
int hal_video_vpe_stop_channel(int chn, int port);

/** Load ISP parameter file
 * @param dev ISP device ID. always 0.
 * @param path parameter file.
 * @param magic user-defined magic key.
 * @returns 0 on success, error code on failure.
 */
int hal_video_vpe_isp_load_para_file(int dev, char *path, unsigned int magic);

#ifdef __cplusplus
}
#endif

#endif
