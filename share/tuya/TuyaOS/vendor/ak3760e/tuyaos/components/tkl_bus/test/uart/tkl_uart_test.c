/*
 * Copyright (C) 2021 Ty Inc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include "tkl_uart.h"

typedef struct {
    UINT_T          port_id;    /* uart device ID */
    TUYA_UART_BASE_CFG_T  cfg;
} tkl_uart_test_para_t;

static tkl_uart_test_para_t test_para;

static void print_usage(const char *prog)
{
    printf("Usage: %s [OPTION]\n", prog);
    printf(
"  -n <num>             uart device number: 0 - 31\n"
"  [ -t <type> ]        uart type:\n"
"                           0 - system uart. e.g. /dev/ttyS0 (default)\n"
"                           1 - usb uart. e.g. /dev/ttyUSB0\n"
"                           2 - sdio uart\n"
"  [ -b <baudrate> ]    baud rate. default 115200\n"
"  [ -d <databits> ]    data bits. 5,6,7,8 bits, default 8\n"
"  [ -p <parity> ]      parity:\n"
"                           0 - none (default)\n"
"                           1 - odd\n"
"                           2 - even\n"
"  [ -s <stopbits> ]    stop bits:\n"
"                           0 - 1 bit (default)\n"
"                           1 - 1.5 bit\n"
"                           2 - 2 bits\n"
"  [ -f <flowctrl> ]    flow ctrl:\n"
"                           0 - none (default)\n"
"                           1 - RTS/CTS\n"
"                           2 - XON/XOFF\n"
"                           3 - other\n"
    );
}

static int parse_args(int argc, char **argv, tkl_uart_test_para_t *para)
{
    int option_index;
    int c;
    const char short_options[] = "n:t:b:d:p:s:f:h";
    const struct option long_options[] = {
        {"help", 0, 0, 'h'},
        {0, 0, 0, 0}
    };
    TUYA_UART_TYPE_E type = TUYA_UART_SYS;
    int num = -1;
    int baudrate = 115200;
    int databits;
    int parity;
    int stopbits;
    int flowctrl;
    int tmp;

    /* set the default parameters */
    para->cfg.baudrate = 115200;
    para->cfg.databits = TUYA_UART_DATA_LEN_8BIT;
    para->cfg.parity = TUYA_UART_PARITY_TYPE_NONE;
    para->cfg.stopbits = TUYA_UART_STOP_LEN_1BIT;
    para->cfg.flowctrl = TUYA_UART_FLOWCTRL_NONE;

    while ((c = getopt_long(argc, argv, short_options, long_options,
                            &option_index)) != -1) {
        switch (c) {
        case 'h':
            return -1;
        case 'n':
            num = atoi(optarg);
            break;
        case 't':
            type = atoi(optarg);
            if (type == 0) {
                type = TUYA_UART_SYS;
            } else if (type == 1) {
                type = TUYA_UART_USB;
            } else if (type == 2) {
                type = TUYA_UART_SDIO;
            } else {
                printf("error: invalid uart type %d\n", type);
                return -1;
            }
            break;
        case 'b':
            para->cfg.baudrate = (UINT_T)atoi(optarg);
            break;
        case 'd':
            databits = atoi(optarg);
            if (databits == 5) {
                para->cfg.databits = TUYA_UART_DATA_LEN_5BIT;
            } else if (databits == 6) {
                para->cfg.databits = TUYA_UART_DATA_LEN_6BIT;
            } else if (databits == 7) {
                para->cfg.databits = TUYA_UART_DATA_LEN_7BIT;
            } else if (databits == 8) {
                para->cfg.databits = TUYA_UART_DATA_LEN_8BIT;
            } else {
                return -1;
            }
            break;
        case 'p':
            parity = atoi(optarg);
            if (parity == 0) {
                para->cfg.parity = TUYA_UART_PARITY_TYPE_NONE;
            } else if (parity == 1) {
                para->cfg.parity = TUYA_UART_PARITY_TYPE_ODD;
            } else if (parity == 2) {
                para->cfg.parity = TUYA_UART_PARITY_TYPE_EVEN;
            } else {
                return -1;
            }
            break;
        case 's':
            stopbits = atoi(optarg);
            if (stopbits == 0) {
                para->cfg.stopbits = TUYA_UART_STOP_LEN_1BIT;
            } else if (stopbits == 1) {
                para->cfg.stopbits = TUYA_UART_STOP_LEN_1_5BIT1;
            } else if (stopbits == 2) {
                para->cfg.stopbits = TUYA_UART_STOP_LEN_2BIT;
            } else {
                return -1;
            }
            break;
        case 'f':
            flowctrl = atoi(optarg);
            // TODO: Add support for flowctrl settings
            para->cfg.flowctrl = TUYA_UART_FLOWCTRL_NONE;
            break;
        default:
            return -1;
        }
    }

    // The uart ID must be specified.
    if (num == -1) {
        return -1;
    }

    para->port_id = TUYA_UART_PORT_ID(type, num);
    return 0;
}

