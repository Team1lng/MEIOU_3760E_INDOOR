#include "i2c_ctrl.h"
#include "Amy_sdk.h"
#include "Amy_log.h"
#include <sys/ioctl.h>
#include "fcntl.h"
#include <stdio.h>
#include <stdlib.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>

typedef struct i2c_context {
    int fd;
    int refcnt;
    Amy_mutex_t mutex;
}i2c_context;

static i2c_context i2c_dev[5];

int i2c_device_init(void) {
    int i;
    for (i = 0; i < sizeof(i2c_dev) / sizeof(i2c_context);i++) {
        i2c_dev[i].fd = -1;
        i2c_dev[i].refcnt = 0;
        Amy_mutex_init(&i2c_dev[i].mutex, NULL);
    }
    return 0;
}

i2c_context* i2c_device_open(int index) {
    char path[32] = { 0 };
    if (index >= sizeof(i2c_dev) / sizeof(i2c_context)) {
        return NULL;
    }
    Amy_mutex_lock(&i2c_dev[index].mutex);
    if (i2c_dev[index].refcnt == 0) {
        snprintf(path, sizeof(path) - 1, "/dev/i2c-%d", index);
        i2c_dev[index].fd = open(path, O_WRONLY);
        if (i2c_dev[index].fd < 0) {
            AMY_LOG_ERROR("open %s failed\n", path);
            return NULL;
        }
    }
    i2c_dev[index].refcnt++;
    Amy_mutex_unlock(&i2c_dev[index].mutex);
    return &i2c_dev[index];
}

int i2c_device_write(i2c_context* ctx, char addres, int reg, int reg_len, char* data, int data_len) {
    Amy_mutex_lock(&ctx->mutex);
    if (ioctl(ctx->fd, I2C_SLAVE_FORCE, addres) < 0) {
        AMY_LOG_ERROR("setslave addres failed\n");
        Amy_mutex_unlock(&ctx->mutex);
        return -1;
    }

    //写地址
    if (write(ctx->fd, &reg, reg_len) != reg_len) {
        AMY_LOG_ERROR("write reg:0x%#x failed\n", reg);
        Amy_mutex_unlock(&ctx->mutex);
        return -1;
    }

    //写数据
    if (write(ctx->fd, data, data_len) != data_len) {
        AMY_LOG_ERROR("write value failed\n");
        Amy_mutex_unlock(&ctx->mutex);
        return -1;
    }

    Amy_mutex_unlock(&ctx->mutex);
    return 0;
}

int i2c_device_read(i2c_context* ctx, char addres, int reg, int reg_len, char* data, int datasize) {
    Amy_mutex_lock(&ctx->mutex);
    if (ioctl(ctx->fd, I2C_SLAVE_FORCE, addres) < 0) {
        AMY_LOG_ERROR("oictl:setslave address failed\n");
        return -1;
    }

    //写地址
    if (write(ctx->fd, &reg, reg_len) != reg_len) {
        AMY_LOG_ERROR("write reg:0x%#x failed\n", reg);
        Amy_mutex_unlock(&ctx->mutex);
        return -1;
    }

    if (read(ctx->fd, data, datasize) <= 0) {
        AMY_LOG_ERROR("dev addr:%02x read failed \n", addres);
        Amy_mutex_unlock(&ctx->mutex);
        return -1;
    }
    Amy_mutex_unlock(&ctx->mutex);
    return 0;
}

int i2c_device_close(i2c_context* ctx) {
    Amy_mutex_lock(&ctx->mutex);
    if (ctx->refcnt > 0) {
        ctx->refcnt--;
        if (ctx->refcnt == 0) {
            close(ctx->fd);
            ctx->fd = -1;
        }
    }
    Amy_mutex_unlock(&ctx->mutex);
    return 0;
}


#if 0

int i2c_device_write_ex(i2c_context* ctx, char addres, char* reg, char* data, int len) {
    int i;
    Amy_mutex_lock(&ctx->mutex);

    ioctl(ctx->fd, I2C_SLAVE_FORCE, addres);
    for (i = 0; i < len; i++) {
        if (write(ctx->fd, &reg[i], 1) <= 0) {
            AMY_LOG_ERROR("i2c write addres %#x,reg:%#x failed\n", addres, reg[i]);
            Amy_mutex_unlock(&ctx->mutex);
            return -1;
        }
        if (write(ctx->fd, &data[i], 1) <= 0) {
            AMY_LOG_ERROR("i2c write addres %#x,reg:%#x data:%#x failed\n", addres, reg[i], data[i]);
            Amy_mutex_unlock(&ctx->mutex);
            return -1;
        }
    }
    Amy_mutex_unlock(&ctx->mutex);
    return 0;
}


int i2c_device_read_ex(i2c_context* ctx, char addres, char* reg, char* data, int len) {
    int i;
    Amy_mutex_lock(&ctx->mutex);
    ioctl(ctx->fd, I2C_SLAVE_FORCE, addres);
    for (i = 0;i < len;i++) {
        if (write(ctx->fd, &reg[i], 1) < 0) {
            AMY_LOG_ERROR("i2c sample write reg :%#x fail\n", reg[i]);
            Amy_mutex_unlock(&ctx->mutex);
            return -1;
        }

        if (read(ctx->fd, &data[i], 1) <= 0) {
            AMY_LOG_ERROR("i2c read addres %#x,reg:%d failed\n", addres, reg[i]);
            Amy_mutex_unlock(&ctx->mutex);
            return -1;
        }
    }
    Amy_mutex_unlock(&ctx->mutex);
    return 0;
}
#endif