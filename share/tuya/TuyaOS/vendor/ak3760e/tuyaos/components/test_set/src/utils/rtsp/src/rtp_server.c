#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include "rtp_server.h"
#include "rtp.h"
#include "sock_util.h"
#include "rtp_queue.h"


typedef struct {
    rtsp_session_t*       psession;
    rtsp_stream_src_t*    pstream_src;
    activate_net_cb       activate_net;
    int                   user_id;
    pthread_t             thread_id;
    int                   thread_exit;
    uint16_t              seq[RTSP_MEDIA_TYPE_NUM];
    uint32_t              rtp_ts[RTSP_MEDIA_TYPE_NUM];
    uint64_t              last_pts[RTSP_MEDIA_TYPE_NUM];
    uint32_t              ssrc;
    RTP_QUEUE_HANDLE      q_hdl[RTSP_MEDIA_TYPE_NUM];
} rtp_server_t;

RTP_SERVER_HANDLE __rtp_server_to_handle(rtp_server_t* prs)
{
    return (RTP_SERVER_HANDLE)prs;
}

rtp_server_t* __rtp_server_from_handle(RTP_SERVER_HANDLE handle)
{
    if (RTP_SERVER_INVALID_HANDLE == handle){
        return NULL;
    }

    return (rtp_server_t*)handle;
}

RTP_SERVER_HANDLE rtp_server_create(rtp_server_param_t* param)
{
    rtp_server_t* psvr = malloc(sizeof(rtp_server_t));

    if(psvr){
        memset(psvr, 0, sizeof(rtp_server_t));
        psvr->psession = param->psession;
        psvr->pstream_src = param->pstream_src;
        psvr->activate_net = param->activate_net;
        psvr->ssrc = rand();
    }
    else {
        return RTP_SERVER_INVALID_HANDLE;
    }

    return __rtp_server_to_handle(psvr);
}

void rtp_server_destroy(RTP_SERVER_HANDLE handle)
{
    rtp_server_t* psvr = __rtp_server_from_handle(handle);
    if (NULL == psvr){
        return;
    }

    if (psvr->thread_id){
        rtp_server_stop(psvr);
    }

    free(psvr);
}

/**
 * 从缓存中取出对应的rtp包，并重新写入发送缓冲
 */
static int __resend_rtp_packet(rtp_server_t* psvr, RTSP_MEDIA_TYPE_E type, uint16_t seq)
{
    int ret = 0;

    char* buf = NULL;
    RTSP_SESSION_CHN_E chn = RTSP_VID_DATA_CHN;
    if (type == RTSP_MEDIA_TYPE_AUDIO){
        chn = RTSP_AUD_DATA_CHN;
    }
    byte_array_buf_t* pbuf = &psvr->psession->transport[chn].tx_buf;
    int len = byte_array_buf_write_ptr_lock(pbuf, &buf);
    if (len > 0){
        ret = rtp_queue_get(psvr->q_hdl[type], seq, buf + 4, &len);
        if (ret == 0){
            memcpy(buf, &len, 4);
            byte_array_buf_widx_advance(pbuf, 4 + len);
        }
        else{
            printf("get rtp(%u) failed\n", seq);
        }
    }
    byte_array_buf_write_ptr_unlock(pbuf, 0);

    return 0;
}


static int __handle_rtcp_app(rtp_server_t* psvr, RTSP_SESSION_CHN_E chn, rtcp_header_t* phdr, char* buf)
{
    RTSP_MEDIA_TYPE_E media_type = RTSP_MEDIA_TYPE_VIDEO;
    if (chn == RTSP_AUD_CTRL_CHN){
        media_type = RTSP_MEDIA_TYPE_AUDIO;
    }

    switch (phdr->subtype){
    case RTCP_APP_SUBTYPE_NACK:
        if (phdr->length >= 3){
            uint16_t seq = ntohl(*(uint32_t*)(buf + 8)); //skip ssrc and name
            printf("resend rtp(%u)\n", seq);
            __resend_rtp_packet(psvr, media_type, seq);
        }
        break;
    default:
        break;
    }

    return 0;
}

int rtp_server_parse_data(rtp_server_t* psvr, RTSP_SESSION_CHN_E chn, char* buf, int len)
{
    int ret = 0;

    if (chn == RTSP_VID_CTRL_CHN || chn == RTSP_AUD_CTRL_CHN){
        rtcp_header_t hdr = {0};
        ret = rtcp_unpack_header(&hdr, buf, len);
        if (ret > 0){
            printf("recv RTCP(%d)\n", hdr.payload_type);
            switch (hdr.payload_type){
            case RTCP_TYPE_APP:
                __handle_rtcp_app(psvr, chn, &hdr, buf + ret);
                break;
            default:
                break;
            }
        }
    }
    else{
        printf("recv unhandled rtp in chn(%d)\n", chn);
    }

    return ret;
}

