#ifdef CONFIG_MEDIA_TKL_SUPPORT
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <pthread.h>
#include "product_test_camera.h"
#include "ty_dsp_adapter.h"
#include "tuya_g711_utils.h"
#include "ty_dsp_adapter_wav.h"
#include "tkl_audio.h"
#include "tkl_video_in.h"
#include "tkl_video_enc.h"
#include "tb_log.h"

#define DSP_AUDIO_FRAME_LEN             1280
#define DSP_AUDIO_THREAD_STACK_SIZE     0x100000


typedef struct {
    int                             isInit;                  /* 音频初始化标志 */
    int                             start;                   /* 播放状态,0，未播放，1，播放中 */
    pthread_t                       stid;                    /* 播放线程pid   */
    char                            filename[128];           /* 播放的文件名，包含路径 */
    E_DSP_AUDIO_FILE_TYPE           fileType;                /* 文件类型    */
    int                             count;                   /* 播放次数，0，代表循环播放 */
    int                             fileStop;                /* 文件播放停止    */
    unsigned int                    audioStartType;          /* 音频打开者,采用移位保存状态 */
    int                             spkRequestCount;         /* 记录请求开启spk的数量*/
    sem_t                           spkSem;                  /* 多个线程对spkRequestCount操作时的保护*/
} ty_dsp_audio_handle_s;


typedef struct{
    S_DSP_MEDIA_INFO *pstrDspInfo;                           /* 音频属性配置 */
    ty_dsp_audio_handle_s audioHandle;                       /* 音频播放属性 */
}TY_DSP_ADAPTER_S;


static TY_DSP_ADAPTER_S gdspAdapter = {0};

static int ty_dev_vi_init(void)
{
    int iret = 0;

    TKL_VI_CONFIG_T config ={0};
    TKL_VI_ISP_CONFIG_T *pispconfig = &config.isp;

    TKL_ISP_DN_SWITCH_CONFIG_T dn_swich_config_day =  {
        1,                      //dnSwitchMode
        400,                    //dayToNight
        1180,                   //nightToDay
        0,                              //dnSwitchOffset
        {0.002,0.45},   //offsetR[2]
        {0.002,0.57},   //offsetG[2]
        {0.01 ,0.6 },   //offsetB[2]
        {-1.9731,1.3635,1.6105,3.9506,3.4239,-8.3081},                  //coffR[6]
        {-2.90551,0.835232,3.11107,6.28917,4.24424,-11.447},    //coffG[6]
        {-2.62056,3.38979,0.58672,6.07542,6.15896,-13.4769}             //coffB[6]
    };

    config.enable =  1;
    config.chn = TKL_VI_0;
    config.mirror = 0;
    config.filp = 0;


    product_test_camera_get_type(&pispconfig->name[0],16 -1); 
    product_test_camera_get_version(&pispconfig->version[0],32-1);
    product_test_camera_get_config(&pispconfig->conf[0],128-1);


    TB_LOGD("name:%s,version:%s,conf:%s\n",pispconfig->name,pispconfig->version,pispconfig->conf);

    memcpy(&pispconfig->isp_dn_switch_config ,&dn_swich_config_day,sizeof(TKL_ISP_DN_SWITCH_CONFIG_T));


    if(tkl_vi_init(&config,1)) {
   
	TB_LOGE("tkl_vi_init fail\n"); 
        iret = -1; 
    }

    return iret;
}

static int ty_dev_venc_init(int devId, S_DSP_MEDIA_INFO *info)
{
    int i;
    int totals = 8;
    int iret = 0;
    TKL_VENC_CONFIG_T tkl_config[E_DSP_MEDIA_VIDEO_MAX];

    TB_LOGD();

    memset(&tkl_config,0,sizeof(TKL_VENC_CONFIG_T)*E_DSP_MEDIA_VIDEO_MAX);

    for(i=0;i < E_DSP_MEDIA_VIDEO_MAX;i++) { 

        S_DSP_VIDEO_CONFIG *config  = &info->video[i];
    
        if(!config->channel_enable) {
            continue;	
	    }
        
    	tkl_config[i].enable = 1;
    	tkl_config[i].chn = 1; //??

    	tkl_config[i].type = (TKL_VENC_TYPE_E)config->media_type;
    	tkl_config[i].codectype = (TKL_MEDIA_CODEC_TYPE_E)config->codectype;
    	tkl_config[i].fps = config->fps;
    	tkl_config[i].gop = config->gop;
    	tkl_config[i].bitrate = config->bitrate;
    	tkl_config[i].width = config->width;
    	tkl_config[i].height = config->height;
    	tkl_config[i].min_qp = config->minQp;
    	tkl_config[i].max_qp = config->maxQp;
    }

    if(tkl_venc_init((INT32_T)devId,&tkl_config[0],totals)) {
	TB_LOGE("tkl_venc_init fail\n");
        iret = -1; 
    }

    return iret;
}

