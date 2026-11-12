#include <arpa/inet.h>
#include <string.h>

#include "rtp.h"

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