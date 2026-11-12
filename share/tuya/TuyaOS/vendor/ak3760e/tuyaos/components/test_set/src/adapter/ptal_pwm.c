/*
 *  @copyright Copyright (c) tuya.inc 2021
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "product_test_config.h"
#include "product_test_led.h"
#include "product_test_utils.h"
#include "ptal_led.h"
#include "ptal_pwm.h"
#include "tkl_pwm.h"
#include "tb_log.h"

#define PWM_DUTY_CYCLE "/sys/class/pwm/pwmchip%d/pwm%d/duty_cycle"
#define PWM_PERIOD "/sys/class/pwm/pwmchip%d/pwm%d/period"

typedef struct led_parm_s
{
    int type;
    char *dev_name;
    char *pwm_name;
    char *hal_name;
}LED_PARM_T;

typedef struct led_pwm_param_s
{
    int dev;
    int chan;
    int duty;
    int freq;
    int pol; //polarity_inversed = 0 ,normal ,1,inverse
}LED_PWM_PARAM_T;

PWM_PARAM_T  g_pwm_parm __attribute__((weak)) = {
    .total = 2,
    .pwm[0] = {
        .type = E_LED_WHITE,
        .chip = 0,
        .channel = 1,
        .period  = 2500,
        .min_value = 20,
        .max_value = 90,
    },
    .pwm[1] = {
        .type = E_IRLED_RED,
        .chip = 0,
        .channel = 0,
        .period  = 2500,
        .min_value = 20,
        .max_value = 90,
    }
};


static LED_PARM_T sg_led_parm_table[] = {
    {E_LED_RED,CONFIG_DEVICE_LED_RED,CONFIG_DEVICE_LED_RED_PWM,"led_red"},
    {E_LED_GREEN,CONFIG_DEVICE_LED_GREEN,CONFIG_DEVICE_LED_GREEN_PWM,"led_green"},
    {E_LED_WHITE,CONFIG_DEVICE_LED_WHITE,CONFIG_DEVICE_LED_WHITE_PWM,"led_white_light"},
    {E_LED_BLUE,CONFIG_DEVICE_LED_BLUE,CONFIG_DEVICE_LED_BLUE_PWM,"led_blue"},
    {E_LED_IDLE,CONFIG_DEVICE_LED_IDLE,CONFIG_DEVICE_LED_IDLE_PWM,"led_idle"},
};

static LED_PARM_T sg_irled_parm_table[] = {
    {E_IRLED_RED,CONFIG_DEVICE_IRLED_RED,CONFIG_DEVICE_IRLED_RED_PWM,"led_ir"},
    {E_IRLED_GREEN,CONFIG_DEVICE_IRLED_GREEN,CONFIG_DEVICE_IRLED_RED_PWM,"led_ir_green"},
};

static LED_PARM_T* __find_led_parm (LED_TYPE_E type) {
    int i;
    int n = sizeof(sg_led_parm_table)/sizeof(sg_led_parm_table[0]); 

    for(i= 0;i<n;i++) {
        LED_PARM_T *parm = &sg_led_parm_table[i];

        if(parm->type == type) {
            return parm;
        }
    }

    return NULL;
}

static LED_PARM_T * __find_irled_parm (IRLED_TYPE_E type) {
    int i;
    int n = sizeof(sg_irled_parm_table)/sizeof(sg_irled_parm_table[0]); 

    for(i= 0;i<n;i++) {
        LED_PARM_T *parm = &sg_irled_parm_table[i];

        if(parm->type == type) {
            return parm;    
        }
    }

    return NULL;
}

static int __ptal_get_pwm_info(char * dev,LED_PWM_PARAM_T *param)
{
    char *key;
    int enable;
    int tmp;

    if(param == NULL) {
        TB_LOGE("invalid parm");    
        return -1;  
    }

    key = CONFIG_KEY_ENABLE;

    if(product_test_config_read_int(dev,key,&enable) || enable != 1) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return-1;
    }

    key = CONFIG_KEY_LED_PWM_FREQ;

    if(product_test_config_read_int(dev,key,&tmp)) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return-1;
    }

    param->freq = tmp;

    key = CONFIG_KEY_LED_PWM_DUTY;

    if(product_test_config_read_int(dev,key,&tmp)) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return-1;
    }

    param->duty = tmp;


    key = CONFIG_KEY_LED_PWM_DEV;

    if(product_test_config_read_int(dev,key,&tmp)) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return-1;
    }

    param->dev = tmp;


    key = CONFIG_KEY_LED_PWM_CHN;

    if(product_test_config_read_int(dev,key,&tmp)) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return-1;
    }

    param->chan = tmp;


    key = CONFIG_KEY_LED_PWM_POL_INV;

    if(product_test_config_read_int(dev,key,&tmp)) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return-1;
    }

    param->pol = tmp;

    TB_LOGD("pol= %d,chn = %d,dev= %d,duty= %d,freq= %d\n",param->pol,param->chan,param->dev,param->duty,param->freq);

    return 0;
}


int __attribute__((weak)) ptal_pwm_led_on(LED_TYPE_E type) 
{
    OPERATE_RET ret;
    UINT32_T port = 0;
    TUYA_PWM_BASE_CFG_T cfg;

    LED_PWM_PARAM_T param = {0};

    LED_PARM_T *parm = __find_led_parm (type);

    if(parm == NULL) {
        TB_LOGE("type:%d not find config node",type);
        return -1;  
    }

    TB_LOGD("%s",parm->pwm_name);

    if(__ptal_get_pwm_info(parm->pwm_name,&param)) {
        TB_LOGD("invalid parm");
        return -1;
    }

    cfg.frequency = param.freq;
    cfg.duty = param.duty;
    cfg.polarity = (TUYA_PWM_POLARITY_E) param.pol;
    port = param.chan; 

    ret = tkl_pwm_init(port, &cfg);

    if (ret != OPRT_OK) {
        printf("error: tkl_pwm_init failed\n");
        return -1;
    }

    // disable pwm channel
    tkl_pwm_stop(port);

    // configure pwm channel
    ret = tkl_pwm_info_set(port, &cfg);
    if (ret != OPRT_OK) {
        printf("error: tkl_pwm_info_set failed\n");
        return -1;
    }

    // enable pwm channel
    ret = tkl_pwm_start(port);
    if (ret != OPRT_OK) {
        printf("error: tkl_pwm_start failed\n");
        return -1;
    }

    return 0;

}

int __attribute__((weak))ptal_pwm_led_off(LED_TYPE_E type)
{
    OPERATE_RET ret;
    UINT32_T port = 0;
    LED_PWM_PARAM_T param = {0};
    TUYA_PWM_BASE_CFG_T cfg;

    LED_PARM_T *parm = __find_led_parm (type);

    if(parm == NULL) {
        TB_LOGE("type:%d not find config node",type);
        return -1;  
    }

    TB_LOGD("%s",parm->pwm_name);

    if(__ptal_get_pwm_info(parm->pwm_name,&param)) {
        TB_LOGD("invalid parm");
        return -1;
    }

    cfg.frequency = param.freq;
    cfg.duty = param.duty;
    cfg.polarity = (TUYA_PWM_POLARITY_E) param.pol;
    port = param.dev*32 + param.chan; 

    ret = tkl_pwm_init(port, &cfg);
    if (ret != OPRT_OK) {
        printf("error: tkl_pwm_init failed\n");
        return -1;
    }

    // disable pwm channel
    tkl_pwm_stop(port);

    return 0;
}

int __attribute((weak))ptal_pwm_irled_on(IRLED_TYPE_E type)
{
    OPERATE_RET ret;
    UINT32_T port = 0;
    TUYA_PWM_BASE_CFG_T cfg;
    LED_PWM_PARAM_T param = {0};


    LED_PARM_T *parm = __find_irled_parm (type);

    if(parm == NULL) {
        TB_LOGE("type:%d not find config node",type);
        return -1;  
    }

    TB_LOGD("%s",parm->pwm_name);

    if(__ptal_get_pwm_info(parm->pwm_name,&param)) {
        TB_LOGD("invalid parm");
        return -1;
    }

    cfg.frequency = param.freq;
    cfg.duty = param.duty;
    cfg.polarity = (TUYA_PWM_POLARITY_E) param.pol;
    port = param.dev*32 + param.chan; 

    ret = tkl_pwm_init(port, &cfg);
    if (ret != OPRT_OK) {
        printf("error: tkl_pwm_init failed\n");
        return -1;
    }

    // disable pwm channel
    tkl_pwm_stop(port);

    // configure pwm channel
    ret = tkl_pwm_info_set(port, &cfg);
    if (ret != OPRT_OK) {
        printf("error: tkl_pwm_info_set failed\n");
        return -1;
    }

    // enable pwm channel
    ret = tkl_pwm_start(port);
    if (ret != OPRT_OK) {
        printf("error: tkl_pwm_start failed\n");
        return -1;
    }

    return 0;
}

int __attribute__((weak))ptal_pwm_irled_off(IRLED_TYPE_E type) 
{
    OPERATE_RET ret;
    UINT32_T port = 0;
    TUYA_PWM_BASE_CFG_T cfg;
    LED_PWM_PARAM_T param = {0};

    LED_PARM_T *parm = __find_irled_parm (type);

    if(parm == NULL) {
        TB_LOGE("type:%d not find config node",type);
        return -1;  
    }

    TB_LOGD("%s",parm->pwm_name);

    if(__ptal_get_pwm_info(parm->pwm_name,&param)) {
        TB_LOGD("invalid parm");
        return -1;
    }

    cfg.frequency = param.freq;
    cfg.duty = param.duty;

    cfg.polarity = (TUYA_PWM_POLARITY_E) param.pol;
    port = param.dev*32 + param.chan; 


    ret = tkl_pwm_init(port, &cfg);
    if (ret != OPRT_OK) {
        printf("error: tkl_pwm_init failed\n");
        return -1;
    }

    // disable pwm channel
    tkl_pwm_stop(port);
    return 0;
}



int __attribute__((weak)) ptal_pwm_init(int chip,int channel)
{
    TB_LOGD();
    return 0;
}

int __attribute__((weak)) ptal_set_duty_cycle(int chip,int channel,int value) 
{
    int iret;
    int chan;
    char path[128] = {0};
    char svalue[32]={0};
    PWM_PARAM_T *parm = &g_pwm_parm;

    TB_LOGD("value:%d",value);

    if(chip >= MAX_PWM_CHIP || channel >= MAX_PWM_CHAN) {
        TB_LOGE("chip(%d) > MAX(%d) or channel(%d) > MAX(%d)",chip,MAX_PWM_CHIP,channel,MAX_PWM_CHAN);
        return -1;
    }

    chan = chip*channel;

    if(chan >= parm->total) {
        TB_LOGE("chan(%d) > max(%d)",chan,parm->total);
        return -1;
    }

    if(value < parm->pwm[chan].min_value || value > parm->pwm[chan].max_value) {
        TB_LOGE("value(%d),%d~%d",value,parm->pwm[chan].min_value,parm->pwm[chan].max_value);
        return -1;
    }

    sprintf(path,PWM_DUTY_CYCLE,chip,channel);
    sprintf(svalue,"%d",value);
    
    iret = product_test_file_write(path,svalue,strlen(svalue));

    return iret;
}

int __attribute__((weak)) ptal_pwm_set_period(int chip,int channel,int value) 
{
    int iret;
    int chan;
    char path[128] = {0};
    char svalue[32]={0};
    PWM_PARAM_T *parm = &g_pwm_parm;

    if(chip >= MAX_PWM_CHIP || channel >= MAX_PWM_CHAN) {
        TB_LOGE("chip(%d) > MAX(%d) or channel(%d) > MAX(%d)",chip,MAX_PWM_CHIP,channel,MAX_PWM_CHAN);
        return -1;
    }

    chan = chip*channel;

    if(chan >= parm->total) {
        TB_LOGE("chan(%d) > %d",chan,parm->total);
        return -1;
    }

    sprintf(path,PWM_PERIOD,chip,channel);
    sprintf(svalue,"%d",value);
    
    iret = product_test_file_write(path,svalue,strlen(svalue));

    return iret;
}
