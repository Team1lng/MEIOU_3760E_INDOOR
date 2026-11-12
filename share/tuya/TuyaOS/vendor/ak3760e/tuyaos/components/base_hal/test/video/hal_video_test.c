
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "rtsp.h"
#include "rtsp_server.h"
#include "hal_video.h"
#include "hal_audio.h"
#include "hal_video_test.h"

#define HAL_TEST_AUDIO
#define HAL_TEST_VIDEO

typedef struct {
	int active;
	CHANNEL_E vchn;
	CHANNEL_E achn;
} rtsp_user_info_s;

#define MAX_RTSP_USER 2
static rtsp_user_info_s users[MAX_RTSP_USER] = {{0}};

#define FRAME_RES_WIDTH    640
#define FRAME_RES_HEIGHT   360
#define MAX_FRAME_COUNT    3
#define MAX_FRAME_SIZE     (300*1024)
static char g_media_buf[MAX_FRAME_COUNT * MAX_FRAME_SIZE] = {0};

#define VENC_MAX_PAKETS    32

static int hal_audio_init(void)
{
/* AI */
	int ai_dev = 0;
	int ai_chn = 0;
	hal_aud_in_attr_t ai_attr;

	memset(&ai_attr, 0, sizeof(ai_attr));

	ai_attr.channels = 1;
	ai_attr.rate = 8000;
	ai_attr.format = HAL_PCM_FORMAT_S16_LE;
	ai_attr.period_count = 1;
	ai_attr.period_size = 5;
	ai_attr.volume = 15;

	if (ai_attr.rate == 16000) {
		ai_attr.points = 640;
	} else if (ai_attr.rate == 8000) {
		ai_attr.points = 320;
	} else {
		ai_attr.points = 320;
	}

	if (hal_aud_in_init(ai_dev, ai_chn, &ai_attr) != 0) {
		printf("failed to initialize audio input device\n");
		return -1;
	}

/* AO */
	int ao_dev = 0;
	int ao_chn = 0;
	hal_aud_out_attr_t ao_attr;

	memset(&ao_attr, 0, sizeof(ao_attr));

	ao_attr.channels = 1;
	ao_attr.rate = 8000;
	ao_attr.format = HAL_PCM_FORMAT_S16_LE;
	ao_attr.period_count = 1;
	ao_attr.period_size = 5;
	ao_attr.volume = 5;

	if (ao_attr.rate == 16000) {
		ao_attr.points = 640;
	} else if (ao_attr.rate == 8000) {
		ao_attr.points = 320;
	} else {
		ao_attr.points = 320;
	}

	if (hal_aud_out_init(ao_dev, ao_chn, &ao_attr) != 0) {
		printf("failed to initialize audio output device\n");
		return -1;
	}

	return 0;
}


