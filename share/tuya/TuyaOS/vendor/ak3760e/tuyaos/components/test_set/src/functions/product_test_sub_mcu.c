#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include <pthread.h>
#include "ptal_sub_mcu.h"
#include "product_test_utils.h"
#include "product_test_sub_mcu.h"
#include "product_test_key.h"
#include "tb_log.h"

static int sub_device_init_flag = 0;

static int product_test_sub_mcu_init(void) 
{
    return ptal_sub_mcu_init();
}

int product_test_sub_mcu_get_version(unsigned int *pver)
{
    if(sub_device_init_flag== 0) {
        if (product_test_sub_mcu_init()) {
            TB_LOGE("product_test_sub_mcu_init failed");
            return -1;        
        }
        sub_device_init_flag = 1;
        usleep(100000);
    }

    return ptal_sub_mcu_get_version(pver);
}

int product_test_sub_mcu_get_wifi_rf_flag(int *flag)
{

    if(sub_device_init_flag== 0) {
        if (product_test_sub_mcu_init()) {
            TB_LOGE("####failed\n");
            return -1;        
        }
        sub_device_init_flag = 1;
        usleep(100000);
    }

        return ptal_sub_mcu_get_wifi_rf_flag(flag);
}

int product_test_sub_mcu_connect_ap(char *ssid,char *psk)
{
    return ptal_sub_mcu_connect_ap(ssid,psk);
}

int product_test_sub_mcu_write_mac(char *mac) 
{
    return ptal_sub_mcu_write_mac(mac);
}

int product_test_sub_mcu_get_pir_events(int* cnts)
{
    if(sub_device_init_flag== 0) {
        if (product_test_sub_mcu_init()) {
            TB_LOGE("product_test_sub_mcu_init failed\n");
            return -1;        
        }
        sub_device_init_flag = 1;
        usleep(100000);
    }

    return ptal_sub_mcu_get_pir_event(cnts);
}

int product_test_sub_mcu_pir_test(int counts) 
{
    int cnts= 0;
    int iret = -1;
    int totals;


    TB_LOGD();

    if(counts <= 0) {
   	totals = 1; 
    }else {
        totals = counts; 
    }

    if(product_test_sub_mcu_get_pir_events(&cnts) == 0 && cnts >= totals) {
        iret = 0;   
    }

    return iret;
}

int product_test_sub_mcu_get_key(int n,int *flag) 
{
    TB_LOGD();
    if(n  >= MAX_SUB_MCU_KEY_NUM ) {
        TB_LOGE(" n(%d) > MAX(%d)",n,MAX_SUB_MCU_KEY_NUM);
        return -1;
    }

    return ptal_sub_mcu_get_key(n,flag);
}

int product_test_sub_mcu_get_sleep_flag(int *flag) 
{
    return ptal_sub_mcu_get_sleep_flag(flag);
}

int product_test_sub_mcu_set_deep_sleep(void) 
{
    if(sub_device_init_flag== 0) {
        if (product_test_sub_mcu_init()) {
            TB_LOGE("product_test_sub_mcu_init failed\n");
            return -1;        
        }
        sub_device_init_flag = 1;
        usleep(100000);
    }

    return ptal_sub_mcu_set_deep_sleep();
}

int product_test_sub_mcu_get_wifi_rssi(int *rssi)
{
    if(sub_device_init_flag== 0) {
        if (product_test_sub_mcu_init()) {
            TB_LOGE("product_test_sub_mcu_init failed\n");
            return -1;        
	}
        sub_device_init_flag = 1;
        usleep(100000);
    }

    return ptal_sub_mcu_get_wifi_rssi(rssi);
}

int product_test_sub_mcu_set_mac(char *mac)
{
    if(mac == NULL) {
        TB_LOGE("invalid param"); 
	return -1;
    }

    return ptal_sub_mcu_set_mac(mac);
}
