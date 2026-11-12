#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include "ptal_zigbee.h"
#include "tb_log.h"


int __attribute__((weak))ptal_zigbee_get_version_info(ZIGBEE_PARAM_T *param,char *buf, int size) 
{
    TB_LOGD();
    return -1;
}

int __attribute__((weak))ptal_zigbee_get_firmware_info(ZIGBEE_PARAM_T *param,char *buf, int size) 
{
    TB_LOGD();
    return -1;
}


int __attribute__((weak)) ptal_zigbee_test(ZIGBEE_PARAM_T *param,int *recv_ptks) 
{
    TB_LOGD();
    return -1;
}

