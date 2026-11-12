/* 
 *  @copyright Copyright (c) tuya.inc 2021
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <inttypes.h>
#include <pthread.h>
#include "rtsp.h"
#include "rtsp_server.h"
#include "product_test_media.h"
#include "ptal_media.h"
#include "tuya_g711_utils.h"
#include "product_test_config.h"
#include "product_test_camera.h"
#include "list.h"
#include "tb_log.h"

#define MAX_FIFO (256)

typedef struct {
    int type;
    char *buf;
    uint32_t len;
    uint64_t pts;
}FIFO_T;


typedef struct {
    char *buf;
    int totals;
    int tails;

    int push_idx;
    int pop_idx;
    int fifo_max;
    FIFO_T fifo[MAX_FIFO]; 

    int new_iframe_idx;
    int init;

}FIFO_HEAD_T;


static FIFO_HEAD_T sg_video_fifo_head;
static FIFO_HEAD_T sg_mic_fifo_head;
static pthread_t sg_video_thread;
static pthread_t sg_mic_thread;
static int sg_video_exit = 1;
static int sg_mic_exit = 1;

static int sg_media_init_flag = 0;
static int sg_rtsp_first_frame = 1;
static int sg_product_test_media_rtsp_init = 0;
static MEDIA_CONFIG_T sg_media_config = {0};
static int sg_media_config_flag = 0;
static pthread_mutex_t sg_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t sg_fifo = PTHREAD_MUTEX_INITIALIZER;

/** 0,sync,1,video async,2,audio async,3,async*/
static int sg_rtsp_mode =  0;
static int sg_delay_us = 100000;
static int sg_fifo_depth = 250;


int product_test_g711_to_pcm(unsigned short *src,unsigned int src_len, unsigned char *dst, unsigned int *dst_len)
{
    return tuya_g711_decode(TUYA_G711_MU_LAW,src,src_len,dst,dst_len);
}

int product_test_pcm_to_g711(unsigned short *src, unsigned int src_len, unsigned char *dst, unsigned int *dst_len)
{
    return tuya_g711_encode(TUYA_G711_MU_LAW,src,src_len,dst,dst_len);
}

static int __product_test_config_read_video_config(MEDIA_CONFIG_T *config) 
{
    char *dev,*key;
    int value;
    int num;

    dev = CONFIG_DEVICE_MEDIA_CFG;
    key = CONFIG_KEY_VIDEO_NUM;

    if(product_test_config_read_int(dev,key,&num)) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
	return -1;
    }

    if(num >MAX_VIDEO) {
        TB_LOGE("invalid parm:%d",num);
        return -1;	
    }

    key = CONFIG_KEY_VIDEO_FPS;
    if(product_test_config_read_int(dev,key,&value)) {
	    TB_LOGE("device:%s parm:%s fail",dev,key);
	    return -1;
    }
    config->video[num].fps = value;

    key = CONFIG_KEY_VIDEO_GOP;
    if(product_test_config_read_int(dev,key,&value)) {
	TB_LOGE("device:%s parm:%s fail",dev,key);
	return -1;
    }
    config->video[num].gop = value;

    key = CONFIG_KEY_VIDEO_BITRATE;
    if(product_test_config_read_int(dev,key,&value)) {
	    TB_LOGE("device:%s parm:%s fail",dev,key);
	    return -1;
    }
    config->video[num].bitrate = (VIDEO_BITRATE_E)value;

    key = CONFIG_KEY_VIDEO_WIDTH;
    if(product_test_config_read_int(dev,key,&value)) {
	    TB_LOGE("device:%s parm:%s fail",dev,key);
	    return -1;
    }
    config->video[num].width = value;

    key = CONFIG_KEY_VIDEO_HEIGHT;
    if(product_test_config_read_int(dev,key,&value)) {
	    TB_LOGE("device:%s parm:%s fail",dev,key);
	    return -1;
    }
    config->video[num].height = value;

    key = CONFIG_KEY_VIDEO_CODECTYPE;
    if(product_test_config_read_int(dev,key,&value)) {
	    TB_LOGE("device:%s parm:%s fail",dev,key);
	    return -1;
    }
    config->video[num].codectype = (MEDIA_CODEC_ID_E)value;

    key = CONFIG_KEY_VIDEO_MINQP;
    if(product_test_config_read_int(dev,key,&value)) {
	    TB_LOGE("device:%s parm:%s fail",dev,key);
	    return -1;
    }
    config->video[num].minQp = value;

    key = CONFIG_KEY_VIDEO_MAXQP;
    if(product_test_config_read_int(dev,key,&value)) {
	    TB_LOGE("device:%s parm:%s fail",dev,key);
	    return -1;
    }
    config->video[num].maxQp = value;
    config->video[num].media_type = (VIDEO_TYPE_E)num;
    config->video[num].channel_enable = 1;

    return 0;
}

