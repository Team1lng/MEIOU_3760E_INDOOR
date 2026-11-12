/* 
 *  @copyright Copyright (c) tuya.inc 2021
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include <fcntl.h>
#include "tb_log.h"
#include "product_test_config.h"
#include "product_test_crc32.h"
#include "product_test_utils.h"
#include "product_test_ota.h"

#define OTA_MAX_PATH 128

typedef struct ota_param_s {
    int init_flag; // init flag 
    char file_path[OTA_MAX_PATH];// ota firmware path
    uint32_t crc32; //ota firmware path crc32
    uint32_t size;  //ota firmware size
    uint32_t recv;  // ota firmware receive bytes
    uint32_t hash;  // ota firmware hash value
    int  cpu;       // ota sub cpu names
    int fd;         // ota firmware  opened fd
}OTA_PARAM_T;

static OTA_PARAM_T sg_ota_param = {0};
static char sg_ota_file[OTA_MAX_PATH] = {0};

#define OTA_DEFAULT_FILEPATH  "/tmp/ota_tmp.bin"

#define OTA_BUF_ALIGN (0x10 -1)
#define OTA_DEFAULT_SIZE 2048 

static unsigned long sg_ota_packet_size = OTA_DEFAULT_SIZE;

unsigned long  product_test_get_ota_packet_size(void)
{
    return sg_ota_packet_size;
}

int product_test_set_ota_packet_size(unsigned long  size) 
{

    if((size&OTA_BUF_ALIGN) != 0) {

         size = size - (size&OTA_BUF_ALIGN) + OTA_BUF_ALIGN +1;
    }

    if(size < OTA_DEFAULT_SIZE) {
        size = OTA_DEFAULT_SIZE; 
    }
      
    sg_ota_packet_size = size;

    return 0;
}

static int __product_test_config_get_ota_file(char *file,int len)
{
    char *dev,*key;
    int enable;

    dev = CONFIG_DEVICE_PRODUCT_CFG;
    key = CONFIG_KEY_ENABLE;

    if(product_test_config_read_int(dev,key,&enable) || enable != 1) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return-1;
    }

    key = CONFIG_KEY_OTA_FILE;

    if(product_test_config_read_string(dev,key,file,len)) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return -1;
    }

    return 0;
}

int product_test_get_ota_file(char *file, int len)
{
    int iret;

    if(file == NULL || len == 0) {
        TB_LOGE("invalid param"); 
	return -1;
    }

    if(strlen(sg_ota_file)) {
        strncpy(file,sg_ota_file,len);
	return 0;
    }

    iret = __product_test_config_get_ota_file(file,len);

    if(iret || strlen(file) == 0) {
        strncpy(file,OTA_DEFAULT_FILEPATH,len);
    }

    return 0;
}

int product_test_set_ota_file(char *file)
{
    if(file == NULL || strlen(file) == 0) {
        TB_LOGE("invalid param");
        return -1;	
    }

    if(strlen(file) >= OTA_MAX_PATH ) {
        TB_LOGE("file is too long");
        return -1;	
    }

    memset(sg_ota_file,0,sizeof(sg_ota_file));

    strcpy(sg_ota_file,file);

    return 0; 
}


int product_test_ota_init(unsigned long baud,unsigned long crc32,unsigned long size)
{
    OTA_PARAM_T *ota =  &sg_ota_param;

    if(!ota->cpu )
        memset(ota,0,sizeof(OTA_PARAM_T));

    ota->crc32 = crc32; 
    ota->size = size;   
    ota->init_flag = 1;
    TB_LOGD("crc32:%x,size:%x\n",ota->crc32,ota->size);

    return 0;
}

int product_test_ota_start(unsigned long *packet_size)
{
    char file_path[OTA_MAX_PATH] = {0};

    OTA_PARAM_T *ota =  &sg_ota_param;

    if(!ota->init_flag || packet_size == NULL) {
        TB_LOGE("invaild parm");
        return -1;
    }

    if(product_test_get_ota_file(file_path,OTA_MAX_PATH)) {
        TB_LOGE("get ota file fail\n");
        return -1;	
    }
   
    TB_LOGD("file:%s",file_path);

    ota->fd = open(file_path,O_RDWR|O_CREAT|O_TRUNC, 0600);

    if(ota->fd < 0){
        TB_LOGE("open %s fail\n",file_path);
        return -1;
    }

    strcpy(ota->file_path,file_path);

    *packet_size =  product_test_get_ota_packet_size();

    TB_LOGD("upgrade start!");
    return 0;
}


int product_test_ota_recv(char *data,unsigned int data_len)
{
    int write_size = 0;
    OTA_PARAM_T *ota =  &sg_ota_param;

    if(!ota->init_flag) {
        TB_LOGE("invaild parm");
        return -1;
    }

    if(ota->recv == 0)
        ota->hash = product_test_hash_crc32_init();

    ota->hash = product_test_hash_crc32_update(ota->hash,data,data_len);

    write_size = write(ota->fd,data,data_len);

    if(write_size !=  data_len) {
        TB_LOGE("write to file failed\n");
        ota->init_flag = 0;
        ota->recv = 0;
        close(ota->fd);
        unlink(ota->file_path);
        sync();
        return -1;
    }

    ota->recv += write_size;

    return 0;
}

#define UPGRADE_CMD "unpack -m -f %s"

static int __product_test_ota_upgrade(char *file_path)
{
    return product_test_exec_cmd_no_return(UPGRADE_CMD,file_path);
}

static int __product_test_slave_ota_upgrade(int cpu)
{

    return 0;
}

int product_test_ota_end(void) 
{
    int iret = 0;
    OTA_PARAM_T *ota =  &sg_ota_param;

    if(!ota->init_flag) {
        TB_LOGE("invaild parm");
        return -1;
    }

    TB_LOGD("recv size:%08x, size:%08x",ota->recv,ota->size);

    if(ota->recv != ota->size) {
        TB_LOGE("recv size:%08x, size:%08x",ota->recv,ota->size);
	iret = -1;
	goto err;
    }

    ota->hash = product_test_hash_crc32_finish(ota->hash);

    TB_LOGI("hash:%x,crc32:%x\n",ota->hash,ota->crc32);
    if(ota->hash != ota->crc32) {
        TB_LOGE("cal crc %x != crc %x\n",ota->hash,ota->crc32);
	iret = -1;
	goto err;
    }

    if(ota->cpu == 0) {        
        iret =  __product_test_ota_upgrade(ota->file_path);
    }else if(ota->cpu) {
        iret =  __product_test_slave_ota_upgrade(ota->cpu);
    }

err:
    close(ota->fd);
    unlink(ota->file_path);
    sync();
    memset(ota,0,sizeof(OTA_PARAM_T));

    TB_LOGW("iret %d",iret);

    return iret;
}


int product_test_slave_upgrade(SUB_IMAGE_TYPE_E image_type) 
{
    int iret = 0;
    OTA_PARAM_T *ota = &sg_ota_param;

    if(ota->init_flag)
        return 0;

    memset(ota,0,sizeof(OTA_PARAM_T));

    switch (image_type) {
        case E_ZIGBEE_IMG:
            ota->cpu = 1;
            break;
        case E_BT_IMG:
        case E_SUBG_IMG:
            break;
        default:
            TB_LOGE("invalid image_type:%d",image_type);
            iret = -1;
    }

    return iret;
}
