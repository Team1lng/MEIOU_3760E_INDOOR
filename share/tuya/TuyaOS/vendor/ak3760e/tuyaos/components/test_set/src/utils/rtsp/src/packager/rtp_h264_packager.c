#include <stdint.h>

#include "rtp.h"
#include "rtp_packager.h"

extern rtp_packager_t rtp_h264_packager;

static int rtp_h264_do_pack(rtp_param_t *rtp_param, const char* frm_buf, int frm_len,rtp_pkt_handler handler, void* arg)
{
    int ret = 0;

    if (frm_len <= MAX_PAYLOAD_SIZE){

         ret = rtp_h264_packager.assemble_rtp_packet(rtp_param, 1, frm_buf, frm_len, NULL, 0, handler, arg);
         if(ret != 0) {
             return ret;
	 }
			             
        //更新序列号
        (*rtp_param->pseq)++;
    }
    else {
        int i;
        for(i = 1; i < frm_len; i += MAX_PAYLOAD_SIZE) { //第一个字节为nalu header
            int mark, pkt_len, fu_header = 0;

            if (i == 1){ //first
                mark = 0;
                pkt_len = MAX_PAYLOAD_SIZE;
                fu_header = (1 << 7) | (frm_buf[0] & 0x1f);
            }
            else if (i >= frm_len - MAX_PAYLOAD_SIZE) { //end
                mark = 1;
                pkt_len = (frm_len - 1) % MAX_PAYLOAD_SIZE;
                fu_header = (1 << 6) | (frm_buf[0] & 0x1f);
            }
            else { //middle
                mark = 0;
                pkt_len = MAX_PAYLOAD_SIZE;
                fu_header = frm_buf[0] & 0x1f;
            }

            uint32_t fu_indicator = (frm_buf[0] & 0xe0) | 28;
            char fu_buf[2] = {fu_indicator, fu_header};

	    ret = rtp_h264_packager.assemble_rtp_packet(rtp_param, mark, frm_buf+i, pkt_len, fu_buf, 2, handler, arg);
	    if(ret != 0) {
		    return ret;
	    }


            //更新序列号
            (*rtp_param->pseq)++;
        }
    }

    return ret;
}


static int rtp_h264_pack(rtp_param_t *rtp_param, const char* frm_buf, int frm_len,rtp_pkt_handler handler, void* arg)
{
    int ret  = -1;
    const char *pos = frm_buf;
    const char *frm_start = NULL, *frm_end = NULL;
    while (pos < frm_buf + frm_len - 4)
    {

        if (pos[0] == 0 && pos[1] == 0 && pos[2] == 0 && pos[3] == 1)
        {
            if (frm_start == NULL)
            {
                frm_start = pos + 4;
                pos += 3; //后面会再++
            }
            else
            {
                frm_end = pos;
            }
        }
        else if (pos[0] == 0 && pos[1] == 0 && pos[2] == 1)
        {
            if (frm_start == NULL)
            {
                frm_start = pos + 3;
                pos += 2; //后面会再++
            }
            else
            {
                frm_end = pos;
            }
        }

        if (frm_start && frm_end)
        {
            ret = rtp_h264_do_pack(rtp_param, frm_start, frm_end - frm_start, handler, arg);
	    if(ret != 0) {
	        return ret;
	    }

            frm_start = frm_end = NULL;
        }
        else
        {
            pos++;
        }
    }

    if (frm_start)
    {
        ret = rtp_h264_do_pack(rtp_param, frm_start, frm_buf + frm_len - frm_start, handler, arg);
	
    }

    return ret;
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
static int rtp_h264_unpack(rtsp_data_packet_s* pkt, rtp_frame_t* pfrm)
{
    char* payload = pfrm->payload;
    int pld_len = pfrm->pld_len;
    int frm_start = 1;
    int frm_end = 1;
    int offset = 0;

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
        if (frm_start)
        {
            offset = 1;
        }
        else
        {
            offset = 2;
        }
        
        payload = pfrm->payload + offset;
        pld_len -= offset;
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


rtp_packager_t rtp_h264_packager = {
    .pack = rtp_h264_pack,
    .unpack = rtp_h264_unpack,
};
