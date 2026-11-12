#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <arpa/inet.h>

#include "rtp_client.h"
#include "rtp.h"
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
    rtsp_media_info_s*  pmedia_info;
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

typedef struct {
    rtp_header_t        hdr;
    char*               payload;
    int                 pld_len;     //payload_len
} rtp_frame_t;

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

static int __parse_rtp_frame(rtp_frame_t *pfrm, char* rtp_buf, int buf_len)
{
    rtp_header_t* phdr = &pfrm->hdr;
    int payload_len = buf_len;

    //从rtp_buf中解析出header, 返回header的长度
    int hdr_len = rtp_unpack_header(phdr, rtp_buf, buf_len);
    if (hdr_len <= 0){
        printf("parse rtp header failed\n");
        return -1;
    }

    payload_len -= hdr_len;

    if (phdr->version != 2){
        printf("rtp version error(%u)\n", phdr->version);
        return -1;
    }

    if (phdr->padding == 1){
        uint8_t padding_len = *(uint8_t*)(rtp_buf + buf_len - 1);
        payload_len -= padding_len;
    }

    if (phdr->extension == 1){
        //TODO
        printf("rtp extension not handled\n");
    }

    pfrm->payload = rtp_buf + hdr_len;
    pfrm->pld_len = payload_len;

    return 0;
}

/**
 *  fu indicator:
 *  +---------------+
 *  |0|1|2|3|4|5|6|7|
 *  +-+-+-+-+-+-+-+-+
 *  |F|NRI| Type    |
 *  +---------------+
 *  fu header:
 *  +---------------+
 *  |0|1|2|3|4|5|6|7|
 *  +-+-+-+-+-+-+-+-+
 *  |S|E|R| Type    |
 *  +---------------+
 **/
static int __put_rtp_frame_h264(rtsp_data_packet_s* pkt, rtp_frame_t* pfrm, RTSP_MEDIA_TYPE_E type)
{
    char* payload = pfrm->payload;
    int pld_len = pfrm->pld_len;
    int frm_start = 1;
    int frm_end = 1;

    int nal_type = pfrm->payload[0] & 0x1f;

    switch (nal_type){

    case 24:  //STAP-A
    case 25:  //STAP-B
    case 26:  //MTAP16
    case 27:  //MTAP24
        printf("nal type(%d) not implemented\n", nal_type);
        break;
    case 28:
    case 29:  //fragmentation units
        frm_start = pfrm->payload[1] & 0x80;
        frm_end = pfrm->payload[1] & 0x40;
        nal_type = pfrm->payload[1] & 0x1f;

        pfrm->payload[1] = (pfrm->payload[0] & 0xE0) | (pfrm->payload[1] & 0x1F);
        payload = pfrm->payload + 1;
        pld_len -= 1;
        break;
    default:
        break;
    }

    switch (nal_type){

    case 1:  //p frame
        pkt->type = E_RTSP_VIDEO_PB_FRAME;
        break;
    case 5:  //i frame
        pkt->type = E_RTSP_VIDEO_I_FRAME;
        break;
    case 7:  //sps
        pkt->type = E_RTSP_VIDEO_SPS_FRAME;
        break;
    case 8: //pps
        pkt->type = E_RTSP_VIDEO_PPS_FRAME;
        break;
    default:
        printf("nal_type(%d) not implemented\n", nal_type);
    }

    pkt->buf = payload;
    pkt->frame_end = frm_end ? 1 : 0;
    pkt->frame_start = frm_start ? 1 : 0;
    pkt->size = pld_len;

    return 0;
}

