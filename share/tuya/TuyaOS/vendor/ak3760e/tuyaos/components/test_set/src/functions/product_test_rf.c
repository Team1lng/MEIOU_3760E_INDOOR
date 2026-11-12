/*
 *  @copyright Copyright (c) tuya.inc 2021
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "product_test_utils.h"
#include "product_test_sub_mcu.h"
#include "product_test_product_info.h"
#include "ptal_rf.h"
#include "tb_log.h"


int product_test_bt_get_rf_detect_flag(int *flag) 
{
    return ptal_get_bt_rf_flag(flag); 
}

int product_test_wifi_get_rf_detect_flag(int *flag) 
{
    int mode;
    int iret = -1;

    if(flag == NULL) {
        TB_LOGE("invalid parm");	
        return -1;
    }

    *flag = 0;
    
    if(product_test_get_low_power_mode(&mode) == 0 && mode ==1) {
        iret = product_test_sub_mcu_get_wifi_rf_flag(flag);
    }else{
        iret = ptal_get_wifi_rf_flag(flag);
    }
    return iret;
}


int product_test_check_all_rf_status(void)
{
    int flag;

    if(product_test_bt_get_rf_detect_flag(&flag) ||  flag != 1) {
        TB_LOGE("test bt rf error\n");
        return -1;
    }

    if(product_test_wifi_get_rf_detect_flag(&flag) || flag != 1) {
        TB_LOGE("test wifi  rf error\n");
        return -1;	
    }

    return 0;
}

int product_test_wifi_set_rf_detect_flag(int flag) 
{
    return ptal_set_wifi_rf_flag(flag);
}

int product_test_bt_set_rf_detect_flag(int flag) 
{
    return ptal_set_bt_rf_flag(flag);
}
