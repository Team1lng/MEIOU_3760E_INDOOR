/*
 * @copyright Copyright (c) tuya inc 2021
 */
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/statfs.h>  
#include "product_test_media.h"
#include "product_test_led.h"
#include "product_test_ircut.h"
#include "product_test_config.h"
#include "product_test_utils.h"
#include "product_test_camera.h"
#include "product_test_adc.h"
#include "product_test_camera.h"
#include "tb_log.h"

#define CAMERA_TYPE_KEY "camera_type"
#define CAMERA_MIRROR_FLIP_KEY "camera_mirror_flip"

#define MAX_VERSION 32
#define MAX_CONFIG 128

#define CAMERA_DEFAULT_VERSION "20211102.0.1"
#define CAMERA_DEFAULT_CONFIG  "/tuya/app/skyeye/resource/isp/isp.conf"
#define CAMERA_DEFAULT_TYPE "ssc2335"

static char sg_config[MAX_CONFIG] = {0};
static char sg_version[MAX_VERSION] = {0};

static int sg_day2night = 0;
static int sg_night2day = 0;
static int sg_reverse = 0;
static int sg_mode = 0;

static int sg_day_night_run_flag = 0;

static int __product_test_config_camera_get_camera_flip_mirror(CAMERA_MIRROR_FLIP_TYPE_T *mirror_flip)
{
    char *dev,*key;
    int enable,val;

    if(mirror_flip == NULL) {
        TB_LOGE("invalid param");
        return -1;	
    }

    *mirror_flip = 0;

    dev = CONFIG_DEVICE_CAMERA;
    key = CONFIG_KEY_ENABLE;

    if(product_test_config_read_int(dev,key,&enable) || enable != 1) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return-1;
    }

    key = CONFIG_KEY_SENSOR_MIRROR;

    if(product_test_config_read_int(dev,key,&val) == 0 && val == 1) {
	    *mirror_flip |= E_CAMERA_MF_MIRROR;
    }

    key = CONFIG_KEY_SENSOR_FLIP;

    if(product_test_config_read_int(dev,key,&val) == 0 && val == 1) {
	    *mirror_flip |= E_CAMERA_MF_FLIP;
    }

    TB_LOGD(" mirror_flip:%d",*mirror_flip);

    return 0;
}

int product_test_camera_get_mirror_flip(CAMERA_MIRROR_FLIP_TYPE_T *mirror_flip) 
{
    int val;

    if(mirror_flip == NULL) {
        TB_LOGE("invalid param");
        return -1;	
    }

    if(product_test_utils_read_int(CAMERA_MIRROR_FLIP_KEY,&val) == 0 && val >0 && val < E_CAMERA_MF_MAX) {
    
	*mirror_flip = (CAMERA_MIRROR_FLIP_TYPE_T) val;
	return 0;
    }

    return  __product_test_config_camera_get_camera_flip_mirror(mirror_flip);
}

int product_test_camera_set_mirror_flip(CAMERA_MIRROR_FLIP_TYPE_T mirror_flip)
{
    return product_test_utils_write_int(CAMERA_MIRROR_FLIP_KEY,(int)mirror_flip,0);
}

static int __product_test_config_camera_get_type(char *type,int len) 
{
    char *dev,*key;
    int enable;

    dev =  CONFIG_DEVICE_CAMERA;
    key = CONFIG_KEY_ENABLE;

    if(product_test_config_read_int(dev,key,&enable) || enable != 1) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return-1;
    }

    key = CONFIG_KEY_SENSOR_TYPE;

    if(product_test_config_read_string(dev,key,type,len)) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return -1;
    }
    return 0;
}

int product_test_camera_get_type(char* type,int len) 
{
    int iret;

    if(type == NULL || len == 0) {
        TB_LOGE("invalid parm"); 
	return -1;
    } 

    iret = product_test_utils_read_string(CAMERA_TYPE_KEY,type);

    if(iret == 0 && strlen(type) != 0) {
        return 0; 
    }

    iret = __product_test_config_camera_get_type(type,len);

    if(iret == 0 || strlen(type) != 0 ) {
        return 0;
    }

    strncpy(type,CAMERA_DEFAULT_TYPE,len -1);

//  TB_LOGD("type:%s",type);

    return iret;
}