static int uart_loopback_test(void)
{
    UINT_T port_id;
    TUYA_UART_BASE_CFG_T cfg;
    OPERATE_RET ret = OPRT_OK;
    const int bufsize = 8;
    unsigned char tx[bufsize], rx[bufsize];
    INT_T bytes;
    int i;

    printf("uart loopback test...\n");

    port_id = test_para.port_id;

    cfg.baudrate = test_para.cfg.baudrate;
    cfg.databits = test_para.cfg.databits;
    cfg.parity = test_para.cfg.parity;
    cfg.stopbits = test_para.cfg.stopbits;
    cfg.flowctrl = test_para.cfg.flowctrl;

    ret = tkl_uart_init(port_id, &cfg);
    if (ret != OPRT_OK) {
        printf("error: tkl_uart_init failed\n");
        return ret;
    }

    bzero(rx, sizeof(rx));
    for (i = 0; i < bufsize; i++) {
        tx[i] = 'A' + i;
    }

    bytes = tkl_uart_write(port_id, tx, sizeof(tx));
    if (bytes <= 0) {
        printf("error: tkl_uart_write failed\n");
        ret = OPRT_COM_ERROR;
    } else {
        // wait at most 5 seconds until the data is ready
        ret = tkl_uart_wait_for_data(port_id, 5000);
        if (ret == OPRT_TIMEOUT) {
            printf("error: timeout\n");
        }

        bytes = tkl_uart_read(port_id, rx, sizeof(rx));
        if (bytes < 0) {
            printf("error: tkl_uart_read failed\n");
            ret = OPRT_COM_ERROR;
        } else {
            if (memcmp(tx, rx, bufsize) != 0) {
                printf("tx:\n");
                for (i = 0; i < bufsize; i++) {
                    printf("0x%02X ", tx[i]);
                }
                printf("\n");

                printf("rx:\n");
                for (i = 0; i < bufsize; i++) {
                    printf("0x%02X ", rx[i]);
                }
                printf("\n");

                ret = OPRT_COM_ERROR;
            } else {
                ret = OPRT_OK;
            }
        }
    }

    if (ret != OPRT_OK) {
        printf("uart loopback test failed\n");
    } else {
        printf("uart loopback test OK\n");
    }

    tkl_uart_deinit(port_id);
    return ret;
}

int main(int argc, char **argv)
{
    printf("%s %s %s\n", argv[0], __DATE__, __TIME__);

    if (argc == 1) {
        print_usage(argv[0]);
        return -1;
    }

    if (parse_args(argc, argv, &test_para)) {
        print_usage(argv[0]);
        return -1;
    }

    printf("uart %d: baudrate=%d, databits=%d, parity=%d, stopbits=%d, flowctrl=%d\n",
           TUYA_UART_GET_PORT_NUMBER(test_para.port_id), test_para.cfg.baudrate,
           test_para.cfg.databits, test_para.cfg.parity,
           test_para.cfg.stopbits, test_para.cfg.flowctrl);

    uart_loopback_test();
    return 0;
}
