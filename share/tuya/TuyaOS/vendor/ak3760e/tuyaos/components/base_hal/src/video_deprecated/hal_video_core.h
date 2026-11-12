/*
 * Copyright (C) 2021 Tuya Inc.
 */

#ifndef __HAL_VIDEO_CORE_H
#define __HAL_VIDEO_CORE_H

#include "hal_video.h"

/* HAL interface defination */

typedef struct {
	int (*init)(void);
	int (*deinit)(void);
} hal_if_video_sys_t;

typedef struct {
	int (*init)(int id, hal_video_snr_attr_t *attr);
	int (*deinit)(int id);
	int (*enable)(int id);
	int (*disable)(int id);

	int (*set_fps)(int id, int fps);
	int (*get_fps)(int id, int *fps);

	int (*get_res_num)(int id, int *num);
	int (*get_one_res)(int id, int index, hal_video_res_t *res);
	int (*get_cur_res)(int id, int *index, hal_video_res_t *res);
	int (*set_res)(int id, int index);

	int (*set_mirror)(int id, hal_video_mirror_e mirror);
	int (*get_mirror)(int id, hal_video_mirror_e *mirror);
} hal_if_video_sensor_t;

typedef struct {
	int (*init)(int dev, int chn, int port, hal_video_vif_attr_t *attr);
	int (*deinit)(int dev, int chn, int port);
	int (*enable)(int dev, int chn, int port);
	int (*disable)(int dev, int chn, int port);
	int (*get_info)(int dev, int chn, int port, hal_video_vif_info_t *info);
} hal_if_video_vif_t;

typedef struct {
	int (*init)(hal_video_vpe_attr_t *attr);
	int (*deinit)(void);
	int (*create_channel)(int chn, int port, hal_video_vpe_chn_attr_t *attr);
	int (*destroy_channel)(int chn, int port);
	int (*start_channel)(int chn, int port);
	int (*stop_channel)(int chn, int port);
	int (*isp_load_para_file)(int dev, char *path, unsigned int magic);
} hal_if_video_vpe_t;

typedef struct {
	int (*init)(hal_video_vpp_attr_t *attr);
	int (*deinit)(void);
	int (*create_channel)(int chn, hal_video_vpp_chn_attr_t *attr);
	int (*destroy_channel)(int chn);
	int (*start_channel)(int chn);
	int (*stop_channel)(int chn);
} hal_if_video_vpp_t;

typedef struct {
	int (*init)(hal_video_venc_attr_t *attr);
	int (*deinit)(void);

	int (*create_channel)(int chn, hal_video_venc_chn_attr_t *attr);
	int (*destroy_channel)(int chn);
	int (*start_channel)(int chn);
	int (*stop_channel)(int chn);

	int (*wait_for_stream)(int chn, int timeout);
	int (*get_packet_num)(int chn, int *num);
	int (*get_stream)(int chn, hal_video_venc_stream_t *stream, int timeout);
	int (*release_stream)(int chn, hal_video_venc_stream_t *stream);
} hal_if_video_venc_t;

typedef struct {
	hal_if_video_sys_t sys;
	hal_if_video_sensor_t snr;
	hal_if_video_vif_t vif;
	hal_if_video_vpe_t vpe;
	hal_if_video_vpp_t vpp;
	hal_if_video_venc_t venc;
} hal_if_video_t;

extern hal_if_video_t hal_if_video;

#endif