static int ty_dev_venc_start(int devId, E_DSP_MEDIA_VIDEO_TYPE type) 
{
    return 0;
}

static int ty_dev_venc_get(int devId, E_DSP_MEDIA_VIDEO_TYPE type, S_DSP_VIDEO_FRAME *frame) 
{
    TKL_VENC_FRAME_T tkl_frame = {0};

    tkl_frame.buf_size = frame->bufSize;
    tkl_frame.pbuf = frame->pbuf;

    if(tkl_venc_get_frame((TKL_VI_CHN_E)devId,(TKL_VENC_CHN_E)type,&tkl_frame)) {
        TB_LOGE("tkl_venc_get_frame fail");
	return -1;
    }

    frame->usedSize = tkl_frame.used_size;
    frame->height = tkl_frame.height;
    frame->width  = tkl_frame.width;
    frame->pts = tkl_frame.pts;
    frame->type = tkl_frame.frametype;

    return 0;
}

static int ty_dev_audio_init(S_DSP_MEDIA_INFO *info)
{
    TKL_AUDIO_CONFIG_T ai_config = {0};
    TKL_AUDIO_CONFIG_T ao_config = {0};

    TB_LOGD();

    ai_config.channel = info->audio[E_DSP_MEDIA_AUDIO].channnel;
    ai_config.datebits = info->audio[E_DSP_MEDIA_AUDIO].datebits;
    ai_config.sample = info->audio[E_DSP_MEDIA_AUDIO].sample;
    ai_config.mic_volume = info->audio[E_DSP_MEDIA_AUDIO].mic_volume;

    if(tkl_ai_init(&ai_config,1)) {
         TB_LOGE("tkl_ai_init fail");
         return -1;
    }

    ao_config.channel = info->audio[E_DSP_MEDIA_AUDIO].channnel;
    ao_config.datebits = info->audio[E_DSP_MEDIA_AUDIO].datebits;
    ao_config.sample = info->audio[E_DSP_MEDIA_AUDIO].sample;
    ao_config.spk_volume = info->audio[E_DSP_MEDIA_AUDIO].spk_volume;

    if(tkl_ao_init(&ao_config, 1,NULL)) {
         TB_LOGE("tkl_ai_init fail");
         return -1;
    }

    return 0;
}

static int ty_dev_ai_start(E_DSP_MEDIA_AUDIO_TYPE type)
{
    int iret = 0;

    if(tkl_ai_start(0,(TKL_AI_CHN_E)type)) {
        TB_LOGE("tkl_ai_start fail");
        iret = -1; 	
    }

    return iret;
}


static int ty_dev_ao_start(E_DSP_MEDIA_AUDIO_TYPE type) 
{
    int iret = 0;

    if(tkl_ao_start(0,(TKL_AO_CHN_E)type,NULL)) {
        TB_LOGE("tkl_ao_start fail");
	iret = -1;
    }

    return iret;
}

static int ty_dev_ao_stop(E_DSP_MEDIA_AUDIO_TYPE type)
{
    int iret = 0;

    if(tkl_ao_stop(0,(TKL_AO_CHN_E)type,NULL)) {
        TB_LOGE("tkl_ao_stop fail");
        iret =  -1;
    }

    return iret;
}

