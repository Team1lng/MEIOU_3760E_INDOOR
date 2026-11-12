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
#include "hal_adc.h"

typedef struct {
    int dev;    /* device number */
    int chn;    /* channel number */
    int do_disable;
} hal_adc_test_para_t;

static hal_adc_test_para_t test_para;

static void print_usage(const char *prog)
{
    printf("Usage: %s [OPTION]\n", prog);
    printf(
"  -d <device>          default 0\n"
"  -c <channel>         default 0\n"
"  --disable            disable channel\n");

    printf("e.g. Read adc0-channel0:\n");
    printf("        %s -d 0 -c 0\n", prog);
    printf("e.g. Read adc0-channel0 and then disable the adc device\n");
    printf("        %s -d 0 -c 0 --disable\n", prog);
}

enum {
    OPT_DISABLE = 1,
};

#define HAS_ARG     1

static int parse_args(int argc, char **argv, hal_adc_test_para_t *para)
{
    int option_index;
    int c;
    const char short_options[] = "d:c:h";
    const struct option long_options[] = {
        {"help", 0, 0, 'h'},
        {"disable", 0, 0, OPT_DISABLE},
        {0, 0, 0, 0}
    };

    /* set the default parameters */
    para->dev = 0;
    para->chn = 0;
    para->do_disable = 0;

    while ((c = getopt_long(argc, argv, short_options, long_options,
                            &option_index)) != -1) {
        switch (c) {
        case 'h':
            return -1;
        case 'd':
            para->dev = atoi(optarg);
            break;
        case 'c':
            para->chn = atoi(optarg);
            break;
        case OPT_DISABLE:
            para->do_disable = 1;
            break;
        default:
            return -1;
        }
    }

    return 0;
}

int main(int argc, char **argv)
{
    int value = -1;
    int ret;

    printf("%s %s %s\n", argv[0], __DATE__, __TIME__);

    if (parse_args(argc, argv, &test_para)) {
        print_usage(argv[0]);
        return -1;
    }

    hal_adc_init(test_para.dev);

    ret = hal_adc_get_value(test_para.dev, test_para.chn, &value);
    if (ret != 0) {
        printf("hal_adc_get_value failed\n");
        return -1;
    }
    printf("[adc %d chan %d] value=%d\n", test_para.dev, test_para.chn, value);

    if (test_para.do_disable) {
        ret = hal_adc_deinit(test_para.dev);
        if (ret != 0) {
            printf("hal_adc_deinit failed\n");
            return -1;
        }
    }

    return 0;
}
