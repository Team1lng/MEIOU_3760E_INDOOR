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
#include "hal_types.h"
#include "hal_led.h"
#include "hal_board.h"

typedef struct {
    char *name;
    int freq;       /* frequency, in Hz */
    int duty_cycle; /* range [0, 10000], e.g. 1234 means 12.34% */
} hal_led_pwm_test_para_t;

static hal_led_pwm_test_para_t test_para;

static void print_usage(const char *prog)
{
    printf("Usage: %s [OPTION]\n", prog);
    printf(
"  -n <name>        led name\n"
"  -f <frequency>   in Hz\n"
"  -d <duty_cycle>  range [0, 10000], e.g. 1234 means 12.34%%\n"
    );
    printf("e.g. Configure led_white_light with freq=25KHz duty_cycle=50%%\n");
    printf("        %s -n led_white_light -f 25000 -d 5000\n", prog);

    printf("possible led name:\n");
    printf("        %s\n", ITEM_LED_IR);
    printf("        %s\n", ITEM_LED_WHITE_LIGHT);
}

static int parse_args(int argc, char **argv, hal_led_pwm_test_para_t *para)
{
    int option_index;
    int c;
    const char short_options[] = "n:f:d:h";
    const struct option long_options[] = {
        {"help", 0, 0, 'h'},
        {0, 0, 0, 0}
    };

    /* set the default parameters */
    para->name = NULL;
    para->freq = 25000;
    para->duty_cycle = 5000;

    while ((c = getopt_long(argc, argv, short_options, long_options,
                            &option_index)) != -1) {
        switch (c) {
        case 'h':
            return -1;
        case 'n':
            para->name = optarg;
            break;
        case 'f':
            para->freq = atoi(optarg);
            break;
        case 'd':
            para->duty_cycle = atoi(optarg);
            break;
        default:
            return -1;
        }
    }

    return 0;
}

int main(int argc, char **argv)
{
    hal_led_pwm_attr_t attr = {0};
    int value;
    int ret;

    printf("%s %s %s\n", argv[0], __DATE__, __TIME__);

    if (parse_args(argc, argv, &test_para)) {
        print_usage(argv[0]);
        return -1;
    }

    printf("%s: freq=%d, duty_cycle=%d\n",
        test_para.name, test_para.freq, test_para.duty_cycle);

    attr.freq = test_para.freq;
    attr.duty_cycle = test_para.duty_cycle;
    ret = hal_led_pwm_control(test_para.name, &attr);
    if (ret != 0) {
        printf("hal_led_pwm_control failed\n");
    }

    return 0;
}