static int ty_dev_ai_get(E_DSP_MEDIA_AUDIO_TYPE type,S_DSP_AUDIO_FRAME *frame)
{
    TKL_AUDIO_FRAME_INFO_T tkl_frame = {0};


    tkl_frame.buf_size = frame->bufSize;
    tkl_frame.pbuf = frame->pbuf;

    if(tkl_ai_get_frame(0,(TKL_AI_CHN_E)type, &tkl_frame)) {
//      TB_LOGE("tkl_ai_get_frame fail");
	return -1;
    }

    frame->usedSize = tkl_frame.used_size ;

    frame->codectype = (TUYA_CODEC_ID) tkl_frame.codectype;
    frame->pts = tkl_frame.pts;
    frame->sample = tkl_frame.sample;

    return 0;
}

static int ty_dev_ao_put(S_DSP_AUDIO_FRAME *pframe)
{
    int iret = 0;
    TKL_AUDIO_FRAME_INFO_T frame = {0};

    frame.pbuf = pframe->pbuf;
    frame.used_size = pframe->usedSize;
    frame.buf_size = pframe->bufSize;

    if(tkl_ao_put_frame(0,(TKL_AO_CHN_E)0,NULL,&frame)) {
	TB_LOGE("tkl_ao_put_frame fail\n");
        iret = -1; 
    }
    return iret;
}

int ty_dsp_audio_set_volume(int devid, int channel, unsigned int value)
{
     int iret = 0;

     if(tkl_ao_set_vol(devid,(TKL_AO_CHN_E)channel, NULL,(int)value)) {
	 TB_LOGE("tkl_ao_set_vol fail");
         iret = -1; 
     }

     return iret;
}


/***********************************************************
*  函数名: ty_dsp_init
*  功能：dsp 初始化函数
*  参数: Input，S_DSP_MEDIA_INFO *pinfo，音视频的初始化参数
*      
*  返回值: <0 失败，=0 成功
***********************************************************/
int ty_dsp_init(S_DSP_MEDIA_INFO *pinfo)
{
    int ret = -1;
    int i = 0;

    if (pinfo == NULL)
    {
        TB_LOGE("ty_dsp_init pinfo is NULL\n");
        return -1;
    }

    for (i = 0; i < E_DSP_MEDIA_AUDIO_MAX; i ++)
    {
        if (pinfo->audio[i].channel_enable)
        {
            TUYA_CODEC_ID codectype = pinfo->audio[i].codectype;
            if ((pinfo->audio[i].sample != TUYA_AUDIO_SAMPLE_8K) && (
                (codectype == TUYA_CODEC_AUDIO_G711U) || (codectype == TUYA_CODEC_AUDIO_G711A)))
            {
                TB_LOGE("Not support:codetype:%d sample:%d\n",codectype, pinfo->audio[i].sample);
                return -1;
            }
        }
    }

    gdspAdapter.pstrDspInfo = pinfo;

    ret = ty_dev_vi_init();

    if(0 != ret) {
        TB_LOGE("ty_dev_vi_init fail");
	return -1;
    }

    ret = ty_dev_venc_init(0, pinfo);

    if (0 != ret)
    {
        TB_LOGE("ty_dsp_init failed,%d\n", ret);
        return -1;
    }

    ret = ty_dev_audio_init(pinfo);

    if (0 != ret)
    {
        TB_LOGE("ty_dev_audio_init failed,%d\n", ret);
        return -1;
    }

    for (i = 0; i < E_DSP_MEDIA_VIDEO_MAX; i ++)
    {
        if (pinfo->video[i].channel_enable)
        {
            ret = ty_dev_venc_start(0, i);
            if (0 != ret)
            {
                TB_LOGE("ty_dev_venc_start failed,%d\n", ret);
                return -1;
            }
        }
    }

    for (i = 0; i < E_DSP_MEDIA_AUDIO_MAX; i ++)
    {
        if (pinfo->audio[i].channel_enable)
        {
            ret = ty_dev_ai_start(i);
            if (0 != ret)
            {
                TB_LOGE("ty_dev_ai_start failed,%d\n", ret);
                return -1;
            }
        }
    }
    gdspAdapter.audioHandle.spkRequestCount = 0;        //初始化时 spk为关闭请求数量为0
    sem_init(&gdspAdapter.audioHandle.spkSem, 0, 1);    //初始化 信号量
    gdspAdapter.audioHandle.isInit = 1;

    return 0;
}


