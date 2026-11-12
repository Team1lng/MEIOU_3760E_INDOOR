
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <inttypes.h>
#include "product_test_media.h"
#include "ptal_media.h"
#include "tb_log.h"

#ifdef CONFIG_MEDIA_TKL_SUPPORT
#include "tkl_video_enc.h"
#include "tkl_isp.h"
#include "ty_dsp_adapter.h"
//#include "ty_isp_dev_common.h"
//#include "ty_isp_adapter.h"

extern S_DSP_MEDIA_INFO gMediaInfo;

static int sg_video_codec[E_DSP_MEDIA_VIDEO_MAX];
static S_DSP_MEDIA_INFO ptal_media_info = {0};

static char g_media_buf[512*1024] = {0};

static uint64_t get_now_time()
{
    struct timeval tv = {0};
    gettimeofday(&tv, NULL);

    return tv.tv_sec * 1000LL + tv.tv_usec /1000;
}

int __attribute__((weak)) ptal_video_code_type(int channel)
{
    int codec = E_CODEC_VIDEO_H265;

    if(channel < E_DSP_MEDIA_VIDEO_MAX) {

        codec =  sg_video_codec[channel];
    }

//    TB_LOGD("codec:%d",codec);

    return  codec;
}


static S_DSP_MEDIA_INFO * ptal_get_media_info(void)
{
    return &ptal_media_info; 
}


static int ptal_refresh_media_info(MEDIA_CONFIG_T *config,S_DSP_MEDIA_INFO *media_info)
{
    int i;

    TB_LOGD();

    if(config == NULL || media_info == NULL) {
   	TB_LOGW("invalid parm,used default config");
	return 0;		
    }

    for(i=0;i<MAX_AUDIO;i++) {
   	if(config->audio[i].channel_enable == 1 && i < E_DSP_MEDIA_AUDIO_MAX) {
            media_info->audio[i].channel_enable = config->audio[i].channel_enable;
            media_info->audio[i].media_type     = (E_DSP_MEDIA_AUDIO_TYPE)config->audio[i].media_type;
            media_info->audio[i].sample         = (TUYA_AUDIO_SAMPLE_E)config->audio[i].sample;
            media_info->audio[i].datebits       = (TUYA_AUDIO_DATABITS_E)config->audio[i].datebits;
            media_info->audio[i].channnel       = (TUYA_AUDIO_CHANNEL_E)config->audio[i].channel;
            media_info->audio[i].codectype      = (TUYA_CODEC_ID)config->audio[i].codectype;
            media_info->audio[i].fps            = config->audio[i].fps;
            media_info->audio[i].mic_volume     = config->audio[i].mic_volume;
            media_info->audio[i].spk_volume     = config->audio[i].spk_volume;

            TB_LOGI("%s(%d)=%d","channel_enable",i,media_info->audio[i].channel_enable);
            TB_LOGI("%s(%d)=%d","media_type",i,media_info->audio[i].media_type);
            TB_LOGI("%s(%d)=%d","sample",i, media_info->audio[i].sample);
            TB_LOGI("%s(%d)=%d","datebits",i,media_info->audio[i].datebits);
            TB_LOGI("%s(%d)=%d","channnel",i,media_info->audio[i].channnel);
            TB_LOGI("%s(%d)=%d","codectype",i,media_info->audio[i].codectype);
            TB_LOGI("%s(%d)=%d","fps",i, media_info->audio[i].fps);
            TB_LOGI("%s(%d)=%d","mic_volume",i,media_info->audio[i].mic_volume);
            TB_LOGI("%s(%d)=%d","spk_volume",i,media_info->audio[i].spk_volume);
	} 
    }

    for(i=0;i<MAX_VIDEO;i++) {
        if(config->video[i].channel_enable == 1 && i < E_DSP_MEDIA_VIDEO_MAX) {
            media_info->video[i].channel_enable = config->video[i].channel_enable;
            media_info->video[i].media_type     = (E_DSP_MEDIA_VIDEO_TYPE)config->video[i].media_type;
            media_info->video[i].fps            = config->video[i].fps;
            media_info->video[i].gop            = config->video[i].gop;
            media_info->video[i].bitrate        = (TUYA_VIDEO_BITRATE_E)config->video[i].bitrate;
            media_info->video[i].width          = config->video[i].width;
            media_info->video[i].height         = config->video[i].height;
            media_info->video[i].codectype      = (TUYA_CODEC_ID)config->video[i].codectype;
            media_info->video[i].firstFrame     = config->video[i].firstFrame;
            media_info->video[i].minQp          = config->video[i].minQp;
            media_info->video[i].maxQp          = config->video[i].maxQp;

            TB_LOGI("%s(%d)=%d","channel_enable",i,media_info->video[i].channel_enable);
            TB_LOGI("%s(%d)=%d","channel_enable",i,media_info->video[i].channel_enable);
            TB_LOGI("%s(%d)=%d","media_type",i,media_info->video[i].media_type);
            TB_LOGI("%s(%d)=%d","fps",i,media_info->video[i].fps);
            TB_LOGI("%s(%d)=%d","gop",i,media_info->video[i].gop);
            TB_LOGI("%s(%d)=%d","bitrate",i,media_info->video[i].bitrate);
            TB_LOGI("%s(%d)=%d","width",i,media_info->video[i].width);
            TB_LOGI("%s(%d)=%d","height",i,media_info->video[i].height);
            TB_LOGI("%s(%d)=%d","codectype",i,media_info->video[i].codectype);
            TB_LOGI("%s(%d)=%d","firstFrame",i,media_info->video[i].firstFrame);
            TB_LOGI("%s(%d)=%d","minQp",i,media_info->video[i].minQp);
            TB_LOGI("%s(%d)=%d","maxQp",i,media_info->video[i].maxQp);
	} 
    }

    return 0;
}

