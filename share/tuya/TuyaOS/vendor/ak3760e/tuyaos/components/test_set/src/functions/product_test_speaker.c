/*
 *  @copyright Copyright (c) tuya.inc 2021
 */
#include <inttypes.h> 
#include <stdio.h>
#include <malloc.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <time.h>
#include <locale.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <assert.h>
#include <stdbool.h>
#include <signal.h>
#include <pthread.h>
#include "product_test_cfg_info.h"
#include "product_test_utils.h"
#include "product_test_gpio.h"
#include "product_test_media.h"
#include "product_test_speaker.h"
#include "product_test_config.h"
#include "ptal_audio.h"
#include "tb_log.h"

#define SPEAKER_EN_KEY     ("speaker_en")
#define SPEAKER_LEVEVL_KEY ("speaker_level")
#define SPEAKER_PLAY_FILENAME_KEY "speaker_play_file"
#define SPEAKER_DEFAULT_PLAY_FILENAME "test.pcm"

static int sg_speaken_init_flag = 0;
static int sg_speaken_enable_flag = 0;

static int __product_test_config_speaker_get_gpio_num(int *gpio_num)
{
    char *dev,*key;
    int enable;

    dev = CONFIG_DEVICE_SPEAKER_1;
    key = CONFIG_KEY_ENABLE;

    if(product_test_config_read_int(dev,key,&enable) || enable != 1) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return-1;
    }

    key = CONFIG_KEY_KEY_GPIO_NUM;
    
    if(product_test_config_read_int(dev,key,gpio_num)) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return -1;
    }

    return 0;   
}

static int __product_test_config_speaker_get_gpio_level(int*level) 
{
    char *dev,*key;
    int enable;

    dev = CONFIG_DEVICE_SPEAKER_1;
    key = CONFIG_KEY_ENABLE;

    if(product_test_config_read_int(dev,key,&enable) || enable != 1) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return-1;
    }

    key = CONFIG_KEY_KEY_HI_ACTIVE;
    
    if(product_test_config_read_int(dev,key,level)) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return -1;
    }

    return 0;   
}

int product_test_speaker_get_volume(int devid,int channel,uint32_t *value)
{
    return product_test_audio_get_volume(E_AUDIO_SPEAKER,devid, channel,value);
}

int product_test_speaker_set_volume(int devid, int channel, uint32_t value)
{
    if (value > 100)
        value = 100;
    return product_test_audio_set_volume(E_AUDIO_SPEAKER,devid, channel,value);
}


int product_test_speaker_set_gpio_num(int gpio_num)
{
    return product_test_utils_write_int(SPEAKER_EN_KEY,gpio_num,0);
}


int product_test_speaker_get_gpio_num(int*gpio_num) 
{
    int iret;
    iret = product_test_utils_read_int(SPEAKER_EN_KEY,gpio_num);

    if(iret) {
        iret = __product_test_config_speaker_get_gpio_num(gpio_num);
    }

    return iret;
}

int product_test_speaker_set_gpio_level(int level)
{
    return product_test_utils_write_int(SPEAKER_LEVEVL_KEY,level,0);
}

int product_test_speaker_get_gpio_level(int*level) 
{
    int iret;
    
    iret = product_test_utils_read_int(SPEAKER_LEVEVL_KEY,level);

    if(iret) {
        iret = __product_test_config_speaker_get_gpio_level(level);   
    }

    return iret;
}

int product_test_speaker_enable(void)
{
    int num,level;
    int iret = 0;
    iret |= product_test_speaker_get_gpio_num(&num);
    iret |= product_test_speaker_get_gpio_level(&level);

    if(iret) {
        TB_LOGD("speaker enable fail"); 
        return 0;   
    }

    if(sg_speaken_init_flag == 0) {
        if(product_test_gpio_init(num,E_GPIO_OUT,level^1)) {
            TB_LOGE("irled gpio init fali");
            return -1;
        }
        sg_speaken_init_flag = 1;
    }   

    return product_test_gpio_set_value(num,level);
}

