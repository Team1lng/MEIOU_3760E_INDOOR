/*
 *  @copyright Copyright (c) tuya.inc 2021
 */ 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <linux/fs.h>
#include <sys/ioctl.h>


#define DDR_ERROR_RANGE         6
#define READ_DDR_COMMAND        "cat /proc/zoneinfo | busybox grep present | busybox awk 'BEGIN{a=0}{a+=$2}END{print a}'"

static int __ddr_exec(const char *cmd, char  *ddrsize_char, unsigned int  length)
{
    FILE *pp = popen(cmd, "r");

    if (!pp) {
        char  * mesg = strerror(errno);
        printf("errno = %d, errmsg:%s\n", errno, mesg);
        return -1;
    }
    
    if (fgets(ddrsize_char, length, pp) == NULL) {
        printf("popen read from %s is NULL!\n",cmd);
        pclose(pp);
        return -1;
    }
    
    pclose(pp);
    
    return 0;
}

int  product_test_ddr_capacity(int  ddr_capacity)
{
    int  ddr_ret = 0;
    char  ddrsize_char[20] = {0};
    int  ddr_size = 0;
    printf("=======  ddr test starting   ========\n");
    memset(ddrsize_char, 0, sizeof(ddrsize_char));
    ddr_ret = __ddr_exec(READ_DDR_COMMAND,ddrsize_char, sizeof(ddrsize_char));
    if (ddr_ret == 0) {
        printf("======%s value is %s=====.\n",READ_DDR_COMMAND,ddrsize_char);
        ddr_size = (int)(atoi(ddrsize_char)*4/1024);
        printf("====== read ddr_size is : %dMB;  capacity = %dMB ==========\n",ddr_size, ddr_capacity);
        if (abs(ddr_capacity - ddr_size) > DDR_ERROR_RANGE) goto fail;
    } else {
        goto fail;
    }
    printf("=========== ddr test success ==========\n");

    return ddr_ret;

fail:

    printf("=========== ddr test failed ==========\n");
    ddr_ret = -1;
    
    return ddr_ret;
}

