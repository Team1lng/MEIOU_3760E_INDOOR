/* 
 *  @copyright Copyright (c) tuya.inc 2021
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include "product_test_utils.h"
#include "tb_log.h"

#define BATTERY_FACTORY_KEY    "BATTERY_FACTORY"
#define BATTERY_TYPE_KEY       "BATTERY_TYPE"
#define BATTERY_CAPACITY_KEY   "BATTERY_CAPACITY"
#define BATTERY_INFO_KEY       "BATTERY_INFO" 

#define BATTERY_CAPACITY_DEF_PATH "/sys/class/power_supply/cw-bat/capacity"
#define BATTERY_CAPACITY_PATH_KEY "BATTERY_CAPACITY_PATH"

int product_test_battery_set_capacity_path(char *path)
{
    return product_test_utils_write_string(BATTERY_CAPACITY_PATH_KEY,path,1);
}

int product_test_battery_get_capacity_path(char *path) 
{
    return product_test_utils_read_string(BATTERY_CAPACITY_PATH_KEY,path);
}


int product_test_battery_get_current_capacity(uint32_t *value)
{
    char path_name[128] = {0};
    char result[32] = {0};
    char *path = BATTERY_CAPACITY_DEF_PATH;

    if(product_test_battery_get_capacity_path(path_name) == 0 && strlen(path_name)) {
        path = path_name;           
    }

    if(access(path,0)) {
        TB_LOGE("access %s fail",path);
        return -1;  
    }

    if(product_test_file_read(path,result,32) <= 0) {
        TB_LOGE("read %s fail",path);
        return -1;
    }

    *value =  atoi(result);

    return 0;
}

int product_test_battery_set_factory(char *factory) 
{
    return product_test_utils_write_string(BATTERY_FACTORY_KEY,factory,1);
}

int product_test_battery_get_factory(char *factory) 
{
    return product_test_utils_read_string(BATTERY_FACTORY_KEY,factory);
}

int product_test_battery_set_type(char *type) 
{
    return product_test_utils_write_string(BATTERY_TYPE_KEY,type,1);
}

int product_test_battery_get_type(char *type) 
{
    return product_test_utils_read_string(BATTERY_TYPE_KEY,type);
}

int product_test_battery_set_capacity(int capacity)
{
    return product_test_utils_write_int(BATTERY_CAPACITY_KEY,capacity,1);
}

int product_test_battery_get_capacity(int *capacity)
{
    return product_test_utils_read_int(BATTERY_CAPACITY_KEY,capacity);
}

int product_test_battery_set_info(char *info) 
{
    return product_test_utils_write_string(BATTERY_INFO_KEY,info,1);
}

int product_test_battery_get_info(char *info) 
{
    return product_test_utils_read_string(BATTERY_INFO_KEY,info);
}
