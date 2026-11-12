/*
 *  @copyright Copyright (c) tuya.inc 2021
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include "tb_log.h"
#include "list.h"
#include "tkl_input_event.h"
#include "product_test_utils.h"
#include "product_test_gpio.h"
#include "product_test_key.h"
#include "product_test_sub_mcu.h"
#include "product_test_config.h"
#include "product_test_product_info.h"

#define BUTTON_TOTAL_KEY "button_total"
#define BUTTON_GPIO_KEY "button%d_gpio"
#define SUB_MCU_BUTTON_TOTAL_KEY "sub_mcu_button_total"
#define INPUT_EVENT 32

extern int product_test_key_detect (void);

static int __product_test_config_key_get_num(int*num)
{
    char *key;
    int enable;
    int i=0;
    char dev[32]= {0};
    int iret = 0;

    while(1) {
    
        sprintf(dev,"key_%d",i+1);
        key = CONFIG_KEY_ENABLE;
    
        if(product_test_config_read_int(dev,key,&enable) || enable != 1) {
            break;
        }

        i++;
    }

    if(!i) {
        iret = -1;  
    }

    TB_LOGD("key total:%d",i);
    *num = i;
    return iret;    
}

static int __product_test_config_sub_mcu_get_key_num(int*num)
{
    char *dev,*key;
    int enable;

    dev = CONFIG_DEVICE_PRODUCT_CFG;
    key = CONFIG_KEY_ENABLE;

    if(product_test_config_read_int(dev,key,&enable) || enable != 1) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return-1;
    }

    key =CONFIG_KEY_CFG_SUB_MCU_KEYS;
    
    if(product_test_config_read_int(dev,key,num)) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return -1;
    }

    return 0;
}   

static int __product_test_config_key_get_gpio(int key_num,int *gpio_num)
{
    char *key;
    int enable;
    char dev[32]= {0};

    sprintf(dev,"key_%d",key_num);
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

int product_test_key_set_num(int num)
{
    return product_test_utils_write_int(BUTTON_TOTAL_KEY,num,0);
}

int product_test_key_get_num(int*num)
{
    int iret;
    iret =  product_test_utils_read_int(BUTTON_TOTAL_KEY,num);

    if(iret) {
        iret = __product_test_config_key_get_num(num);    
    }
    return iret;
}


int product_test_sub_mcu_set_key_num(int num)
{
    return product_test_utils_write_int(SUB_MCU_BUTTON_TOTAL_KEY,num,0);
}


int product_test_sub_mcu_get_key_num(int*num)
{
    int iret;

    iret = product_test_utils_read_int(SUB_MCU_BUTTON_TOTAL_KEY,num);

    if(iret) {
        iret = __product_test_config_sub_mcu_get_key_num(num);
    }

    return iret;
}

int product_test_key_set_gpio(int key_num,int gpio)
{
    char key[64] = {0};
    sprintf(key,BUTTON_GPIO_KEY,key_num);
    return product_test_utils_write_int(key,gpio,0);
}


int product_test_key_get_gpio(int key_num,int *gpio)
{
    int iret;
    char key[64] = {0};
    sprintf(key,BUTTON_GPIO_KEY,key_num);
    iret = product_test_utils_read_int(key,gpio);

    if(iret) {
        iret = __product_test_config_key_get_gpio(key_num+1,gpio);
    }
    return iret;
}

int product_test_main_cpu_key_get(int n,int *flag)
{
    return ptal_key_get(n,flag); 
}

int product_test_sub_mcu_key_get(int n,int *flag) 
{
    return product_test_sub_mcu_get_key(n,flag); 
}

int product_test_key_get(int n,int *flag) 
{
    int iret = -1;
    int mode;
    int num=0,snum=0;
    product_test_key_get_num(&num);
    product_test_sub_mcu_get_key_num(&snum);

    if(num > MAX_KEY_NUM || snum > MAX_SUB_MCU_KEY_NUM) {
        TB_LOGE("num(%d) > max(%d) || snum(%d) > max(%d)",num,MAX_KEY_NUM,snum,MAX_SUB_MCU_KEY_NUM);
        return -1;           
    }

    if(n > num+snum) {
        TB_LOGE("key index(%d) > total(%d)",n,num+snum);
	return -1;
    }

    if(n < num ) {
        iret = product_test_main_cpu_key_get(n,flag);
    }else {
        if(product_test_get_low_power_mode(&mode)==0 && mode == 1) {
            iret  = product_test_sub_mcu_key_get(n,flag);
	}
    }

    return iret;
}

int product_test_key_loop_wait(void)
{
    return product_test_key_detect();
}
