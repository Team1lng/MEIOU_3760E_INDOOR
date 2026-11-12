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
#include "product_test_cfg_info.h"
#include "product_test_config.h"
#include "ptal_zigbee.h"
#include "tb_log.h"

static int __product_test_config_zigbee_info(UART_INFO_T *info) 
{
    int enable;
    char *dev,*key;


    if(info == NULL) {
        TB_LOGE("invalid parm");
        return -1;  
    }

    memset(info,0,sizeof(UART_INFO_T));

    dev = CONFIG_DEVICE_ZIGBEE;
    key = CONFIG_KEY_ENABLE;

    if(product_test_config_read_int(dev,key,&enable) || enable != 1) {
        TB_LOGW("device:%s key:%s fail",dev,key);
        return-1;
    }

    key = CONFIG_KEY_UART_BAUD;

    if(product_test_config_read_int(dev,key,&info->baud) || info->baud == 0) {
        TB_LOGW("device:%s key:%s fail",dev,key);
        return-1;
    } 

    key = CONFIG_KEY_DEV_NAME;

    if(product_test_config_read_string(dev,key,info->name,sizeof(info->name)) || strlen(info->name) == 0) {
        TB_LOGW("device:%s key:%s fail",dev,key);
        return-1;
    }

    return 0;
}



int product_test_zigbee_get_version_info(char *buf, int size)
{

    UART_INFO_T info= {0};
    ZIGBEE_PARAM_T param = {0};


    if(__product_test_config_zigbee_info(&info) == 0) {
        strcpy(param.name,info.name);
	param.baud = info.baud;
    }

    return ptal_zigbee_get_version_info(&param,buf,size);
}

int product_test_zigbee_get_firmware_info(char *buf,int size) 
{
    UART_INFO_T info= {0};
    ZIGBEE_PARAM_T param = {0};

    TB_LOGD();


    if(__product_test_config_zigbee_info(&info) == 0) {
        strcpy(param.name,info.name);
	param.baud = info.baud;
    }

    return ptal_zigbee_get_firmware_info(&param,buf,size);
}


int product_test_zigbee_test(int channel,int send_pkts,int *recv_pkts)
{
    int pkts;
    int iret;

    UART_INFO_T info= {0};
    ZIGBEE_PARAM_T param = {0};

    if(__product_test_config_zigbee_info(&info) == 0) {
        strcpy(param.name,info.name);
	param.baud = info.baud;
    }

    param.channel= channel;
    param.pkts = send_pkts;

    iret = ptal_zigbee_test(&param,&pkts);

    if(iret == 0) {
        *recv_pkts =  pkts;
	TB_LOGD("succeed send %d pkts",*recv_pkts);
    }else {
        TB_LOGE("zigbee test fail"); 
    }

    return iret;
}
