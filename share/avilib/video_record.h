#ifndef _VIDEO_RECORD_H_
#define _VIDEO_RECORD_H_
#include "Amy_sdk.h"
typedef struct avi_record_context avi_record_context;

avi_record_context* avi_record_open(int w, int h, int sample_rate, const char* path,const char* compressor, int(*video_record_finish_func)(const char* file,int));
int avi_record_close(avi_record_context* ctx);

stream_data* avi_buffer_get(avi_record_context* ctx, int size);

int avi_buffer_put(avi_record_context* ctx, stream_data* data);

int avi_buffer_put(avi_record_context* ctx, stream_data* data);

int avi_video_send(avi_record_context* ctx, stream_data* data);

int avi_audio_send(avi_record_context* ctx, stream_data* data);

int avi_has_audio(avi_record_context* ctx);
#endif