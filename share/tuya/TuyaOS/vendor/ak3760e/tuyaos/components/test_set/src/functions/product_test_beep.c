/* 
 *  @copyright Copyright (c) tuya.inc 2021
 */
#include <inttypes.h> 
#include <stdio.h>
#include <malloc.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <time.h>
#include <locale.h>
#include <sys/unistd.h>
#include "product_test_cfg_info.h"
#include "product_test_utils.h"
#include "product_test_gpio.h"
#include "product_test_config.h"
#include "tb_log.h"

int product_test_beep_set_gpio(char *name, int level)
{
    return 0;
}


int product_test_beep_set_gpio_num(int gpio_num)
{
    return 0;
}

int product_test_beep_get_gpio_num(int*gpio_num)
{
    return 0;
}

int product_test_beep_set_gpio_level(int level)
{
    return 0;
}

int product_test_beep_get_gpio_level(int*level)
{
    return 0;
}


int product_test_beep_start(int hz,int times_s) 
{
    return 0;
}

int product_test_beep_stop(void) 
{
    return 0;
}
