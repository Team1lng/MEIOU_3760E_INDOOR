/* 
 *  @copyright Copyright (c) tuya.inc 2021
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "ptal_bt.h"


int product_test_bt_get_rssi(char *name,int *rssi)
{
    return ptal_bt_get_rssi(name,rssi);
}