int product_test_camera_set_type(char *type)
{
    return product_test_utils_write_string(CAMERA_TYPE_KEY,type,0);
}




int product_test_camera_set_version(char *version)
{
    if(version == NULL || strlen(version) <= 0 || strlen(version) > MAX_VERSION -1) {
        TB_LOGE("invalid parm\n");
        return -1; 
    }

    memset(sg_version,0,MAX_VERSION);
    strcpy(sg_version,version);

    return 0;
}

static int __product_test_config_camera_version(char *version,int len) 
{
    char *dev,*key;
    int enable;

    dev =  CONFIG_DEVICE_CAMERA;
    key = CONFIG_KEY_ENABLE;

    if(product_test_config_read_int(dev,key,&enable) || enable != 1) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return-1;
    }

    key = CONFIG_KEY_SENSOR_VERSION;

    if(product_test_config_read_string(dev,key,version,len )) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return -1;
    }

    return 0;
}

int product_test_camera_get_version(char *version ,int len) 
{
    int iret =  -1;

    if(version == NULL || len <= 0) {
        TB_LOGE("invalid param\n");
        return -1;	
    }

   iret = __product_test_config_camera_version(version,len);

   
   if(iret == 0 &&  strlen(version)) {
       return 0;
   }

   if(strlen(sg_version)) {
       strncpy(version,sg_version,len);
   }else {
       strncpy(version,CAMERA_DEFAULT_VERSION,len);
   }
   return 0;
}

int product_test_camera_set_config(char *config)
{
    if(config == NULL || strlen(config) <= 0 || strlen(config) > MAX_CONFIG -1) {
        TB_LOGE("invalid parm\n");
        return -1; 
    }

    memset(sg_config,0,MAX_CONFIG);
    strcpy(sg_config,config);

    return 0;
}

static int __product_test_config_camera_config(char *config,int len) 
{
    char *dev,*key;
    int enable;

    dev =  CONFIG_DEVICE_CAMERA;
    key = CONFIG_KEY_ENABLE;

    if(product_test_config_read_int(dev,key,&enable) || enable != 1) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return-1;
    }

    key = CONFIG_KEY_SENSOR_CONFIG;

    if(product_test_config_read_string(dev,key,config,len )) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return -1;
    }

    return 0;
}

int product_test_camera_get_config(char *config,int len)
{
    int iret =  -1;

    if(config == NULL || len <= 0) {
        TB_LOGE("invalid param\n");
        return -1;
    }

   iret = __product_test_config_camera_config(config,len);

   if(iret == 0 &&  strlen(config)) {
       return 0;
   }

   if(strlen(sg_version)) {
       strncpy(config,sg_config,len);
   }else {
       strncpy(config,CAMERA_DEFAULT_CONFIG,len);
   }

   return 0;
}


