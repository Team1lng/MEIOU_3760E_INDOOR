/* 
 *  @copyright Copyright (c) tuya.inc 2021
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "product_test_protocol.h"
#include "product_test_gpio.h"
#include "product_test_alarm.h"
#include "product_test_utils.h"
#include "product_test_config.h"
#include "tb_log.h"

static  pthread_t sg_alarm_tid;

#define ALARM_GPIO_NUM_KEY   "alarm%d_gpio"
#define ALARM_GPIO_LEVEL_KEY "alarm%d_level"
#define ALARM_TOOL_KEY "alarm_tool_enable"

static int sg_alarm_test_flag = 0;
static int sg_alarm_out_init_flag=0;
static int sg_alarm_in_init_flag=0;

static int _product_test_config_get_alarm_tool_support(int *flag)
{
    char *dev,*key;
    int enable;

    dev = CONFIG_DEVICE_ALARM_TOOL;
    key = CONFIG_KEY_ENABLE;

    if(product_test_config_read_int(dev,key,&enable) || enable != 1) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return-1;
    }

    *flag = enable;

    return 0;
}


static int _product_test_config_alarm_get_gpio_num(ALARM_TYPE_E type, int *gpio_num)
{
    char *dev,*key;
    int enable;

    if(type == E_ALARM_TOOL) {
        dev = CONFIG_DEVICE_ALARM_TOOL;
    }else if(type == E_ALARM_IN) {
        dev = CONFIG_DEVICE_ALARM_IN;
    }else if(type == E_ALARM_OUT) {
        dev = CONFIG_DEVICE_ALARM_OUT;
    }else {
        TB_LOGE("invalid type %d",type);
        return -1;
    }

    key = CONFIG_KEY_ENABLE;

    if(product_test_config_read_int(dev,key,&enable) || enable != 1) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return-1;
    }

    key = CONFIG_KEY_KEY_GPIO_NUM;

    if(product_test_config_read_int(dev,key,gpio_num)) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return-1;
    }

    return 0;
}


int product_test_config_alarm_get_gpio_level(ALARM_TYPE_E type, int *level)
{
    char *dev,*key;
    int enable;

    if(type == E_ALARM_TOOL) {
        dev = CONFIG_DEVICE_ALARM_TOOL;
    }else if(type == E_ALARM_IN) {
        dev = CONFIG_DEVICE_ALARM_IN;
    }else if(type == E_ALARM_OUT) {
        dev = CONFIG_DEVICE_ALARM_OUT;
    }else {
        TB_LOGE("invalid type %d",type);
        return -1;
    }

    key = CONFIG_KEY_ENABLE;

    if(product_test_config_read_int(dev,key,&enable) || enable != 1) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return-1;
    }

    key = CONFIG_KEY_KEY_HI_ACTIVE;

    if(product_test_config_read_int(dev,key,level)) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return-1;
    }

    return 0;
}

int product_test_set_alarm_tool_support(int flag) 
{
    return product_test_utils_write_int(ALARM_TOOL_KEY,flag,0);
}

int product_test_get_alarm_tool_support(int *flag)
{
    int iret;

    iret =  product_test_utils_read_int(ALARM_TOOL_KEY,flag);

    if(iret) {

        iret = _product_test_config_get_alarm_tool_support(flag);
    }

    return iret;
}

int product_test_alarm_set_gpio_num(ALARM_TYPE_E type,int gpio_num)
{
    char key[64] = {0};

    sprintf(key,ALARM_GPIO_NUM_KEY,(int)type);

    return product_test_utils_write_int(key,gpio_num,0);
}

int product_test_alarm_get_gpio_num(ALARM_TYPE_E type,int*gpio_num)
{
    int iret;
    char key[64] = {0};

    sprintf(key,ALARM_GPIO_NUM_KEY,(int)type);
    
    iret =  product_test_utils_read_int(key,gpio_num);

    if(iret) {
        iret = _product_test_config_alarm_get_gpio_num(type,gpio_num);
    }

    return iret;        
}

int product_test_alarm_set_gpio_level(ALARM_TYPE_E type,int level)
{
    char key[64] = {0};

    sprintf(key,ALARM_GPIO_LEVEL_KEY,(int)type);

    return product_test_utils_write_int(key,level,0);
}



int product_test_alarm_get_gpio_level(ALARM_TYPE_E type,int*level) 
{
    int iret;
    char key[64] = {0};

    sprintf(key,ALARM_GPIO_LEVEL_KEY,(int)type);

    iret = product_test_utils_read_int(key,level);

    if(iret) {
        iret =  product_test_config_alarm_get_gpio_level(type,level); 
    }

    return iret;
}

int product_test_alarm_in_test(void) 
{
    int flag;
    int alarm_tool_num;
    int alarm_tool_level;
    int alarm_in_num;
    unsigned long start_time;
    int key,old_key=0;
    int key_cnts=0; 

    TB_LOGD();

    if(product_test_get_alarm_tool_support(&flag) == 0 && flag == 1) {
        if(product_test_alarm_get_gpio_num(E_ALARM_TOOL,&alarm_tool_num)) {
            TB_LOGE("get alarm gpio fail");
            return -1;
        }

        if(product_test_alarm_get_gpio_level(E_ALARM_TOOL,&alarm_tool_level)) {
            TB_LOGE("get alarm control pin fail");
            return -1;
        }
        if(sg_alarm_in_init_flag == 0) {
            if(product_test_gpio_init(alarm_tool_num,E_GPIO_OUT,alarm_tool_level^1)) {
                TB_LOGE("alarm gpio %d init fali",alarm_tool_num);
                return -1;
            }
        }
    }

    if(product_test_alarm_get_gpio_num(E_ALARM_IN,&alarm_in_num)) {
        TB_LOGE("get alarm gpio fail");
        return -1;
    }

    if(sg_alarm_in_init_flag == 0) {
        if(product_test_gpio_init(alarm_in_num,E_GPIO_IN,0)) {
            TB_LOGE("alarm gpio %d init fali",alarm_in_num);
            return -1;
        }

        sg_alarm_in_init_flag = 1;
    }

    start_time = product_test_get_curtime();
    
    while(LESS_TARGET(product_test_get_curtime(),start_time,30)) {

        if(product_test_get_alarm_tool_support(&flag) == 0 && flag == 1) {
            product_test_gpio_set_value(alarm_tool_num,alarm_tool_level);
            alarm_tool_level ^= 1;
        }

        usleep(500000);

        if(product_test_gpio_get_value(alarm_in_num,&key)) {
            TB_LOGE("get key %d value fail",alarm_in_num);
            return -1;      
        }

        if(key != old_key) {
            old_key = key;
            key_cnts ++;
        }

        TB_LOGD("key %d,old key %d,cnts:%d",key,old_key,key_cnts);

        if(key_cnts >= 3) {
            return 0;   
        }
    }
    return -1;
}

int product_test_alarm_out_on(void) 
{
    int num;
    int level;

    TB_LOGD();

    if(product_test_alarm_get_gpio_num(E_ALARM_OUT,&num)) {
        TB_LOGE("get alarm gpio fail");
        return -1;
    }

    if(product_test_alarm_get_gpio_level(E_ALARM_OUT,&level)) {
        TB_LOGE("get alarm control pin fail");
        return -1;
    }

    if(sg_alarm_out_init_flag == 0) {
        TB_LOGD();
        if(product_test_gpio_init(num,E_GPIO_OUT,level^1)) {
            TB_LOGE("alarm gpio %d init fali",num);
            return -1;
        }
        sg_alarm_out_init_flag = 1;
    }   


    return product_test_gpio_set_value(num,level);
}

int product_test_alarm_out_off(void)
{
    int num;
    int iret;
    int level;

    TB_LOGD();


    if(sg_alarm_test_flag) {
        sg_alarm_test_flag = 0;
        pthread_join(sg_alarm_tid,NULL);
    }

    if(product_test_alarm_get_gpio_num(E_ALARM_OUT,&num)) {
        TB_LOGE("get alarm gpio fail");
        return -1;
    }

    if(product_test_alarm_get_gpio_level(E_ALARM_OUT,&level)) {
        TB_LOGE("get alarm control pin fail");
        return -1;
    }

    if(sg_alarm_out_init_flag == 0) {
        if(product_test_gpio_init(num,E_GPIO_OUT,level^1)) {
            TB_LOGE("alarm gpio %d init fali",num);
            return -1;
        }
        sg_alarm_out_init_flag = 1;
    }

    iret = product_test_gpio_set_value(num,level^1);

    return iret;
}

static void *_alarm_thread_proc(void *arg) 
{
    int num;
    int level;

    TB_LOGD();

    if(product_test_alarm_get_gpio_num(E_ALARM_OUT,&num)) {
        TB_LOGE("get led gpio fail");
        return NULL;
    }

    if(product_test_alarm_get_gpio_level(E_ALARM_OUT,&level)) {
        TB_LOGE("get led control pin fail");
        return NULL;
    }

    if(sg_alarm_out_init_flag == 0) {
        TB_LOGD();
        if(product_test_gpio_init(num,E_GPIO_OUT,level^1)) {
            TB_LOGE("led gpio %d init fali",num);
            return NULL;
        }
        sg_alarm_out_init_flag = 1;
    }   

    while (sg_alarm_test_flag) {
        product_test_gpio_set_value(num,level);
        usleep(500000);
        product_test_gpio_set_value(num,level^1);
        usleep(500000);
    }

    return NULL;
}

int product_test_alarm_out_flash(void)
{
    TB_LOGD();

    sg_alarm_test_flag = 1;
    pthread_create(&sg_alarm_tid,NULL,_alarm_thread_proc,NULL);

    return 0;
}
