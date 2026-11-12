#include "hal_ir.h"
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "hal_log.h"
#include "hal_board.h"


/*IR ID define*/
#define IR_LIRC  "/dev/lirc"
#define IR_LIRC0 "/dev/lirc0"
#define IR_LIRC1 "/dev/lirc1"

/*macro from lirc.h*/
#define PULSE_BIT       0x01000000
#define PULSE_MASK      0x00FFFFFF

#define LIRC_MODE2_SPACE     0x00000000
#define LIRC_MODE2_PULSE     0x01000000
#define LIRC_MODE2_FREQUENCY 0x02000000
#define LIRC_MODE2_TIMEOUT   0x03000000

#define LIRC_VALUE_MASK      0x00FFFFFF
#define LIRC_MODE2_MASK      0xFF000000

#define LIRC_SPACE(val) (((val)&LIRC_VALUE_MASK) | LIRC_MODE2_SPACE)
#define LIRC_PULSE(val) (((val)&LIRC_VALUE_MASK) | LIRC_MODE2_PULSE)
#define LIRC_FREQUENCY(val) (((val)&LIRC_VALUE_MASK) | LIRC_MODE2_FREQUENCY)
#define LIRC_TIMEOUT(val) (((val)&LIRC_VALUE_MASK) | LIRC_MODE2_TIMEOUT)

#define LIRC_VALUE(val) ((val)&LIRC_VALUE_MASK)
#define LIRC_MODE2(val) ((val)&LIRC_MODE2_MASK)

#define LIRC_IS_SPACE(val) (LIRC_MODE2(val) == LIRC_MODE2_SPACE)
#define LIRC_IS_PULSE(val) (LIRC_MODE2(val) == LIRC_MODE2_PULSE)
#define LIRC_IS_FREQUENCY(val) (LIRC_MODE2(val) == LIRC_MODE2_FREQUENCY)
#define LIRC_IS_TIMEOUT(val) (LIRC_MODE2(val) == LIRC_MODE2_TIMEOUT)

#define lirc_t int
/*end of macro from lirc.h*/


/* Don't modify this IR max duration limit, because kernel have a same limitation */
#define IR_MAX_DURATION         500000000    /* 500 ms */

typedef unsigned int __u32;
/* IOCTL commands for lirc driver */
#define LIRC_GET_FEATURES              _IOR('i', 0x00000000, __u32)
#define LIRC_GET_SEND_MODE             _IOR('i', 0x00000001, __u32)
#define LIRC_GET_REC_MODE              _IOR('i', 0x00000002, __u32)
#define LIRC_GET_REC_RESOLUTION        _IOR('i', 0x00000007, __u32)
#define LIRC_GET_MIN_TIMEOUT           _IOR('i', 0x00000008, __u32)
#define LIRC_GET_MAX_TIMEOUT           _IOR('i', 0x00000009, __u32)
/* Note: these can reset the according pulse_width */
#define LIRC_SET_SEND_CARRIER          _IOW('i', 0x00000013, __u32)
#define LIRC_SET_REC_CARRIER           _IOW('i', 0x00000014, __u32)
#define LIRC_SET_SEND_DUTY_CYCLE       _IOW('i', 0x00000015, __u32)
#define LIRC_SET_TRANSMITTER_MASK      _IOW('i', 0x00000017, __u32)

/* get IR device name from configuration file */
static void get_ir_dev_name(int is_rx, char *dev_name)
{
    char buf[MAX_PATH] = {0};
    int ret;

    if (is_rx) {
        ret = hal_cfg_get_item_string(ITEM_IR, KEY_IR_DEVICE_RX, buf, sizeof(buf));
        if (ret != 0) {
            strncpy(dev_name, IR_LIRC0, strlen(IR_LIRC0) + 1);
            return;
        }
    } else {
        ret = hal_cfg_get_item_string(ITEM_IR, KEY_IR_DEVICE_TX, buf, sizeof(buf));
        if (ret != 0) {
            strncpy(dev_name, IR_LIRC1, strlen(IR_LIRC1) + 1);
            return;
        }
    }

    strncpy(dev_name, buf, strlen(buf) + 1);
    return;
}

