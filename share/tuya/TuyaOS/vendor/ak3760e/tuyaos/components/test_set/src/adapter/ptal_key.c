#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include "tb_log.h"
#include "ptal_gpio.h"
#include "ptal_key.h"
#include "product_test_utils.h"
#include "product_test_gpio.h"
#include "product_test_key.h"

static int sg_key_init_flag[MAX_KEY_NUM]={0};
static int sg_key_value[MAX_KEY_NUM] = {1};
static int sg_key_cnts[MAX_KEY_NUM] = {0};


int __attribute__((weak)) ptal_key_get(int n,int *flag) 
{
    int gpio;
    int key;
    int num;

    if(product_test_key_get_num(&num) || num <= 0) {
        TB_LOGW("invalid gpio_num");
	return -1;
    }

    if(num > MAX_KEY_NUM) {
        TB_LOGE("invalid key num:%d",num);  
        return -1;  
    }

    if(n >= MAX_KEY_NUM) {
        TB_LOGE("key num(%d) >  MAX(%d)",n,MAX_KEY_NUM);    
    }

    if(product_test_key_get_gpio(n,&gpio)) {
        TB_LOGE("get key %d gpio fail",n);
        return -1;  
    }

    if(sg_key_init_flag[n] == 0) {
        TB_LOGD();
        if(product_test_gpio_init(gpio,E_GPIO_IN,0)) {
            TB_LOGE("led gpio %d init fali",gpio);
            return -1;
        }
        sg_key_init_flag[n] = 1;
    }   

    if(product_test_gpio_get_value(gpio,&key)) {
        TB_LOGE("get key %d value fail",gpio);
        return -1;      
    }

    if(key != sg_key_value[n]) {

        usleep(1*1000);
        if(product_test_gpio_get_value(gpio,&key)) {
            TB_LOGE("get key %d value fail",gpio);
            return -1;      
        }

        if(key != sg_key_value[n]) {
            sg_key_value[n] = key;
            sg_key_cnts[n] ++; 
        }
    }
                
    if(sg_key_cnts[n] >= 2) {
        *flag = 1;
        sg_key_value[n] = 1;
        sg_key_cnts[n] = 0;
    }

    return 0;
}

