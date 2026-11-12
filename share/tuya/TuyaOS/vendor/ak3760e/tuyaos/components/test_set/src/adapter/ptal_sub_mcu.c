#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include "ptal_sub_mcu.h"
#include "tb_log.h"

int __attribute__((weak)) ptal_sub_mcu_init(void)
{
    TB_LOGD();
    return -1;  
}

int __attribute__((weak)) ptal_sub_mcu_get_version(unsigned int *pver) 
{
    TB_LOGD();
    return -1;
}

int __attribute__((weak))ptal_sub_mcu_get_wifi_rf_flag(int *flag)
{
    TB_LOGD();
    return -1;
}

int __attribute__((weak)) ptal_sub_mcu_connect_ap(char * ssid, char * passwd)
{
    TB_LOGD();
    return -1;
}

int __attribute__((weak)) ptal_sub_mcu_write_mac(char *mac)
{
    TB_LOGD();
    return -1;
}

int __attribute__((weak))ptal_sub_mcu_get_pir_event(int *cnts) 
{
    TB_LOGD();
    return -1;  
}

int __attribute__((weak)) ptal_sub_mcu_get_key(int n,int *flag) 
{
    TB_LOGD();
    return -1;
}

int __attribute__((weak))ptal_sub_mcu_get_sleep_flag(int *flag)
{
    TB_LOGD();
    return -1;
}

int __attribute__((weak)) ptal_sub_mcu_set_deep_sleep(void) 
{
    TB_LOGD();
    return -1;
}

int __attribute__((weak)) ptal_sub_mcu_get_wifi_rssi(int *rssi) 
{
    TB_LOGD();
    return -1;
}

int __attribute__((weak)) ptal_sub_mcu_get_wake_in(int *value)
{
    TB_LOGD();
    return -1;
}

int __attribute__((weak))  ptal_sub_mcu_set_wake_out(int value)
{
    TB_LOGD();
    return -1;
}

int __attribute__((weak)) ptal_sub_mcu_get_wake_panel(int *value)
{
    TB_LOGD();
    return -1;
}

int __attribute__((weak)) ptal_sub_mcu_set_mac(char *mac)
{
    TB_LOGD();
    return -1;
}