int __attribute__((weak)) ptal_media_init(MEDIA_CONFIG_T *config)
{
    int i;
    int iret;

    S_DSP_MEDIA_INFO * media_info = ptal_get_media_info();

    TB_LOGD();
    memcpy(media_info,&gMediaInfo,sizeof(S_DSP_MEDIA_INFO));

    if(ptal_refresh_media_info(config,media_info)) {
        TB_LOGE("refresh media config fail");
        return -1;  
    }

    for(i=0;i<MAX_VIDEO;i++) {
        sg_video_codec[i] = media_info->video[i].codectype;
        TB_LOGD("%d codec:%d",i,media_info->video[i].codectype);
    }

    iret = ty_dsp_init(media_info);

    if (iret != 0) {
        TB_LOGE("ty_dsp_init fail");
        return -1;
    }

    return 0;
}

int __attribute__((weak)) ptal_mic_get_audio_rate(int *rate) 
{
    S_DSP_MEDIA_INFO * media_info = ptal_get_media_info();
    *rate =(int) media_info->audio[E_DSP_MEDIA_AUDIO].sample;       
    return 0;
}

int __attribute__((weak)) ptal_video_get_frame(int video_type,int *frame_type,char **buf,int *plen,uint64_t *pts) 
{
    int iret;
    S_DSP_VIDEO_FRAME frame = {0};
    frame.pbuf = g_media_buf;
    frame.bufSize = MAX_MEDIA_FRAME_SIZE;

    if((iret = ty_dsp_video_get(0,video_type,&frame)) == 0) {
        *buf = frame.pbuf;
        *plen =  frame.usedSize;
        if(pts) {
        //  *pts = frame.pts;
            *pts = get_now_time();
        }

        if(frame_type) {
            if(frame.type == E_VIDEO_I_FRAME){
                *frame_type =  E_VIDEO_FRAME_I;
            }else if(frame.type == E_VIDEO_PB_FRAME) {
                *frame_type = E_VIDEO_FRAME_PB;
            }else if(frame.type  == E_VIDEO_TS_FRAME) {
                *frame_type = E_VIDEO_FRAME_TS;
            }else {
                *frame_type = E_VIDEO_FRAME_MAX; 
            }

        }
    }

    return iret;
}

