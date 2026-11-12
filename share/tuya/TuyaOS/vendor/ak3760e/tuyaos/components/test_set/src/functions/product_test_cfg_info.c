/*
 *  @copyright Copyright (c) tuya.inc 2021
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include "tb_log.h"
#include "product_test_config.h"
#include "product_test_utils.h"
#include "ptal_mac.h"
#include "ptal_auth.h"

#define CFG_MASTER_MAC_KEY "master_mac"

#define CFG_SSID_KEY "ssid"
#define CFG_PWD_KEY "pwd"
#define CFG_PSKKEY_KEY "pskkey"
#define CFG_SLAVE_MAC_KEY "slave_mac%d"
#define CFG_BSN_KEY "BSN"
#define CFG_SN_KEY "SN"
#define CFG_COUNTRY_KEY "country"
#define CFG_PROD_TEST_KEY "prod_test"
#define CFG_ZIGBEE_KEY                 "ZIGBEE_ENABLE"
#define CFG_LED_BRIGHTNESS_KEY         "led_brightness"
#define CFG_BLE_NAME_KEY               "ble_name"
#define CFG_MCU_UART_SPEED_KEY         "MCU_UART_SPEED"
#define CFG_FIRM_TYPE_KEY              "FIRM_TYPE"
#define CFG_GST_MAX_VALUE_KEY          "GST_MAX_VOLUME"
#define CFG_AVS_PID_KEY                "AVS_PID"
#define CFG_AVS_RESPAND_LUANUAGE_KEY   "respand_luanuge"
#define CFG_AVS_WAKEUP_LUANUAGE_KEY    "wakeup_luanuge"
#define CFG_AVS_GATEWAY_ADDR_KEY       "avs_gateway_addr"
#define CFG_WF_COUNTRY_CODE_KEY        "WF_COUNTRY_CODE"
#define CFG_AGEING_MODE_KEY            "persist.ageing.mode"
#define CFG_AGEING_TIME_KEY            "persist.ageing.time"

#define CFG_STATUS_KEY "STATUS"
#define DYNAMIC_PASSWD_KEY "dynamic_passwd_support"
#define CFG_PERSIST_SDK_VARIETY   "persist.sdk.variety"

#define TUYA_GST_MAX_VOL_DEFAULT  1.0
#define MAX_VALUE_LEN       100
#define CFG_RUN_DIR_KEY "run_dir"
#define DEFUALT_RUN_DIR "/tmp/"

static int __product_test_config_get_config_dir(char *dir)
{
    char *dev,*key;
    int enable;

    dev = CONFIG_DEVICE_PRODUCT_CFG;
    key = CONFIG_KEY_ENABLE;

    if(product_test_config_read_int(dev,key,&enable) || enable != 1) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return-1;
    }

    key = CONFIG_KEY_CONFIG_DIR;

    if(product_test_config_read_string(dev,key,dir,65)) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return -1;
    }

    return 0;
}

static int __product_test_config_get_dynamic_passwd_support(int *flag)
{
    char *dev,*key;
    int enable;

    dev = CONFIG_DEVICE_PRODUCT_CFG;
    key = CONFIG_KEY_ENABLE;

    if(product_test_config_read_int(dev,key,&enable) || enable != 1) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return-1;
    }

    key = CONFIG_KEY_CFG_DYNAMIC_PASSWD;

    if(product_test_config_read_int(dev,key,flag)) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return -1;
    }

    return 0;
}

int product_test_get_config_dir(char * dir)
{
    int iret;
    iret = product_test_utils_read_string(CFG_RUN_DIR_KEY,dir);

    if(iret == 0 &&  strlen(dir)) {
        return 0;
    }

    iret = __product_test_config_get_config_dir(dir);

    if(iret || strlen(dir) == 0) {
        strcpy(dir,DEFUALT_RUN_DIR);
    }

    TB_LOGD("dir:%s",dir);

    return 0;
}

int product_test_set_config_dir(char *dir)
{
    return product_test_utils_write_string(CFG_RUN_DIR_KEY,dir,0);
}

int product_test_set_auth_info(char *pid,char *uuid,char *auzkey)
{
    return ptal_set_auth_info(pid,uuid,auzkey);
}


int product_test_get_auth_info(char *pid,char *uuid,char *auzkey)
{
    if(pid == NULL || uuid == NULL || auzkey ==NULL) {
        TB_LOGE("invalid param");
        return -1; 
    }

    return ptal_get_auth_info(pid,uuid,auzkey);
}


int product_test_set_ssid(char *ssid)
{
    return product_test_utils_write_string(CFG_SSID_KEY,ssid,1);
}

int product_test_get_ssid(char *ssid)
{
    return product_test_utils_read_string(CFG_SSID_KEY,ssid);
}


int product_test_set_pwd(char *pwd)
{
    return product_test_utils_write_string(CFG_PWD_KEY,pwd,1);
}

int product_test_get_pwd(char *pwd)
{
    return product_test_utils_read_string(CFG_PWD_KEY,pwd);
}

int product_test_set_pskkey(char *pskkey)
{
    return product_test_utils_write_string(CFG_PSKKEY_KEY,pskkey,1);
}

int product_test_get_pskkey(char *pskkey)
{
    return product_test_utils_read_string(CFG_PSKKEY_KEY,pskkey);
}

int product_test_set_mmac(char *mac)
{
	return ptal_set_mmac(mac);
}

int product_test_get_mmac(char *mac)
{
	return ptal_get_mmac(mac);
}


int product_test_set_smac(int cpu,char *mac)
{
	return ptal_set_smac(cpu, mac);
}


int product_test_get_smac(int cpu,char *mac)
{
	return ptal_get_smac(cpu, mac);
}

int product_test_set_bsn(char *bsn)
{
    return product_test_utils_write_string(CFG_BSN_KEY,bsn,1);
}

int product_test_get_bsn(char*bsn)
{
    return product_test_utils_read_string(CFG_BSN_KEY,bsn);
}


int product_test_set_sn(char *sn)
{
    return product_test_utils_write_string(CFG_SN_KEY,sn,1);
}

int product_test_get_sn(char*sn)
{
    return product_test_utils_read_string(CFG_SN_KEY,sn);
}

int product_test_set_country(char *country)
{
    return product_test_utils_write_string(CFG_COUNTRY_KEY,country,1);
}

int product_test_get_country(char*country)
{
    return product_test_utils_read_string(CFG_COUNTRY_KEY,country);
}

int product_test_set_prod_test(int prod_test)
{
    return product_test_utils_write_int(CFG_PROD_TEST_KEY,prod_test,1);
}

int product_test_get_prod_test(int *prod_test)
{
    return product_test_utils_read_int(CFG_PROD_TEST_KEY,prod_test);
}

// cyl
int product_test_set_zigbee_enable(int enable)
{
    return product_test_utils_write_int(CFG_ZIGBEE_KEY,enable,1);
}

int product_test_get_zigbee_enable(int *enable)
{
    return product_test_utils_read_int(CFG_ZIGBEE_KEY,enable);
}

int product_test_set_led_brightness(int val)
{
    return product_test_utils_write_int(CFG_LED_BRIGHTNESS_KEY,val,1);
}

int product_test_get_led_brightness(int *val)
{
    return product_test_utils_read_int(CFG_LED_BRIGHTNESS_KEY,val);
}

int product_test_set_ble_name(char *name)
{
    return product_test_utils_write_string(CFG_BLE_NAME_KEY,name,1);
}

int product_test_get_ble_name(char  *name)
{
    return product_test_utils_read_string(CFG_BLE_NAME_KEY,name);
}
int product_test_set_uart_speed(int val)
{
    return product_test_utils_write_int(CFG_MCU_UART_SPEED_KEY,val,1);
}

int product_test_get_uart_speed(int *val)
{
    return product_test_utils_read_int(CFG_MCU_UART_SPEED_KEY,val);
}
int product_test_set_firm_type(char *type)
{
    return product_test_utils_write_string(CFG_FIRM_TYPE_KEY,type,1);
}

int product_test_get_firm_type(char  *type)
{
    return product_test_utils_read_string(CFG_FIRM_TYPE_KEY,type);
}

int product_test_set_max_volume(float fval)
{
    if (fval > 1.0) {
        fval = 1.0;
    }
    if (fval < 0.0) {
        fval = 0.0;
    }
    char  str[MAX_VALUE_LEN] = {0x0};
    printf("save max gst volume : %.2f\n", fval);
    snprintf(str, MAX_VALUE_LEN,"%.2f", fval);
    return product_test_utils_write_string(CFG_GST_MAX_VALUE_KEY,str,1);
}

int product_test_get_max_volume(float *fval)
{
    char  gst_vol_buf[MAX_VALUE_LEN + 1] = {0x0};
    float tmp_fval = TUYA_GST_MAX_VOL_DEFAULT;
    product_test_utils_read_string(CFG_GST_MAX_VALUE_KEY,gst_vol_buf);
    tmp_fval = atof(gst_vol_buf);
    if (*fval <= 0.000001 || *fval > 1.0) {
        tmp_fval = TUYA_GST_MAX_VOL_DEFAULT;
    }
    *fval = tmp_fval;
    return 0;
}

int product_test_set_avs_pid(char *pid)
{
    return product_test_utils_write_string(CFG_AVS_PID_KEY,pid,1);
}

int product_test_get_avs_pid(char  *pid)
{
    return product_test_utils_read_string(CFG_AVS_PID_KEY,pid);
}

int product_test_set_respond_lanuage(char *lanuage)
{
    return product_test_utils_write_string(CFG_AVS_RESPAND_LUANUAGE_KEY,lanuage,1);
}

int product_test_get_respond_lanuage(char  *lanuage)
{
    return product_test_utils_read_string(CFG_AVS_RESPAND_LUANUAGE_KEY,lanuage);
}

int product_test_set_wakeup_lanuage(char *lanuage)
{
    return product_test_utils_write_string(CFG_AVS_WAKEUP_LUANUAGE_KEY,lanuage,1);
}

int product_test_get_wakeup_lanuage(char  *lanuage)
{
    return product_test_utils_read_string(CFG_AVS_WAKEUP_LUANUAGE_KEY,lanuage);
}

int product_test_set_gateway_addr(char *address)
{
    return product_test_utils_write_string(CFG_AVS_GATEWAY_ADDR_KEY,address,1);
}

int product_test_get_gateway_addr(char  *address)
{
    return product_test_utils_read_string(CFG_AVS_GATEWAY_ADDR_KEY,address);
}

int product_test_set_active_status(char *status)
{
    return product_test_utils_write_string(CFG_STATUS_KEY,status,1);
}

int product_test_get_active_status(char  *status)
{
    return product_test_utils_read_string(CFG_STATUS_KEY,status);
}

int product_test_set_dynamic_passwd_support(int flag)
{
    return product_test_utils_write_int(DYNAMIC_PASSWD_KEY,flag,0);
}

int product_test_get_dynamic_passwd_support(int *flag)
{
    int iret;

    iret =  product_test_utils_read_int(DYNAMIC_PASSWD_KEY,flag);

    if(iret) {
        iret = __product_test_config_get_dynamic_passwd_support(flag);
    }

    return iret;
}

int product_test_set_persist_sdk_variety(char *status)
{
    return product_test_utils_write_string(CFG_PERSIST_SDK_VARIETY,status,1);
}

int product_test_get_persist_sdk_variety(char  *status)
{
    return product_test_utils_read_string(CFG_PERSIST_SDK_VARIETY,status);
}
