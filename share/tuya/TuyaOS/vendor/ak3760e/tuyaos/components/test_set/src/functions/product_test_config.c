/*
 * Copyright (C) 2021 Tuya Inc.
 */

#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/reboot.h>
#include <sys/stat.h>
#include "tb_log.h"
#include "cJSON.h"
#include "product_test_cfg_info.h"
#include "product_test_utils.h"
#include "product_test_config.h"
#include "product_test_protocol.h"

#define CONFIG_DEFAUT_FILE "/tmp/test_set.json"
#define CONFIG_FILE_PATH_KEY "config_file_path"

static cJSON *g_croot = NULL;

int product_test_config_get_file_path(char *path) 
{
    return product_test_utils_read_string(CONFIG_FILE_PATH_KEY,path);
}

int product_test_config_set_file_path(char *path)
{
    return product_test_utils_write_string(CONFIG_FILE_PATH_KEY,path,0);
}

int product_test_config_init(char *file_name)
{
    int size;
    char *buf = NULL;
    char *path = file_name;
    char file_path[128] = {0};
    int iret = -1;

    if (g_croot != NULL) {
            return 0;
    }

    if(path == NULL) {
        if (product_test_config_get_file_path(file_path) == 0 && strlen(file_path)) {
            path = file_path;
        }else {
            path = CONFIG_DEFAUT_FILE;
        }
    }

    TB_LOGD("path:%s",path);

    if (product_test_file_size(path, &size) != 0) {
        TB_LOGD("failed to get cfg file size\n");
        return -1;
    }

    TB_LOGD("cfg file:%s size: %d\n",path,size);

    buf = (char *)malloc(size + 1);

    if (NULL == buf) {
        TB_LOGD("malloc fail");
        return -1;
    }

    memset(buf, 0, size + 1);

    iret = product_test_file_read(path, buf, size);

    if (iret != size) {
        free(buf);
        buf = NULL;
        TB_LOGD("product test file read fail");
        return -1;
    }

    g_croot = cJSON_Parse(buf);

    if (NULL == g_croot) {
        TB_LOGD("cJSON_Parse fail\n");
        iret = -1;
    }

    free(buf);

    return 0;
}

int product_test_config_uinit(void)
{
    if(g_croot) {
        cJSON_Delete(g_croot);
        g_croot = NULL; 
    }

    return 0;   
}

static int json_node_to_string(cJSON *j_root, char *node, char *buf, int buf_len)
{
    cJSON *j_node = NULL;

    if (j_root == NULL || node == NULL || buf == NULL) {
        TB_LOGE("invalid parm");
        return -1;
    }

    j_node = cJSON_GetObjectItemCaseSensitive(j_root, node);

    if (j_node && cJSON_IsString(j_node) && (j_node->valuestring != NULL)) {
        strncpy(buf, j_node->valuestring, buf_len);
    } else {
        TB_LOGW("cJSON_GetObjectItemCaseSensitive fail");
        return -1;
    }


    return 0;
}

int product_test_config_read_string(char *device, char *key, char *buf, int bufsize)
{
    cJSON *j_device = NULL;

    if ((device == NULL) || (key == NULL) || (buf == NULL)) {
        TB_LOGE("invalid parm");
        return -1;
    }

    TB_LOGD("device:%s,key:%s",device,key);

    if (product_test_config_init(NULL) != 0) {
        TB_LOGE("product_test_config_init fail");
        return -1;
    }

    if (NULL == g_croot) {
        TB_LOGE("g_croot is not init"); 
        return -1;
    }

    j_device = cJSON_GetObjectItemCaseSensitive(g_croot, device);

    if (j_device == NULL) {
        TB_LOGW("g_croot,device %s fail",device);
        return -1;
    }

    if (json_node_to_string(j_device, key, buf, bufsize) < 0) {
        TB_LOGW("json_node_to_string device %s,key %s fail",device,key);
        return -1;
    }

    TB_LOGD("device:%s,key:%s,value:%s",device,key,buf);

    return 0;
}

int product_test_config_read_int(char *device, char *key, int *value)
{
    char buf[32] = {0};

    TB_LOGD("device:%s,key:%s",device,key);

    if (value == NULL) {
        TB_LOGE("invalid parm");
        return -1;
    }

    if (product_test_config_read_string(device, key, buf, sizeof(buf)) < 0) {
        TB_LOGW("device:%s,key:%s fail",device,key);
        return -1;
    }

    if ((0 == strncmp(buf, "0x", 2)) || (0 == strncmp(buf, "0X", 2))) {
        sscanf(buf + 2, "%x", value);
    } else {
        sscanf(buf, "%d", value);
    }

    TB_LOGD("device:%s,key:%s:value:%d",device,key,*value);

    return 0;
}

int product_test_config_get_uart_info(UART_INFO_T *info) 
{
    int enable;
    char *dev,*key;


    if(info == NULL) {
        TB_LOGE("invalid parm");
        return -1;  
    }

    memset(info,0,sizeof(UART_INFO_T));

    dev = CONFIG_DEVICE_UART;
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

int product_test_config_check_uart_loop_support(void) 
{
    int enable;
    int loop;
    char *dev,*key;
    int iret = -1;


    dev = CONFIG_DEVICE_UART;
    key = CONFIG_KEY_ENABLE;

    if(product_test_config_read_int(dev,key,&enable) || enable != 1) {
        TB_LOGW("device:%s key:%s fail",dev,key);
	return -1;
    }

    dev = CONFIG_DEVICE_UART;
    key = CONFIG_KEY_UART_LOOP;

    if(product_test_config_read_int(dev,key,&loop) == 0 || loop == 1) {
	iret = 0;
    }

    TB_LOGW("device:%s key:%s loop:%d",dev,key,loop);

    return iret;
}

int product_test_config_get_tcp_info(TCP_INFO_T *info)
{
    int enable;
    char *dev,*key;

    if(info == NULL) {
        TB_LOGE("invalid parm");
        return -1;  
    }

    dev = CONFIG_DEVICE_TCP;
    key = CONFIG_KEY_ENABLE;

    if(product_test_config_read_int(dev,key,&enable) || enable != 1) {
        TB_LOGW("device:%s key:%s fail",dev,key);
        return-1;
    }

    dev = CONFIG_DEVICE_TCP;
    key = CONFIG_KEY_NET_POART;

    memset(info,0,sizeof(TCP_INFO_T));

    if(product_test_config_read_int(dev,key,&info->port) || info->port == 0) {
        TB_LOGW("device:%s key:%s fail",dev,key);
        return -1;
    }

    return 0;   
}