int product_test_speaker_disable(void)
{
    int num,level;
    int iret = 0;
    iret |= product_test_speaker_get_gpio_num(&num);
    iret |= product_test_speaker_get_gpio_level(&level);

    if(iret) {
        TB_LOGD("speaker enable fail"); 
        return 0;   
    }

    if(sg_speaken_init_flag == 0) {
        if(product_test_gpio_init(num,E_GPIO_OUT,level^1)) {
            TB_LOGE("irled gpio init fali");
            return -1;
        }
        sg_speaken_init_flag = 1;
    }   

    return product_test_gpio_set_value(num,level^1);
}

int product_test_speaker_pcm_play_start(char *file_path)
{
    TB_LOGD();
    if(file_path == NULL) {
        TB_LOGE("invalid parm");
        return -1;  
    }

    if(!sg_speaken_enable_flag) {
        product_test_speaker_enable();
        sg_speaken_enable_flag = 1;
    }

    return product_test_speaker_start_play_pcm(file_path);
}


int product_test_speaker_wav_play_start(char *file_path)
{
    TB_LOGD();
    if(file_path == NULL) {
        TB_LOGE("invalid parm");
        return -1;  
    }

    if(!sg_speaken_enable_flag) {
        product_test_speaker_enable();
        sg_speaken_enable_flag = 1;
    }

    return product_test_speaker_start_play_wav(file_path);
}

int product_test_speaker_play_stop(void) 
{
    TB_LOGD();
    product_test_speaker_disable();
    product_test_speaker_stop();
    sg_speaken_enable_flag = 0;

    return 0;
}

static int __product_test_config_speaker_get_play_filename(char *name)
{
    char *dev,*key;
    int enable;

    if(name == NULL) {
        TB_LOGE("invalid parm");
	return -1;	
    }

    dev = CONFIG_DEVICE_PRODUCT_CFG;
    key = CONFIG_KEY_ENABLE;

    if(product_test_config_read_int(dev,key,&enable) || enable != 1) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
	return-1;
    }

    key = CONFIG_KEY_CFG_PLAY_FILENAME;

    if(product_test_config_read_string(dev,key,name,64) || strlen(name) == 0) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return -1;
    }

    return 0;
}

int product_test_speaker_get_play_filename(char *name) 
{
    int iret = 0;

    if(name == NULL) {
        TB_LOGE("invalid parm");
        return -1;	
    }

    iret = product_test_utils_read_string(SPEAKER_PLAY_FILENAME_KEY,name);

    if(iret == 0 &&  strlen(name)) {
        TB_LOGD("name:%s",name);
        return 0;
    }

    iret = __product_test_config_speaker_get_play_filename(name);

    if (iret || strlen(name)==0) {
        strcpy(name,SPEAKER_DEFAULT_PLAY_FILENAME);
    }

    return 0;
}

int product_test_speaker_set_playfilename(char *name) 
{
    return product_test_utils_write_string(SPEAKER_PLAY_FILENAME_KEY,name,0);
}

int product_test_speaker_put_frame(char *buf,int plen)
{
    if(!sg_speaken_enable_flag) {
        product_test_speaker_enable();
        sg_speaken_enable_flag = 1;
    }

    if(buf== NULL || plen == 0){
        TB_LOGE("invalid parm");
	return -1;
    }

    return product_test_audio_put_frame(buf,plen); 
}

int product_test_audio_test(void)
{
	return ptal_test_audio_test();
}

int product_test_swept_sine_start(int num)
{
	//TODO: need chuan ru number
	return ptal_swept_sine_start(num);
}

int product_test_swept_over(void)
{
	return ptal_swept_over();
}

int product_test_read_swept_sine_result(void)
{
	return ptal_read_swept_sine_result();
}
