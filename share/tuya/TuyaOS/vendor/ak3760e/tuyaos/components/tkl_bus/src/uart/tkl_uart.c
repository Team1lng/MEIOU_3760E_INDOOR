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
#include <termios.h>
#include "sys_log.h"
#include "tkl_uart.h"

#ifndef TAG
#define TAG "tkl_uart"
#endif

#ifndef MAX_PATH 
#define MAX_PATH 256
#endif

#define UART_DEV_MAX            32
#define UART_TYPE_VALID(type)   (((type) >= 0) && ((type) < TUYA_UART_MAX_TYPE))
#define UART_DEV_VALID(num)     (((num) >= 0) && ((num) < UART_DEV_MAX))

#ifndef TKL_UART_PATTTERN_SYS
#define UART_NAME_PATTERN_SYS "/dev/ttyS%d"
#else
#define UART_NAME_PATTERN_SYS TKL_SYS_UART_PATTTERN
#endif
/* USB uart name */
#define UART_NAME_PATTERN_USB   "/dev/ttyUSB%d"

// uart fd-list
static int sg_uart_list[TUYA_UART_MAX_TYPE][UART_DEV_MAX];

static int __uart_dev_is_valid(int type, int num)
{
    if (!UART_TYPE_VALID(type)) {
        LOGE(TAG,"invalid uart type %d\n", type);
        return 0;
    }
    if (!UART_DEV_VALID(num)) {
        LOGE(TAG, "invalid uart number %d\n", num);
        return 0;
    }
    return 1;
}

static int __save_fd(UINT_T port_id, int fd)
{
    int type = TUYA_UART_GET_PORT_TYPE(port_id);
    int num = TUYA_UART_GET_PORT_NUMBER(port_id);

    if (!__uart_dev_is_valid(type, num)) {
        return -1;
    }

    sg_uart_list[type][num] = fd;

    return 0;
}

static int __get_fd(UINT_T port_id)
{
    int type = TUYA_UART_GET_PORT_TYPE(port_id);
    int num = TUYA_UART_GET_PORT_NUMBER(port_id);

    if (!__uart_dev_is_valid(type, num)) {
        return -1;
    }

    return sg_uart_list[type][num];
}

static void __disable_flowcontrol(int fd)
{
    struct termios tios;

    memset(&tios, 0, sizeof(tios));

    tcgetattr(fd, &tios);
    tios.c_cflag &= ~CRTSCTS;
    tcsetattr(fd, TCSANOW, &tios);
}

