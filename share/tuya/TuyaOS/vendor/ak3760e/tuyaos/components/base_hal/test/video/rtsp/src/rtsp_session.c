#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "rtsp_session.h"
#include "rtsp.h"


int rtsp_session_create(rtsp_session_t* psession, RTSP_TRANSPORT_MODE_E mode,
                byte_array_buf_t* prx_buf, byte_array_buf_t *ptx_buf)
{
    memset(psession, 0, sizeof(rtsp_session_t));

    psession->mode = mode;
    psession->active = 1;
    psession->prx_buf = prx_buf;
    psession->ptx_buf = ptx_buf;

    int i;
    for (i = 0; i < 8; i++){
        sprintf(psession->session_id + i, "%d", rand() % 10);
    }

    return 0;
}

int rtsp_session_destroy(rtsp_session_t* psession)
{
    memset(psession, 0, sizeof(rtsp_session_t));
    return 0;
}

int rtsp_session_set_cli_port(rtsp_session_t* psession, RTSP_SESSION_CHN_E chn, int port)
{
    if (psession->mode != RTSP_TRANSPORT_MODE_UDP){
        printf("can't set port in tcp mode\n");
        return -1;
    }

    psession->transport[chn].cli_port = port;
    return 0;
}

int rtsp_session_set_svr_port(rtsp_session_t* psession, RTSP_SESSION_CHN_E chn, int port)
{
    if (psession->mode != RTSP_TRANSPORT_MODE_UDP){
        printf("can't set port in tcp mode\n");
        return -1;
    }

    psession->transport[chn].svr_port = port;
    return 0;
}

int rtsp_session_set_fd(rtsp_session_t* psession, RTSP_SESSION_CHN_E chn, int fd)
{
    if (psession->mode != RTSP_TRANSPORT_MODE_UDP){
        printf("can't set port in tcp mode\n");
        return -1;
    }

    psession->transport[chn].fd = fd;
    return 0;
}

int rtsp_session_set_interleaved(rtsp_session_t* psession, RTSP_SESSION_CHN_E chn, int intl)
{
    if (psession->mode != RTSP_TRANSPORT_MODE_TCP){
        printf("can't set interleaved in udp mode\n");
        return -1;
    }

    psession->transport[chn].interleave = intl;
    return 0;
}

int rtsp_session_init_buffer(rtsp_session_t* psession, RTSP_SESSION_CHN_E chn)
{
    if (psession->mode != RTSP_TRANSPORT_MODE_UDP){
        printf("can't init buffer in tcp mode\n");
        return -1;
    }

    byte_array_buf_init(&psession->transport[chn].rx_buf, NULL, 64 * 1024, 1);
    byte_array_buf_init(&psession->transport[chn].tx_buf, NULL, 64 * 1024, 1);

    return 0;
}