#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "tb_log.h"

int __attribute__((weak)) ptal_bt_get_rssi(char *name,int *rssi) 
{
    TB_LOGD();
    return -1;
}