OPERATE_RET tkl_uart_init(TUYA_UART_NUM_E port_id, TUYA_UART_BASE_CFG_T *cfg)
{
    int fd;
    speed_t speed;
    char path[MAX_PATH] = {0};
    struct termios tty_attr;
    int type = TUYA_UART_GET_PORT_TYPE((UINT_T)port_id);
    int num = TUYA_UART_GET_PORT_NUMBER((UINT_T)port_id);
    TUYA_UART_FLOWCTRL_TYPE_E flowcntl;

    if (!__uart_dev_is_valid(type, num)) {
        return OPRT_INVALID_PARM;
    }

    if (cfg == NULL) {
        return OPRT_INVALID_PARM;
    }

    // get uart name pattern
    if (type == TUYA_UART_SYS) {
        snprintf(path,MAX_PATH,UART_NAME_PATTERN_SYS,num);
    } else if (type == TUYA_UART_USB) {
        snprintf(path,MAX_PATH,UART_NAME_PATTERN_USB,num);
    } else {
        LOGE(TAG,"Invalid uart type %d\n",type);
        return OPRT_INVALID_PARM;
    }

    LOGD(TAG,"uart name:%s\n",path);

    fd = open(path, O_RDWR|O_NOCTTY|O_NONBLOCK);
    if (fd < 0) {
        LOGE(TAG,"Failed to open %s\n", path);
        return OPRT_NOT_FOUND;
    }

    // configure uart

    memset(&tty_attr, 0, sizeof(struct termios));
    tcgetattr(fd, &tty_attr);

    /*
     * Enable Raw mode: all special processing of input and output
     * characters is disabled.
     */
    tty_attr.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP
                          | INLCR | IGNCR | ICRNL | IXON);
    tty_attr.c_oflag &= ~OPOST;
    tty_attr.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);

    // set baudrate
    switch (cfg->baudrate)
    {
    case 1800:
        speed = B1800; break;
    case 2500:
        speed = B2400; break;
    case 4800:
        speed = B4800; break;
    case 9600:
        speed = B9600; break;
    case 38400:
        speed = B38400; break;
    case 57600:
        speed = B57600; break;
    case 115200:
        speed = B115200; break;
    case 230400:
        speed = B230400; break;
    case 460800:
        speed = B460800; break;
    case 921600:
        speed = B921600; break;
    default:
        speed = B9600; break;
    }
    cfsetospeed(&tty_attr, speed);
    cfsetispeed(&tty_attr, speed);

    // set databits
    switch (cfg->databits) {
    case TUYA_UART_DATA_LEN_5BIT:
        tty_attr.c_cflag = (tty_attr.c_cflag & ~CSIZE) | CS5;
        break;
    case TUYA_UART_DATA_LEN_6BIT:
        tty_attr.c_cflag = (tty_attr.c_cflag & ~CSIZE) | CS6;
        break;
    case TUYA_UART_DATA_LEN_7BIT:
        tty_attr.c_cflag = (tty_attr.c_cflag & ~CSIZE) | CS7;
        break;
    default: // TUYA_UART_DATA_BIT8
        tty_attr.c_cflag = (tty_attr.c_cflag & ~CSIZE) | CS8;
        break;
    }

    // set parity
    switch (cfg->parity) {
    case TUYA_UART_PARITY_TYPE_EVEN:
        tty_attr.c_cflag &= ~(PARODD /*| CMSPAR*/);
        tty_attr.c_cflag |= PARENB;
        break;
    case TUYA_UART_PARITY_TYPE_ODD:
       // tty_attr.c_cflag &= ~CMSPAR;
        tty_attr.c_cflag |= PARENB | PARODD;
        break;
    default: // TUYA_UART_PARITY_NONE
        tty_attr.c_cflag &= ~(PARENB | PARODD /*| CMSPAR*/);
        break;
    }

    // set stopbits
    switch (cfg->stopbits) {
    case TUYA_UART_STOP_LEN_2BIT:
        tty_attr.c_cflag |= CSTOPB;
        break;
    default: // TUYA_UART_STOP_BIT1
        tty_attr.c_cflag &= ~CSTOPB;
        break;
    }

    // set flowcntrl
    flowcntl = cfg->flowctrl;
    switch (flowcntl) {
    case TUYA_UART_FLOWCTRL_RTSCTS:
        tty_attr.c_cflag |= CRTSCTS;
        tty_attr.c_iflag &= ~(IXON | IXOFF | IXANY);
        break;
    case TUYA_UART_FLOWCTRL_XONXOFF:
        tty_attr.c_cflag &= ~(CRTSCTS);
        tty_attr.c_iflag |= IXON | IXOFF;
        break;
    default: // TUYA_UART_FLOWCTRL_NONE
        tty_attr.c_cflag &= ~(CRTSCTS);
        tty_attr.c_iflag &= ~(IXON | IXOFF | IXANY);
        break;
    }

    // set local: local or modem
    tty_attr.c_cflag |= CLOCAL;
    // tty_attr.c_cflag &= ~CLOCAL;

    // set hupcl: hup-on-close
    tty_attr.c_cflag |= HUPCL;
    // tty_attr.c_cflag &= ~HUPCL;

    tty_attr.c_cflag |= CREAD;  // enable reception of characters
    tty_attr.c_cc[VMIN] = 1;

    if (tcsetattr(fd, TCSANOW, &tty_attr) < 0) {
        LOGE(TAG, "Failed to config uart: %s\n", path);
        close(fd);
        return OPRT_COM_ERROR;
    }

    // save fd into list
    __save_fd(port_id, fd);
    return OPRT_OK;
}

