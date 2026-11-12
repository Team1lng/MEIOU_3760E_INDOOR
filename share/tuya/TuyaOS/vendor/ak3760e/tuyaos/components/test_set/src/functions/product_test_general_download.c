/* 
 *  @copyright Copyright (c) tuya.inc 2021
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <inttypes.h>
#include "tb_log.h"
#include "product_test_utils.h"
#include "product_test_config.h"
#include "ptal_general_download.h"

typedef struct general_download_info_s {
    char file_path[128]; /*download file path*/
    int start_flag;  /*download start flag */
    int fd;  /*download file save fd */
    char *buf; /*download data buffer*/
    char type [32]; /*download file type */
    unsigned int size; /*download file size */
    int recvs; /*download receive bytes*/
    char md5 [64]; /*download file md5sum value*/
}GENERAL_DOWNLOAD_INFO_T;

#define MD5_CMD "md5sum %s"
#define DOWNLOAD_FILENAME_KEY  "download_file_name"

static GENERAL_DOWNLOAD_INFO_T  sg_general_download_info = {0};

static int product_test_config_get_download_filename(char *name)
{
    char *dev,*key;
    int enable;

    if(name == NULL) {
        TB_LOGE("invalid parm");
        return -1;  
    }

    dev = CONFIG_DEVICE_PRODUCT_CFG;
    key = CONFIG_KEY_ENABLE;

    if(product_test_config_read_int(dev,key,&enable) || enable != 1) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return-1;
    }

    key = CONFIG_KEY_CFG_DOWNLOAD_FILENAME;

    if(product_test_config_read_string(dev,key,name,64)) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return -1;
    }

    return 0;
}


int product_test_get_download_filename(char *name)
{
    int iret = 0;

    if(name == NULL) {
        TB_LOGE("invalid parm");
        return -1;  
    }

    iret = product_test_utils_read_string(DOWNLOAD_FILENAME_KEY,name);

    if(iret == 0 &&  strlen(name)) {
        TB_LOGD("name:%s",name);
        return 0;
    }

    if(product_test_config_get_download_filename(name) ||  strlen(name) == 0) {
        iret = -1;  
    }

    return iret;
}

int product_test_set_download_filename(char *name) 
{
    return product_test_utils_write_string(DOWNLOAD_FILENAME_KEY,name,0);
}

int product_test_general_download_start(char *name,char *md5,int size,char *type)
{
    int fd;
    char *buf;
    GENERAL_DOWNLOAD_INFO_T *gdl = &sg_general_download_info;

    if(md5 == NULL || size == 0)
        return -1;

    if(strlen(md5)+1 >= 64) {
        TB_LOGE(" md5 is to long");
        return -1;
    }


//  if(strcmp(type,CFG_TYPE)) {
//      TB_LOGE("type %s != %s",type,CFG_TYPE);
//      return -1;  
//  }

    memset(gdl,0,sizeof(GENERAL_DOWNLOAD_INFO_T));
    strcpy(gdl->md5,md5);
    strcpy(gdl->type,type);
    gdl->size = size;

    TB_LOGD("md5:%s,size:%d,type:%s\n",gdl->md5,size,gdl->type);

    if((fd=open(name,O_WRONLY | O_CREAT | O_TRUNC, 0600)) < 0) {
        printf("open %s fail\n",name);
        return -1;  
    }

    if((buf=(char*)malloc(size)) == NULL) {
        printf("malloc %d fail\n",size);
        close(fd);
        return -1;
    }

    gdl->buf = buf;
    gdl->fd = fd;
    gdl->start_flag = 1;

    strcpy(gdl->file_path,name);

    return 0;
}

int product_test_general_downloading(char *data,uint32_t data_len)
{
    GENERAL_DOWNLOAD_INFO_T * gdl = &sg_general_download_info;

    if(data == NULL) {
        printf("invaild parm\n");
        return -1;  
    }

    if(!gdl->start_flag) {
        printf("gdl file download is not start \n");
        return -1;  
    }

    memcpy(&gdl->buf[gdl->recvs],data,data_len);
    gdl->recvs += data_len;
    return 0;
}

int product_test_general_download_end(void)
{
    char md5[64]= {0};
    char cmd[256]= {0};
    GENERAL_DOWNLOAD_INFO_T * gdl = &sg_general_download_info;

    gdl->start_flag = 0;

    if(gdl->recvs != gdl->size) {
        TB_LOGE("recv %d != size %d",gdl->recvs,gdl->size);
        return -1;
    }

    if(write(gdl->fd,gdl->buf,gdl->size) != gdl->size) {
        close(gdl->fd);
        TB_LOGE("gdl write fail");
        return -1;              
    }

    close(gdl->fd);

    sprintf(cmd,MD5_CMD,gdl->file_path);

    if(product_test_exec_cmd_with_return(cmd, md5,64)) {
        product_test_exec_cmd_no_return("rm -rf %s",gdl->file_path);
        TB_LOGE("md5 cal fail");
        return -1;
    }

    if(strncmp(md5,gdl->md5,strlen(gdl->md5))) {
        TB_LOGE("md5(%s) != cal(%s)\n",md5,gdl->md5);
        return -1;
    }

    return 0;
}


int  product_test_general_download_file_analysis(char  *file)
{
    GENERAL_DOWNLOAD_INFO_T * gdl = &sg_general_download_info;

    TB_LOGD("file:%s:type:%s\n",file,gdl->type);

    return ptal_general_download_file_analysis(file,gdl->type);

}