//udp模式下收数据到rx_buf中
int rtp_server_recv_data(RTP_SERVER_HANDLE handle, RTSP_SESSION_CHN_E chn)
{
    int ret = 0;
    rtp_server_t* psvr = __rtp_server_from_handle(handle);

    byte_array_buf_t* prx_buf = &psvr->psession->transport[chn].rx_buf;
    char* buf = NULL;
    int len = byte_array_buf_write_ptr_lock(prx_buf, &buf);
    if (len > 0){
        int fd = psvr->psession->transport[chn].fd;
        char *ip = psvr->psession->client_ip;
        uint16_t port = psvr->psession->transport[chn].cli_port;

        ret = udp_recvfrom(fd, ip, port, buf, len);
        if (ret > 0){
            //udp模式下收到的包是原子的，不需要处理rx_buf中索引
            rtp_server_parse_data(psvr, chn, buf, ret);
        }
    }
    byte_array_buf_write_ptr_unlock(prx_buf, 0);

    return ret;
}

//udp模式下发送tx_buf中的数据
//| 4B len | RTP | 4B len | RTP |...
int rtp_server_send_data(RTP_SERVER_HANDLE handle, RTSP_SESSION_CHN_E chn)
{
    int ret = 0;
    char* buf = NULL;
    rtp_server_t* psvr = __rtp_server_from_handle(handle);
    byte_array_buf_t* ptx_buf = &psvr->psession->transport[chn].tx_buf;

    int len = byte_array_buf_read_ptr_lock(ptx_buf, &buf);
    while (len > 4) {
        int pkt_len = 0;
        memcpy(&pkt_len, buf, 4);

        if (len >= 4 + pkt_len){

            int fd = psvr->psession->transport[chn].fd;
            char* ip = psvr->psession->client_ip;
            uint16_t port = psvr->psession->transport[chn].cli_port;

            ret = udp_sendto(fd, ip, port, buf + 4, pkt_len);
            if (ret > 0) {
                byte_array_buf_ridx_advance(ptx_buf, 4 + pkt_len);
                buf += 4 + pkt_len;
                len -= 4 + pkt_len;
                continue;
            }
        }
        break;
    }
    byte_array_buf_trim(ptx_buf);
    byte_array_buf_read_ptr_unlock(ptx_buf, 0);

    return ret;
}

typedef struct {
    rtp_server_t* psvr;
    RTSP_SESSION_CHN_E chn;
} rtp_server_arg_t;

int rtp_server_send_to_buf(char* data, int len, void* parg)
{
    int ret = 0;
    byte_array_buf_t* ptx_buf;
    rtp_server_arg_t* arg = (rtp_server_arg_t*)parg;
    rtp_server_t* psvr = arg->psvr;
    RTSP_SESSION_CHN_E rtsp_chn = arg->chn;

    if (psvr->psession->mode == RTSP_TRANSPORT_MODE_TCP){
        ptx_buf = psvr->psession->ptx_buf;
    }
    else {
        ptx_buf = &psvr->psession->transport[rtsp_chn].tx_buf;
    }

    char* buf = NULL;
    int buf_len = 0;
    //udp模式下先写入4个字节的长度，再写入rtp包，每次发送一个完整的rtp包
    //tcp模式下先写入4个字节的interleave
    //预留10个字节
    while(!psvr->thread_exit && buf_len < len + 4){
        if (buf){
            byte_array_buf_write_ptr_unlock(ptx_buf, 0);
            psvr->activate_net();
            usleep(1000);
        }
        buf_len = byte_array_buf_write_ptr_lock(ptx_buf, &buf);
    }

    if (psvr->thread_exit){
        if (buf){
            byte_array_buf_write_ptr_unlock(ptx_buf, 0);
        }
        return -1;
    }

    if (psvr->psession->mode == RTSP_TRANSPORT_MODE_TCP){
        byte_array_buf_write_byte(ptx_buf, '$');
        byte_array_buf_write_byte(ptx_buf, psvr->psession->transport[rtsp_chn].interleave);
        byte_array_buf_write_uint16_be(ptx_buf, len);
    }
    else{
        byte_array_buf_write_uint32(ptx_buf, len);
    }

    byte_array_buf_write(ptx_buf, data, len);

    //如果是UDP模式，则把已发送的rtp包缓存起来用于重传
    if (psvr->psession->mode == RTSP_TRANSPORT_MODE_UDP){
        if (rtsp_chn == RTSP_VID_DATA_CHN || rtsp_chn == RTSP_AUD_DATA_CHN){
            RTSP_MEDIA_TYPE_E media_type = (rtsp_chn == RTSP_VID_DATA_CHN ? RTSP_MEDIA_TYPE_VIDEO : RTSP_MEDIA_TYPE_AUDIO);

            rtp_queue_push(psvr->q_hdl[media_type], data, len);
        }
    }

    byte_array_buf_write_ptr_unlock(ptx_buf, 0);
    psvr->activate_net();

    return ret;
}

