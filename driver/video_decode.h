#ifndef _AMY_MJPEG_DECODEC_H_
#define _AMY_MJPEG_DECODEC_H_
#include "Amy_sdk.h"
#include "ak_common.h"
#include "ak_vdec.h"
typedef struct video_decode_context video_decode_context;


video_decode_context* video_decodec_open(int max_w, int max_h, int enc_type);

stream_data* video_decodec_buf_get(video_decode_context* ctx, int size);

int video_decodec_buf_put(video_decode_context* ctx, stream_data* data);

int video_decodec_send_stream(video_decode_context* ctx, stream_data* data);

int video_decodec_close(video_decode_context* ctx);

int video_decodec_get_frame(video_decode_context* ctx, struct ak_vdec_frame* pframe, int* is_new);

int video_decodec_stream_query(video_decode_context* ctx, unsigned int* free, unsigned int* used, unsigned int* total);

int video_decodec_release_frame(video_decode_context* ctx);

#endif