static int __product_test_config_read_audio_config(MEDIA_CONFIG_T *config) 
{
    char *dev,*key;
    int num;
    int value;

    dev = CONFIG_DEVICE_MEDIA_CFG;
    key = CONFIG_KEY_AUDIO_NUM;

    if(product_test_config_read_int(dev,key,&num)) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
	return -1;
    }

    if(num >MAX_AUDIO) {
        TB_LOGE("invalid parm:%d",num);
        return -1;	
    }

    key = CONFIG_KEY_AUDIO_SAMPLE; 

    if(product_test_config_read_int(dev,key,&value)) {
	TB_LOGE("device:%s parm:%s fail",dev,key);
	return -1;
    }
    config->audio[num].sample = (AUDIO_SAMPLE_E)value;

    key =  CONFIG_KEY_AUDIO_DATABITS;
    if(product_test_config_read_int(dev,key,&value)) {
	TB_LOGE("device:%s parm:%s fail",dev,key);
	return -1;
    }
    config->audio[num].datebits = (AUDIO_DATABITS_E)value;

    key = CONFIG_KEY_AUDIO_CHANNEL;
    if(product_test_config_read_int(dev,key,&value)) {
	TB_LOGE("device:%s parm:%s fail",dev,key);
	return -1;
    }
    config->audio[num].channel = (AUDIO_CHANNEL_E)value;


    key = CONFIG_KEY_AUDIO_CODECTYPE;
    if(product_test_config_read_int(dev,key,&value)) {
	TB_LOGE("device:%s parm:%s fail",dev,key);
	return -1;
    }
    config->audio[num].codectype = (MEDIA_CODEC_ID_E)value;

    key = CONFIG_KEY_AUDIO_FPS;
    if(product_test_config_read_int(dev,key,&value)) {
	TB_LOGE("device:%s parm:%s fail",dev,key);
	return -1;
    }
    config->audio[num].fps = value;

    key = CONFIG_KEY_MIC_VOLUME;
    if(product_test_config_read_int(dev,key,&value)) {
	TB_LOGE("device:%s parm:%s fail",dev,key);
	return -1;
    }
    config->audio[num].mic_volume = value;

    key = CONFIG_KEY_SPK_VOLUME;
    if(product_test_config_read_int(dev,key,&value)) {
	TB_LOGE("device:%s parm:%s fail",dev,key);
	return -1;
    }
    config->audio[num].spk_volume = value;

    config->audio[num].media_type = (AUDIO_TYPE_E)num;
    config->audio[num].channel_enable = 1;

    return 0;
}

static int __product_test_config_read_media_config(MEDIA_CONFIG_T *config)
{
    char *dev,*key;
    int enable;

    if(config == NULL) {
        TB_LOGE("invalid parm");   
        return -1;    
    }

    dev = CONFIG_DEVICE_MEDIA_CFG;
    key = CONFIG_KEY_ENABLE;

    if(product_test_config_read_int(dev,key,&enable) || enable != 1) {
	TB_LOGE("device:%s parm:%s fail",dev,key);
	return-1;
    }

    __product_test_config_read_video_config(config);
    __product_test_config_read_audio_config(config);

    return 0;

}

int product_test_init_media_config(MEDIA_CONFIG_T *config)
{
   if(config) {
	memcpy(&sg_media_config,config,sizeof(MEDIA_CONFIG_T));
        sg_media_config_flag = 1;
   }
   return 0;
}

