#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "product_test_config.h"
#include "product_test_led.h"
#include "product_test_gpio.h"
#include "ptal_led.h"
#include "ptal_gpio.h"
#include "tb_log.h"

static int sg_irled_init_flag[E_IRLED_MAX] = {0};
static int sg_led_init_flag[E_LED_MAX]= {0};

int __attribute__((weak)) ptal_led_one_led_on(LED_TYPE_E type) 
{

    int num;
    int level;

    TB_LOGD();

    if(type >= E_LED_MAX) {
        TB_LOGE("type %d invalid",type);    
        return -1;
    }

    if(product_test_led_get_gpio_num(type,&num)) {
        TB_LOGE("get led gpio fail");
        return -1;
    }

    if(product_test_led_get_gpio_level(type,&level)) {
        TB_LOGE("get led control pin fail");
        return -1;
    }

    if(sg_led_init_flag[type] == 0) {
        TB_LOGD();
        if(product_test_gpio_init(num,E_GPIO_OUT,level^1)) {
            TB_LOGE("led gpio %d init fali",num);
            return -1;
        }
        sg_led_init_flag[type] = 1;
    }   


    return product_test_gpio_set_value(num,level);
}

int __attribute__((weak)) ptal_led_one_led_off(LED_TYPE_E type)
{
    int num;
    int iret;
    int level;

    TB_LOGD();

    if(type >= E_LED_MAX) {
        TB_LOGE("type %d invalid",type);    
        return -1;
    }

    if(product_test_led_get_gpio_num(type,&num)) {
        TB_LOGE("get led gpio fail");
        return -1;
    }

    if(product_test_led_get_gpio_level(type,&level)) {
        TB_LOGE("get led control pin fail");
        return -1;
    }

    if(sg_led_init_flag[type] == 0) {
        if(product_test_gpio_init(num,E_GPIO_OUT,level^1)) {
            TB_LOGE("led gpio %d init fali",num);
            return -1;
        }
        sg_led_init_flag[type] = 1;
    }   

    iret = product_test_gpio_set_value(num,level^1);

    return iret;

}

int __attribute__((weak)) ptal_led_one_irled_on(IRLED_TYPE_E type) 
{
    int num;
    int level;

    TB_LOGD();

    if(type >= E_IRLED_MAX) {
        TB_LOGE("type %d invalid",type);    
        return 0;
    }

    if(product_test_irled_get_gpio_num(type,&num)) {
        TB_LOGE("get irled gpio fail");
        return -1;
    }

    if(product_test_irled_get_gpio_level(type,&level)) {
        TB_LOGE("get irled control pin fail");
        return -1;
    }

    if(sg_irled_init_flag[type] == 0) {
        if(product_test_gpio_init(num,E_GPIO_OUT,level^1)) {
            TB_LOGE("irled gpio %d init fali",num);
            return -1;
        }
        sg_irled_init_flag[type] = 1;
    }   

    return product_test_gpio_set_value(num,level);
}

int  __attribute__((weak)) ptal_led_one_irled_off(IRLED_TYPE_E type) 
{
    int num;
    int level;

    TB_LOGD();

    if(type >= E_IRLED_MAX) {
        TB_LOGE("type %d invalid",type);    
        return 0;
    }

    if(product_test_irled_get_gpio_num(type,&num)) {
        TB_LOGE("get irled gpio fail");
        return -1;
    }

    if(product_test_irled_get_gpio_level(type,&level)) {
        TB_LOGE("get irled control pin fail");
        return -1;
    }

    if(sg_irled_init_flag[type] == 0) {
        if(product_test_gpio_init(num,E_GPIO_OUT,level^1)) {
            TB_LOGE("irled gpio %d init fali",num);
            return -1;
        }
        sg_irled_init_flag[type] = 1;
    }   

    return product_test_gpio_set_value(num,level^1);

}
