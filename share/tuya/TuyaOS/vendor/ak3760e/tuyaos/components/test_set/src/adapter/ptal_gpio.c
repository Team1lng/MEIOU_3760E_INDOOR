#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "product_test_config.h"
#include "tkl_gpio.h"
#include "ptal_gpio.h"
#include "tb_log.h"

GPIO_GROUP_T g_gpio_group[] __attribute__((weak)) = 
{

};

GPIO_GROUP_T g_gpio_sub_mcu_group[] __attribute__((weak)) =
{

};

int g_gpio_array_counts __attribute__((weak)) = sizeof(g_gpio_group) / sizeof(g_gpio_group[0]);
int g_gpio_sub_mcu_array_counts __attribute__((weak)) = sizeof(g_gpio_sub_mcu_group) / sizeof(g_gpio_sub_mcu_group[0]);

int __attribute__((weak)) ptal_gpio_group_test(GPIO_GROUP_T *group) 
{
    TB_LOGD();
    return  -1;
}

int __attribute__((weak)) ptal_gpio_sub_mcu_group_test(GPIO_GROUP_T *group) 
{
    TB_LOGD();
    return -1; 
}


int __attribute__((weak))ptal_gpio_init(int num,int direction,int init)
{
    int iret=0;
    UINT32_T port;
    TUYA_GPIO_BASE_CFG_T cfg;
    OPERATE_RET ret;
    TUYA_GPIO_LEVEL_E level = TUYA_GPIO_LEVEL_LOW;
    TUYA_GPIO_DRCT_E direct = TUYA_GPIO_OUTPUT;

    TB_LOGD("num:%d,direction:%d,init:%d\n",num,direction,init);

    port = (UINT32_T)num;

    if(direction) {
        direct = TUYA_GPIO_INPUT;
    }

    if(init) {
        level = TUYA_GPIO_LEVEL_HIGH;
    }

    cfg.direct = direct;
    cfg.level =  level;

    ret = tkl_gpio_init(port, &cfg);
    if (ret != OPRT_OK) {
        TB_LOGE("error: tkl_gpio_init failed\n");
	iret = -1;
    }

    return iret;
}

int __attribute__((weak)) ptal_gpio_set_direction(int num,int direction,int init)
{
    return ptal_gpio_init(num,direction,init);
}

int __attribute__((weak))ptal_gpio_set_value(int num,int value)
{
    TUYA_GPIO_LEVEL_E level;
    UINT32_T port = (UINT32_T)num;

    TB_LOGD();

    if(value) {
        level = TUYA_GPIO_LEVEL_HIGH;
    }else {
        level = TUYA_GPIO_LEVEL_LOW;   
    }

    return  tkl_gpio_write(port,level);
}

int __attribute__((weak))ptal_gpio_get_value(int num,int *value) 
{
    TUYA_GPIO_LEVEL_E level; 
    UINT32_T port = (UINT32_T)num;

    TB_LOGD();

    if(tkl_gpio_read(port, &level)) {
        return -1;  
    }

    if(level == TUYA_GPIO_LEVEL_HIGH) {
        *value = 1;
    }else {
        *value = 0;
    }

    return 0;
}