int product_test_media_init(void)
{
    int iret;
    MEDIA_CONFIG_T *p_config = NULL;
    CAMERA_MIRROR_FLIP_TYPE_T mirror_flip;

 // TB_LOGD("sg_media_init_flag= %d",sg_media_init_flag);

    pthread_mutex_lock(&sg_mutex);

    if(sg_media_init_flag == 1 ) {
    	pthread_mutex_unlock(&sg_mutex);
        return 0;
    }

    TB_LOGD();


    if(sg_media_config_flag == 1) {
   	p_config = &sg_media_config; 
    }

    if(p_config == NULL) {
	memset(&sg_media_config,0,sizeof(MEDIA_CONFIG_T));
   	iret = __product_test_config_read_media_config(&sg_media_config);

	if(iret == 0) {
            p_config = &sg_media_config;
	}
    }
    iret = ptal_media_init(p_config);


    if (iret != 0) {
        TB_LOGE("ptal_media_init fail");
    	pthread_mutex_unlock(&sg_mutex);
        return -1;
    }

    if(product_test_camera_get_mirror_flip(&mirror_flip) == 0) {
        ptal_video_set_mirror_flip((int)mirror_flip);
    }

    sg_media_init_flag  = 1;

    pthread_mutex_unlock(&sg_mutex);

  // TB_LOGD("sg_media_init_flag= %d",sg_media_init_flag);

    return 0;
}

int product_test_speaker_start_play_pcm(char *file_path)
{
    TB_LOGD("%s",file_path);

    if(product_test_media_init()) {
        TB_LOGE("media init fail");
        return -1;
    }

    return ptal_speaker_play_file(file_path,E_AUDIO_FILE_PCM,1,0);
}


int product_test_speaker_start_play_wav(char *file_path)
{
    TB_LOGD("%s",file_path);

    if(product_test_media_init()) {
        TB_LOGE("media init fail");
        return -1;
    }

    return ptal_speaker_play_file(file_path,E_AUDIO_FILE_WAV,1,0);
}


int product_test_speaker_stop(void)
{
    return ptal_speaker_stop();
}

int product_test_media_load_day_isp_file(void)
{
    return  ptal_load_day_isp_file();
}

int product_test_media_load_night_isp_file(void)
{
    return  ptal_load_night_isp_file();
}

int product_test_mic_get_sample_rate(int *rate) 
{
    return ptal_mic_get_audio_rate(rate); 
}

int product_test_audio_set_volume(AUDIO_DEV_TYPE_E type,int devid, int channel, unsigned int value)
{
    int iret = -1;

    if(product_test_media_init()) {
        TB_LOGE("media init fail");
        return -1;
    }

    if(type == E_AUDIO_SPEAKER) {
        iret = ptal_speaker_set_volume(devid, channel, value);
    
    }else if(type == E_AUDIO_MIC) {

        iret = ptal_mic_set_volume(devid, channel, value);
    }

    return iret;

}

int product_test_audio_get_volume(AUDIO_DEV_TYPE_E type, int devid, int channel, unsigned int *value)
{
    int iret = -1;

    if(product_test_media_init()) {
        TB_LOGE("media init fail");
        return -1;
    }

    if(type == E_AUDIO_SPEAKER) {
        iret = ptal_speaker_get_volume(devid, channel, value);
    }else if(type == E_AUDIO_MIC) {
        iret = ptal_mic_get_volume(devid, channel, value);
    }

    return iret;

}

int product_test_video_get_frame(VIDEO_TYPE_E video_type,int *frame_type,char **buf,int *plen,uint64_t *pts)
{
    if(product_test_media_init()) {
        TB_LOGE("media init fail");
        return -1;
    }

    return ptal_video_get_frame(video_type,frame_type,buf,plen,pts);
}


int product_test_audio_get_frame(char **buf,int *plen,uint64_t *pts)
{
    if(product_test_media_init()) {
        TB_LOGE("media init fail");
        return -1;
    }

    return ptal_audio_get_frame(buf,plen,pts);
}

int product_test_audio_put_frame(char *buf,int plen) 
{

    if(product_test_media_init()) {
        TB_LOGE("media init fail");
        return -1;
    }

    return ptal_audio_put_frame(buf,plen);
}

