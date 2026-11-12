#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "product_test_utils.h"
#include "product_test_config.h"
#include "tb_log.h"

#define SUBG_DEVNAME_KEY "subg_dev"

static int __product_test_config_subg_get_devname(char* name)
{
    char *dev,*key;
    int enable;

    dev = CONFIG_DEVICE_SUBG;
    key = CONFIG_KEY_ENABLE;

    if(product_test_config_read_int(dev,key,&enable) || enable != 1) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return-1;
    }

    key = CONFIG_KEY_DEV_NAME;
    
    if(product_test_config_read_string(dev,key,name,32)) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return -1;
    }

    return 0;   
}


int product_test_subg_set_devname(char *name)
{
    return product_test_utils_write_string(SUBG_DEVNAME_KEY,name,0);
}

int product_test_subg_get_devname(char *name)
{
    int iret;
    iret = product_test_utils_read_string(SUBG_DEVNAME_KEY,name);

    if(iret || strlen(name) == 0) {
        iret =  __product_test_config_subg_get_devname(name);
    }

    return iret;
}


static const char sg_subg_device_tables[][4] = {
    {0x14,0x1c,0xd5,0x00},
    {0x46,0x0e,0x01,0x29},
    {0x46,0x0e,0x01,0x3a},
    {0x1f,0x32,0xc6,0x00},
    {0x0e,0x7a,0x03,0x1d},
    {0x1d,0x34,0x04,0x1b},
    {0xab,0xcd,0x48,0x00},
    {0xab,0xcd,0x44,0x00},
    {0xab,0xcd,0x42,0x00},
    {0xab,0xcd,0x48,0x00},
    {0x80,0x6d,0x04,0x1a},
    {0x80,0x6d,0x04,0x29},
    {0x80,0x6d,0x04,0x3a},
};

int product_test_subg_receive_test(int timeout)
{
    int fd;
    char name[65] = {0};
    char read_buf[4];
    int i,n;
    unsigned long start_time;


    if(timeout <= 0) {
        timeout = 30; 
    }

    if(product_test_subg_get_devname(name)) {
        TB_LOGD("invalid parm");
        return -1;
    }

    fd = open(name,O_RDWR);

    if(fd < 0){
        TB_LOGD("open %s error\n",name);
        return -1;
    }

    start_time = product_test_get_curtime();
    
    while(LESS_TARGET(product_test_get_curtime(),start_time,timeout)) {
        int total= sizeof(sg_subg_device_tables)/sizeof(sg_subg_device_tables[0])
            ;
        n = sizeof(read_buf)/sizeof(char);
        if(read(fd, read_buf,n) !=n ) {
            TB_LOGE("read error %d",n);
            continue;
        }

        TB_LOGD("read (%d)bytes(0x%x, 0x%x, 0x%x, 0x%x)\n",n,read_buf[0], read_buf[1], read_buf[2], read_buf[3]);

        for(i=0;i<total;i++) {
            if(memcmp(read_buf,sg_subg_device_tables[i],n) == 0) {
                close(fd);
                return 0;   
            }
        } 

        sleep(1);
    }

    close(fd);

    return -1;
}

int product_test_subg_test(int *num) 
{
   return product_test_subg_receive_test(0);
}
