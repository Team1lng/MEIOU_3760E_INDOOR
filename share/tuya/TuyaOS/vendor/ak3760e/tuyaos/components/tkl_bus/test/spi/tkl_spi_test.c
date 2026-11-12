/*
 * Copyright (C) 2021 Tuya Inc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include "tkl_spi.h"

typedef struct {
    int id;         /* spi device ID */
    int freq;       /* frequency, in Hz */
    int databits;   /* data bits, 8 or 16 */
    /*
     * 0 - MODE0,  CPOL = 0, CPHA = 0
     * 1 - MODE1,  CPOL = 0, CPHA = 1
     * 2 - MODE2,  CPOL = 1, CPHA = 0
     * 3 - MODE3,  CPOL = 1, CPHA = 1
     */
    int mode;
} tkl_spi_test_para_t;

static tkl_spi_test_para_t test_para;

static void print_usage(const char *prog)
{
    printf("Usage: %s [OPTION]\n", prog);
    printf(
"  -i <ID>          spi device ID:\n"
"                       0 - SPI0\n"
"                       1 - SPI1\n"
"                       2 - SPI2\n"
"                       3 - SPI3\n"
"  -f <freq>        frequency (in Hz), default: 1000000\n"
"  -d <databits>    data bits(8 or 16). default: 8\n"
"  -m <mode>        spi mode (default: 0):\n"
"                       0 - MODE0,  CPOL = 0, CPHA = 0\n"
"                       1 - MODE1,  CPOL = 0, CPHA = 1\n"
"                       2 - MODE2,  CPOL = 1, CPHA = 0\n"
"                       3 - MODE3,  CPOL = 1, CPHA = 1\n"
    );
}

static int parse_args(int argc, char **argv, tkl_spi_test_para_t *para)
{
    int option_index;
    int c;
    const char short_options[] = "i:f:d:m:h";
    const struct option long_options[] = {
        {"help", 0, 0, 'h'},
        {0, 0, 0, 0}
    };
    int mode;
    int databits;

    /* set the default parameters */
    para->id = 0;
    para->freq = 1000000;
    para->databits = TUYA_SPI_DATA_BIT8;
    para->mode = TUYA_SPI_MODE0;

    while ((c = getopt_long(argc, argv, short_options, long_options,
                            &option_index)) != -1) {
        switch (c) {
        case 'h':
            return -1;
        case 'i':
            para->id = atoi(optarg);
            break;
        case 'f':
            para->freq = atoi(optarg);
            break;
        case 'd':
            databits = atoi(optarg);
            if (databits == 8) {
                para->databits = TUYA_SPI_DATA_BIT8;
            } else if (databits == 16) {
                para->databits = TUYA_SPI_DATA_BIT16;
            } else {
                return -1;
            }
            break;
        case 'm':
            mode = atoi(optarg);
            if (mode == 0) {
                para->mode = TUYA_SPI_MODE0;
            } else if (mode == 1) {
                para->mode = TUYA_SPI_MODE1;
            } else if (mode == 2) {
                para->mode = TUYA_SPI_MODE2;
            } else if (mode == 3) {
                para->mode = TUYA_SPI_MODE3;
            } else {
                return -1;
            }
            break;
        default:
            return -1;
        }
    }

    return 0;
}

static int spi_loopback_test(void)
{
    UINT32_T port;
    TUYA_SPI_BASE_CFG_T cfg;
    OPERATE_RET ret = OPRT_OK;
    const int bufsize = 8;
    unsigned char send_buf[bufsize];
    unsigned char receive_buf[bufsize];
    int i;

    printf("spi loopback test...\n");

    port = (UINT32_T)test_para.id;

    cfg.freq_hz = test_para.freq;
    cfg.databits = (TUYA_SPI_DATABITS_E)test_para.databits;
    cfg.mode = (TUYA_SPI_MODE_E)test_para.mode;
    cfg.type = TUYA_SPI_AUTO_TYPE;

    ret = tkl_spi_init(port, &cfg);
    if (ret != OPRT_OK) {
        printf("error: tkl_spi_init failed\n");
        return ret;
    }

    bzero(receive_buf, sizeof(receive_buf));
    for (i = 0; i < bufsize; i++) {
        send_buf[i] = 'A' + i;
    }

    ret = tkl_spi_transfer(port,send_buf,receive_buf,bufsize);

    if (ret != OPRT_OK) {
        printf("error: tkl_spi_xfer failed\n");
    } else {
        if (memcmp(send_buf, receive_buf, bufsize) != 0) {
            printf("send_buf:\n");
            for (i = 0; i < bufsize; i++) {
                printf("0x%02X ", send_buf[i]);
            }
            printf("\n");

            printf("receive_buf:\n");
            for (i = 0; i < bufsize; i++) {
                printf("0x%02X ", receive_buf[i]);
            }
            printf("\n");

            ret = OPRT_COM_ERROR;
        } else {
            ret = OPRT_OK;
        }
    }

    if (ret != OPRT_OK) {
        printf("spi loopback test failed\n");
    } else {
        printf("spi loopback test OK\n");
    }

    tkl_spi_deinit(port);
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

    printf("spi %d: freq=%d, databits=%d, mode=%d\n",
           test_para.id, test_para.freq, test_para.databits, test_para.mode);

    spi_loopback_test();
    return 0;
}
