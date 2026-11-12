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
#include "hal_pwm.h"

typedef struct {
    bool enable;
    int dev;            /* device number */
    int chn;            /* channel number */
    int freq;           /* frequency, in Hz */
    int duty_cycle;     /* range [0, 10000], e.g. 1234 means 12.34% */
    int polarity;       /* normal or inversed */
} hal_pwm_test_para_t;

static hal_pwm_test_para_t test_para;

static void print_usage(const char *prog)
{
    printf("Usage: %s [OPTION]\n", prog);
    printf(
"  -d <device>          default 0\n"
"  -c <channel>         default 0\n"
"  --disable            disable channel\n"
"  --freq <frequency>   in Hz\n"
"  --dc   <duty_cycle>  range [0, 10000], e.g. 1234 means 12.34%%\n"
"  --pol  <polarity>    0: normal (default),  1: inversed\n");

    printf("e.g. Set pwm0-channel0: output 2KHz frequency, 50%% duty cycle\n");
    printf("        %s -d 0 -c 0 --freq 2000 --dc 5000\n", prog);
    printf("e.g. Disable pwm0-channel0\n");
    printf("        %s -d 0 -c 0 --disable\n", prog);
}

enum {
    OPT_DISABLE = 1,
    OPT_FREQ,
    OPT_DUTY_CYCLE,
    OPT_POLARITY,
};

#define HAS_ARG     1

static int parse_args(int argc, char **argv, hal_pwm_test_para_t *para)
{
    int option_index;
    int c;
    const char short_options[] = "d:c:h";
    const struct option long_options[] = {
        {"help", 0, 0, 'h'},
        {"disable", 0, 0, OPT_DISABLE},
        {"freq", HAS_ARG, 0, OPT_FREQ},
        {"dc", HAS_ARG, 0, OPT_DUTY_CYCLE},
        {"pol", HAS_ARG, 0, OPT_POLARITY},
        {0, 0, 0, 0}
    };

    /* set the default parameters */
    para->enable = TRUE;
    para->dev = 0;
    para->chn = 0;
    para->freq = 1000;
    para->duty_cycle = 2500;
    para->polarity = HAL_PWM_POL_NORMAL;

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
            para->enable = FALSE;
            break;
        case OPT_FREQ:
            para->freq = atoi(optarg);
            break;
        case OPT_DUTY_CYCLE:
            para->duty_cycle = atoi(optarg);
            break;
        case OPT_POLARITY:
            para->polarity = atoi(optarg);
            break;
        default:
            return -1;
        }
    }

    return 0;
}

int main(int argc, char **argv)
{
    hal_pwm_chn_attr_t attr;
    int value;
    int ret;

    printf("%s %s %s\n", argv[0], __DATE__, __TIME__);

    if (parse_args(argc, argv, &test_para)) {
        print_usage(argv[0]);
        return -1;
    }

    printf("dev=%d, chn=%d, freq=%d, duty_cycle=%d\n",
           test_para.dev,
           test_para.chn,
           test_para.freq,
           test_para.duty_cycle);
    printf("polarity: %s\n",
           (test_para.polarity == HAL_PWM_POL_NORMAL) ? "normal" : "inversed");

    // disable pwm channel
    hal_pwm_disable(test_para.dev, test_para.chn);

    // configure and enable pwm channel
    attr.freq = test_para.freq;
    attr.duty_cycle = test_para.duty_cycle;
    attr.polarity = test_para.polarity;
    ret = hal_pwm_config(test_para.dev, test_para.chn, &attr);
    if (ret != 0) {
        printf("hal_pwm_config failed\n");
        return -1;
    }

    // enable pwm channel
    if (test_para.enable) {
        ret = hal_pwm_enable(test_para.dev, test_para.chn);
        if (ret != 0) {
            printf("hal_pwm_enable failed\n");
            return -1;
        }
    }

    return 0;
}