/**
 * hevc nal unit header
 * +---------------+---------------+
 * |0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |F|      Type | LayerId   | TID |
 * +-------------+-----------------+
**/
static int __put_rtp_frame_h265(rtsp_data_packet_s* pkt, rtp_frame_t* pfrm, RTSP_MEDIA_TYPE_E type)
{
    int frm_start = 1;
    int frm_end = 1;
    char* payload = pfrm->payload;
    int pld_len = pfrm->pld_len;

    int nal_type = (pfrm->payload[0] >> 1) & 0x3f;

    switch (nal_type){

    case 48: //aggregation packets
        printf("nal type(%d) not implemented\n", nal_type);
        break;
    
    case 49: //fragmentation units
        frm_start = pfrm->payload[2] & 0x80;
        frm_end = pfrm->payload[2] & 0x40;
        nal_type = pfrm->payload[2] & 0x3f;

        pfrm->payload[0] = (pfrm->payload[0] & 0x81) | ((pfrm->payload[2] & 0x3f) << 1);
        pfrm->payload[2] = pfrm->payload[1];
        pfrm->payload[1] = pfrm->payload[0];

        payload = pfrm->payload + 1;
        pld_len -= 1;
        break;

    default:
        break;
    }

    switch (nal_type){

    case 32: //vps
        pkt->type = E_RTSP_VIDEO_VPS_FRAME;
        break;
    case 33: //sps
        pkt->type = E_RTSP_VIDEO_SPS_FRAME;
        break;
    case 34:
        pkt->type = E_RTSP_VIDEO_PPS_FRAME;
        break;
    case 39:
        pkt->type = E_RTSP_VIDEO_SEI_FRAME;
        break;
    case 19:
        pkt->type = E_RTSP_VIDEO_I_FRAME;
        break;
    default:
        pkt->type = E_RTSP_VIDEO_PB_FRAME;
        break;
    }

    pkt->buf = payload;
    pkt->frame_end = frm_end ? 1 : 0;
    pkt->frame_start = frm_start ? 1 : 0;
    pkt->size = pld_len;

    return 0;
}

static int __put_rtp_frame_audio(rtsp_data_packet_s* pkt, rtp_frame_t* pfrm, RTSP_MEDIA_TYPE_E type)
{
    int frm_start = 1;
    int frm_end = 1;
    char* payload = pfrm->payload;
    int pld_len = pfrm->pld_len;

    pkt->type = E_RTSP_AUDIO_FRAME;
    pkt->buf = payload;
    pkt->frame_end = frm_end ? 1 : 0;
    pkt->frame_start = frm_start ? 1 : 0;
    pkt->size = pld_len;
    return 0;
}

static int __put_rtp_frame(rtp_client_t* prc, rtp_frame_t* pfrm, RTSP_MEDIA_TYPE_E type)
{
    rtsp_data_packet_s pkt = {0};
    rtsp_media_info_s* pmedia_info = &prc->pmedia_info[type];

    switch (pmedia_info->codec){

    case RTSP_CODEC_H264:
        __put_rtp_frame_h264(&pkt, pfrm, type);
        break;

    case RTSP_CODEC_H265:
        __put_rtp_frame_h265(&pkt, pfrm, type);
        break;

    case RTSP_CODEC_G711A:
    case RTSP_CODEC_G711U:
    case RTSP_CODEC_PCM:
        __put_rtp_frame_audio(&pkt, pfrm, type);
        break;

    default:
        printf("unknown codec(%d)\n", pmedia_info->codec);
        break;
    }

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
    rtp_frame_t rtp_frm = {{0}};
    int ret = 0;

    switch (chn){

    case RTSP_VID_DATA_CHN:
        ret = __parse_rtp_frame(&rtp_frm, rtp_buf, buf_len);
        if (ret < 0) {
            printf("parse video rtp failed\n");
            return -1;
        }

        __put_rtp_frame(p, &rtp_frm, RTSP_MEDIA_TYPE_VIDEO);
        break;

    case RTSP_VID_CTRL_CHN:
        break;

    case RTSP_AUD_DATA_CHN:
        ret = __parse_rtp_frame(&rtp_frm, rtp_buf, buf_len);
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