static int hal_video_init(void)
{
	int ret;

/* sensor */

	int snr_id = 0;
	hal_video_snr_attr_t snr_attr;
	snr_attr.hdr_en = 0;
	ret = hal_video_snr_init(snr_id, &snr_attr);
	if (ret != 0) {
		printf("ERROR: hal_video_snr_init failed\n");
		return -1;
	}

	ret = hal_video_snr_enable(snr_id);
	if (ret != 0) {
		printf("ERROR: hal_video_snr_enable failed\n");
		return -1;
	}

	ret = hal_video_snr_set_fps(snr_id, 25);
	if (ret != 0) {
		printf("ERROR: hal_video_snr_set_fps failed\n");
		return -1;
	}

	printf("Sensor OK\n");

	/* set res */

	int snr_res_num;
	int i;
	hal_video_res_t snr_one_res;

	ret = hal_video_snr_get_res_num(snr_id, &snr_res_num);
	if (ret != 0) {
		printf("ERROR: hal_video_snr_get_res_num failed\n");
		return -1;
	}
	printf("snr res num: %d\n", snr_res_num);

	for (i = 0; i < snr_res_num; i++) {
		ret = hal_video_snr_get_one_res(snr_id, i, &snr_one_res);
		if (ret != 0) {
			printf("ERROR: hal_video_snr_get_one_res failed! idx=%d\n", i);
			return -1;
		}

		printf("Supported Res: %d x %d\n", snr_one_res.width, snr_one_res.height);
		if ((snr_one_res.width == FRAME_RES_WIDTH)
			&& (snr_one_res.height == FRAME_RES_HEIGHT)) {
			/* set target res */
			ret = hal_video_snr_set_res(snr_id, i);
			if (ret != 0) {
				printf("ERROR: hal_video_snr_get_one_res failed! idx=%d\n", i);
				return -1;
			}
			printf("res changed to: %d x %d\n",
				snr_one_res.width, snr_one_res.height);
		}
	}

/* VIF */

	/* init vif */

	int vif_dev = 0;
	int vif_chn = 0;
	int vif_port = 0;

	int res_idx = 0;
	hal_video_res_t snr_res;
	hal_video_snr_get_cur_res(snr_id, &res_idx, &snr_res);
	printf("snr res: %d x %d\n", snr_res.width, snr_res.height);

	hal_video_vif_attr_t vif_attr;
	vif_attr.snr_id = snr_id;
	vif_attr.hdr_en = 0;
	vif_attr.res.width = FRAME_RES_WIDTH; //snr_res.width;
	vif_attr.res.height = FRAME_RES_HEIGHT; //snr_res.height;

	ret = hal_video_vif_init(vif_dev, vif_chn, vif_port, &vif_attr);
	if (ret != 0) {
		printf("ERROR: hal_video_vif_init failed\n");
		return -1;
	}

	/* enable vif */
	ret = hal_video_vif_enable(vif_dev, vif_chn, vif_port);
	if (ret != 0) {
		printf("ERROR: hal_video_vif_enable failed\n");
		return -1;
	}
	printf("VIP OK\n");

	/* set sensor mirror */
	ret = hal_video_snr_set_mirror(snr_id, HAL_VIDEO_MIRROR_NONE);
	if (ret != 0) {
		printf("ERROR: hal_video_snr_set_mirror failed\n");
		return -1;
	}

/* VPE */

	/* init vpe */
	hal_video_vpe_attr_t vpe_attr;
	ret = hal_video_vpe_init(&vpe_attr);
	if (ret != 0) {
		printf("ERROR: hal_video_vpe_init failed\n");
		return -1;
	}

	/* create vpe channel */

	hal_video_vif_info_t vif_info;
	ret = hal_video_vif_get_info(vif_dev, vif_chn, vif_port, &vif_info);
	if (ret != 0) {
		printf("ERROR: hal_video_vif_get_info failed\n");
		return -1;
	}

	int vpe_chn = 0;
	int vpe_port = 0;

	hal_video_vpe_chn_attr_t vpe_chn_attr;
	memset(&vpe_chn_attr, 0, sizeof(vpe_chn_attr));
	vpe_chn_attr.vif_dev = vif_dev;
	vpe_chn_attr.vif_chn = vif_chn;
	vpe_chn_attr.vif_port = vif_port;
	vpe_chn_attr.pixfmt = vif_info.pixfmt; // TODO: check pixfmt
	vpe_chn_attr.res.width = vif_info.res.width;
	vpe_chn_attr.res.height = vif_info.res.height;
	vpe_chn_attr.rotate = vif_info.rotate;
	vpe_chn_attr.mirror = vif_info.mirror;
	vpe_chn_attr.src_fps = vif_info.fps;
	vpe_chn_attr.dst_fps = vif_info.fps;
	vpe_chn_attr.hdr_en = 0;
	ret = hal_video_vpe_create_channel(vpe_chn, vpe_port, &vpe_chn_attr);
	if (ret != 0) {
		printf("ERROR: hal_video_vpe_create_channel failed\n");
		return -1;
	}
	printf("VPE create channel OK\n");

	/* start vpe channel */
	ret = hal_video_vpe_start_channel(vpe_chn, vpe_port);
	if (ret != 0) {
		printf("ERROR: hal_video_vpe_start_channel failed\n");
		return -1;
	}

	printf("VPE OK\n");

/* VPP */
	/* VENC is bound to VPE, VPP is omitted */

/* VENC */

	hal_video_venc_attr_t venc_attr;
	ret = hal_video_venc_init(&venc_attr);
	if (ret != 0) {
		printf("ERROR: hal_video_venc_init failed\n");
		return -1;
	}

	int venc_chn = 0;
	hal_video_venc_chn_attr_t venc_chn_attr;
	venc_chn_attr.bind_mod = HAL_VIDEO_MOD_VPE;
	venc_chn_attr.bind_type = HAL_VIDEO_VENC_BIND_TYPE_FRAME_BASE;
	venc_chn_attr.vpe_chn = vpe_chn;
	venc_chn_attr.vpe_port = vpe_port;

	venc_chn_attr.std = HAL_VIDEO_ENC_STD_H265;
	venc_chn_attr.res.width = vpe_chn_attr.res.width;
	venc_chn_attr.res.height = vpe_chn_attr.res.height;
	venc_chn_attr.fps_src = vpe_chn_attr.dst_fps;
	venc_chn_attr.fps_dst = vpe_chn_attr.dst_fps;
	venc_chn_attr.bitrate = 512 * 1024;
	venc_chn_attr.gop = 50;
	venc_chn_attr.qp_min = VIDEO_H265_MIN_QP;
	venc_chn_attr.qp_max = VIDEO_H265_MAX_QP;
	ret = hal_video_venc_create_channel(venc_chn, &venc_chn_attr);
	if (ret != 0) {
		printf("ERROR: hal_video_venc_create_channel failed\n");
		return -1;
	}
	printf("VENC create channel OK\n");

	ret = hal_video_venc_start_channel(venc_chn);
	if (ret != 0) {
		printf("ERROR: hal_video_venc_start_channel failed\n");
		return -1;
	}

	printf("VENC OK\n");

/* ISP file */
	//hal_video_vpe_isp_load_para_file(0, char *path, unsigned int magic);

	return 0;
}

