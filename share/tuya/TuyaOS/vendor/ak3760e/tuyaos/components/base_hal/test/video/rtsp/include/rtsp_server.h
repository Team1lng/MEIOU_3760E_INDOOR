#ifndef __RTSP_SERVER_H__
#define __RTSP_SERVER_H__


#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "rtsp.h"

typedef struct {
    int (*get_sample_rate)(RTSP_MEDIA_TYPE_E type);
    int (*get_frame)(int user_id, RTSP_MEDIA_TYPE_E type, char** buf, int *plen, uint64_t *pts); //成功返回0, 第一个视频帧需要I帧
    RTSP_CODEC_E (*get_codec)(RTSP_MEDIA_TYPE_E type);
    int (*start)(); //返回user_id
    int (*stop)(int user_id);
} rtsp_stream_src_t;

int rtsp_server_register_stream_src(rtsp_stream_src_t* psrc);
int rtsp_server_start(char* username, char* password);
int rtsp_server_stop();

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __RTSP_SERVER_H__
