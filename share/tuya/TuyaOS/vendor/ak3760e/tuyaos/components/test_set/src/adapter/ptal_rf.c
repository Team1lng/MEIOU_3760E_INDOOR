#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <inttypes.h>
#include <string.h>
#include "product_test_sub_mcu.h"
#include "product_test_utils.h"
#include "tb_log.h"

#define WIFI_RF_TEST_FLAG_KEY   "wifi_rf"
#define BT_RF_TEST_FLAG_KEY     "bt_rf"


#define HW_WLAN0_TX_POWER_CCK_A_CMD "flash get HW_WLAN0_TX_POWER_CCK_A"
#define HW_WLAN0_TX_POWER_CCK_B_CMD "flash get HW_WLAN0_TX_POWER_CCK_B"
/*
int __attribute__((weak)) ptal_get_wifi_rf_flag(int *flag)
{
    return product_test_utils_read_int(WIFI_RF_TEST_FLAG_KEY,flag);
}
*/

int __attribute__((weak)) ptal_get_wifi_rf_flag(int *flag)
{
    int cck_a=1,cck_b=1;
    char *ptr;
    char buf[128] ={0};

    TB_LOGD();

    sprintf(buf,HW_WLAN0_TX_POWER_CCK_A_CMD);

    if(product_test_exec_cmd_with_return(HW_WLAN0_TX_POWER_CCK_A_CMD,buf,sizeof(buf))) {
        TB_LOGE("%s fail",HW_WLAN0_TX_POWER_CCK_A_CMD); 
        return -1;  
    }

    if((ptr=strstr(buf,"=")) == 0 || strtoul(ptr+1,NULL,16) == 0) {
        cck_a = 0;  
    }


    if(product_test_exec_cmd_with_return(HW_WLAN0_TX_POWER_CCK_B_CMD,buf,sizeof(buf))) {
        TB_LOGE("%s fail",HW_WLAN0_TX_POWER_CCK_A_CMD); 
        return -1;  
    }

    if((ptr = strstr(buf,"=")) == 0 || strtoul(ptr+1,NULL,16) == 0) {
        cck_b = 0;
    }

    if(cck_a == 0 || cck_b == 0) {
        *flag = 0;
    }else {
        *flag = 1;
    }

    TB_LOGD("cc_a:%d,cc_b:%d,*flag:%d",cck_a,cck_b,*flag);

    return  0;
}

int __attribute__((weak)) ptal_set_wifi_rf_flag(int flag)
{
    return product_test_utils_write_int(WIFI_RF_TEST_FLAG_KEY,flag,1);
}

int __attribute__((weak)) ptal_get_bt_rf_flag(int *flag)
{
    return product_test_utils_read_int(BT_RF_TEST_FLAG_KEY,flag);
}

int __attribute__((weak)) ptal_set_bt_rf_flag(int flag)
{
    return product_test_utils_write_int(BT_RF_TEST_FLAG_KEY,flag,1);
}



