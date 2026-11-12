#ifndef __RTP_PACKAGER_H__
#define __RTP_PACKAGER_H__

#include <stdint.h>
#include <stdio.h>
#include "rtp.h"

typedef struct {
    int (*pack)(rtp_param_t *rtp_param, const char* frm_buf, int frm_len,rtp_pkt_handler handler, void* arg);
    int (*unpack)(rtsp_data_packet_s* pkt, rtp_frame_t* pfrm);
    int (*assemble_rtp_packet)(rtp_param_t* rtp_param, uint32_t marker,
            const char* pkt_buf, int pkt_len, char* fu_buf, int fu_len,
            rtp_pkt_handler handler, void* arg);
} rtp_packager_t;

rtp_packager_t* rtp_packager_get(RTP_CODEC_E codec);


#endif