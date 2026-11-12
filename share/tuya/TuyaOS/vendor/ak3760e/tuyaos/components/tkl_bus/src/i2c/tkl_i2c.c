/*
 * Copyright (C) 2021 Ty Inc.
 */

#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <linux/types.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include "sys_log.h"
#include "tkl_i2c.h"

#ifndef TAG
#define TAG "tkl_i2c"
#endif

#ifndef MAX_PATH
#define MAX_PATH 256
#endif

#define I2C_DEV_PATH    "/dev/i2c-%d"   // the name of 'i2c-dev' device

typedef struct i2c_param
{
    int init;
    int fd; 
    TUYA_IIC_BASE_CFG_T cfg;
} I2C_PARAM_T;

static I2C_PARAM_T sg_i2c_param[TUYA_I2C_NUM_MAX] = {0};


OPERATE_RET tkl_i2c_init(TUYA_I2C_NUM_E port, CONST TUYA_IIC_BASE_CFG_T *cfg)
{
    int fd;
    int index = (int)port;
    char path[MAX_PATH] = {0};
    I2C_PARAM_T *param;

    if(index < 0 || index >=  TUYA_I2C_NUM_MAX || cfg == NULL) {
        LOGE(TAG,"invalid param\n");
        return OPRT_INVALID_PARM;
    }

    param = &sg_i2c_param[index];

    if(param->init == 1) {
        return OPRT_OK;
    }

    if(cfg->role == TUYA_IIC_MODE_SLAVE) {
        LOGE(TAG,"slave mode is unsupport\n");
        return OPRT_NOT_SUPPORTED;
    }

    param->cfg.role = cfg->role;
    param->cfg.speed = cfg->speed;
    param->cfg.addr_width = cfg->addr_width;

    snprintf(path,MAX_PATH,I2C_DEV_PATH,index);

    fd = open(path, O_RDWR);

    if (fd < 0) {
        LOGE(TAG, "Failed to open %s\n", path);
        return OPRT_FILE_NOT_FIND;
    }

    param->fd = fd;
    param->init = 1;

    return OPRT_OK;
}


OPERATE_RET tkl_i2c_deinit(TUYA_I2C_NUM_E  port)
{
    int index = (int)port;
    I2C_PARAM_T *param;

    if(index < 0 || index >=  TUYA_I2C_NUM_MAX) {
        LOGE(TAG,"invalid param\n");
        return OPRT_INVALID_PARM;
    }

    param = &sg_i2c_param[index];

    if(param->init) {
	close(param->fd);
	memset(&param->cfg,0,sizeof(TUYA_IIC_BASE_CFG_T));
	param->init = 0;
    }

    return OPRT_OK;
}

OPERATE_RET tkl_i2c_irq_init(TUYA_I2C_NUM_E port,TUYA_I2C_IRQ_CB cb)
{
    return OPRT_NOT_SUPPORTED;
}


OPERATE_RET tkl_i2c_irq_enable(TUYA_I2C_NUM_E port)
{
    return OPRT_NOT_SUPPORTED;
}


OPERATE_RET tkl_i2c_irq_disable(TUYA_I2C_NUM_E port)
{
    return OPRT_NOT_SUPPORTED;
}