static int __product_test_medio_fifo_init(FIFO_HEAD_T *head,int frame_size,int frame_counts) 
{
    char *buf;
    int buf_size;

    if(head == NULL ||  frame_size <= 0 || frame_counts <= 0 ) {
        TB_LOGE("invalid parm");
        return -1;	
    }

    if(head->init) {
        return 0; 
    }

    buf_size = frame_size *frame_counts;

    TB_LOGD("total buffer size:0x%x(%d) bytes",buf_size,buf_size);


    if((buf = (char*)malloc(buf_size)) == NULL) {
        TB_LOGE("malloc %d bytes fail",buf_size); 
    }

    memset(head,0,sizeof(FIFO_HEAD_T));

    head->buf = buf;
    head->totals = buf_size;
    head->tails = 0;
    head->push_idx = 0;
    head->pop_idx = 0;
    head->init = 1;
    head->fifo_max = MAX_FIFO;


    return 0;
}

static int __product_test_medio_fifo_unint(FIFO_HEAD_T *head)
{
   if(head == NULL) {
      TB_LOGE("invalid parm");
      return -1; 
   }

   if(head->init) {  
       free(head->buf);
       head->init = 0;
   }

   return 0;
}

#if 0

static int dubug_save_file(char *filename,char *buf,int len) 
{
    static FILE * pFile = NULL;
    
    if(filename == NULL || buf == NULL || len == 0) {
        TB_LOGE("invalid param"); 
        return -1; 
    }

    if(pFile == NULL) {
         pFile = fopen(filename,"wb");

	 if(pFile == NULL) {
	     TB_LOGE("file open fail"); 
	     return -1;
	 }
    }

    if(pFile) {

         fwrite(buf,1,len,pFile); 
    }
    
    return 0;
    
}
#endif


static int __product_test_media_fifo_push(FIFO_HEAD_T *head,char *buf, uint32_t len,uint64_t pts,int type)
{

    if(head == NULL || buf == NULL || len <= 0 || !head->init) {
        TB_LOGE("invalid param");
        return -1;	
    }


    if(head->tails + len <= head->totals) {
        head->fifo[head->push_idx].buf = head->buf + head->tails;
	head->tails += len;

    }else {
        head->fifo[head->push_idx].buf = head->buf;
        head->tails = 0;
    }

    head->fifo[head->push_idx].type = type;
    head->fifo[head->push_idx].pts = pts; 
    head->fifo[head->push_idx].len = len;
    memcpy(head->fifo[head->push_idx].buf,buf,len);

// TB_LOGD("push_idx:%d,buf:%p,size:%d",head->push_idx,head->fifo[head->push_idx].buf,head->fifo[head->push_idx].len);

    head->push_idx ++;

    if(head->push_idx >= head->fifo_max) {
        head->push_idx = 0;
    }


    return 0;
}

static int __product_test_media_fifo_pop(FIFO_HEAD_T *head,char **buf,uint32_t *len,uint64_t *pts,int *type)
{

    if(head == NULL || buf == NULL || len == NULL) {
        TB_LOGE("invalid parm");
        return -1;	
    }

    if(head->pop_idx == head->push_idx) {
       // TB_LOGW("fifo is empty");
        return -1;
    }

    *buf = head->fifo[head->pop_idx].buf;
    *len = head->fifo[head->pop_idx].len;
    *pts = head->fifo[head->pop_idx].pts;
    *type = head->fifo[head->pop_idx].type;

  //TB_LOGD("pop_idx:%d,buf:%p,size:%d",head->pop_idx,*buf,*len);

    head->pop_idx ++;

    if(head->pop_idx >= head->fifo_max) {
        head->pop_idx = 0;     
    }



    return 0;
}



static int __rtsp_user_start(void)
{
    return 0;
}

static int __rtsp_user_stop(int user_id)
{
    TB_LOGD();
    sg_rtsp_first_frame  = 1;
    return 0;
}

static int __get_sample_rate(RTSP_MEDIA_TYPE_E type)
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

static RTP_CODEC_E __get_codec_main(RTSP_MEDIA_TYPE_E type)
{
    RTP_CODEC_E codec;

    switch (type){
        case RTSP_MEDIA_TYPE_VIDEO:

            if(ptal_video_code_type(0) == E_CODEC_VIDEO_H264) {
               codec = RTP_CODEC_H264;
	    }else {
               codec = RTP_CODEC_H265;
	    }

//	    TB_LOGD("==codec value:%d==",codec);

	    return codec;

        case RTSP_MEDIA_TYPE_AUDIO:
            return RTP_CODEC_G711U;
        default:
            return 0;
    }
}