int __attribute__((weak)) ptal_audio_get_frame(char **buf,int *plen,uint64_t *pts)
{
    int iret;
    S_DSP_AUDIO_FRAME frame = {0};
    frame.pbuf = g_media_buf;
    frame.bufSize = MAX_MEDIA_FRAME_SIZE;

    if((iret = ty_dsp_audio_get(E_DSP_MEDIA_AUDIO, &frame)) == 0) {
        *buf = frame.pbuf;
        *plen = frame.usedSize;
        if(pts) {
        //  *pts = frame.pts;
            *pts = get_now_time();
        }
    }

    return iret;
}

int __attribute__((weak)) ptal_audio_put_frame(char *buf,int plen)
{
    int iret;
    int rate;
    S_DSP_AUDIO_FRAME frame = {0};

    S_DSP_MEDIA_INFO * media_info = ptal_get_media_info();
    rate =(int) media_info->audio[E_DSP_MEDIA_AUDIO].sample;       

    frame.pbuf = buf;
    frame.usedSize  = plen;
    frame.codectype = TUYA_CODEC_AUDIO_PCM;
    frame.sample = rate;
    frame.bufSize = 1280; 

    TB_LOGD("rate:%d\n",rate);

  // pframe->sample = sample;
  // frame.bufSize = DSP_AUDIO_FRAME_LEN;

  // need add sample rate  type in ty_dsp_audio_put
  
    iret = ty_dsp_audio_put(E_DSP_AUDIO_SOURCE_TYPE_FILE, &frame);

    return iret;
}


int __attribute__((weak)) ptal_speaker_set_volume(int devid, int channel, unsigned int value)
{
    TB_LOGD();
    return ty_dsp_audio_set_volume(devid, channel, value);
//  return 0;
}

int __attribute__((weak))ptal_speaker_play_file(const char* fileName, int fileType, int count, int timeOut)
{
    E_DSP_AUDIO_FILE_TYPE type = E_DSP_AUDIO_FILE_TYPE_MAX;
    TB_LOGD();

    switch (fileType)
    {
        case E_AUDIO_FILE_PCM:
            type = E_DSP_AUDIO_FILE_PCM;
            break;
        case E_AUDIO_FILE_WAV:
            type = E_DSP_AUDIO_FILE_WAV;
            break;
        default:
            break;
    }

    return ty_dsp_audio_play_file(fileName,type,count,timeOut);

}

int __attribute__((weak))ptal_speaker_stop(void) 
{
    TB_LOGD();
    return ty_dsp_audio_stop_play_file();
}

int __attribute__((weak))ptal_load_day_isp_file(void)
{
    int iret = 0;
    OPERATE_RET ret;

    TB_LOGD();

    ret = tkl_vi_isp_load_isp_file(DAY_MODE);

    if(ret) {
        iret = -1; 
    }

    sleep(1);

    return iret;
}

int __attribute__((weak)) ptal_load_night_isp_file(void) 
{
    int iret = 0;
    OPERATE_RET ret;

    TB_LOGD();

    ret = tkl_vi_isp_load_isp_file(NIGHT_MODE);

    if(ret) {
        iret = -1; 
    }

    sleep(1);

    return iret;
}


int __attribute__((weak)) ptal_isp_set_color_to_gray(ISP_MODE_E mode) 
{
    TB_LOGD();

    TKL_ISP_DN_MODE_E irmode;

    if (mode == 1) {
        irmode = 0;
    }else if( mode == 2) {
   	irmode = 1;
    }else {
        irmode = 0;
    }

    return tkl_vi_isp_set_color_to_gray((TKL_VI_CHN_E)0,irmode);
}


int __attribute__((weak)) ptal_get_virtual_adc(ISP_MODE_E mode,uint32_t* val)
{
    OPERATE_RET ret;
    INT32_T illumin;
    TKL_ISP_DN_MODE_E irmode;

    TB_LOGD();

    if(mode == E_MODE_AUTO) {
        irmode = AUTO_MODE; 
    }else if(mode == E_MODE_DAY) {
        irmode = DAY_MODE; 
    }else if (mode == E_MODE_NIGHT) {
        irmode = NIGHT_MODE; 
    }else {
        irmode = AUTO_MODE; 
    }

    ret = tkl_vi_dnswitch_get_illumin((TKL_VI_CHN_E)0,irmode, &illumin);

    if(ret) {
        TB_LOGE("get illumin fail\n");
         return -1; 
    }

    *val = (uint32_t)illumin;

    TB_LOGD("immumin:%d\n",illumin);

    return 0;
}