OPERATE_RET tkl_i2c_master_send(TUYA_I2C_NUM_E port, UINT16_T dev_addr, CONST VOID_T *data, UINT32_T size, BOOL_T xfer_pending)
{
    int fd;
    int index = (int)port;
    I2C_PARAM_T *param;
    uint16_t flags = 0;
    struct i2c_rdwr_ioctl_data packets;
    struct i2c_msg messages[1];
    uint8_t *buf = (uint8_t*)data;


    if(index < 0 || index >=  TUYA_I2C_NUM_MAX || buf == NULL || size < 2 ) {
        LOGE(TAG,"invalid param\n");
        return OPRT_INVALID_PARM;
    }

    param = &sg_i2c_param[index];

    if(!param->init) {
        LOGE(TAG,"i2c%d uninit\n",index);
        return OPRT_COM_ERROR;
    }

    if (param->cfg.addr_width & TUYA_IIC_ADDRESS_10BIT) {
        flags |= I2C_M_TEN;
    }

    fd = param->fd;

/*
    if (xfer_pending & TUYA_FLAG_I2C_IGNORE_NACK) {
            flags |= I2C_M_IGNORE_NAK;
    }
    */

    messages[0].addr = dev_addr;
    messages[0].buf = buf;
    messages[0].len = size;
    messages[0].flags = flags;

    packets.msgs = messages;
    packets.nmsgs = 1;

    if (ioctl(fd, I2C_RDWR, &packets) < 0) {
        LOGE(TAG,"i2c_send: I2C_RDWR failed\n");
        return OPRT_COM_ERROR;
    }

    return OPRT_OK;
}

OPERATE_RET tkl_i2c_master_receive(TUYA_I2C_NUM_E port, UINT16_T dev_addr, VOID *data, UINT32_T size, BOOL_T xfer_pending)
 {
    int fd;
    int index = (int)port;
    I2C_PARAM_T *param;
    unsigned char reg_addr[2] = {0};
    uint16_t flags = I2C_M_RD;
    struct i2c_rdwr_ioctl_data packets;
    struct i2c_msg messages[2];
    uint8_t *buf = (uint8_t*)data;

    if(index < 0 || index >=  TUYA_I2C_NUM_MAX || buf == NULL || size < 1) {
        LOGE(TAG,"invalid param\n");
        return OPRT_INVALID_PARM;
    }

    param = &sg_i2c_param[index];

    if(!param->init) {
        LOGE(TAG,"i2c%d uninit\n",index);
        return OPRT_COM_ERROR;
    }

    if (param->cfg.addr_width & TUYA_IIC_ADDRESS_10BIT) {
        flags |= I2C_M_TEN;
    }

    fd = param->fd;

/*
    if (xfer_pending & TUYA_FLAG_I2C_IGNORE_NACK) {
            flags |= I2C_M_IGNORE_NAK;
    }
    */

    // ????
    reg_addr[0] = buf[0];

    messages[0].addr = dev_addr;
    messages[0].buf = &reg_addr[0];

    if(xfer_pending) {
        reg_addr[1] = buf[1];
        messages[0].len =2;
    }else {
        messages[0].len =1;
    }

    messages[0].flags = flags & (~I2C_M_RD);

    // read data
    messages[1].addr = dev_addr;
    messages[1].buf = buf;
    messages[1].len = size;
    messages[1].flags = flags;

    packets.msgs = messages;
    packets.nmsgs = 2;

    if (ioctl(fd, I2C_RDWR, &packets) < 0) {
        LOGE(TAG,"i2c_receive: I2C_RDWR failed\n");
        return OPRT_COM_ERROR;
    }

    return OPRT_OK;
 }

OPERATE_RET tkl_i2c_set_slave_addr(TUYA_I2C_NUM_E port, UINT16_T dev_addr)
{
    return OPRT_NOT_SUPPORTED;
}


OPERATE_RET tkl_i2c_slave_send(TUYA_I2C_NUM_E port, CONST VOID *data, UINT32_T size)
{
    return OPRT_NOT_SUPPORTED;
}


OPERATE_RET tkl_i2c_slave_receive(TUYA_I2C_NUM_E port, VOID *data, UINT32_T size)
{
    return OPRT_NOT_SUPPORTED;
}


OPERATE_RET tkl_i2c_get_status(TUYA_I2C_NUM_E port, TUYA_IIC_STATUS_T *status)
{
    return OPRT_NOT_SUPPORTED;
}


OPERATE_RET  tkl_i2c_reset(TUYA_I2C_NUM_E port)
{
    return OPRT_NOT_SUPPORTED;
}

INT32_T tkl_i2c_get_data_count(TUYA_I2C_NUM_E port)
{
    return OPRT_NOT_SUPPORTED;
}