/***********************************************************
*  函数名: ty_dsp_video_get
*  功能：获取视频码流，包括主子码流，动检人形YUV数据等
*  参数: Input，int devId，多目预留
        Input，E_DSP_MEDIA_VIDEO_TYPE type，获取的码流类型
        Input，Output,pframe->pbuf和pframe->bufSize 入参，
              返回时，pbuf由底层cp数据，usedSize 为实际数据长度
*      
*  返回值: <0 失败，=0 成功
***********************************************************/
int ty_dsp_video_get(int devId, E_DSP_MEDIA_VIDEO_TYPE type, S_DSP_VIDEO_FRAME *frame)
{
    int ret = -1;
    if (frame == NULL || frame->pbuf == NULL)
    {
        TB_LOGE("input is NULL\n");
        return -1;
    }

    ret = ty_dev_venc_get(devId, type, frame);
    if (0 != ret)
    {
        TB_LOGE("get frame failed type:%d\n", type);
        return -1;
    }
   // TB_LOGD("type:[%d] bufSize:%d usedSize:%d width, height:%d %d\n", type, frame->bufSize, frame->usedSize, frame->width, frame->height);    
    return 0;
}


/***********************************************************
*  函数名: ty_dsp_audio_set
*  功能：设置音频属性
*  参数:  Input，E_DSP_MEDIA_AUDIO_TYPE type，操作的码流类型
        Input，E_DSP_SET_AUDIO set，cmd
        Input，void *parg, 设置的参数
*      
*  返回值: <0 失败，=0 成功
***********************************************************/
int ty_dsp_audio_set(E_DSP_MEDIA_AUDIO_TYPE type, E_DSP_SET_AUDIO set, void *parg)
{
    int ret = -1;
    ty_dsp_audio_handle_s* phdl = &gdspAdapter.audioHandle;
    switch (set)
    {
        case E_DSP_SET_AUDIO_SPEAKER_START:
            sem_wait(&phdl->spkSem);
            if(0 == phdl->spkRequestCount)
            {
                ret = ty_dev_ao_start(type);
                if (0 != ret)
                {
                    TB_LOGE("ty_dev_ao_start failed,%d\n", ret);
                    sem_post(&phdl->spkSem);
                    return -1;
                }
            }
            phdl->spkRequestCount ++;
            sem_post(&phdl->spkSem);
            break;

        case E_DSP_SET_AUDIO_SPEAKER_STOP:
            sem_wait(&phdl->spkSem);
            if(0 == phdl->spkRequestCount)
            {
                sem_post(&phdl->spkSem);
                break;
            }
            phdl->spkRequestCount --;
            if(0 == phdl->spkRequestCount)
            {
                ret = ty_dev_ao_stop(type);
                if (0 != ret)
                {
                    TB_LOGE("ty_dev_ao_stop failed,%d\n", ret);
                    sem_post(&phdl->spkSem);
                    return -1;
                }
            }
            sem_post(&phdl->spkSem);
            break;
/*
        case E_DSP_SET_AUDIO_SPEAKER_VOLUME:
            ty_dev_set_audio_param(E_DSP_MEDIA_AUDIO,set,(S_DSP_AUDIO_PARAM *)parg);
            break;
*/
        default:
            break;
    }

    return 0;
}

