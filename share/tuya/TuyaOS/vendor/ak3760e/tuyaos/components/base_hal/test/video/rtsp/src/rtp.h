#ifndef __RTP_H__
#define __RTP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define RTP_HDR_LEN 12

typedef enum {
    RTCP_TYPE_SR = 200,
    RTCP_TYPE_RR = 201,
    RTCP_TYPE_SDES = 202,
    RTCP_TYPE_BYE = 203,
    RTCP_TYPE_APP = 204,
} RTCP_TYPE_E;

typedef enum {
    RTCP_APP_SUBTYPE_NACK = 1,    //重传
} RTCP_APP_SUBTYPE_E;

typedef struct {
    uint32_t csrc_cnt:4;
    uint32_t extension:1;
    uint32_t padding:1;
    uint32_t version:2;

    uint32_t payload_type:7;
    uint32_t marker:1;

    uint16_t seq:16;  //initial value should be random
    uint32_t timestamp:32; //initial value should be random
    uint32_t ssrc:32; //should be random
    uint32_t csrc_list[15];
} rtp_header_t;

typedef struct {
    uint32_t subtype : 5;
    uint32_t padding:1;
    uint32_t version:2;

    uint32_t payload_type:8;
    uint32_t length:16;

} rtcp_header_t;

int rtp_pack_header(rtp_header_t* phdr, char* buf, int len);
int rtp_unpack_header(rtp_header_t* phdr, char* buf, int len);
int rtcp_pack_header(rtcp_header_t* phdr, char* buf, int len);
int rtcp_unpack_header(rtcp_header_t* phdr, char* buf, int len);
#ifdef __cplusplus
} // extern "C"
#endif

#endif // __RTP_H__