OPERATE_RET tkl_uart_deinit(TUYA_UART_NUM_E port_id)
{
    int fd;

    fd = __get_fd((UINT_T)port_id);

    if (fd == -1 || fd == 0) {
        return OPRT_OK;
    }

    __disable_flowcontrol(fd);
    __save_fd(port_id, -1);
    close(fd);

    return OPRT_OK;
}

INT_T tkl_uart_write(TUYA_UART_NUM_E port_id, VOID_T *buff, UINT16_T len)
{
    int fd;
    int ret;

    fd = __get_fd((UINT_T)port_id);

    if (fd == -1 || fd == 0) {
        return OPRT_COM_ERROR;
    }

    ret = write(fd, buff, len);

    if (ret < 0) {
        LOGE(TAG,"uart send failed, port=0x%X\n", port_id);
        return OPRT_SEND_ERR;
    }

    fsync(fd);

    return ret;
}

INT_T tkl_uart_read(TUYA_UART_NUM_E port_id, VOID_T *buff, UINT16_T len)
{
    int fd;
    int ret = 0;

    fd = __get_fd((UINT_T)port_id);

    if (fd == -1 || fd == 0) {
        return OPRT_COM_ERROR;
    }

    ret = read(fd, buff, len);

    if (ret < 0) {
        return OPRT_RECV_ERR;
    }

    return ret;
}

OPERATE_RET tkl_uart_wait_for_data(TUYA_UART_NUM_E port_id, INT_T timeout_ms)
{
    int fd;
    int ret;
    fd_set read_set;

    struct timeval *p_timeval = NULL;

    struct timeval timeval_uart = {
        .tv_sec = timeout_ms / 1000,
        .tv_usec = (timeout_ms % 1000) * 1000
    };


    if (timeout_ms != -1) {    // -1 : block indefinitely
        p_timeval = &timeval_uart;
    }

    fd = __get_fd((UINT_T)port_id);

    if (fd == -1 || fd == 0) {
        return OPRT_INVALID_PARM;
    }

    FD_ZERO(&read_set);
    FD_SET(fd, &read_set);

    ret = select(fd + 1, &read_set, NULL, NULL, p_timeval);
    if (ret < 0) {
      //  LOGE(TAG, "uart recv timeout\n");
        return OPRT_COM_ERROR;
    } else if (ret == 0) {
        return OPRT_TIMEOUT;
    }

    if (!FD_ISSET(fd, &read_set)) {
        return OPRT_NOT_FOUND;
    }

    return OPRT_OK;
}

VOID_T tkl_uart_rx_irq_cb_reg(TUYA_UART_NUM_E port_id, TUYA_UART_IRQ_CB rx_cb)
{
    // do nothing
    return;
}

VOID_T tkl_uart_tx_irq_cb_reg(TUYA_UART_NUM_E port_id, TUYA_UART_IRQ_CB tx_cb)
{
    // do nothing
    return;
}

OPERATE_RET tkl_uart_set_tx_int(TUYA_UART_NUM_E port_id, BOOL_T enable)
{
    return OPRT_NOT_SUPPORTED;
}

OPERATE_RET tkl_uart_set_rx_flowctrl(TUYA_UART_NUM_E port_id, BOOL_T enable)
{
    // flowctrl should be set in tkl_uart_init
    return OPRT_NOT_SUPPORTED;
}


OPERATE_RET tkl_uart_ioctl(TUYA_UART_NUM_E port_id, UINT32_T cmd, VOID *arg)
{
    return OPRT_NOT_SUPPORTED;
}

INT_T tkl_uart_write_common(UINT_T port_id, VOID_T *buff, UINT16_T len)
{
    int fd;
    int ret;

    fd = __get_fd(port_id);
    if (fd == -1 || fd == 0) {
        return OPRT_COM_ERROR;
    }

    ret = write(fd, buff, len);
    if (ret < 0) {
        LOGE(TAG, "uart send failed, port=0x%X\n", port_id);
        return OPRT_SEND_ERR;
    }
    fsync(fd);

    return ret;
}