/***********************************************************
*  函数名: ty_dsp_audio_get
*  功能：获取音频帧数据
*  参数:  Input，E_DSP_MEDIA_AUDIO_TYPE type，获取的码流类型
        Input，Output,  pframe->pbuf和pframe->bufSize 入参，
              返回时，pbuf由底层cp数据，usedSize 为实际数据长度
*      
*  返回值: <0 失败，=0 成功
***********************************************************/
int ty_dsp_audio_get(E_DSP_MEDIA_AUDIO_TYPE type, S_DSP_AUDIO_FRAME *frame)
{
    int ret = -1; 
    char buf[DSP_AUDIO_FRAME_LEN] = {0};
    char *tmpBuf = NULL;
    int bufSize = 0;
    int outLen = 0;
    TUYA_CODEC_ID codectype = gdspAdapter.pstrDspInfo->audio[type].codectype;

    if (frame == NULL || frame->pbuf == NULL)
    {
        TB_LOGE("input is NULL\n");
        return -1;
    }

    switch (codectype)
    {
        case TUYA_CODEC_AUDIO_PCM:
            ret = ty_dev_ai_get(type, frame);
            if (0 != ret)
            {
                TB_LOGE("get frame failed type:%d\n", type);
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

            ret = tuya_g711_encode(codectype, (unsigned short*)buf, frame->usedSize, (unsigned char *)tmpBuf, (unsigned int *)&outLen);
            if (0 != ret)
            {
                TB_LOGE("codectype:%d failed\n", codectype);
                return -1;
            }
            frame->pbuf = tmpBuf;
            frame->bufSize = bufSize;
            frame->usedSize = outLen;
            frame->codectype = codectype;
            break;

        default:
            TB_LOGE("input is NULL\n");
            return -1;
    }

    //TB_LOGD("[%d]type:%d Size:%d usedSize:%d sam:%d seq:%u codec:%d pts:%llu\n", 
    //type, frame->type, frame->bufSize, frame->usedSize, frame->sample, frame->seq,frame->codectype,frame->pts);
    return 0;
}

/***********************************************************
*  函数名: ty_dsp_audio_put
*  功能：音频播放
*  参数:  Input，E_DSP_AUDIO_SOURCE_TYPE type，音频源
        Input，S_DSP_AUDIO_FRAME *pframe，音频帧数据
*      
*  返回值: <0 失败，=0 成功
***********************************************************/
int ty_dsp_audio_put(E_DSP_AUDIO_SOURCE_TYPE type, S_DSP_AUDIO_FRAME *pframe)
{
    int ret = -1; 
    char buf[DSP_AUDIO_FRAME_LEN] = {0};
    int outLen = 0;
    TUYA_CODEC_ID codectype = pframe->codectype;
    TUYA_AUDIO_SAMPLE_E sample = gdspAdapter.pstrDspInfo->audio[0].sample;
    S_DSP_AUDIO_FRAME frame = {0};
    S_DSP_AUDIO_FRAME *ptmpFrame = NULL;

    if (pframe == NULL || pframe->pbuf == NULL)
    {
        TB_LOGE("input is NULL\n");
        return -1;
    }

    if (pframe->sample != sample)
    {
        TB_LOGE("sample not support, %d %d\n", pframe->sample, sample);
        return -1;
    }

    switch (codectype)
    {
        case TUYA_CODEC_AUDIO_PCM:
            ptmpFrame = pframe;
            break;

        case TUYA_CODEC_AUDIO_G711U:
        case TUYA_CODEC_AUDIO_G711A:
            if (pframe->usedSize*2 > DSP_AUDIO_FRAME_LEN)
            {
                TB_LOGE("frame->usedSize:%d > %d\n", pframe->usedSize, DSP_AUDIO_FRAME_LEN);
                return -1;
            }

            ret = tuya_g711_decode(codectype, (unsigned short *)pframe->pbuf, pframe->usedSize, (unsigned char *)buf, (unsigned int *)&outLen);
            if (0 != ret)
            {
                TB_LOGE("tuya_g711_decode codectype:%d failed\n", codectype);
                return -1;
            }
            frame = *pframe;
            frame.pbuf = buf;
            frame.usedSize = outLen;
            frame.bufSize = DSP_AUDIO_FRAME_LEN;
            ptmpFrame = &frame;
            break;

        default:
            TB_LOGE("input is NULL\n");
            return -1;
    }

    ret = ty_dev_ao_put(ptmpFrame);
    if (0 != ret)
    {
        TB_LOGE("ty_dsp_audio_put frame failed type:%d\n", type);
        return -1;
    }

    return 0;
}


int ty_dsp_audio_play_wav(ty_dsp_audio_handle_s* phdl)
{
    int ret = -1; 
    char buffer[DSP_AUDIO_FRAME_LEN/4] = {0};
    ty_dsp_audio_wav_s *pwav = NULL;
    S_DSP_AUDIO_FRAME frame = {0};
    int count = phdl->count;  
    int flag = 0;

    pwav = ty_dsp_audio_wav_open(phdl->filename);
    if(pwav == NULL)
    {
        TB_LOGE("open failed\n");
        return -1;
    }

    if (count == 0)
    {
        flag = 1;
    }

    frame.bufSize = DSP_AUDIO_FRAME_LEN/4;
    frame.pbuf = buffer;
    frame.sample = pwav->format.samples_per_sec;

    if (pwav->format.compression_code == 1)
    {
        frame.codectype = TUYA_CODEC_AUDIO_PCM;
    }
    else if (pwav->format.compression_code == 6) 
    {
        frame.codectype = TUYA_CODEC_AUDIO_G711A;
    }
    else if (pwav->format.compression_code == 7) 
    {
        frame.codectype = TUYA_CODEC_AUDIO_G711U;
    }
    else
    {  
        TB_LOGE("wav not support\n");
        ty_dsp_audio_wav_close(&pwav);
        return -1;
    }
    TB_LOGI("codectype, sample:%d %d, bufSize:%d\n", frame.codectype, frame.sample, frame.bufSize);

    while((flag == 1) || (count > 0))
    {
        count --;

        fseek(pwav->fp, pwav->data_offset, SEEK_SET);
        while((frame.usedSize = fread(buffer, 1, sizeof(buffer), pwav->fp)) > 0)
        {
            ret = ty_dsp_audio_put(E_DSP_AUDIO_SOURCE_TYPE_FILE, &frame);
            if (0 != ret)
            {
                TB_LOGE("ty_dsp_audio_play_wav failed\n");
                ty_dsp_audio_wav_close(&pwav);
                return -1;
            }
            usleep(20* 1000);
            if (phdl->fileStop)
            {
                TB_LOGD("fileStop\n");
                ty_dsp_audio_wav_close(&pwav);
                return 0;
            }
        }
    }

    ty_dsp_audio_wav_close(&pwav);
    return 0;
}


int ty_dsp_audio_play_pcm(ty_dsp_audio_handle_s* phdl)
{
    int ret = 0;
    int flag = 0;
    S_DSP_AUDIO_FRAME frame = {0};
    unsigned char buffer[DSP_AUDIO_FRAME_LEN/2] = {0};
    FILE *fp = NULL;
    int count = phdl->count;  

    fp = fopen(phdl->filename, "rb");
    if (NULL == fp) 
    {
         TB_LOGE("open audio file %s fail\n", phdl->filename);
         return -1;
    }

    if (count == 0)
    {
        flag = 1;
    }

    frame.bufSize = DSP_AUDIO_FRAME_LEN/2;
    frame.pbuf = (char *)buffer;
    frame.codectype = TUYA_CODEC_AUDIO_PCM;
    frame.sample = gdspAdapter.pstrDspInfo->audio[0].sample;   /* 与配置的音频采样率一致 */
    
    while((flag == 1) || (count > 0))
    {
        count --;
        while((frame.usedSize = fread(buffer, 1, sizeof(buffer), fp)) > 0)
        {
            ret = ty_dsp_audio_put(E_DSP_AUDIO_SOURCE_TYPE_FILE, &frame);
            if (0 != ret)
            {
                TB_LOGE("ty_dsp_audio_play_wav failed\n");
                fclose(fp);
                return -1;
            }
            usleep(20* 1000);
            if (phdl->fileStop)
            {
                TB_LOGD("fileStop\n");
                fclose(fp);
                return 0;
            }
        }

        fseek(fp, 0, SEEK_SET);
    }

    fclose(fp);
    return 0;
}


void *ty_dsp_audio_play_file_proc(void *arg)
{
    int ret = 0;
    ty_dsp_audio_handle_s* phdl = &gdspAdapter.audioHandle;
    E_DSP_AUDIO_SOURCE_TYPE src_type = E_DSP_AUDIO_SOURCE_TYPE_FILE;

    TB_LOGI("play %s, type:%d, count:%d\n", phdl->filename, phdl->fileType, phdl->count);

    ret = ty_dsp_audio_set(E_DSP_MEDIA_AUDIO, E_DSP_SET_AUDIO_SPEAKER_START, &src_type);
    if (0 != ret)
    {
        TB_LOGE("ty_dsp_audio_set stop failed\n");
        phdl->start = 0;
        return NULL;
    }

    switch (phdl->fileType)
    {
        case E_DSP_AUDIO_FILE_PCM:
            ret = ty_dsp_audio_play_pcm(phdl);
            if (0 != ret)
            {
                TB_LOGE("ty_dsp_audio_play_pcm failed\n");
                goto err;
            }
            break;

        case E_DSP_AUDIO_FILE_WAV:
            ret = ty_dsp_audio_play_wav(phdl);
            if (0 != ret)
            {
                TB_LOGE("ty_dsp_audio_play_wav failed\n");
                goto err;
            }
            break;

        default:
            TB_LOGE("Not support filetype:%d\n", phdl->fileType);
            goto err;
    }
    usleep(1000*1000);

    ret = ty_dsp_audio_set(E_DSP_MEDIA_AUDIO, E_DSP_SET_AUDIO_SPEAKER_STOP, &src_type);
    if (0 != ret)
    {
        TB_LOGE("ty_dsp_audio_set stop failed\n");
        goto err;
    }

err:
    phdl->start = 0;
    return NULL;
}


/***********************************************************
*  函数名: ty_dsp_audio_play_file
*  功能：播放文件接口
*  参数:  const char* fileName, 文件路径
       E_DSP_AUDIO_FILE_TYPE fileType, 文件格式类型
       int count, 播放次数，0，代表一直循环播放
       int timeOut， 播放阻塞时间单位，ms
*      
*  返回值: <0 失败，=0 成功
***********************************************************/
int ty_dsp_audio_play_file(const char* fileName, E_DSP_AUDIO_FILE_TYPE fileType, int count, int timeOut)
{
    int ret = 0;
    pthread_attr_t attr = {0};

    if (fileName == NULL) 
    {
        TB_LOGE("the file is null\n");
        return -1;
    }

    while(gdspAdapter.audioHandle.isInit == 0)
    {
        usleep(100*1000);
        TB_LOGI("wait dsp init:%d\n", gdspAdapter.audioHandle.isInit);
    }

    ty_dsp_audio_handle_s* phdl = &gdspAdapter.audioHandle;
    
    while(phdl->start && timeOut > 0)
    {
        TB_LOGD("The last play is not over, timeOut:%d\n", timeOut);
        usleep(200*1000);
        timeOut = timeOut - 200;
    }

    if(phdl->start)
    {
        TB_LOGE("speaker thread have start...\n");
        return -1;
    }

    strncpy(phdl->filename, fileName, 127);
    phdl->fileType = fileType;
    phdl->count = count;
    phdl->fileStop = 0;
    phdl->start = 1;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, DSP_AUDIO_THREAD_STACK_SIZE);
    ret = pthread_create(&phdl->stid, &attr, ty_dsp_audio_play_file_proc, NULL);
    if(ret != 0) 
    {
       TB_LOGE(" tycam_hisi_speaker_play_file end___,create fail!  ret:%d\n", ret);
       phdl->start = 0;
       return -1;
    }

    pthread_join(phdl->stid,NULL);

    return 0;
}