static int ID_to_IR_devname(hal_board_id id, int flag, char *dev_name)
{
    switch (id) {
    case HAL_BOARD_RK3308:
        if (flag) {
            strncpy(dev_name, IR_LIRC0, strlen(IR_LIRC0) + 1);
        } else {
            strncpy(dev_name, IR_LIRC1, strlen(IR_LIRC1) + 1);
        }
        break;
    default:
        get_ir_dev_name(flag, dev_name);
        break;
    }

    return 0;
}

int hal_ir_set_tx_carrier(hal_board_id id, unsigned int frequency){

    int fd = 0;
    int ret = 0;
    char dev_name[80] = {0};

    if(!frequency){
        hal_log(HAL_LOG_ERR, "%s: invalid argument\n", __func__);
        return -EINVAL;
    }

    ret = ID_to_IR_devname(id, 0, dev_name);
    if(ret < 0){
        hal_log(HAL_LOG_ERR, "%s: Invaild board ID:%d\n", __func__, id);
        return ret;
    }

    fd = open(dev_name, O_WRONLY);
    if(fd < 0) {
        hal_log(HAL_LOG_ERR, "%s: open device failed errno:%d\n", __func__, -errno);
        return -errno;
    }

    ret = ioctl(fd, LIRC_SET_SEND_CARRIER, &frequency);
    if(ret < 0){
        hal_log(HAL_LOG_ERR, "%s: ioctl failed errno:%d\n", __func__, -errno);
        close(fd);
        return -errno;
    }


    close(fd);
    return 0;
}

int hal_ir_rawcode_tx(hal_board_id id, ir_raw_code *buf){

    int i = 0;
    int fd = 0;
    int ret = 0;
    char dev_name[80] = {0};
    unsigned int duration = 0; /* signal duration in us */

    if(!buf){
        hal_log(HAL_LOG_ERR, "%s: invalid argument\n", __func__);
        return -EINVAL;
    }

    if(buf->count % 2 == 0){
        hal_log(HAL_LOG_ERR, "%s: invalid argument, IR rawcode need paried space/pulse\n", __func__);
        return -EINVAL;
    }

    for(i = 0; i < buf->count; i++){
        if(buf->codelist[i] > (IR_MAX_DURATION / 1000 - duration) || !(buf->codelist[i])){
            hal_log(HAL_LOG_ERR, "%s: invalid argument, IR rawcode over max duration or code = 0\n", __func__);
            return -EINVAL;
        }
        duration += buf->codelist[i];
    }

    ret = ID_to_IR_devname(id, 0, dev_name);
    if(ret < 0){
        hal_log(HAL_LOG_ERR, "%s: Invaild board ID:%d\n", __func__, id);
        return ret;
    }

    fd = open(dev_name, O_WRONLY);
    if(fd < 0) {
        hal_log(HAL_LOG_ERR, "%s: open device failed errno:%d\n", __func__, -errno);
        return -errno;
    }

    ret = write(fd, buf->codelist, buf->count * sizeof(lirc_t));
    if(ret < 0){
        hal_log(HAL_LOG_ERR, "%s: write failed errno:%d\n", __func__, -errno);
        close(fd);
        return -errno;
    }


    close(fd);
    return 0;
}

