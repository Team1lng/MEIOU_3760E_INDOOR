/*
 * Copyright (C) 2021 Tuya Inc.
 */

#ifndef HAL_VIDEO_COMMON_H
#define HAL_VIDEO_COMMON_H

#include "hal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* video module */
typedef enum {
	HAL_VIDEO_MOD_UNDEFINE,
	HAL_VIDEO_MOD_SNR,
	HAL_VIDEO_MOD_VIF,
	HAL_VIDEO_MOD_VPE,
	HAL_VIDEO_MOD_VPP,
	HAL_VIDEO_MOD_VENC,
	HAL_VIDEO_MOD_MAX
} hal_video_module_e;

/* video resolution */
typedef struct hal_video_res_s
{
	unsigned short width;
	unsigned short height;
} hal_video_res_t;

/* video window rectangle */
typedef struct hal_video_rect_s
{
	unsigned short x;
	unsigned short y;
	unsigned short width;
	unsigned short height;
} hal_video_rect_t;

typedef enum {
	HAL_VIDEO_MIRROR_NONE,	/* normal */
	HAL_VIDEO_MIRROR_X,	/* flip horizontally */
	HAL_VIDEO_MIRROR_Y,	/* flip vertically */
	HAL_VIDEO_MIRROR_XY,	/* flip horizontally and vertically */
	HAL_VIDEO_MIRROR_MAX,
} hal_video_mirror_e;

typedef enum {
	HAL_VIDEO_ROTATE_NONE,
	HAL_VIDEO_ROTATE_90,
	HAL_VIDEO_ROTATE_180,
	HAL_VIDEO_ROTATE_270,
	HAL_VIDEO_ROTATE_MAX,
} hal_video_rotate_e;

typedef enum {
	HAL_VIDEO_PIXFMT_YUV422_YUYV = 0,
	HAL_VIDEO_PIXFMT_ARGB8888,
	HAL_VIDEO_PIXFMT_ABGR8888,
	HAL_VIDEO_PIXFMT_BGRA8888,

	HAL_VIDEO_PIXFMT_RGB565,
	HAL_VIDEO_PIXFMT_ARGB1555,
	HAL_VIDEO_PIXFMT_ARGB4444,
	HAL_VIDEO_PIXFMT_I2,
	HAL_VIDEO_PIXFMT_I4,
	HAL_VIDEO_PIXFMT_I8,

	HAL_VIDEO_PIXFMT_YUV_SEMIPLANAR_422,
	HAL_VIDEO_PIXFMT_YUV_SEMIPLANAR_420,
	HAL_VIDEO_PIXFMT_YUV_SEMIPLANAR_420_NV21,
	HAL_VIDEO_PIXFMT_YUV_MST_420,
	HAL_VIDEO_PIXFMT_YUV422_UYVY,
	HAL_VIDEO_PIXFMT_YUV422_YVYU,
	HAL_VIDEO_PIXFMT_YUV422_VYUY,

	HAL_VIDEO_PIXFMT_FORMAT_MAX,
} hal_video_pixfmt_e;

typedef enum {
	HAL_VIDEO_ENC_STD_H264,
	HAL_VIDEO_ENC_STD_H265,
	HAL_VIDEO_ENC_STD_JPEG,
	HAL_VIDEO_ENC_STD_MAX
} hal_video_enc_std_e;

#ifdef __cplusplus
}
#endif

#endif