static int __rtsp_get_frame_main(int user_id, RTSP_MEDIA_TYPE_E type, char** buf, int *plen, uint64_t *pts)
{
    int iret=0;
    int v_type;

    if (type == RTSP_MEDIA_TYPE_VIDEO) {
	if(sg_rtsp_mode == 0 || sg_rtsp_mode == 2) {
            do{
                if((iret = product_test_video_get_frame(E_MEDIA_VIDEO_MAIN,&v_type,buf,plen,pts)) == 0) {
                    if(sg_rtsp_first_frame == 1 && v_type == E_VIDEO_FRAME_I) {
			//TB_LOGD();
                        sg_rtsp_first_frame = 0;
                    }
                }

	//	TB_LOGD("iret:%d,len:%d\n",iret,*plen);
                usleep(10000);
            }while(sg_rtsp_first_frame == 1);
	 }else {

	    do {
                pthread_mutex_lock(&sg_fifo);
                iret = __product_test_media_fifo_pop(&sg_video_fifo_head,buf,(uint32_t *)plen,pts,&v_type);
                pthread_mutex_unlock(&sg_fifo);

	        if(iret == 0 && sg_rtsp_first_frame == 1 && v_type == E_VIDEO_FRAME_I ){
                    sg_rtsp_first_frame = 0;
	        }

		if(iret) {
	            usleep(10000);	
		}

	    }while(sg_rtsp_first_frame == 1);
	 }

//	if(iret == 0) {
//          TB_LOGD("iret:%d,len:%d\n",iret,*plen);
//          dubug_save_file("test.h264",*buf,*plen);
//	}
    }else {

	if(sg_rtsp_mode == 0 || sg_rtsp_mode == 1) {
            iret = product_test_audio_get_frame(buf,plen,pts);
	}else {
            iret = __product_test_media_fifo_pop(&sg_mic_fifo_head,buf,(uint32_t *)plen,pts,&v_type);

	}
    }

    return iret;
}

static void* __video_task(void *arg) 
{

    TB_LOGD("__video_task");
    pthread_detach(pthread_self());

    __product_test_medio_fifo_init(&sg_video_fifo_head,10*1024,sg_fifo_depth);

    sg_video_exit = 0;

    while(!sg_video_exit) {
        char* buf;
	int len;
	uint64_t pts;
        int type;

        if(product_test_video_get_frame(E_MEDIA_VIDEO_MAIN,&type,&buf,&len,&pts) == 0) {
            // dubug_save_file("test.h265",buf,len);
               pthread_mutex_lock(&sg_fifo);
	       __product_test_media_fifo_push(&sg_video_fifo_head,buf,len,pts,type);
               pthread_mutex_unlock(&sg_fifo);
        }

        usleep(sg_delay_us);
    }

    __product_test_medio_fifo_unint(&sg_video_fifo_head);

    return NULL;
}

static void *__mic_task(void *arg)
{

    TB_LOGD("__mic_task");

    pthread_detach(pthread_self());

    __product_test_medio_fifo_init(&sg_mic_fifo_head,1024,sg_fifo_depth);

    sg_mic_exit = 0;

    while(!sg_mic_exit) {

        char* buf;
	int len;
	uint64_t pts;

        if(product_test_audio_get_frame(&buf,&len,&pts) == 0) {
	    __product_test_media_fifo_push(&sg_mic_fifo_head,buf,len,pts,E_AUDIO_FRAME_MIC);
	}

        usleep(10000);
    }

    __product_test_medio_fifo_unint(&sg_mic_fifo_head);

    return NULL;
}


static int __product_test_config_get_camera_mode(int* mode)
{

    char *dev,*key;
    int enable;

    dev = CONFIG_DEVICE_CAMERA;
    key = CONFIG_KEY_ENABLE;

    if(product_test_config_read_int(dev,key,&enable) || enable != 1) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return-1;
    }

    key = CONFIG_KEY_MODE;

    if(product_test_config_read_int(dev,key,mode)) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return -1;
    }

    return 0;

}