void* rtp_server_proc(void* arg)
{
    rtp_server_t* psvr = (rtp_server_t*)arg;
    char* frm_buf = NULL;
    int frm_len;
    uint64_t pts;
    int ret;

    while(!psvr->thread_exit) {
        int i;
        for(i = 0; i < RTSP_MEDIA_TYPE_NUM; i++){
            //printf("track_flag: %x\n", psvr->psession->track_flag);
            if (psvr->psession->track_flag & (1 << i)){
                //取帧
                ret = psvr->pstream_src->get_frame(psvr->user_id, i, &frm_buf, &frm_len, &pts);
                if (ret != 0){
                    continue;
                }
                //printf("get frame: len(%d)\n", frm_len);
                //更新时间戳
                if (psvr->last_pts[i] == 0){
                    psvr->last_pts[i] = pts;
                }
                else{
                    uint64_t diff = pts - psvr->last_pts[i];
                    psvr->last_pts[i] = pts;
                    int sample_rate = psvr->pstream_src->get_sample_rate(i);
                    psvr->rtp_ts[i] += sample_rate * diff / 1000;
                }
                //封包,发送
                RTP_CODEC_E codec = psvr->pstream_src->get_codec(i);
                rtp_server_arg_t arg = {0};
                arg.psvr = psvr;
                arg.chn = (i == RTSP_MEDIA_TYPE_VIDEO ? RTSP_VID_DATA_CHN : RTSP_AUD_DATA_CHN);

                int payload_type = 0;

                switch(codec){
                case RTP_CODEC_G711A:
                    payload_type = 8;
                    break;
                case RTP_CODEC_G711U:
                    payload_type = 0;
                    break;
                case RTP_CODEC_PCM:
                    payload_type = 98;
                    break;
                case RTP_CODEC_H264:
                case RTP_CODEC_H265:
                    payload_type = 96;
                    break;
                case RTP_CODEC_JPEG:
                    payload_type = 26;
                    break;
                default:
                    printf(" codec not supported yet\n");
                    break;
                }
               rtp_param_t rtp_param = {
                .pseq = psvr->seq + i,
                .codec = codec,
                .rtp_ts = psvr->rtp_ts[i],
                .ssrc =  psvr->ssrc,
                .payload = payload_type,
                 };
                rtp_pack_nal(rtp_param, frm_buf, frm_len,rtp_server_send_to_buf, &arg);
            }
        }

        usleep(1000);
    }

    return NULL;
}

int rtp_server_start(RTP_SERVER_HANDLE handle, uint16_t seq, uint32_t ts)
{
    int ret = 0;
    rtp_server_t* psvr = __rtp_server_from_handle(handle);

    psvr->user_id = psvr->pstream_src->start();
    int i;
    for(i = 0; i < RTSP_MEDIA_TYPE_NUM; i++){
        psvr->seq[i] = seq;
        psvr->rtp_ts[i] = ts;
    }

    if (psvr->psession->mode == RTSP_TRANSPORT_MODE_UDP){
        rtp_queue_param_s q_param = {
            .pool_size = 512 * 1024,
            .queue_len = 512,
            .begin_seq = seq,
            .output_rtp = NULL,
            .pri_data = NULL,
            .chn = RTSP_VID_DATA_CHN,
            .strategy = RTP_QUEUE_LAZY_POP,
        };
        psvr->q_hdl[RTSP_MEDIA_TYPE_VIDEO] = rtp_queue_create(&q_param);

        q_param.pool_size = 32 * 1024;
        q_param.queue_len = 32;
        q_param.chn = RTSP_AUD_DATA_CHN;
        psvr->q_hdl[RTSP_MEDIA_TYPE_AUDIO] = rtp_queue_create(&q_param);
    }

    pthread_create(&psvr->thread_id, NULL, rtp_server_proc, psvr);

    return ret;
}

int rtp_server_stop(RTP_SERVER_HANDLE handle)
{
    int ret = 0;
    rtp_server_t* psvr = __rtp_server_from_handle(handle);

    if (psvr->thread_id == 0){
        printf("rtp server not run\n");
        return -1;
    }

    psvr->thread_exit = 1;

    pthread_join(psvr->thread_id, NULL);
    psvr->pstream_src->stop(psvr->user_id);
    psvr->thread_id = 0;

    if (psvr->psession->mode == RTSP_TRANSPORT_MODE_UDP){
        rtp_queue_destroy(psvr->q_hdl[RTSP_MEDIA_TYPE_VIDEO]);
        rtp_queue_destroy(psvr->q_hdl[RTSP_MEDIA_TYPE_AUDIO]);
    }

    return ret;
}