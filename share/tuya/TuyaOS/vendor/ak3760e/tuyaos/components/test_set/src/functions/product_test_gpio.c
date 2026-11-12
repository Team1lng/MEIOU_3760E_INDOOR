/* 
 *  @copyright Copyright (c) tuya.inc 2021
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include "product_test_protocol.h"
#include "product_test_utils.h"
#include "product_test_product_info.h"
#include "ptal_gpio.h"
#include "tb_log.h"

int product_test_gpio_init(int num,int direction,int init)
{
    return ptal_gpio_init(num,direction,init);
}

int product_test_gpio_set_direction(int num,int direction,int init)
{
    return ptal_gpio_set_direction(num,direction,init);
}

int product_test_gpio_set_value(int num, int value)
{
    return ptal_gpio_set_value(num,value);
}

int product_test_gpio_get_value(int num,int *value) 
{
    return ptal_gpio_get_value(num,value);  
}

static int __product_test_sub_mcu_gpio_group_test(int idx)
{
    GPIO_GROUP_T *pg = &g_gpio_sub_mcu_group[idx];

    if(ptal_gpio_sub_mcu_group_test(pg)) {
        TB_LOGE("group %d fail",idx);
        return -1;
     }

    return  0;
}

static int __product_test_gpio_group_test(int idx)
{
    GPIO_GROUP_T *pg = &g_gpio_group[idx];

    if(ptal_gpio_group_test(pg)) {
       TB_LOGE("group %d fail",idx);
       return -1;
    }

    return 0;
}

int product_test_gpio(GPIO_GROUP_T *group) 
{
    int i,n;
    int mode;

    if(group ==NULL) {
        TB_LOGE("invalid parm");
        return -1;
    }

    n = g_gpio_array_counts;

    TB_LOGD("gpio group %d",n);

    for(i=0;i<n;i++) {
        if(__product_test_gpio_group_test(i)) {
            memcpy(group,&g_gpio_group[i],sizeof(GPIO_GROUP_T));
            return -1;
        }
    }

    if(product_test_get_low_power_mode(&mode) ||  mode != 1) {      
        return 0;
    }

    n = g_gpio_sub_mcu_array_counts;

    TB_LOGD("sub gpio group %d",n);

    for(i=0;i<n;i++) {
        if(__product_test_sub_mcu_gpio_group_test(i)) {
            memcpy(group,&g_gpio_sub_mcu_group[i],sizeof(GPIO_GROUP_T));
            return -1;
        }
    }

    return 0;
}