/***********************************************************
*  函数名: ty_dsp_audio_stop_play_file
*  功能：停止播放文件
*  参数:  无
*      
*  返回值: <0 失败，=0 成功
***********************************************************/
int ty_dsp_audio_stop_play_file(void)
{
    TB_LOGI("stop to play file\n");
    ty_dsp_audio_handle_s* phdl = &gdspAdapter.audioHandle;
    phdl->fileStop = 1;
    return 0;
}

/***********************************************************
*  函数名: ty_dsp_audio_get_play_file_status
*  功能：获取文件播放状态
*  参数:  无
*      
*  返回值: 0未播放文件，1正在播放文件
***********************************************************/
int ty_dsp_audio_get_play_file_status(void)
{
    ty_dsp_audio_handle_s* phdl = &gdspAdapter.audioHandle;
    return phdl->start;
}

/***********************************************************
*  函数名: ty_dsp_uninit
*  功能：dsp去初始化
*  参数:  无
*      
*  返回值: <0 失败，=0 成功
***********************************************************/
int ty_dsp_uninit(void)
{
    return 0;
}

OPERATE_RET tuya_ipc_get_utc_time(OUT TIME_T *time_utc)
{
        *time_utc = time(NULL);
        return 0;
}


OPERATE_RET tuya_ipc_check_in_dls(IN TIME_T time_utc, OUT BOOL_T * pIsDls)
{        
        * pIsDls = false;
        return 0;
}
#endif

