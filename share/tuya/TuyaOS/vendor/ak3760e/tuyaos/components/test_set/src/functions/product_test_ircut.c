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
#include "product_test_gpio.h"
#include "product_test_ircut.h"
#include "product_test_utils.h"
#include "product_test_config.h"
#include "ptal_ircut.h"
#include "tb_log.h"

#define IRCUT_PIN_KEY  "ircut%d_pin"
#define IRCUT_LEVEL_KEY "ircut%d_level"


static int __product_test_config_ircut_get_gpio_num(IRCUT_PIN_E pin,int* num) 
{
    char *dev,*key;
    int enable;

    dev = CONFGI_DEVICE_IRCUT_1;
    key = CONFIG_KEY_ENABLE;

    if(product_test_config_read_int(dev,key,&enable) || enable != 1) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return-1;
    }

    dev = CONFGI_DEVICE_IRCUT_1;

    if(pin == E_IRCUT_PIN1) {
        key = CONFIG_KEY_IRCUT_PIN1;
    }else if(pin == E_IRCUT_PIN2) {
        key = CONFIG_KEY_IRCUT_PIN2;
    }else {
        TB_LOGE("invalid pin %d",pin);  
    }
    
    if(product_test_config_read_int(dev,key,num)) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return -1;
    }

    return 0;   
}

static int __product_test_config_ircut_get_gpio_level(IRCUT_PIN_E pin,int*level)
{
    char *dev,*key;
    int enable;

    dev = CONFGI_DEVICE_IRCUT_1;
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

int product_test_ircut_set_gpio_num(IRCUT_PIN_E pin,int num)
{
    char key[64] = {0};

    sprintf(key,IRCUT_PIN_KEY,pin);

    if(pin >= E_IRCUT_MAX) {
        TB_LOGE("type %d invalid",pin); 
        return -1;  
    }

    return product_test_utils_write_int(key,num,0);
}


int product_test_ircut_get_gpio_num(IRCUT_PIN_E pin,int* num) 
{
    int iret;
    char key[64] = {0};

    sprintf(key,IRCUT_PIN_KEY,pin);

    if(pin >= E_IRCUT_MAX) {
        TB_LOGE("type %d invalid",pin);
        return -1;  
    }

    iret =  product_test_utils_read_int(key,num);

    if(iret) {
        iret = __product_test_config_ircut_get_gpio_num(pin,num); 
    }
    return iret;
}

int product_test_ircut_set_gpio_level(IRCUT_PIN_E pin,int level) 
{
    char key[64] = {0};

    sprintf(key,IRCUT_LEVEL_KEY,pin);

    if(pin >= E_IRCUT_MAX) {
        TB_LOGE("type %d invalid",pin);
        return -1;  
    }

    return product_test_utils_write_int(key,level,0);
}

int product_test_ircut_get_gpio_level(IRCUT_PIN_E pin,int*level)
{
    int iret;
    char key[64] = {0};

    sprintf(key,IRCUT_LEVEL_KEY,pin);

    if(pin >= E_IRCUT_MAX) {
        TB_LOGE("type %d invalid",pin); 
        return -1;  
    }

    iret =  product_test_utils_read_int(key,level);

    if(iret) {
        iret = __product_test_config_ircut_get_gpio_level(pin,level);
    }

    return iret;
}


int product_test_ircut_day(void) 
{
    int iret = 0;

    iret |=  ptal_test_ircut_day();
 // iret |= ptal_ircut_off();

    return iret;
}


int product_test_ircut_night(void) 
{
    int iret = 0;

    iret |= ptal_ircut_night();
  //iret |= ptal_ircut_off();

    return iret;

}

int product_test_ircut_off(void) 
{
    return ptal_ircut_off();
}


int product_test_ircut_test(int num)
{
    int i;
    for(i= 0; i< num;i++) {
        product_test_ircut_day();
        usleep(200000);
        product_test_ircut_night(); 
        usleep(200000);
    }

    product_test_ircut_off(); 

    return 0;
}