static int __product_test_config_get_camera_param(int* delay,int* depth) 
{
    char *dev,*key;
    int enable;

    dev = CONFIG_DEVICE_CAMERA;
    key = CONFIG_KEY_ENABLE;

    if(product_test_config_read_int(dev,key,&enable) || enable != 1) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return-1;
    }

    key = CONFIG_KEY_DELAY;

    if(product_test_config_read_int(dev,key,delay)) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return -1;
    }

    key = CONFIG_KEY_DEPTH;

    if(product_test_config_read_int(dev,key,depth)) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return -1;
    }

    return 0;
}


static int __product_test_media_capture_init(void)
{
     int iret;
     int mode;
     int delay;
     int depth;

     TB_LOGD();

     if(__product_test_config_get_camera_mode(&mode) == 0 && mode >= 0 && mode <= 3) {
         sg_rtsp_mode =  mode;     
     
     }

     TB_LOGD("== current mode :%d ==\n",sg_rtsp_mode);

     if (sg_rtsp_mode == 0) {
         return 0; 
     }

     if(__product_test_config_get_camera_param(&delay,&depth) == 0) {
     
	 if(delay >= 0) {
             sg_delay_us = delay; 
	 }

	 if(depth >= 0) {
	     sg_fifo_depth = depth; 
	 }
     }

     TB_LOGD("== current param: delay (%d),depth (%d) ==\n",sg_delay_us,sg_fifo_depth);

     if(sg_rtsp_mode == 1 || sg_rtsp_mode == 3) {
         pthread_attr_t attr;
         struct sched_param param;

     	if(pthread_attr_init(&attr) != 0) {
             TB_LOGE("Attribute create failed\n");
	     return -1;
        }

        memset(&param,0,sizeof(struct sched_param));
        pthread_attr_setschedpolicy(&attr, SCHED_RR);
        param.sched_priority  = 81;
        pthread_attr_setschedparam(&attr,   &param);
        pthread_attr_setinheritsched(&attr,PTHREAD_EXPLICIT_SCHED);
        iret = pthread_create(&sg_video_thread,&attr,__video_task,NULL);


        if(iret != 0) {
            TB_LOGE("pthread create video fail");
            (void)pthread_attr_destroy(&attr);
	    return -1;
       }

       (void)pthread_attr_destroy(&attr);
    }

    if(sg_rtsp_mode ==2  || sg_rtsp_mode == 3) {
        if((iret = pthread_create(&sg_mic_thread,NULL, __mic_task, NULL)) != 0) {
         TB_LOGE("pthread create mic fail"); 
	 return -1;
        }
    }

     sleep(1);

     return 0;
}


static int product_test_media_capture_uninit(void) 
{
    sg_video_exit = 0;
    sg_mic_exit = 0;
    return 0;
}


int product_test_media_server_start(void)
{
    if(sg_product_test_media_rtsp_init) {
        return 0;
    }

    __product_test_media_capture_init(); 

    rtsp_stream_src_t src[1] = {0};

    src[0].get_codec = __get_codec_main;
    src[0].get_frame = __rtsp_get_frame_main;
    src[0].get_sample_rate = __get_sample_rate;
    src[0].start = __rtsp_user_start;
    src[0].stop = __rtsp_user_stop;


    rtsp_server_start(NULL, NULL);
    rtsp_server_register_stream_src(src); 
    sg_product_test_media_rtsp_init  = 1;
    return 0;
}    

int product_test_media_server_stop(void)
{
    int iret = 0;
    if(sg_product_test_media_rtsp_init) {
        iret = rtsp_server_stop();
        product_test_media_capture_uninit(); 
        sg_product_test_media_rtsp_init = 0;
    }

    return iret;
}

int proudct_test_media_set_color_to_gray(int mode) 
{
    return ptal_isp_set_color_to_gray(mode); 
}

/**mode =1,day,night 2 **/

int product_test_media_get_virtual_adc(int mode, uint32_t *val) 
{
     int iret;
     *val  = 0;
     iret = ptal_get_virtual_adc(mode,val);

     if(iret == 0) {
         TB_LOGD("mode %d:get val:%d",mode,*val);
     }

     return iret;
}

