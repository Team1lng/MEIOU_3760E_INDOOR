#include <stdint.h>

#include "rtp.h"
#include "rtp_packager.h"

extern rtp_packager_t rtp_h265_packager;

/**
 * hevc nal unit header
 * +---------------+---------------+
 * |0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |F|      Type | LayerId   | TID |
 * +-------------+-----------------+
**/
static int rtp_h265_do_pack(rtp_param_t *rtp_param, const char* frm_buf, int frm_len,rtp_pkt_handler handler, void* arg)
{
    int ret = 0;

    if (frm_len <= MAX_PAYLOAD_SIZE){

	ret = rtp_h265_packager.assemble_rtp_packet(rtp_param, 1, frm_buf, frm_len, NULL, 0, handler, arg);
	if(ret != 0) {
		return ret;
	}


        //更新序列号
        (*rtp_param->pseq)++;
    }
    else {
        int i;
        for(i = 2; i < frm_len; i += MAX_PAYLOAD_SIZE) { //前两个字节为nalu header
            int mark, pkt_len, fu_header = 0;

            if (i == 2){ //first
                mark = 0;
                pkt_len = MAX_PAYLOAD_SIZE;
                fu_header = (1 << 7) | ((frm_buf[0] >> 1) & 0x3f);
            }
            else if (i >= frm_len - MAX_PAYLOAD_SIZE) { //end
                mark = 1;
                pkt_len = (frm_len - 2) % MAX_PAYLOAD_SIZE;
                fu_header = (1 << 6) | ((frm_buf[0] >> 1) & 0x3f);
            }
            else { //middle
                mark = 0;
                pkt_len = MAX_PAYLOAD_SIZE;
                fu_header = (frm_buf[0] >> 1) & 0x3f;
            }

            char fu_buf[3] = {frm_buf[0], frm_buf[1], fu_header};
            fu_buf[0] = (fu_buf[0] & 0x81) | 0x62;  //set type to 49

	    ret = rtp_h265_packager.assemble_rtp_packet(rtp_param, mark, frm_buf+i, pkt_len, fu_buf, 3, handler, arg);
	    if(ret != 0) {
		    return ret;
	    }


            //更新序列号
            (*rtp_param->pseq)++;
        }
    }

    return ret;
}


static int rtp_h265_pack(rtp_param_t *rtp_param, const char* frm_buf, int frm_len,rtp_pkt_handler handler, void* arg)
{
    int ret = 0;
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
	    ret = rtp_h265_do_pack(rtp_param, frm_start, frm_end - frm_start, handler, arg);
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
        ret = rtp_h265_do_pack(rtp_param, frm_start, frm_buf + frm_len - frm_start, handler, arg);
    }

    return ret;
}


/**
 * hevc nal unit header
 * +---------------+---------------+
 * |0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |F|      Type | LayerId   | TID |
 * +-------------+-----------------+
**/
static int rtp_h265_unpack(rtsp_data_packet_s* pkt, rtp_frame_t* pfrm)
{
    int frm_start = 1;
    int frm_end = 1;
    char* payload = pfrm->payload;
    int pld_len = pfrm->pld_len;
    int offset = 0;

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

        if (frm_start)
        {
            offset = 1;
        }
        else
        {
            offset = 3;
        }
        
        payload = pfrm->payload + offset;
        pld_len -= offset;
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


rtp_packager_t rtp_h265_packager = {
    .pack = rtp_h265_pack,
    .unpack = rtp_h265_unpack,
};
