/* 
 *  @copyright Copyright (c) tuya.inc 2021
 */
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "ptal_passwd.h"
#include "product_test_utils.h"
#include "product_test_config.h"
#include "product_test_protocol.h"
#include "tb_log.h"

#define DEF_SHADOW_PATH "/etc/shadow"
#define PASSWD_PATH_KEY  "passwd_path"

static int __product_test_config_passwd_path(char *passwd_path) 
{
    char *dev,*key;
    int enable;

    dev = CONFIG_DEVICE_PRODUCT_CFG;
    key = CONFIG_KEY_ENABLE;

    if(product_test_config_read_int(dev,key,&enable) || enable != 1) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return-1;
    }

    key = CONFIG_KEY_CFG_PASSWD_PATH;

    if(product_test_config_read_string(dev,key,passwd_path,PARM_MAX_SIZE)) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return -1;
    }

    return 0;
}

int product_test_set_passwd_path(char *passwd_path) 
{
    return product_test_utils_write_string(PASSWD_PATH_KEY,passwd_path,0);
}

int product_test_get_passwd_path(char *passwd_path)
{
    int iret;

    iret = product_test_utils_read_string(PASSWD_PATH_KEY,passwd_path);

    if(iret || strlen(passwd_path)==0) {
        iret =  __product_test_config_passwd_path(passwd_path);
    }

    if(iret || strlen(passwd_path) == 0) {
        strcpy(passwd_path,DEF_SHADOW_PATH);
    }

    return 0;
}


int product_test_passwd_change(char *uuid,char *authkey)
{
    return ptal_passwd_set(uuid,authkey);;
}

int product_test_passwd_reset(void)
{
    return ptal_passwd_reset();
}
