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
#include "tkl_i2c.h"

typedef struct {
    int id;                     /* i2c device ID */
    int freq;                   /* frequency, in Hz */
    unsigned short dev_addr;    /* i2c device address */
    unsigned short reg_addr;    /* i2c register address */
    int reg_addr_width;         /* i2c register address width, 8 or 16 bits */
    int is_write;               /* 1: write, 0: read */
    unsigned int data;          /* i2c data to write, only 1 bytes supported */
} tkl_i2c_test_para_t;

static tkl_i2c_test_para_t test_para;

static void print_usage(const char *prog)
{
    printf("Usage: %s [OPTION]\n", prog);
    printf(
"  -i <ID>                  i2c device ID:\n"
"                               0 - I2C0\n"
"                               1 - I2C1\n"
"                               2 - I2C2\n"
"                               3 - I2C3\n"
"  -f <freq>                frequency, in Hz. default 100000\n"
"  -a <dev_addr>            i2c device Address. Hexadecimal with a '0x' or '0X' prefix\n"
"  -r <reg_addr>            i2c Register address. Hexadecimal with a '0x' or '0X' prefix\n"
"  --rw <reg_addr_width>    i2c register address width, 8 or 16 bits.  default 8\n"
"  -w                       If set, perform I2C write; otherwise perform I2C read\n"
"  -d <i2c data>            i2c data to write. Hexadecimal with a '0x' or '0X' prefix. Only 1 bytes supported\n"
    );

    printf("e.g. Read SC3335 sensor ID:\n");
    printf("        %s -i 0 -f 100000 -a 0x30 -r 0x3107 --rw 16\n", prog);
    printf("        %s -i 0 -f 100000 -a 0x30 -r 0x3108 --rw 16\n", prog);
    printf("e.g. Write SC3335 MIPI Clock delay:\n");
    printf("        %s -i 0 -f 100000 -a 0x30 -r 0x3654 --rw 16  -w -d 0x3\n", prog);
    printf("e.g. Read  SC3335 MIPI Clock delay:\n");
    printf("        %s -i 0 -f 100000 -a 0x30 -r 0x3654 --rw 16\n", prog);
}

enum {
    OPT_REG_ADDR_WIDTH = 1,
};

#define HAS_ARG     1

static int parse_args(int argc, char **argv, tkl_i2c_test_para_t *para)
{
    int option_index;
    int c;
    const char short_options[] = "i:f:a:r:wd:h";
    const struct option long_options[] = {
        {"help", 0, 0, 'h'},
        {"rw", HAS_ARG, 0, OPT_REG_ADDR_WIDTH},
        {0, 0, 0, 0}
    };

    /* set the default parameters */
    para->id = 0;
    para->freq = 100000;
    para->dev_addr = 0;
    para->reg_addr = 0;
    para->reg_addr_width = 8;
    para->is_write = 0;
    para->data = 0xAA;

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
        case 'a':
            para->dev_addr = (unsigned short)strtol(optarg, NULL, 16);
            break;
        case 'r':
            para->reg_addr = (unsigned short)strtol(optarg, NULL, 16);
            break;
        case OPT_REG_ADDR_WIDTH:
            para->reg_addr_width = atoi(optarg);
            if ((para->reg_addr_width != 8) && (para->reg_addr_width != 16)) {
                printf("error: invalid reg_addr_width %d\n", para->reg_addr_width);
                return -1;
            }
            break;
        case 'w':
            para->is_write = 1;
            break;
        case 'd':
            para->data = (unsigned int)strtol(optarg, NULL, 16);
            break;
        default:
            return -1;
        }
    }

    return 0;
}

int main(int argc, char **argv)
{
    UINT32_T port;
    TUYA_IIC_BASE_CFG_T cfg; 
    OPERATE_RET ret;
    int len;
    unsigned char i2c_buf[32];

    printf("%s %s %s\n", argv[0], __DATE__, __TIME__);

    if (argc == 1) {
        print_usage(argv[0]);
        return -1;
    }

    if (parse_args(argc, argv, &test_para)) {
        print_usage(argv[0]);
        return -1;
    }

    printf("i2c%d: is_write=%d, freq=%d,  dev_addr=0x%X, reg_addr=0x%X(%d bits)\n",
           test_para.id, test_para.is_write, test_para.freq,
           test_para.dev_addr, test_para.reg_addr, test_para.reg_addr_width);

    port = (UINT32_T)test_para.id;

    if (test_para.freq == 100000) {
        cfg.speed = TUYA_IIC_BUS_SPEED_100K ;
    } else if (test_para.freq == 400000) {
        cfg.speed = TUYA_IIC_BUS_SPEED_400K;
    }else if (test_para.freq == 1000000) {
        cfg.speed = TUYA_IIC_BUS_SPEED_1M;
    }else if (test_para.freq = 340000) {
        cfg.speed = TUYA_IIC_BUS_SPEED_3_4M;
    } else {
        printf("error: unsupported freq %d\n", test_para.freq);
        return -1;
    }

    cfg.role = TUYA_IIC_MODE_MASTER;
    cfg.addr_width = TUYA_IIC_ADDRESS_7BIT;

    ret = tkl_i2c_init(port, &cfg);
    if (ret != OPRT_OK) {
        printf("error: tkl_i2c_init failed\n");
        return -1;
    }

    if (test_para.is_write) {
        if (test_para.reg_addr_width == 16) {
            i2c_buf[0] = (test_para.reg_addr >> 8) & 0xFF;
            i2c_buf[1] = test_para.reg_addr & 0xFF;
            i2c_buf[2] = test_para.data & 0xFF;
	    len = 3;
        } else {
            i2c_buf[0] = test_para.reg_addr & 0xFF;
            i2c_buf[1] = test_para.data & 0xFF;
	    len = 2;
        }

       if(tkl_i2c_master_send(port,test_para.dev_addr,i2c_buf,len,0)) {
            printf("error:tkl_i2c_master_send failed\n");
            return -1;
       }
    } else {
	int data_len = 1;
        if (test_para.reg_addr_width == 16) {
            i2c_buf[0] = (test_para.reg_addr >> 8) & 0xFF;
            i2c_buf[1] = test_para.reg_addr & 0xFF;
	    len = 2;
        } else {
            i2c_buf[0] = test_para.reg_addr & 0xFF;
	    len = 1;
        }

        if(tkl_i2c_master_receive(port,test_para.dev_addr,i2c_buf,data_len,len == 2? 1:0)) {
            printf("error:tkl_i2c_master_recv failed\n");
            return -1;
	}

        printf("i2c read data: 0x%X\n", i2c_buf[0]);
    }

    tkl_i2c_deinit(port);

    return 0;
}
