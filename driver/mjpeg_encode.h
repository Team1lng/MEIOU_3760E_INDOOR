#ifndef _AMY_MJPEG_ENCODE_H_
#define _AMY_MJPEG_ENCODE_H_
#include "Amy_sdk.h"
typedef struct mjpeg_encodec_context mjpeg_encodec_context;

mjpeg_encodec_context* mjpeg_encode_open(int width, int height);

int mjpeg_encode_close(mjpeg_encodec_context* ctx);

int mjpeg_encode_send_frame(mjpeg_encodec_context* ctx, const unsigned char* data, int size, int w, int h);

int mjpeg_encode_get_stream(mjpeg_encodec_context* ctx, stream_data** data);

int mjpeg_encode_release_stream(mjpeg_encodec_context* ctx, stream_data* data);
#endif