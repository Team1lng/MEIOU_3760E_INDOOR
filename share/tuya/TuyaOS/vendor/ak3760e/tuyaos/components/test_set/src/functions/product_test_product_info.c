/*
 *  @copyright Copyright (c) tuya.inc 2021
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include "ptal_version.h"
#include "product_test.h"
#include "product_test_utils.h"
#include "product_test_config.h"
#include "product_test_protocol.h"
#include "product_test_zigbee.h"

#include "tb_log.h"

#define PRODUCT_MAIN_FIRMWARE_NAME_KEY      "pt_main_firmware_name"
#define PRODUCT_MAIN_FIRMWARE_VERSION_KEY       "pt_main_firmware_version"

#define PRODUCT_SUB_FIRMWARE_NAME_KEY       "pt_sub_firmware_name%d"
#define PRODUCT_SUB_FIRMWARE_VERSION_KEY    "pt_sub_firmware_version%d"

#define PRODUCT_PROTOCOL_VERSION_KEY            "pt_protocol_version"
#define PRODUCT_PROTOCOL_TYPE_KEY               "pt_protocol_type"
#define PRODUCT_DEVICE_TYPE_KEY             "pt_device_type"
#define PRODUCT_MAC_WRITE_KEY                   "pt_mac_write"
#define PRODUCT_PSKKEY_WRITE_KEY                "pt_pskkey_write"
#define PRODUCT_HIMEKITTYPE_KEY                 "pt_himekitType"
#define PRODUCT_LOWPOWER_KEY                    "pt_low_power"

static int __product_test_config_get_mac_write(int *mac_write) 
{
    char *dev,*key;
    int enable;

    dev = CONFIG_DEVICE_PRODUCT_INFO;
    key = CONFIG_KEY_ENABLE;

    if(product_test_config_read_int(dev,key,&enable) || enable != 1) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return-1;
    }

    key = CONFIG_KEY_MAC_WRITE;

    if(product_test_config_read_int(dev,key,mac_write)) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return -1;
    }
    return 0;
}

static int __product_test_config_get_main_firmware_name(char *firmware_name) 
{
    char *dev,*key;
    int enable;

    dev = CONFIG_DEVICE_PRODUCT_INFO;
    key = CONFIG_KEY_ENABLE;

    if(product_test_config_read_int(dev,key,&enable) || enable != 1) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return-1;
    }

    key = CONFIG_KEY_FIRMWARE_NAME;

    if(product_test_config_read_string(dev,key,firmware_name,PARM_MIN_SIZE)) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return -1;
    }
    return 0;
}

static int __product_test_config_get_main_firmware_version(char *firmware_version) 
{
    char *dev,*key;
    int enable;

    dev = CONFIG_DEVICE_PRODUCT_INFO;
    key = CONFIG_KEY_ENABLE;

    if(product_test_config_read_int(dev,key,&enable) || enable != 1) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return-1;
    }

    key = CONFIG_KEY_FIRMWARE_VERSION;
    
    if(product_test_config_read_string(dev,key,firmware_version,PARM_MIN_SIZE)) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return -1;
    }
    return 0;
}

static int __product_test_config_get_sub_firmware_name(int cpu,char *firmware_name) 
{
    char *dev;
    char key[128];
    int enable;

    dev = CONFIG_DEVICE_PRODUCT_INFO;

    if(product_test_config_read_int(dev,CONFIG_KEY_ENABLE,&enable) || enable != 1) {
        TB_LOGE("device:%s parm:%s fail",dev,CONFIG_KEY_ENABLE);
        return-1;
    }

    snprintf(key,128,CONFIG_KEY_SUB_FIRMWARE_NAME,cpu);

    if(product_test_config_read_string(dev,key,firmware_name,PARM_MIN_SIZE)) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return -1;
    }
    return 0;
}

static int __product_test_config_get_sub_firmware_version(int cpu,char *firmware_version) 
{
    char *dev;
    char key[128];
    int enable;

    dev = CONFIG_DEVICE_PRODUCT_INFO;

    if(product_test_config_read_int(dev,CONFIG_KEY_ENABLE,&enable) || enable != 1) {
        TB_LOGE("device:%s parm:%s fail",dev,CONFIG_KEY_ENABLE);
        return-1;
    }

    snprintf(key,128,CONFIG_KEY_SUB_FIRMWARE_VERSION,cpu);
    
    if(product_test_config_read_string(dev,key,firmware_version,PARM_MIN_SIZE)) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return -1;
    }
    return 0;
}


static int __product_test_config_get_protocol_version(char *protocol_version)
{
    char *dev,*key;
    int enable;

    dev = CONFIG_DEVICE_PRODUCT_INFO;
    key = CONFIG_KEY_ENABLE;

    if(product_test_config_read_int(dev,key,&enable) || enable != 1) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return-1;
    }

    key = CONFIG_KEY_PROTOCOL_VERSION;
    
    if(product_test_config_read_string(dev,key,protocol_version,PARM_MIN_SIZE)) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return -1;
    }

    return 0;
}

static int __product_test_config_get_protocol_type(char *protocol_type)
{
    char *dev,*key;
    int enable;

    dev = CONFIG_DEVICE_PRODUCT_INFO;
    key = CONFIG_KEY_ENABLE;

    if(product_test_config_read_int(dev,key,&enable) || enable != 1) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return-1;
    }


    key = CONFIG_KEY_PROTOCOL_TYPE;
    
    if(product_test_config_read_string(dev,key,protocol_type,PARM_MIN_SIZE)) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return -1;
    }
    return 0;
}

static int __product_test_config_get_product_type(char *product_type) 
{
    char *dev,*key;
    int enable;

    dev = CONFIG_DEVICE_PRODUCT_INFO;
    key = CONFIG_KEY_ENABLE;

    if(product_test_config_read_int(dev,key,&enable) || enable != 1) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return-1;
    }

    key = CONFIG_KEY_DEV_TYPE;
    
    if(product_test_config_read_string(dev,key,product_type,PARM_MIN_SIZE)) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return -1;
    }
    return 0;
}

static int product_test_config_get_low_power_mode(int* mode)
{
    char *dev,*key;
    int enable;

    dev = CONFIG_DEVICE_PRODUCT_INFO;
    key = CONFIG_KEY_ENABLE;

    if(product_test_config_read_int(dev,key,&enable) || enable != 1) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return-1;
    }

    key = CONFIG_KEY_LOWPOWER_MODE;

    if(product_test_config_read_int(dev,key,mode)) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return -1;
    }

    return 0;
}


int product_test_set_main_firmware_name(char *firmware_name) 
{
    return product_test_utils_write_string(PRODUCT_MAIN_FIRMWARE_NAME_KEY,firmware_name,0);
}

int product_test_get_main_firmware_name(char *firmware_name)
{
    int iret;

    iret = product_test_utils_read_string(PRODUCT_MAIN_FIRMWARE_NAME_KEY,firmware_name);

    if(iret ||  strlen(firmware_name)==0) {
        iret =  __product_test_config_get_main_firmware_name(firmware_name);
    }
    return iret;    
}

int product_test_set_main_firmware_version(char *firmware_version)
{
    return product_test_utils_write_string(PRODUCT_MAIN_FIRMWARE_VERSION_KEY,firmware_version,0);
}

int product_test_get_main_firmware_version(char *firmware_version)
{
    int iret ;

    iret =  product_test_utils_read_string(PRODUCT_MAIN_FIRMWARE_VERSION_KEY,firmware_version);

    if(iret || strlen(firmware_version) == 0 ) {
        iret =  __product_test_config_get_main_firmware_version(firmware_version);
    }

    return iret;
}

int product_test_set_sub_firmware_name(int cpu,char *firmware_name) 
{
    char key[64] = {0};
    snprintf(key,64,PRODUCT_SUB_FIRMWARE_VERSION_KEY,cpu);
    return product_test_utils_write_string(key,firmware_name,0);
}


int product_test_get_sub_firmware_name(int cpu,char *firmware_name) 
{
   
    int iret = -1;
    char key[64] = {0};

    switch(cpu) {
    case 1: //zigbee
         iret = product_test_zigbee_get_firmware_info(firmware_name,PARM_MIN_SIZE); 

	  break;
    default:
	  break;
    
    }

    if(iret == 0) {
        return 0; 
    }

    snprintf(key,64,PRODUCT_SUB_FIRMWARE_NAME_KEY,cpu);

    iret =  product_test_utils_read_string(key,firmware_name);

    if(iret || strlen(firmware_name) == 0 ) {
        iret =  __product_test_config_get_sub_firmware_name(cpu,firmware_name);
    }

    return iret;
}


int product_test_set_sub_firmware_version(int cpu,char *firmware_version) 
{
    char key[64] = {0};
    snprintf(key,64,PRODUCT_SUB_FIRMWARE_NAME_KEY,cpu);

    return product_test_utils_write_string(key,firmware_version,0);
}

int product_test_get_sub_firmware_version(int cpu,char *firmware_version) 
{
    int iret = 0;
    char key[64] = {0};

    switch(cpu) {
    case 1: //zigbee
         iret = product_test_zigbee_get_version_info(firmware_version,PARM_MIN_SIZE); 
	  break;
    default:
	  break;
    }

    if(iret == 0) {
        return 0; 
    }

    snprintf(key,64,PRODUCT_SUB_FIRMWARE_NAME_KEY,cpu);

    iret = product_test_utils_read_string(key,firmware_version);

    if(iret || strlen(firmware_version) == 0 ) {
        iret =  __product_test_config_get_sub_firmware_version(cpu,firmware_version);
    }

    return iret;
}

int product_test_set_protocol_version(char *protocol_version)
{
    return product_test_utils_write_string(PRODUCT_PROTOCOL_VERSION_KEY,protocol_version,0);
}

int product_test_get_protocol_version(char *protocol_version)
{
    int iret;

    iret = product_test_utils_read_string(PRODUCT_PROTOCOL_VERSION_KEY,protocol_version);

    if(iret || strlen(protocol_version) == 0) {
        iret = __product_test_config_get_protocol_version(protocol_version);
    }

    return iret;
}


int product_test_set_protocol_type(char *protocol_type)
{
    return product_test_utils_write_string(PRODUCT_PROTOCOL_TYPE_KEY,protocol_type,0);
}

int product_test_get_protocol_type(char *protocol_type)
{
    int iret;

    iret = product_test_utils_read_string(PRODUCT_PROTOCOL_TYPE_KEY,protocol_type);

    if(iret) {
    
        iret = __product_test_config_get_protocol_type(protocol_type);
    }

    return iret;
}

int product_test_set_product_type(char *product_type) 
{
    return product_test_utils_write_string(PRODUCT_DEVICE_TYPE_KEY,product_type,0);
}

int product_test_get_product_type(char *product_type)
{
    int iret;

    iret =  product_test_utils_read_string(PRODUCT_DEVICE_TYPE_KEY,product_type);

    if(iret || strlen(product_type)==0) {
        iret =  __product_test_config_get_product_type(product_type);
    }

    return iret;
}

int product_test_set_mac_write(int mac_write) 
{
    return product_test_utils_write_int(PRODUCT_MAC_WRITE_KEY,mac_write,0);
}

int product_test_get_mac_write(int *mac_write) 
{
    int iret;

    iret =  product_test_utils_read_int(PRODUCT_MAC_WRITE_KEY,mac_write);

    if(iret) {
        iret =  __product_test_config_get_mac_write(mac_write);
    }

    return iret;
}

int product_test_set_pskkey_write(int pskkey_write) 
{
    return product_test_utils_write_int(PRODUCT_PSKKEY_WRITE_KEY,pskkey_write,0);
}

int product_test_get_pskkey_write(int* pskkey_write) 
{
    return product_test_utils_read_int(PRODUCT_PSKKEY_WRITE_KEY,pskkey_write);
}

int product_test_set_himekitType(int himekitType)
{
    return product_test_utils_write_int(PRODUCT_HIMEKITTYPE_KEY,himekitType,0);
}

int product_test_get_himekitType(int* himekitType)
{
    return product_test_utils_read_int(PRODUCT_HIMEKITTYPE_KEY,himekitType);
}

int product_test_set_low_power_mode(int mode) 
{
    return product_test_utils_write_int(PRODUCT_LOWPOWER_KEY,mode,0);
}

int product_test_get_low_power_mode(int* mode)
{
    int iret;

    iret =  product_test_utils_read_int(PRODUCT_LOWPOWER_KEY,mode);

    if(iret) {

        iret =  product_test_config_get_low_power_mode(mode);
    }

    return iret;
}

#define DEF_VERSION_FILE "/tuya/app/skyeye/conf/version"
#define VERSION_PATH_KEY  "version_path"

static int __product_test_config_get_version_path(char *version_path) 
{
    char *dev,*key;
    int enable;

    dev = CONFIG_DEVICE_PRODUCT_CFG;
    key = CONFIG_KEY_ENABLE;

    if(product_test_config_read_int(dev,key,&enable) || enable != 1) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return-1;
    }

    key = CONFIG_KEY_CFG_VERSION_PATH;

    if(product_test_config_read_string(dev,key,version_path,PARM_MAX_SIZE)) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return -1;
    }

    return 0;
}

int product_test_set_version_path(char *version_path) 
{
    return product_test_utils_write_string(VERSION_PATH_KEY,version_path,0);
}

int product_test_get_version_path(char *version_path)
{
    int iret;

    iret = product_test_utils_read_string(VERSION_PATH_KEY,version_path);

    if(iret || strlen(version_path)==0) {
        iret =  __product_test_config_get_version_path(version_path);
    }

    if(iret || strlen(version_path) == 0) {
        strcpy(version_path,DEF_VERSION_FILE);
    }

    return 0;
}


int product_test_get_version_info(char *buf, int size)
{
    return ptal_get_version_info(buf,size);
}


int product_test_get_uart_info(UART_INFO_T *info) 
{
    return product_test_config_get_uart_info(info); 
}

int product_test_get_tcp_info(TCP_INFO_T *info) 
{
    return product_test_config_get_tcp_info(info);
}

int product_test_init_config(char *file_name) 
{
    return product_test_config_init(file_name);
}
