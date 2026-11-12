#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <arpa/inet.h>

#include "rtp_client.h"
#include "rtp.h"
#include "rtp_packager.h"
#include "sock_util.h"
#include "rtp_queue.h"
#include "utils.h"
#include "wheel_timer.h"

#define VIDEO_POOL_SIZE 128*1024
#define VIDEO_QUEUE_LEN 128
#define AUDIO_POOL_SIZE 32*1024
#define AUDIO_QUEUE_LEN 128

typedef struct {
    rtsp_session_t*     psession;
    rtsp_media_info_t*  pmedia_info;
    uint16_t            last_seq[RTSP_MEDIA_TYPE_NUM];
    uint32_t            last_rtp_ts[RTSP_MEDIA_TYPE_NUM];
    uint64_t            last_pts[RTSP_MEDIA_TYPE_NUM];
    put_rtp_frm_cb      put_rtp_frm;
    void*               cb_pri_data;
    RTP_QUEUE_HANDLE    q_hdl[RTSP_MEDIA_TYPE_NUM];
} rtp_client_t;

typedef struct {
    rtp_client_t*       prc;
    RTSP_SESSION_CHN_E  chn;
} rtp_client_timer_data_t;


static RTP_CLIENT_HANDLE __rtp_client_to_handle(rtp_client_t* pclient)
{
    return pclient;
}

static rtp_client_t* __rtp_client_from_handle(RTP_CLIENT_HANDLE handle)
{
    return (rtp_client_t*)handle;
}

static int __pop_rtp_from_queue(void* pri_data, RTSP_SESSION_CHN_E chn, char* buf, int len)
{
    rtp_client_t* p = (rtp_client_t*)pri_data;
    RTP_CLIENT_HANDLE hdl = __rtp_client_to_handle(p);

    rtp_client_handle_packet(hdl, chn, buf, len);

    return 0;
}

void __send_nack_timer_cb(WHEEL_TIMER_HANDLE handle, void* pri_data)
{
    rtp_queue_nack_s* pdata = (rtp_queue_nack_s*)pri_data;
    rtp_client_t* prc = (rtp_client_t*)pdata->pri_data;

    byte_array_buf_t* pbuf = &(prc->psession->transport[pdata->chn].tx_buf);

    rtcp_header_t rtcp_hdr = {
        .subtype = RTCP_APP_SUBTYPE_NACK,
        .payload_type = RTCP_TYPE_APP,
        .version = 2,
        .length = 3,
    };

    char* buf = NULL;
    int len = byte_array_buf_write_ptr_lock(pbuf, &buf);
    //前4个字节写入数据包的长度
    buf += 4;
    len -= 4;
    int ret = rtcp_pack_header(&rtcp_hdr, buf, len);
    if (ret > 0 && ret + 12 <= len){
        memset(buf + ret, 0, 4); //ssrc
        memcpy(buf + ret + 4, "nack", 4); //name
        int iseq = htonl(pdata->seq);
        memcpy(buf + ret + 8, &iseq, 4);

        int write_len = ret + 12;
        memcpy(buf - 4, &write_len, 4);

        byte_array_buf_widx_advance(pbuf, write_len + 4);
    }
    else{
        printf("pack rtcp failed: seq(%d) chn(%d)\n", pdata->seq, pdata->chn);
    }
    byte_array_buf_write_ptr_unlock(pbuf, 0);
}

RTP_CLIENT_HANDLE* rtp_client_create(rtp_client_param_s* param)
{
    rtp_client_t* p = malloc(sizeof(rtp_client_t));
    if (!p){
        printf("create rtp client failed\n");
        return NULL;
    }
    memset(p, 0, sizeof(rtp_client_t));

    p->psession = param->psession;
    p->pmedia_info = param->pmedia_info;
    p->put_rtp_frm = param->put_rtp_frm;
    p->cb_pri_data = param->cb_pri_data;

    if (p->psession->mode == RTSP_TRANSPORT_MODE_UDP){
        int i;
        for(i = RTSP_VID_DATA_CHN; i < RTSP_SESSION_CHN_NUM; i++){
            byte_array_buf_init(&p->psession->transport[i].rx_buf, NULL, 16 * 1024, 0);
            byte_array_buf_init(&p->psession->transport[i].tx_buf, NULL, 16 * 1024, 0);
        }

        rtp_queue_param_s q_param = {
            .pool_size = VIDEO_POOL_SIZE,
            .queue_len = VIDEO_QUEUE_LEN,
            .begin_seq = param->psession->begin_seq,
            .output_rtp = __pop_rtp_from_queue,
            .send_nack = __send_nack_timer_cb,
            .pri_data = p,
            .chn = RTSP_VID_DATA_CHN,
            .strategy = RTP_QUEUE_ASAP_POP,
        };
        p->q_hdl[RTSP_MEDIA_TYPE_VIDEO] = rtp_queue_create(&q_param);

        q_param.pool_size = AUDIO_POOL_SIZE;
        q_param.queue_len = AUDIO_QUEUE_LEN;
        q_param.chn = RTSP_AUD_DATA_CHN;
        p->q_hdl[RTSP_MEDIA_TYPE_AUDIO] = rtp_queue_create(&q_param);
    }

    return __rtp_client_to_handle(p);
}

void rtp_client_destroy(RTP_CLIENT_HANDLE handle)
{
    rtp_client_t* p = __rtp_client_from_handle(handle);

    if (p->psession->mode == RTSP_TRANSPORT_MODE_UDP){
        rtp_queue_destroy(p->q_hdl[RTSP_MEDIA_TYPE_VIDEO]);
        rtp_queue_destroy(p->q_hdl[RTSP_MEDIA_TYPE_AUDIO]);

        int i;
        for(i = RTSP_VID_DATA_CHN; i < RTSP_SESSION_CHN_NUM; i++){
            byte_array_buf_destroy(&p->psession->transport[i].rx_buf);
            byte_array_buf_destroy(&p->psession->transport[i].tx_buf);
        }
    }
    free(p);
}