static RTSP_CODEC_E get_codec(RTSP_MEDIA_TYPE_E type)
{
    switch (type){
    case RTSP_MEDIA_TYPE_VIDEO:
        return RTSP_CODEC_H265;
    case RTSP_MEDIA_TYPE_AUDIO:
        return RTSP_CODEC_G711U;
    default:
        return 0;
    }
}

static int ty_dsp_video_get(int devId, E_DSP_MEDIA_VIDEO_TYPE type, S_DSP_VIDEO_FRAME *pFrame)
{
	unsigned char *pFrameBuff = pFrame->pbuf;
	int i;

	if (NULL == pFrame) {
		printf("ERROR !!! pvparam->pStPack == NULL\n");
		return -1;
	}

	if (hal_video_venc_wait_for_stream(0, 10) != 0) {
		printf("no stream\n");
		return -1;
	}

	int pkt_num;
	if (hal_video_venc_get_packet_num(0, &pkt_num) != 0) {
		printf("get pkt cnt failed\n");
		return -1;
	}
	//printf("pkt cnt: %d\n", pkt_num);

	hal_video_venc_stream_t stream;
	hal_video_venc_packet_t pkts[VENC_MAX_PAKETS];
	if (pkt_num > VENC_MAX_PAKETS) {
		printf("****** pkt_num %d exceeds the limit %d ******\n",
			pkt_num, VENC_MAX_PAKETS);
		pkt_num = VENC_MAX_PAKETS;
	}
	stream.pkt_num = pkt_num;
	stream.pkts = pkts;

	if (hal_video_venc_get_stream(0, &stream, 0) != 0) {
		printf("get stream failed\n");
		return -1;
	}

	unsigned int frameLen = 0;
	for (i = 0; i < stream.pkt_num; i++) {
		frameLen += stream.pkts[i].length - stream.pkts[i].offset;
	}
	if (frameLen > pFrame->bufSize) {
		printf("buf too small!!! framelen = %d pFrame->bufSize = %d\n",
			frameLen,pFrame->bufSize);
		return -1;
	}

	frameLen = 0;
	for (i = 0; i < stream.pkt_num; i ++) {
		memcpy(pFrameBuff + frameLen,
		stream.pkts[i].virt_addr + stream.pkts[i].offset,
		stream.pkts[i].length - stream.pkts[i].offset);
		frameLen += stream.pkts[i].length - stream.pkts[i].offset;
	}

	pFrame->usedSize = frameLen;
	//pFrame->height = pvparam->u32Height;
	//pFrame->width = pvparam->u32Width;
	pFrame->pts = stream.pkts[0].pts;
	//pFrame->type = frameType;

	hal_video_venc_release_stream(0, &stream);
	return 0;
}

