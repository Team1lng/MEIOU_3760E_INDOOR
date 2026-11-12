#ifndef __RTSP_SERVER_H__
#define __RTSP_SERVER_H__


#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "rtsp.h"
#include "rtp.h"

typedef struct {
    int (*get_sample_rate)(RTSP_MEDIA_TYPE_E type);
    int (*get_frame)(int user_id, RTSP_MEDIA_TYPE_E type, char** buf, int *plen, uint64_t *pts); //成功返回0, 第一个视频帧需要I帧
    RTP_CODEC_E (*get_codec)(RTSP_MEDIA_TYPE_E type);
    int (*start)(); //返回user_id
    int (*stop)(int user_id);
    int (*get_name)(char* buf, int* buf_len);
} rtsp_stream_src_t;

typedef int STREAM_SRC_ID;

//return stream id
STREAM_SRC_ID rtsp_server_register_stream_src(rtsp_stream_src_t* psrc);
int rtsp_server_start(char* username, char* password);
int rtsp_server_stop();
int rtsp_server_set_password(char* password);
int rtsp_server_get_url(STREAM_SRC_ID id, char* url, int* plen);
int rtsp_server_set_url(STREAM_SRC_ID id, int (*get_name)(char* buf, int* buf_len));

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __RTSP_SERVER_H__