int hal_ir_rawcode_rx(hal_board_id id, ir_raw_code *buf, struct timeval *timeout){

    int fd = 0;
    int ret = 0;
    int count = 0;
    fd_set fds;
    char dev_name[80] = {0};

    if(!buf){
        hal_log(HAL_LOG_ERR, "%s: invalid argument\n", __func__);
        return -EINVAL;
    }

    ret = ID_to_IR_devname(id, 1, dev_name);
    if(ret < 0){
        hal_log(HAL_LOG_ERR, "%s: Invaild board ID:%d\n", __func__, id);
        return ret;
    }

    fd = open(dev_name, O_RDONLY);
    if(fd < 0) {
        hal_log(HAL_LOG_ERR, "%s: open device failed errno:%d\n", __func__, -errno);
        return -errno;
    }

    /* clear the real data count in buf */
    buf->count = 0;

    do{
        FD_ZERO(&fds);
        FD_SET(fd, &fds);
        ret = select(fd + 1, &fds, NULL, NULL, timeout);

        if(ret == 0){
            close(fd);
            return -ETIMEDOUT;
        }

        if(ret < 0){
            hal_log(HAL_LOG_ERR, "%s: select failed errno:%d\n", __func__, -errno);
            close(fd);
            return -errno;
        }
        /*LIRC device need a loop to read data, one interrupt one read*/
        ret = read(fd, &(buf->codelist[count]), sizeof(lirc_t));
        if(ret < 0){
            hal_log(HAL_LOG_ERR, "%s: read failed errno:%d\n", __func__, -errno);
            close(fd);
            return -errno;
        }
        if(ret != sizeof(lirc_t))
            break;

        /*LIRC device will return a timeout when no IR signal receive*/
        if(LIRC_IS_TIMEOUT(buf->codelist[count])){
            buf->codelist[count] = LIRC_VALUE(buf->codelist[count]);
            break;
        }

        /*invalid signal*/
        if(LIRC_VALUE(buf->codelist[count]) >= 130 * 1000)
            continue;

        /*store the time value in buf*/
        buf->codelist[count] = LIRC_VALUE(buf->codelist[count]);

        count++;
    }while(count < buf->size);

    close(fd);

    if(count % 2 == 0){
        hal_log(HAL_LOG_ERR, "%s: invalid signal, IR rawcode need paried space/pulse\n", __func__);
        return -EIO;
    }

    buf->count = count;

    return 0;
}

int hal_ir_open(int id, int *lirc_fd)
{
    int fd = -1;
    char dev_name[32] = {0};

    if (id < 0 || lirc_fd == NULL) {
        hal_log(HAL_LOG_ERR, "%s: invalid argument\n", __func__);
        return -EINVAL;
    }

    snprintf(dev_name, sizeof(dev_name)-1, IR_LIRC"%d", id);
    fd = open(dev_name, O_RDWR);
    if (fd < 0) {
        hal_log(HAL_LOG_ERR, "%s: open device failed errno:%d\n", __func__, -errno);
        return -errno;
    }

    *lirc_fd = fd;

    return 0;
}

void hal_ir_close(int lirc_fd)
{
    if (lirc_fd)
        close(lirc_fd);
}

int hal_ir_recv(int lirc_fd, ir_raw_code *buf, struct timeval *timeout)
{
    int ret = 0;
    fd_set fds;

    if (!buf || buf->size <= 0) {
        hal_log(HAL_LOG_ERR, "%s: invalid argument\n", __func__);
        return -EINVAL;
    }

    /* clear the real data count in buf */
    buf->count = 0;
    memset(buf->codelist,0,sizeof(int)*buf->size);

    do {
        FD_ZERO(&fds);
        FD_SET(lirc_fd, &fds);
        ret = select(lirc_fd + 1, &fds, NULL, NULL, timeout);
        if (ret == 0) {
            return -ETIMEDOUT;
        }
        if (ret < 0) {
            hal_log(HAL_LOG_ERR, "%s: select failed errno:%d\n", __func__, -errno);
            return -errno;
        }

        /* LIRC device need a loop to read data, one interrupt one read */
        ret = read(lirc_fd, &(buf->codelist[buf->count]), sizeof(lirc_t));
        if (ret < 0) {
            hal_log(HAL_LOG_ERR, "%s: read failed errno:%d\n", __func__, -errno);
            return -errno;
        }
        if (ret != sizeof(lirc_t))
            break;

        /* LIRC device will return a timeout when no IR signal receive */
        if (LIRC_IS_TIMEOUT(buf->codelist[buf->count])) {
            buf->codelist[buf->count] = LIRC_VALUE(buf->codelist[buf->count]);
            break;
        }

        /* invalid signal */
        if (LIRC_VALUE(buf->codelist[buf->count]) >= 130 * 1000)
            continue;

        /* store the time value in buf */
        buf->codelist[buf->count] = LIRC_VALUE(buf->codelist[buf->count]);

        buf->count++;
    } while(buf->count < buf->size);

    return 0;
}
