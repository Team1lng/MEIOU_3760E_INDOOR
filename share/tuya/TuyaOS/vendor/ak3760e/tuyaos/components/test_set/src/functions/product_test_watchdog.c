/*
 *  @copyright Copyright (c) tuya.inc 2021
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <linux/watchdog.h>
#include "product_test_utils.h"
#include "product_test_config.h"
#include "ptal_watchdog.h"
#include "tb_log.h"

#define WATCHDOG_DEVICE_KEY  "watdog_dev"

static int __product_test_config_watchdog_get_name(char *name) 
{
    char *dev,*key;
    int enable;

    if(name == NULL) {
        TB_LOGE("invalid param"); 
        return -1; 
    }

    dev = CONFIG_DEVICE_WATCHDOG;
    key = CONFIG_KEY_ENABLE;

    if(product_test_config_read_int(dev,key,&enable) || enable != 1) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return-1;
    }

    key = CONFIG_KEY_DEV_NAME;
    

    if(product_test_config_read_string(dev,key,name,sizeof(name))) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
	return -1;
    }

    return 0;   
}

int product_test_watchdog_set_devname(char *name)
{
    return product_test_utils_write_string(WATCHDOG_DEVICE_KEY,name,0);
}

int product_test_watchdog_get_devname(char *name)
{
    int iret ;

    iret =  product_test_utils_read_string(WATCHDOG_DEVICE_KEY,name);

    if(iret || strlen(name) == 0 ) {
        iret = __product_test_config_watchdog_get_name(name);
    }

    return iret;
}

int product_test_watchdog_init(int ms)
{
    return ptal_watchdog_init(ms); 
}

int product_test_watchdog_uinit(void)
{
   return ptal_watchdog_uinit();
}

int product_test_watchdog_feed(void)
{
   return ptal_watchdog_feed();
}
