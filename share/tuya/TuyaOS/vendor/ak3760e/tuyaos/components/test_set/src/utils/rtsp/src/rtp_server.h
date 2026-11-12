#ifndef __RTP_SERVER_H__
#define __RTP_SERVER_H__


#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "rtsp_session.h"
#include "rtsp_server.h"

typedef void (*activate_net_cb)();    //用于使IO线程即时唤醒(从select返回)的回调

typedef struct {
    rtsp_session_t* psession;
    rtsp_stream_src_t* pstream_src;
    activate_net_cb activate_net;
} rtp_server_param_t;

typedef void* RTP_SERVER_HANDLE;
#define RTP_SERVER_INVALID_HANDLE ((void*)0)

int rtp_server_recv_data(RTP_SERVER_HANDLE handle, RTSP_SESSION_CHN_E chn);
int rtp_server_send_data(RTP_SERVER_HANDLE handle, RTSP_SESSION_CHN_E chn);

RTP_SERVER_HANDLE rtp_server_create(rtp_server_param_t* param);
void rtp_server_destroy(RTP_SERVER_HANDLE handle);

int rtp_server_start(RTP_SERVER_HANDLE handle, uint16_t seq, uint32_t ts);
int rtp_server_stop(RTP_SERVER_HANDLE handle);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __RTP_SERVER_H__

