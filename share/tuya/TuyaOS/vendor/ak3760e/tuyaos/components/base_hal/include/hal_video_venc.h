/*
 * Copyright (C) 2021 Tuya Inc.
 */

#ifndef HAL_VIDEO_VENC_H
#define HAL_VIDEO_VENC_H

#include "hal_video_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hal_video_venc_attr_s {

/* This structure is packed to 32 bytes for extension */
} __attribute__((packed, aligned(32))) hal_video_venc_attr_t;

/* venc bind type */
typedef enum {
	HAL_VIDEO_VENC_BIND_TYPE_DEFAULT,	/* default type */
	HAL_VIDEO_VENC_BIND_TYPE_HW_RING,	/* hardware ring buffer */
	HAL_VIDEO_VENC_BIND_TYPE_FRAME_BASE,	/* frame based */
	HAL_VIDEO_VENC_BIND_TYPE_MAX
} hal_video_venc_bind_type_e;


typedef enum {
	HAL_VIDEO_PB_FRAME = 0,
	HAL_VIDEO_I_FRAME,
	HAL_VIDEO_TS_FRAME,
	HAL_VIDEO_FRAME_TYPE_MAX
} hal_video_frame_type_e;

typedef struct hal_video_venc_chn_attr_s {
	/* The previous module connected with VENC */
	hal_video_module_e bind_mod;
	/* The bind type between VENC and previous module */
	hal_video_venc_bind_type_e bind_type;
	/*
	 * data path: VPE -> VPP (optional) -> VENC
	 */
	/* The port/chn of previous module */
	unsigned int vpe_chn;	/* VPE channel */
	unsigned int vpe_port;	/* VPE port */
	unsigned int vpp_chn;	/* VPP channel */
	unsigned int vpp_port;	/* VPP port */

	/* encode standard */
	hal_video_enc_std_e std;
	hal_video_res_t res;	/* resolution */
	unsigned int fps_src;
	unsigned int fps_dst;
	unsigned int bitrate;	/* bit rate */
	unsigned int gop;	/* GOP */
	unsigned int qp_min;	/* minimum QP */
	unsigned int qp_max;	/* maximum QP */
	int frame_depth;  	/** frame buffer depth */
/* This structure is packed to 256 bytes for extension */
} __attribute__((packed, aligned(256))) hal_video_venc_chn_attr_t;

typedef struct hal_video_venc_packet_s {
	void *virt_addr;		/* packet virtual address */
	unsigned long long phy_addr;	/* packet physical address */
	unsigned int length;	/* packet length */
	unsigned int offset;	/* data offset in packet */
	unsigned long long pts;	/* PTS, in us */
	bool frame_end;		/* last packet of current frame ? */
        hal_video_frame_type_e type;
} __attribute__((packed, aligned(4))) hal_video_venc_packet_t;

typedef struct hal_video_venc_stream_s {
	/* 'private_data' is only used by HAL. User codes should ignore it. */
	void *private_data;

	unsigned int seq;		/* stream sequence number */
	unsigned int pkt_num;		/* number of packets */
	hal_video_venc_packet_t *pkts;	/* packets */
} hal_video_venc_stream_t;

/** Initialize VENC.
 * @param void
 * @param attr venc attributes.
 * @returns 0 on success, error code on failure.
 */
int hal_video_venc_init(hal_video_venc_attr_t *attr);

/** Deinitialize VENC.
 * @param void
 * @returns 0 on success, error code on failure.
 */
int hal_video_venc_deinit(void);

/** Create a channel.
 * @param chn channel ID.
 * @param attr channel attributes.
 * @returns 0 on success, error code on failure.
 */
int hal_video_venc_create_channel(int chn, hal_video_venc_chn_attr_t *attr);

/** Destroy a channel.
 * @param chn channel ID.
 * @returns 0 on success, error code on failure.
 */
int hal_video_venc_destroy_channel(int chn);

/** Start a channel.
 * @param chn channel ID.
 * @returns 0 on success, error code on failure.
 */
int hal_video_venc_start_channel(int chn);

/** Stop a channel.
 * @param chn channel ID.
 * @returns 0 on success, error code on failure.
 */
int hal_video_venc_stop_channel(int chn);

/** Waiting for encoded stream.
 * @param chn channel ID.
 * @param timeout
 *     -1 : block indefinitely until stream is ready
 *     0  : non-block
 *     >0 : timeout in milliseconds
 * @returns
 *     0 on success,
 *     -ETIMEDOUT on timeout,
 *     other error code on failure.
 */
int hal_video_venc_wait_for_stream(int chn, int timeout);

/** Get the number of stream packets.
 * @param chn channel ID.
 * @param num packet number.
 * @returns 0 on success, error code on failure.
 */
int hal_video_venc_get_packet_num(int chn, int *num);

/** Get encoded stream packets.
 * @param chn channel ID.
 * @param stream stream packets.
 * @param timeout
 *     -1 : block indefinitely until stream is ready
 *     0  : non-block
 *     >0 : timeout in milliseconds
 * @returns 0 on success,
 *          -ETIMEDOUT on timeout,
 *          other error code on failure.
 */
int hal_video_venc_get_stream(int chn, hal_video_venc_stream_t *stream, int timeout);

/** Release stream packets.
 * @param chn channel ID.
 * @param stream stream packets.
 * @returns 0 on success, error code on failure.
 */
int hal_video_venc_release_stream(int chn, hal_video_venc_stream_t *stream);

#ifdef __cplusplus
}
#endif

#endif
