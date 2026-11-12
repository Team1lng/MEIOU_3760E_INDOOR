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
#include <sys/reboot.h>
#include <sys/ioctl.h>
#include <linux/ioctl.h>
#include <sys/stat.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include "sys_log.h"
#include "tkl_file.h"
#include "tkl_spi.h"

#ifndef TAG 
#define TAG "tkl_spi"
#endif

#ifndef MAX_PATH
#define MAX_PATH 256
#endif

#define SPIDEV_PATH     "/dev/spidev%d.%d"

#define SPIDEV_DEV_MAX  8   // Maximum number of SPI controllers
#define SPIDEV_CS_MAX   16  // Maximum number of CS per SPI controller

#define SPIDEV_DEV_IS_VALID(dev)   (((dev) >= 0) && ((dev) < SPIDEV_DEV_MAX))
#define SPIDEV_CS_IS_VALID(cs)     (((cs) >= 0) && ((cs) < SPIDEV_CS_MAX))

// fd list of spi devices
static int sg_spidev_list[SPIDEV_DEV_MAX][SPIDEV_CS_MAX] = {{-1}};

static int __spidev_is_valid(int dev, int cs)
{
    if (!SPIDEV_DEV_IS_VALID(dev)) {
        LOGE(TAG, "invalid spi device %d\n", dev);
        return 0;
    }
    if (!SPIDEV_CS_IS_VALID(cs)) {
        LOGE(TAG,"invalid spi CS %d\n",cs);
        return 0;
    }
    return 1;
}

OPERATE_RET tkl_spi_init(TUYA_SPI_NUM_E port, CONST TUYA_SPI_BASE_CFG_T *cfg)
{
    int dev = (int)port;
    int cs = 0;
    UINT8_T mode = 0;
    UINT8_T bits;
    UINT_T  speed;
    char path[MAX_PATH] = {0};
    int fd = -1;
    int ret = 0;

    if (!__spidev_is_valid(dev, cs)) {
        return OPRT_INVALID_PARM;
    }

    if (cfg == NULL) {
        return OPRT_INVALID_PARM;
    }

    snprintf(path,MAX_PATH,SPIDEV_PATH, dev, cs);

    fd = open(path, O_RDWR);

    if (fd < 0) {
        LOGE(TAG, "Failed to open %s\n", path);
        return OPRT_FILE_NOT_FIND;
    }

    if (cfg->mode == TUYA_SPI_MODE0) {
        mode |= SPI_MODE_0;
    } else if (cfg->mode == TUYA_SPI_MODE1) {
        mode |= SPI_MODE_1;
    } else if (cfg->mode == TUYA_SPI_MODE2) {
        mode |= SPI_MODE_2;
    } else {
        mode |= SPI_MODE_3;
    }

    if (cfg->databits == TUYA_SPI_DATA_BIT8) {
        bits = 8;
    } else {
        bits = 16;
    }

    speed = cfg->freq_hz;

    ret |= ioctl(fd, SPI_IOC_WR_MODE, &mode);
    ret |= ioctl(fd, SPI_IOC_RD_MODE, &mode);
    // bits per word
    ret |= ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
    ret |= ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
    // max speed hz
    ret |= ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
    ret |= ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);

    if (ret != 0) {
        close(fd);
        LOGE(TAG,"Failed to configure %s\n", path);
        return OPRT_COM_ERROR;
    }

    sg_spidev_list[dev][cs] = fd;

    return OPRT_OK;
}

OPERATE_RET tkl_spi_deinit(TUYA_SPI_NUM_E port)
{
    int cs = 0;
    int dev = (UINT32_T) port;

    if (!__spidev_is_valid(dev, cs)) {
        return OPRT_INVALID_PARM;
    }

    if ((sg_spidev_list[dev][cs] != 0) && (sg_spidev_list[dev][cs] != -1)) {
        close(sg_spidev_list[dev][cs]);
        sg_spidev_list[dev][cs] = -1;
    }

    return OPRT_OK;
}

OPERATE_RET tkl_spi_transfer(TUYA_SPI_NUM_E port, VOID_T* send_buf, VOID_T* receive_buf, UINT32_T length)
{
    int ret;
    int fd;
    int cs = 0;
    int dev = (int)port;

    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)send_buf,
        .rx_buf = (unsigned long)receive_buf,
        .len = length,
        .delay_usecs = 0,
    };

    if ((send_buf == NULL) || (receive_buf == NULL)) {
        return OPRT_INVALID_PARM;
    }

    if (!__spidev_is_valid(dev, cs)) {
        return OPRT_INVALID_PARM;
    }

    fd = sg_spidev_list[dev][cs];

    ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);

    if (ret < 1) {
        LOGE(TAG,"spi transfer failed\n");
        return OPRT_COM_ERROR;
    }

    return OPRT_OK;
}


OPERATE_RET tkl_spi_abort_transfer(TUYA_SPI_NUM_E port)
{
    return OPRT_NOT_SUPPORTED;
}

OPERATE_RET tkl_spi_get_status(TUYA_SPI_NUM_E port, TUYA_SPI_STATUS_T *status)
{
    return OPRT_NOT_SUPPORTED;
}

OPERATE_RET tkl_spi_irq_init(TUYA_SPI_NUM_E port, TUYA_SPI_IRQ_CB cb)
{
    return OPRT_NOT_SUPPORTED;
}

OPERATE_RET tkl_spi_irq_enable(TUYA_SPI_NUM_E port)
{
    return OPRT_NOT_SUPPORTED;
}

OPERATE_RET tkl_spi_irq_disable(TUYA_SPI_NUM_E port)
{
    return OPRT_NOT_SUPPORTED;
}

OPERATE_RET tkl_spi_send(TUYA_SPI_NUM_E port, VOID_T *data, UINT16_T size)
{
    return OPRT_NOT_SUPPORTED;
}

OPERATE_RET tkl_spi_recv(TUYA_SPI_NUM_E port, VOID_T *data, UINT16_T size)
{
    return OPRT_NOT_SUPPORTED;
}

INT32_T tkl_spi_get_data_count(TUYA_SPI_NUM_E port)
{
    return OPRT_NOT_SUPPORTED;
}

OPERATE_RET tkl_spi_ioctl(TUYA_SPI_NUM_E port, UINT32_T cmd,  VOID *args)
{
    return OPRT_NOT_SUPPORTED;
}
