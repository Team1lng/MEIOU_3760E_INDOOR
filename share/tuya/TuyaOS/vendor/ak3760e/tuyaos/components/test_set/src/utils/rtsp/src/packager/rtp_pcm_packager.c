#include <stdint.h>
#include <string.h>

#include "rtp.h"
#include "rtp_packager.h"

extern rtp_packager_t rtp_pcm_packager;

#if 0
void __convert_pcm_endian(const char* frm_buf, int frm_len, char* out_buf)
{
    int i;
    for(i = 0; i < frm_len; i += 2)
    {
        out_buf[i] = frm_buf[i+1];
        out_buf[i+1] = frm_buf[i];
    }
}
#endif

static int rtp_pcm_pack(rtp_param_t *rtp_param, const char* frm_buf, int frm_len,rtp_pkt_handler handler, void* arg)
{
    int ret = 0;

    //char tmp_buf[frm_len];
    //__convert_pcm_endian(frm_buf, frm_len, tmp_buf);

    ret = rtp_pcm_packager.assemble_rtp_packet(rtp_param, 1, frm_buf, frm_len, NULL, 0, handler, arg);
    if(ret != 0) {
	    return ret;
    }


    (*rtp_param->pseq)++;
    return ret;
}

static int rtp_pcm_unpack(rtsp_data_packet_s* pkt, rtp_frame_t* pfrm)
{
    int frm_start = 1;
    int frm_end = 1;
    char* payload = pfrm->payload;
    int pld_len = pfrm->pld_len;

    //char tmp_buf[pld_len];
    //memcpy(tmp_buf, payload, pld_len);
    //__convert_pcm_endian(tmp_buf, pld_len, payload);

    pkt->type = E_RTSP_AUDIO_FRAME;
    pkt->buf = payload;
    pkt->frame_end = frm_end ? 1 : 0;
    pkt->frame_start = frm_start ? 1 : 0;
    pkt->size = pld_len;
    return 0;
}

rtp_packager_t rtp_pcm_packager = {
    .pack = rtp_pcm_pack,
    .unpack = rtp_pcm_unpack,
};
