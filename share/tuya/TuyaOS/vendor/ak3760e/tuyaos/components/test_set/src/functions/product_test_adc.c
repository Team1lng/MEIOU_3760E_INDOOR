/* 
 *  @copyright Copyright (c) tuya.inc 2021
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include <pthread.h>
#include <inttypes.h> 
#include <fcntl.h>
#include <sys/ioctl.h>
#include "ptal_adc.h"
#include "tb_log.h"
#include "product_test_utils.h"
#include "product_test_config.h"

#define ADC_CHANNEL_KEY "adc%d_channel"
#define ADC_D2N_KEY  "adc%d_d2n"
#define ADC_N2D_KEY  "adc%d_n2d"
#define ADC_TYPE     "adc%d_type"

#define ADC_DN_SW_MODE_KEY "adc%d_dn_switch_mode"
#define ADC_DN_REVERSE_KEY "adc%d_dn_reverse"

int product_test_adc_set_type(int n,int type) 
{

    int iret = 0;
    char key[32] ={0};

    sprintf(key,ADC_TYPE,n);

    iret |= product_test_utils_write_int(key,type,0);

    return iret;
}

static int __product_test_config_adc_get_type(int n,int *type) 
{

    char *key;
    int enable;
    char dev[32] = {0};

    sprintf(dev,"adc_%d",n+1);    

    key = CONFIG_KEY_ENABLE;

    if(product_test_config_read_int(dev,key,&enable) || enable != 1) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return-1;
    }

    key = CONFIG_KEY_TYPE;
    
    if(product_test_config_read_int(dev,key,type)) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return -1;
    }

    return 0;
}

int product_test_adc_get_type(int n,int *type)
{
    int iret = 0;
    char key[32] ={0};

    sprintf(key,ADC_TYPE,n);

    iret  |=  product_test_utils_read_int(key,type);

    if(iret) {
        iret =  __product_test_config_adc_get_type(n,type);
    }

    return iret;
}

int product_test_adc_set_threashold(int n,int d2n,int n2d)
{
    int iret = 0;
    char key[32] ={0};

    sprintf(key,ADC_D2N_KEY,n);

    iret |= product_test_utils_write_int(key,d2n,0);

    sprintf(key,ADC_N2D_KEY,n);

    iret |= product_test_utils_write_int(key,n2d,0);

    return iret;
}

static int __product_test_config_adc_get_threshold(int n,int* d2n, int *n2d)
{
    char *key;
    int enable;
    char dev[32] = {0};
    int d2n_val = 0;
    int n2d_val = 0;

    sprintf(dev,"adc_%d",n+1);    

    key = CONFIG_KEY_ENABLE;

    if(product_test_config_read_int(dev,key,&enable) || enable != 1) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return-1;
    }

    key = CONFIG_KEY_DAY2NIGHT_THRESHOLD;
    
    if(product_test_config_read_int(dev,key,&d2n_val)) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return -1;
    }

    key = CONFIG_KEY_NIGHT2DAY_THRESHOLD;

    if(product_test_config_read_int(dev,key,&n2d_val)) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return -1;
    }

    TB_LOGD("d2n_val:%d,n2d_val:%d",d2n_val,n2d_val);

    *d2n = d2n_val;
    *n2d = n2d_val;

    return 0;
}

int product_test_adc_get_threshold(int n,int* d2n, int *n2d)
{
    int iret = 0;
    char key[32] ={0};

    sprintf(key,ADC_D2N_KEY,n);

    iret  |=  product_test_utils_read_int(key,d2n);

    if (!iret) {
        sprintf(key,ADC_N2D_KEY,n);
        iret  |=  product_test_utils_read_int(key,n2d);
    }

    if(iret) {
        iret =  __product_test_config_adc_get_threshold( n,d2n,n2d);
    }

    return iret;
}

int product_test_adc_set_switch_param(int n,int mode,int reverse) 
{
    int iret = 0;
    char key[32] ={0};

    sprintf(key,ADC_DN_SW_MODE_KEY,n);

    iret |= product_test_utils_write_int(key,mode,0);

    sprintf(key,ADC_DN_REVERSE_KEY,n);

    iret |= product_test_utils_write_int(key,reverse,0);

    return iret;
}

static int __product_test_config_adc_get_switch_param(int n,int* mode,int *reverse)
{
    char *key;
    int enable;
    char dev[32] = {0};
    int mode_val = 0;
    int reverse_val = 0;

    sprintf(dev,"adc_%d",n+1);

    key = CONFIG_KEY_ENABLE;

    if(product_test_config_read_int(dev,key,&enable) || enable != 1) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return-1;
    }

    key = CONFIG_KEY_DAY_NIGHT_SWITCH_MODE;
    
    if(product_test_config_read_int(dev,key,&mode_val)) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return -1;
    }

    key = CONFIG_KEY_DAY_NIGHT_REVERSE;

    if(product_test_config_read_int(dev,key,&reverse_val)) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return -1;
    }

    TB_LOGD("mode_val:%d,reverse_val:%d",mode_val,reverse_val);

    *mode = mode_val;
    *reverse = reverse_val;

    return 0;
}

int product_test_adc_get_switch_param(int n,int* mode,int *reverse)
{
    int iret = 0;
    char key[32] ={0};

    sprintf(key,ADC_DN_SW_MODE_KEY,n);

    iret  |=  product_test_utils_read_int(key,mode);

    if (!iret) {
        sprintf(key,ADC_DN_REVERSE_KEY,n);
        iret  |=  product_test_utils_read_int(key,reverse);
    }

    if(iret) {
        iret =  __product_test_config_adc_get_switch_param(n,mode,reverse);
    }

    return iret;
}


int product_test_adc_set_channel(int n,int channel)
{
    char key[32] ={0};

    sprintf(key,ADC_CHANNEL_KEY,n);
    return product_test_utils_write_int(key,channel,0);
}

static int __product_test_config_adc_get_chanel(int n,int* channel)
{
    char *key;
    int enable;
    char dev[32] = {0};

    sprintf(dev,"adc_%d",n+1);

    key = CONFIG_KEY_ENABLE;

    if(product_test_config_read_int(dev,key,&enable) || enable != 1) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return-1;
    }

    key = CONFIG_KEY_INDEX;
    
    if(product_test_config_read_int(dev,key,channel)) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return -1;
    }

    return 0;
}


int product_test_adc_get_chanel(int n,int* channel)
{
    int iret;
    char key[32] ={0};

    sprintf(key,ADC_CHANNEL_KEY,n);

    iret =  product_test_utils_read_int(key,channel);

    if(iret) {
        iret =  __product_test_config_adc_get_chanel(n,channel);
    }

    return iret;
}

int product_test_adc_get_value(int n,uint32_t *value)
{
    int channel;

    if(product_test_adc_get_chanel(n,&channel)) {
        channel = n;        
    }

    TB_LOGD("adc %d  get value",channel);

    return ptal_adc_get_value(channel,value);
}
