#ifndef __RTP_H__
#define __RTP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define RTP_HDR_LEN            (12)
#define MAX_PAYLOAD_SIZE       (1300)
#define H264_PAY_LOAD          (96)
#define H265_PAY_LOAD          (95)
#define RTP_PCMU_PAYLOAD       (0)
#define RTP_PCMA_PAYLOAD       (8)
#define RTP_PCM_PAYLOAD        (99)


typedef enum {
    RTCP_TYPE_SR = 200,
    RTCP_TYPE_RR = 201,
    RTCP_TYPE_SDES = 202,
    RTCP_TYPE_BYE = 203,
    RTCP_TYPE_APP = 204,
} RTCP_TYPE_E;

typedef enum {
    RTP_CODEC_INVALID = 0,
    RTP_CODEC_H264,
    RTP_CODEC_H265,
    RTP_CODEC_JPEG,
    RTP_CODEC_PCM,
    RTP_CODEC_PS,
    RTP_CODEC_G711A,
    RTP_CODEC_G711U,
    RTP_CODEC_MAX,
} RTP_CODEC_E;


typedef enum
{
    E_RTSP_VIDEO_PB_FRAME = 0,
    E_RTSP_VIDEO_I_FRAME,
    E_RTSP_VIDEO_SPS_FRAME,
    E_RTSP_VIDEO_PPS_FRAME,
    E_RTSP_VIDEO_VPS_FRAME,
    E_RTSP_VIDEO_SEI_FRAME,
    E_RTSP_AUDIO_FRAME,
    E_RTSP_MEDIA_FRAME_TYPE_MAX
}RTSP_FRAME_TYPE_E;

typedef struct
{
    RTSP_FRAME_TYPE_E  type;
    RTP_CODEC_E        codec;
    int                frame_start;
    int                frame_end;
    char*              buf;
    uint32_t           size;
    uint64_t           timestamp;
}rtsp_data_packet_s;



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
    rtp_header_t        hdr;
    char*               payload;
    int                 pld_len;     //payload_len
} rtp_frame_t;

typedef struct {
    uint32_t subtype : 5;
    uint32_t padding:1;
    uint32_t version:2;

    uint32_t payload_type:8;
    uint32_t length:16;

} rtcp_header_t;

typedef struct {
    uint16_t*             pseq;
	RTP_CODEC_E           codec;
    uint32_t              rtp_ts;
    uint32_t              ssrc;
    uint32_t              payload;
} rtp_param_t;

int rtp_pack_header(rtp_header_t* phdr, char* buf, int len);
int rtp_unpack_header(rtp_header_t* phdr, char* buf, int len);
int rtcp_pack_header(rtcp_header_t* phdr, char* buf, int len);
int rtcp_unpack_header(rtcp_header_t* phdr, char* buf, int len);


typedef int (*rtp_pkt_handler)(char* buf, int len, void* arg);

/**
* @brief pack a video/audio frame to multiple rtp packets
* @param codec           codec of the input frame
* @param frm_buf         frame data
* @param frm_len         frame data length in bytes
* @param ssrc
* @param rtp_ts
* @param pseq
* @param plist           list to hold the rtp packets(rtp_pkt_list_alloc/rtp_pkt_list_free)
*/
int rtp_pack_nal(rtp_param_t rtp_param, char* frm_buf, int frm_len,rtp_pkt_handler handler, void* arg);
int rtp_frame_parse(rtp_frame_t *pfrm, char* rtp_buf, int buf_len);
#ifdef __cplusplus
} // extern "C"
#endif

#endif // __RTP_H__