static void* __product_day_night_switch(void *arg)
{
    int type = 0;
    int iret;

    int pre_mode = 1, cur_mode = 1;

    pthread_detach(pthread_self());

    sg_day_night_run_flag = 1;


    if(product_test_adc_get_type(0,&type) || type < 0 || type > 1) {
        type = 1;
    }
    

    while(sg_day_night_run_flag) {
	int i;
	int value;
	int cnts;
	int total = 0;

	cnts = sg_mode?10:1;

	for(i = 0;i< cnts;i++) {
	    if(type == 1) {
                iret = product_test_adc_get_value(0,(uint32_t *)&value);
	        value = 1024 - value;
	    }else {
                iret = product_test_media_get_virtual_adc(pre_mode,(uint32_t *)&value);
	    }

	    if(iret == 0) {
	        total += value; 
	    }else {
	        break; 
	    }

	    usleep(200*1000);
	 }

	 value = total/cnts;

        if(iret == 0) {
	    if(sg_mode == 1) {
	        if(value <(sg_day2night + sg_night2day)/2) {
		    cur_mode = sg_reverse ?1:2;
		}else {
		    cur_mode = sg_reverse ?2:1;
		}
	    }else {
                if(value < sg_day2night) {
		    cur_mode = sg_reverse ?1:2;
	        }else if(value > sg_night2day) {
		    cur_mode = sg_reverse ?2:1;
                }
	    }

	    TB_LOGD("cur_value:%d,day2night:%d,night2day:%d,pre_mode:%d,cur_mode:%d(%s)",
			    value,sg_day2night,sg_night2day,pre_mode,cur_mode,cur_mode == 1? "DAY":"NIGHT");

	    if(pre_mode != cur_mode) {
		pre_mode = cur_mode;

		if(pre_mode == 1) {
                    TB_LOGD("++ swicth to day mode");
    		    product_test_irled_off();
                    product_test_ircut_day();
                    usleep(300*1000);
                    product_test_ircut_off();
                    proudct_test_media_set_color_to_gray(1);
		}else {
                    TB_LOGD("-- swicth to night mode");
    		    product_test_irled_on();
                    product_test_ircut_night();
                    usleep(300*1000);
                    product_test_ircut_off();
                    proudct_test_media_set_color_to_gray(2);
		}
	    }
	}

	sleep(2);
    }

    return NULL;
}

int product_test_camera_test_start(void)
{
    int iret = 0;

    TB_LOGD();

     product_test_led_off();
     product_test_irled_off();

    product_test_ircut_day();
    usleep(300*1000);
    product_test_ircut_off();

    TB_LOGD ("iret:%d",iret);

    iret |= product_test_media_init();
    TB_LOGD ("iret:%d",iret);
    iret |= product_test_media_load_day_isp_file();
    TB_LOGD ("iret:%d",iret);

    iret |= proudct_test_media_set_color_to_gray(1);

    iret |= product_test_media_server_start();

    if(product_test_adc_get_threshold(0,&sg_day2night,&sg_night2day) == 0) {
	int mode,reverse;
        if(product_test_adc_get_switch_param(0,&mode,&reverse) == 0) {
            pthread_t thread;
	    sg_mode = mode;
	    sg_reverse = reverse;
            pthread_create(&thread,NULL,__product_day_night_switch,NULL);
	 }
    } 

    TB_LOGD ("iret:%d",iret);

    return iret;
}

int product_test_camera_test_stop(void)
{
    sg_day_night_run_flag = 0;
    product_test_ircut_off();
    product_test_media_server_stop();
    return 0;
}

int product_test_black_camera_test_start(void)
{
    int iret = 0;
    TB_LOGD();

    product_test_irled_on();
    product_test_ircut_night();

    usleep(300*1000);

    product_test_led_off();
    product_test_media_init();

    iret |= product_test_media_load_night_isp_file();
    TB_LOGD ("iret:%d",iret);
    iret |= proudct_test_media_set_color_to_gray(2);
    TB_LOGD ("iret:%d",iret);
    iret |= product_test_media_server_start();
    TB_LOGD ("iret:%d",iret);

    return iret;
}

int product_test_black_camera_test_stop(void)
{
    product_test_irled_off();
    product_test_ircut_off();
    product_test_media_server_stop();
    return 0;
}

static int sg_camera_init_flag = 0;

int product_test_camera_get_frame(int chan_num,char **buf,int *len) 
{
    int iret = 0;
    int type;
    if(!sg_camera_init_flag) {
        TB_LOGD();

        product_test_irled_on();
        product_test_ircut_night();
        usleep(300*1000);
	product_test_ircut_off();
        product_test_led_off();
        iret |= product_test_media_init();
        iret |= product_test_media_load_night_isp_file();
	if(iret) {
            TB_LOGD("camera init fail");
            return -1;	    
	}

        sg_camera_init_flag  = 1;
    }

    return product_test_video_get_frame((VIDEO_TYPE_E)chan_num,&type,buf,len,NULL);
}
