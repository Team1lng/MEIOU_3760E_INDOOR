#include <stdint.h>

#include "rtp.h"
#include "rtp_packager.h"

extern rtp_packager_t rtp_ps_packager;

static int rtp_ps_pack(rtp_param_t *rtp_param, const char* frm_buf, int frm_len,rtp_pkt_handler handler, void* arg)
{
	int ret = 0;
	int send_size = MAX_PAYLOAD_SIZE;
	int sent_size = 0;
	do
	{
		if (frm_len > 0 && frm_len <= send_size){
			ret = rtp_ps_packager.assemble_rtp_packet(rtp_param, 1, frm_buf+sent_size, frm_len, NULL, 0, handler, arg);
			if(ret != 0) {
				return ret;
			}

                       //更新序列号
             	        (*rtp_param->pseq)++;
			break;

		}

	 ret = rtp_ps_packager.assemble_rtp_packet(rtp_param, 0, frm_buf+sent_size, send_size, NULL, 0, handler, arg);
	 if(ret != 0) {
		 return ret;
	 }

        //更新序列号
         (*rtp_param->pseq)++;
		frm_len -= send_size;
		sent_size += send_size;

	} while (frm_len > 0);

	return ret;
}

rtp_packager_t rtp_ps_packager = {
    .pack = rtp_ps_pack,
};