// 0,none,1 mirror,2,flip,3,both
int __attribute__((weak)) ptal_video_set_mirror_flip(int mirror_flip)
{
    int iret = 0;;
    TKL_VI_MIRROR_FLIP_E mf;

    TB_LOGI("mirror_flip:%d",mirror_flip);

    if(mirror_flip ==  1) {
        mf = TKL_VI_MIRROR;
    }else if (mirror_flip == 2) {
        mf =  TKL_VI_FLIP;
    }else if (mirror_flip == 3) {
        mf =  TKL_VI_MIRROR_FLIP;
    }else {
        mf =  TKL_VI_MIRROR_FLIP_NONE;
    }

    if(tkl_vi_set_mirror_flip((TKL_VI_CHN_E)0,mf)) {
	TB_LOGE("tkl_vi_isp_set_mirror_flip fail\n");
        iret = -1; 
    }

    return iret;
}

int __attribute__((weak)) ptal_speaker_get_volume(int devid, int channel, unsigned int *value)
{
    TB_LOGD();
    return -1;
}

int __attribute__((weak)) ptal_mic_set_volume(int devid, int channel, unsigned int value)
{
    TB_LOGD();
    return -1;
}

int __attribute__((weak)) ptal_mic_get_volume(int devid, int channel, unsigned int *value)
{
    TB_LOGD();
    return -1;
}

#else

int __attribute__((weak)) ptal_load_day_isp_file(void)
{
    TB_LOGD();
    return -1;
}

int __attribute__((weak)) ptal_load_night_isp_file(void)
{
    TB_LOGD();
    return -1;
}

int __attribute__((weak)) ptal_media_init(MEDIA_CONFIG_T *config)
{
    TB_LOGD();
    return -1;
}

int __attribute__((weak))  ptal_mic_get_audio_rate(int *rate)
{
    TB_LOGD();
    return -1;
}

int __attribute__((weak)) ptal_audio_get_frame(char **buf,int *plen,uint64_t  *pts)
{
    TB_LOGD();
    return -1;
}

int __attribute__((weak)) ptal_audio_put_frame(char *buf,int plen)
{
    TB_LOGD();
    return -1;
}

int __attribute__((weak)) ptal_speaker_set_volume(int devid, int channel, unsigned int value)
{
    TB_LOGD();
    return -1;
}

int __attribute__((weak)) ptal_speaker_get_volume(int devid, int channel, unsigned int *value)
{
    TB_LOGD();
    return -1;
}

int __attribute__((weak)) ptal_mic_set_volume(int devid, int channel, unsigned int value)
{
    TB_LOGD();
    return -1;
}

int __attribute__((weak)) ptal_mic_get_volume(int devid, int channel, unsigned int *value)
{
    TB_LOGD();
    return -1;
}

int __attribute__((weak))  ptal_speaker_play_file(const char* fileName, int fileType, int count, int timeOut)
{
    TB_LOGD();
    return -1;
}

int __attribute__((weak)) ptal_speaker_stop(void)
{
    TB_LOGD();
    return -1;
}

int __attribute__((weak)) ptal_video_get_frame(int video_type,int*frame_type,char **buf,int *plen,uint64_t *pts)
{
    TB_LOGD();
    return -1;
}

int __attribute__((weak)) ptal_video_set_mirror_flip(int mirror_flip)
{
    TB_LOGD();
    return -1;
}

int __attribute__((weak)) ptal_isp_set_color_to_gray(ISP_MODE_E mode)
{
    TB_LOGD();
    return -1;
}

int __attribute__((weak)) ptal_get_virtual_adc(ISP_MODE_E mode,uint32_t* val)
{
    TB_LOGD();
    return -1;
}

int __attribute__((weak)) ptal_video_code_type(int channel)
{
    TB_LOGD();
    return -1;
}

#endif