static uint64_t get_now_time()
{
	struct timeval tv = {0};
	gettimeofday(&tv, NULL);

	return tv.tv_sec * 1000LL + tv.tv_usec /1000;
}

int product_test_video_get_frame(int *type, char **buf, int *plen, uint64_t *pts)
{
	int iret;
	S_DSP_VIDEO_FRAME frame = {0};
	frame.pbuf = g_media_buf;
	frame.bufSize = sizeof(g_media_buf);

	if((iret = ty_dsp_video_get(0, E_DSP_MEDIA_VIDEO_MAIN, &frame)) == 0) {
		*buf = frame.pbuf;
		*plen = frame.usedSize;

		if(pts) {
		//	*pts = frame.pts;
			*pts = get_now_time();
		}
	}

	return iret;
}


int ty_dev_ai_get(E_DSP_MEDIA_AUDIO_TYPE type, S_DSP_AUDIO_FRAME *frame)
{
	int ret;
	int ai_dev = 0;
	int ai_chn = 0;
	hal_aud_frame_s aud_frame;

	ret = hal_aud_in_wait_for_frame(ai_dev, ai_chn, -1);
	if (ret != 0) {
		printf("no frame\n");
		return -1;
	}

	ret = hal_aud_in_get_frame(ai_dev, ai_chn, &aud_frame, /* 0 */ -1);
	if (ret != 0) {
		printf("failed to get frame\n");
		return -1;
	}

	memcpy(frame->pbuf, (char *)aud_frame.vir_addr[0], aud_frame.len);
	frame->type = E_AUDIO_FRAME;
	frame->codectype = TUYA_CODEC_AUDIO_PCM;
	frame->seq = aud_frame.seq;
	frame->pts = aud_frame.timestamp;
	frame->usedSize = aud_frame.len;
	frame->sample = TUYA_AUDIO_SAMPLE_8K;

	hal_aud_in_release_frame(ai_dev, ai_chn, &aud_frame);
	return 0;
}

#define DSP_AUDIO_FRAME_LEN	1280

int ty_dsp_audio_get(E_DSP_MEDIA_AUDIO_TYPE type, S_DSP_AUDIO_FRAME *frame)
{
    int ret = -1;
    char buf[DSP_AUDIO_FRAME_LEN] = {0};
    char *tmpBuf = NULL;
    int bufSize = 0;
    int outLen = 0;
    TUYA_CODEC_ID codectype = TUYA_CODEC_AUDIO_G711U;

    if (frame == NULL || frame->pbuf == NULL)
    {
        printf("input is NULL\n");
        return -1;
    }

    switch (codectype)
    {
        case TUYA_CODEC_AUDIO_PCM:
            ret = ty_dev_ai_get(type, frame);
            if (0 != ret)
            {
                //printf("get frame failed type:%d\n", type);
                return -1;
            }
            break;

        case TUYA_CODEC_AUDIO_G711U:
        case TUYA_CODEC_AUDIO_G711A:
            tmpBuf = frame->pbuf;
            bufSize = frame->bufSize;
            frame->bufSize = DSP_AUDIO_FRAME_LEN;
            frame->pbuf = buf;
            ret = ty_dev_ai_get(type, frame);
            if (0 != ret)
            {
                frame->pbuf = tmpBuf;
                frame->bufSize = bufSize;
                return -1;
            }

            ret = tuya_g711_encode(codectype, (unsigned short*)buf, frame->usedSize, tmpBuf, &outLen);
            if (0 != ret)
            {
                printf("codectype:%d failed\n", codectype);
                return -1;
            }
            frame->pbuf = tmpBuf;
            frame->bufSize = bufSize;
            frame->usedSize = outLen;
            frame->codectype = codectype;
            break;

        default:
            printf("input is NULL\n");
            return -1;
    }

	return 0;
}

