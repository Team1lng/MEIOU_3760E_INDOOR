#ifndef __RTSP_SESSION_H__
#define __RTSP_SESSION_H__


#ifdef __cplusplus
extern "C" {
#endif

#include "byte_array_buf.h"
#include "rtsp.h"

typedef enum {
    RTSP_VID_DATA_CHN,
    RTSP_VID_CTRL_CHN,
    RTSP_AUD_DATA_CHN,
    RTSP_AUD_CTRL_CHN,
    RTSP_SESSION_CHN_NUM,
} RTSP_SESSION_CHN_E;


typedef struct {
    RTSP_TRANSPORT_MODE_E mode;
    int track_flag;
    int stream_id;
    int active;
    uint16_t begin_seq;
    char session_id[32];
    union{
        char client_ip[64];
        char server_ip[64];
    };
    byte_array_buf_t *prx_buf; //tcp模式下rtsp连接的rx_buf
    byte_array_buf_t *ptx_buf; //tcp模式下rtsp连接的tx_buf
    union {
        struct {
            int fd;
            int cli_port;
            int svr_port;
            byte_array_buf_t rx_buf; //udp模式下rtp, rtcp通道的rx_buf
            byte_array_buf_t tx_buf; //udp模式下rtp, rtcp通道的tx_buf
        };
        struct {
            int interleave;
        };
    } transport[RTSP_SESSION_CHN_NUM];
} rtsp_session_t;


int rtsp_session_create(rtsp_session_t* psession, RTSP_TRANSPORT_MODE_E mode,
                byte_array_buf_t* prx_buf, byte_array_buf_t* ptx_buf);

int rtsp_session_destroy(rtsp_session_t* psession);
int rtsp_session_set_cli_port(rtsp_session_t* psession, RTSP_SESSION_CHN_E chn, int port);
int rtsp_session_set_svr_port(rtsp_session_t* psession, RTSP_SESSION_CHN_E chn, int port);
int rtsp_session_set_fd(rtsp_session_t* psession, RTSP_SESSION_CHN_E chn, int fd);
int rtsp_session_set_interleaved(rtsp_session_t* psession, RTSP_SESSION_CHN_E chn, int intl);
int rtsp_session_init_buffer(rtsp_session_t* psession, RTSP_SESSION_CHN_E chn);
#ifdef __cplusplus
} // extern "C"
#endif

#endif // __RTSP_SESSION_H__

