/*
 * Copyright (C) 2021 Tuya Inc.
 */

#ifndef HAL_VIDEO_VPP_H
#define HAL_VIDEO_VPP_H

#include "hal_video_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* vpp bind type */
typedef enum {
	HAL_VIDEO_VPP_BIND_TYPE_DEFAULT,
	HAL_VIDEO_VPP_BIND_TYPE_MAX
} hal_video_vpp_bind_type_e;

typedef struct hal_video_vpp_attr_s {

/* This structure is packed to 32 bytes for extension */
} __attribute__((packed, aligned(32))) hal_video_vpp_attr_t;

typedef struct hal_video_vpp_chn_attr_s {
	/* The previous module connected with VPP */
	hal_video_module_e bind_mod;
	/* The bind type between VPP and previous module */
	hal_video_vpp_bind_type_e bind_type;
	/* The port/chn of previous module */
	int vpe_chn;	/* VPE channel */
	int vpe_port;	/* VPE port */
	int reserved[4];/* reserved for other module's chn/port */

	int pixfmt;
	hal_video_res_t res;
	hal_video_rotate_e rotate;
	hal_video_mirror_e mirror;
/* This structure is packed to 128 bytes for extension */
} __attribute__((packed, aligned(128))) hal_video_vpp_chn_attr_t;

/** Initialize VPP.
 * @param attr vpp attributes.
 * @returns 0 on success, error code on failure.
 */
int hal_video_vpp_init(hal_video_vpp_attr_t *attr);

/** Deinitialize VPP.
 * @param void
 * @returns 0 on success, error code on failure.
 */
int hal_video_vpp_deinit(void);

/** Create a channel.
 * @param chn channel ID.
 * @param attr channel attributes.
 * @returns 0 on success, error code on failure.
 */
int hal_video_vpp_create_channel(int chn, hal_video_vpp_chn_attr_t *attr);

/** Destroy a channel.
 * @param chn channel ID.
 * @returns 0 on success, error code on failure.
 */
int hal_video_vpp_destroy_channel(int chn);

/** Start a channel.
 * @param chn channel ID.
 * @returns 0 on success, error code on failure.
 */
int hal_video_vpp_start_channel(int chn);

/** Stop a channel.
 * @param chn channel ID.
 * @returns 0 on success, error code on failure.
 */
int hal_video_vpp_stop_channel(int chn);

#ifdef __cplusplus
}
#endif

#endif
