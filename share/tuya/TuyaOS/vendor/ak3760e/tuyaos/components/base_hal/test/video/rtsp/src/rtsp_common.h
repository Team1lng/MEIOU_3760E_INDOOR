#ifndef __RTSP_COMMON_H__
#define __RTSP_COMMON_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "rtsp_session.h"
#include "byte_array_buf.h"
#include "rtsp.h"

typedef int (*rtsp_common_handle_msg_cb)(void* pri_data, rtsp_msg_t* pmsg);
typedef int (*rtsp_common_handle_rtp_cb)(void* pri_data, int chn, char* rtp_buf, int buf_len);

int rtsp_common_parse_data(void* pri_data, rtsp_common_handle_msg_cb msg_cb, rtsp_common_handle_rtp_cb rtp_cb,
                            byte_array_buf_t* prx_buf, rtsp_msg_t* pmsg);
#ifdef __cplusplus
} // extern "C"
#endif

#endif // __RTSP_COMMON_H__