int product_test_audio_get_frame(char **buf,int *plen,uint64_t *pts)
{
	int iret;
	S_DSP_AUDIO_FRAME frame = {0};
	frame.pbuf = g_media_buf;
	frame.bufSize = sizeof(g_media_buf);

	if((iret = ty_dsp_audio_get(E_DSP_MEDIA_AUDIO, &frame)) == 0) {
		*buf = frame.pbuf;
		*plen = frame.usedSize;

		if(pts) {
		//	*pts = frame.pts;
			*pts = get_now_time();
		}
	}

	return iret;
}

static int rtsp_get_frame(int user_id, RTSP_MEDIA_TYPE_E type, char** buf, int *plen, uint64_t *pts)
{
	int iret = 0;

	rtsp_user_info_s *puser= &users[user_id];

	int v_type;
	if (type == RTSP_MEDIA_TYPE_VIDEO) {
#ifdef HAL_TEST_VIDEO
		iret = product_test_video_get_frame(&v_type, buf, plen, pts);
#endif
	} else {
#ifdef HAL_TEST_AUDIO
		iret = product_test_audio_get_frame(buf, plen, pts);
#endif
	}

	return iret;
}

static int rtsp_get_frame_main(int user_id, RTSP_MEDIA_TYPE_E type,
		char** buf, int *plen, uint64_t *pts)
{
	users[user_id].vchn = E_CHANNEL_VIDEO_MAIN;
	users[user_id].achn = E_CHANNEL_AUDIO;
	return rtsp_get_frame(user_id, type, buf, plen, pts);
}

static int get_sample_rate(RTSP_MEDIA_TYPE_E type)
{
    switch (type){
    case RTSP_MEDIA_TYPE_VIDEO:
        return 90000;
    case RTSP_MEDIA_TYPE_AUDIO:
        return 8000;
    default:
        return 8000;
    }
}

static int rtsp_user_start()
{
    int i;
    for(i = 0; i < MAX_RTSP_USER; i++){
        if (0 == users[i].active){
            users[i].active = 1;
            users[i].vchn = E_CHANNEL_MAX;
            users[i].achn = E_CHANNEL_MAX;
            return i;
        }
    }
    return -1;
}

static int rtsp_first_frame = 1;
static int rtsp_user_stop(int user_id)
{
	users[user_id].active = 0;
	if (users[user_id].vchn != E_CHANNEL_MAX) {
	//	tuya_ipc_ring_buffer_clean_user_state(users[user_id].vchn, E_USER_RTSP + user_id);
	}
	if (users[user_id].achn != E_CHANNEL_MAX) {
	//    tuya_ipc_ring_buffer_clean_user_state(users[user_id].achn, E_USER_RTSP + user_id);
	}
	rtsp_first_frame = 1;
	//  printf("%s,rtsp_first_frame= %d\n",__func__,rtsp_first_frame);
	return 0;
}

static int ty_rtsp_server_start(void)
{
	rtsp_stream_src_t src[2] = {{0}};
	src[0].get_codec = get_codec;
	src[0].get_frame = rtsp_get_frame_main;
	src[0].get_sample_rate = get_sample_rate;
	src[0].start = rtsp_user_start;
	src[0].stop = rtsp_user_stop;
	rtsp_server_start(NULL, NULL);
	rtsp_server_register_stream_src(src);
	return 0;
}

int main(void)
{
	hal_video_sys_init();
#ifdef HAL_TEST_AUDIO
	hal_audio_init();
#endif
#ifdef HAL_TEST_VIDEO
	hal_video_init();
#endif
	ty_rtsp_server_start();
	while (1) {
		sleep(3);
	}
	return 0;
}
