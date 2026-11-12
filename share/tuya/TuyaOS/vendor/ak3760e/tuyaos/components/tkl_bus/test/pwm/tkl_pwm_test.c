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
#include "tkl_pwm.h"

typedef struct {
    int id;             /* pwm ID */
    int freq;           /* frequency, in Hz */
    int duty_cycle;     /* range [0, 100*10000], e.g. 123456 means 12.3456% */
    int polarity;       /* normal or inversed */
} tkl_pwm_test_para_t;

static tkl_pwm_test_para_t test_para;

static void print_usage(const char *prog)
{
    printf("Usage: %s [OPTION]\n", prog);
    printf(
"  -i <ID>              default 0\n"
"  --freq <frequency>   in Hz\n"
"  --dc   <duty_cycle>  range [0, 100*10000], e.g. 123456 means 12.3456%%\n"
"  --pol  <polarity>    0: normal (default),  1: inversed\n");

    printf("e.g. Set pwm0-channel0: output 20KHz frequency, 50%% duty cycle\n");
    printf("        %s -i 0 --freq 20000 --dc 500000\n", prog);
}

enum {
    OPT_DISABLE = 1,
    OPT_FREQ,
    OPT_DUTY_CYCLE,
    OPT_POLARITY,
};

#define HAS_ARG     1

static int parse_args(int argc, char **argv, tkl_pwm_test_para_t *para)
{
    int option_index;
    int c;
    const char short_options[] = "i:h";
    const struct option long_options[] = {
        {"help", 0, 0, 'h'},
        {"freq", HAS_ARG, 0, OPT_FREQ},
        {"dc", HAS_ARG, 0, OPT_DUTY_CYCLE},
        {"pol", HAS_ARG, 0, OPT_POLARITY},
        {0, 0, 0, 0}
    };

    /* set the default parameters */
    para->id = 0;
    para->freq = 20000;
    para->duty_cycle = 500000;
    para->polarity = TUYA_PWM_POSITIVE;

    while ((c = getopt_long(argc, argv, short_options, long_options,
                            &option_index)) != -1) {
        switch (c) {
        case 'h':
            return -1;
        case 'i':
            para->id = atoi(optarg);
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
    UINT32_T port;
    TUYA_PWM_BASE_CFG_T cfg;
    OPERATE_RET ret;

    printf("%s %s %s\n", argv[0], __DATE__, __TIME__);

    if (argc == 1) {
        print_usage(argv[0]);
        return -1;
    }

    if (parse_args(argc, argv, &test_para)) {
        print_usage(argv[0]);
        return -1;
    }

    printf("id=%d, freq=%d, duty_cycle=%d\n",
           test_para.id,
           test_para.freq,
           test_para.duty_cycle);
    printf("polarity: %s\n",
           (test_para.polarity == TUYA_PWM_POSITIVE) ? "normal" : "inversed");

    port = (UINT32_T)test_para.id;
    cfg.frequency = test_para.freq;
    cfg.duty = test_para.duty_cycle;
    cfg.polarity = (TUYA_PWM_POLARITY_E)test_para.polarity;

    ret = tkl_pwm_init(port, &cfg);
    if (ret != OPRT_OK) {
        printf("error: tkl_pwm_init failed\n");
        return -1;
    }

    // disable pwm channel
    tkl_pwm_stop(port);

    // configure pwm channel
    ret = tkl_pwm_info_set(port, &cfg);
    if (ret != OPRT_OK) {
        printf("error: tkl_pwm_info_set failed\n");
        return -1;
    }

    // enable pwm channel
    ret = tkl_pwm_start(port);
    if (ret != OPRT_OK) {
        printf("error: tkl_pwm_start failed\n");
        return -1;
    }

    return 0;
}