static int __put_rtp_frame(rtp_client_t* prc, rtp_frame_t* pfrm, RTSP_MEDIA_TYPE_E type)
{
    rtsp_data_packet_s pkt = {0};
    rtsp_media_info_t* pmedia_info = &prc->pmedia_info[type];

    rtp_packager_t* pkger = rtp_packager_get(pmedia_info->codec);
    if (!pkger)
    {
        printf("pkger for codec(%d) not found\n", pmedia_info->codec);
        return -1;
    }
    pkger->unpack(&pkt, pfrm);

    pkt.codec = pmedia_info->codec;

    if (prc->last_pts[type] == 0){
        uint64_t now_ms = get_monotonic_time_ms();

        prc->last_pts[type] = now_ms;
        prc->last_rtp_ts[type] = pfrm->hdr.timestamp;

        pkt.timestamp = now_ms;
    }
    else {
        uint32_t rtp_ts_diff = pfrm->hdr.timestamp - prc->last_rtp_ts[type];

        pkt.timestamp = prc->last_pts[type] + rtp_ts_diff * 1000 / pmedia_info->sample_rate;

        prc->last_pts[type] = pkt.timestamp;
        prc->last_rtp_ts[type] = pfrm->hdr.timestamp;
    }

    prc->put_rtp_frm(prc->cb_pri_data, &pkt);

    return 0;
}

int rtp_client_handle_packet(RTP_CLIENT_HANDLE handle, RTSP_SESSION_CHN_E chn, char* rtp_buf, int buf_len)
{
    rtp_client_t* p = __rtp_client_from_handle(handle);
    rtp_frame_t rtp_frm = {0};
    int ret = 0;

    switch (chn){

    case RTSP_VID_DATA_CHN:
        ret = rtp_frame_parse(&rtp_frm, rtp_buf, buf_len);
        if (ret < 0) {
            printf("parse video rtp failed\n");
            return -1;
        }

        __put_rtp_frame(p, &rtp_frm, RTSP_MEDIA_TYPE_VIDEO);
        break;

    case RTSP_VID_CTRL_CHN:
        break;

    case RTSP_AUD_DATA_CHN:
        ret = rtp_frame_parse(&rtp_frm, rtp_buf, buf_len);
        if (ret < 0) {
            printf("parse audio rtp failed\n");
            return -1;
        }

        __put_rtp_frame(p, &rtp_frm, RTSP_MEDIA_TYPE_AUDIO);
        break;

    case RTSP_AUD_CTRL_CHN:
        break;

    default:
        break;
    }

    return 0;
}

int rtp_client_recv_data(RTP_CLIENT_HANDLE handle, RTSP_SESSION_CHN_E chn)
{
    int ret = 0;
    rtp_client_t* prc = __rtp_client_from_handle(handle);

    byte_array_buf_t* prx_buf = &prc->psession->transport[chn].rx_buf;
    char* buf = NULL;
    int len = byte_array_buf_write_ptr_lock(prx_buf, &buf);
    if (len > 0){
        int fd = prc->psession->transport[chn].fd;
        char *ip = prc->psession->server_ip;
        uint16_t port = prc->psession->transport[chn].svr_port;

        while(1){
            ret = udp_recvfrom(fd, ip, port, buf, len);
            if (ret > 0){
                //每次recvfrom只收取一个udp包，对应一个rtp包，并且会即时处理，
                //不会在rx_buf中缓存，因此不需要处理rx_buf的索引
                switch (chn){

                case RTSP_VID_DATA_CHN:
                    rtp_queue_push(prc->q_hdl[RTSP_MEDIA_TYPE_VIDEO], buf, ret);
                    break;

                case RTSP_AUD_DATA_CHN:
                    rtp_queue_push(prc->q_hdl[RTSP_MEDIA_TYPE_AUDIO], buf, ret);
                    break;
                
                case RTSP_VID_CTRL_CHN:
                case RTSP_AUD_CTRL_CHN:
                    break;
                
                default:
                    break;
                }
            }
            else{
                break;
            }
        }
    }
    byte_array_buf_write_ptr_unlock(prx_buf, 0);

    return ret;
}

//udp模式下发送tx_buf中的数据
//| 4B len | RTP | 4B len | RTP |...
int rtp_client_send_data(RTP_CLIENT_HANDLE handle, RTSP_SESSION_CHN_E chn)
{
    int ret = 0;
    rtp_client_t* prc = __rtp_client_from_handle(handle);

    char* buf = NULL;
    byte_array_buf_t* ptx_buf = &prc->psession->transport[chn].tx_buf;

    int len = byte_array_buf_read_ptr_lock(ptx_buf, &buf);
    if (len > 4) {
        int pkt_len = 0;
        memcpy(&pkt_len, buf, 4);

        if (len >= 4 + pkt_len){

            int fd = prc->psession->transport[chn].fd;
            char* ip = prc->psession->server_ip;
            uint16_t port = prc->psession->transport[chn].svr_port;

            ret = udp_sendto(fd, ip, port, buf + 4, pkt_len);
            if (ret > 0) {
                byte_array_buf_ridx_advance(ptx_buf, 4 + pkt_len);
                byte_array_buf_trim(ptx_buf);
            }
        }
    }
    byte_array_buf_read_ptr_unlock(ptx_buf, 0);

    return ret;
}