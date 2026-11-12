#include <stdint.h>

#include "rtp.h"
#include "rtp_packager.h"

extern rtp_packager_t rtp_g711_packager;


static int rtp_g711_pack(rtp_param_t *rtp_param, const char* frm_buf, int frm_len,rtp_pkt_handler handler, void* arg)
{
    int ret = 0;

    ret = rtp_g711_packager.assemble_rtp_packet(rtp_param, 1, frm_buf, frm_len, NULL, 0, handler, arg);
    if(ret != 0) {
	    return ret;
    }


    (*rtp_param->pseq)++;
    return ret;
}

static int rtp_g711_unpack(rtsp_data_packet_s* pkt, rtp_frame_t* pfrm)
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


rtp_packager_t rtp_g711_packager = {
    .pack = rtp_g711_pack,
    .unpack = rtp_g711_unpack,
};
