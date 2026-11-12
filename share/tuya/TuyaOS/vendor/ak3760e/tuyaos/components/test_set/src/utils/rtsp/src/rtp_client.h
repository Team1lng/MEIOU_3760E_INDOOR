#ifndef __RTP_CLIENT_H__
#define __RTP_CLIENT_H__


#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "rtsp_session.h"
#include "rtsp_internal.h"

typedef void* RTP_CLIENT_HANDLE;

typedef int (*put_rtp_frm_cb)(void* pri_data, rtsp_data_packet_s* pkt);

typedef struct {
    rtsp_session_t*      psession;
    rtsp_media_info_t*   pmedia_info;
    put_rtp_frm_cb       put_rtp_frm;
    void*                cb_pri_data;
} rtp_client_param_s;

RTP_CLIENT_HANDLE* rtp_client_create(rtp_client_param_s* param);

void rtp_client_destroy(RTP_CLIENT_HANDLE handle);

int rtp_client_handle_packet(RTP_CLIENT_HANDLE handle, RTSP_SESSION_CHN_E chn, char* rtp_buf, int buf_len);

int rtp_client_recv_data(RTP_CLIENT_HANDLE handle, RTSP_SESSION_CHN_E chn);

int rtp_client_send_data(RTP_CLIENT_HANDLE handle, RTSP_SESSION_CHN_E chn);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __RTP_CLIENT_H__

