#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "rtp.h"
#include "rtp_packager.h"


//返回写入buf的字节数
int rtp_pack_header(rtp_header_t* phdr, char* buf, int len)
{
    int hdr_len = phdr->csrc_cnt * 4 + RTP_HDR_LEN;

    if (hdr_len > len){
        return 0;
    }

    phdr->seq = htons(phdr->seq);
    phdr->timestamp = htonl(phdr->timestamp);
    phdr->ssrc = htonl(phdr->ssrc);

    int i;
    for(i = 0; i < phdr->csrc_cnt; i++){
        phdr->csrc_list[i] = htonl(phdr->csrc_list[i]);
    }

    memcpy(buf, phdr, hdr_len);

    return hdr_len;
}

//返回从buf读取的字节数
int rtp_unpack_header(rtp_header_t* phdr, char* buf, int len)
{
    if (len < RTP_HDR_LEN){
        return 0;
    }

    int hdr_len = RTP_HDR_LEN;
    memcpy(phdr, buf, hdr_len);

    hdr_len += phdr->csrc_cnt * 4;
    if (len < hdr_len){
        return 0;
    }
    memcpy(phdr, buf, hdr_len);

    phdr->seq = ntohs(phdr->seq);
    phdr->timestamp = ntohl(phdr->timestamp);
    phdr->ssrc = ntohl(phdr->ssrc);

    int i;
    for(i = 0; i < phdr->csrc_cnt; i++){
        phdr->csrc_list[i] = ntohl(phdr->csrc_list[i]);
    }

    return hdr_len;
}

//返回写入buf的字节数
int rtcp_pack_header(rtcp_header_t* phdr, char* buf, int len)
{
    int hdr_len = sizeof(rtcp_header_t);
    if (len < hdr_len){
        return 0;
    }

    phdr->length = htons(phdr->length);
    memcpy(buf, phdr, hdr_len);

    return hdr_len;
}

//返回从buf读取的字节数
int rtcp_unpack_header(rtcp_header_t* phdr, char* buf, int len)
{
    int hdr_len = sizeof(rtcp_header_t);

    if (len < hdr_len){
        return 0;
    }

    memcpy(phdr, buf, hdr_len);

    phdr->length = ntohs(phdr->length);

    if (len < 4 * (phdr->length + 1)){
        return 0;
    }

    return hdr_len;
}

int rtp_pack_nal(rtp_param_t rtp_param, char* frm_buf, int frm_len,rtp_pkt_handler handler, void* arg)
{
    int ret = 0;
    if (NULL == handler){
        printf("handler is NULL\n");
        return -1;
    }

    rtp_packager_t* pkger = rtp_packager_get(rtp_param.codec);
    if (!pkger)
    {
        printf("packager for codec(%d) not implemented\n", rtp_param.codec);
        return -1;
    }
    ret = pkger->pack(&rtp_param, frm_buf, frm_len, handler, arg);

    return ret;
}

int rtp_frame_parse(rtp_frame_t *pfrm, char* rtp_buf, int buf_len)
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

