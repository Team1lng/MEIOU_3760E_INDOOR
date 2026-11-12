/* 
 *  @copyright Copyright (c) tuya.inc 2021
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>

//vendor_storage 
#define DEV_NAME    "/dev/vendor_storage"
#define VENDOR_BUF_MAX_LEN                1024
#define VENDOR_HW_CK_ID                       8
#define VENDOR_REQ_TAG      0x56524551
#define VENDOR_READ_IO      _IOW('v', 0x01, unsigned int)
#define VENDOR_WRITE_IO     _IOW('v', 0x02, unsigned int)

struct rk_vendor_req {
    unsigned int tag;
    unsigned short id;
    unsigned short  len;
    unsigned char data[VENDOR_BUF_MAX_LEN];
};


static int __vendor_storage_read(int id,char *buf)
{
    if (!buf ){
        printf("invalid parameter\n");
        return -1;
    }
    int ret ;
    struct rk_vendor_req req;
    int fd = open(DEV_NAME,O_RDWR,0);
    if(fd < 0){
        printf("vendor_storage open fail\n");
        return -1;
    }
    req.tag = VENDOR_REQ_TAG;
    req.id = id;
    ret = ioctl(fd, VENDOR_READ_IO, &req);
    if (ret < 0) {
        printf("vendor storage ioctl error\n");
        return -1;
    }
    memcpy(buf, req.data,req.len);
    close(fd);
    return 0;
}


static int __vendor_storage_wirte(int id,char *buf, int len)
{
    if ((len <= 0) || (len > VENDOR_BUF_MAX_LEN) || !buf ){
        printf("invalid parameter\n");
        return -1;
    }
    int ret ;
    struct rk_vendor_req req;
    int fd = open(DEV_NAME,O_RDWR,0);
    if(fd < 0){
        printf("vendor_storage open fail\n");
        return -1;
    }
    req.tag = VENDOR_REQ_TAG;
    req.id = id;
    memcpy(req.data,buf,len);
    req.len = len;
    ret = ioctl(fd, VENDOR_WRITE_IO, &req);
    if (ret < 0){
        printf("vendor storage ioctl error\n");
        return -1;
    }
    close(fd);
    return 0;
}

int  product_test_read_tag_bit(int *val)
{
        
    if (val == NULL) {
        printf("invalid parameter\n");
        return  -1;
    }
    int  ret = -1;
    char  buf[VENDOR_BUF_MAX_LEN+1] = {0};
    ret = __vendor_storage_read(VENDOR_HW_CK_ID, buf);

    if (-1  == ret) {
        if (1 == atoi(buf)) {
            *val = 1;
        } else {
            *val = 0;
        }
    }

    return ret;    

}

int  product_test_write_tag_bit(int val)
{
    int  ret = -1;
    char buf[VENDOR_BUF_MAX_LEN+1] = {0};
    snprintf(buf,VENDOR_BUF_MAX_LEN,"%d",val);
    ret = __vendor_storage_wirte(VENDOR_HW_CK_ID, buf,strlen(buf));
    if (ret < 0){
        return -1;
    }
    return 0;
}
