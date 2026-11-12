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

typedef struct {
    char *name;
    bool is_on;
} hal_led_test_para_t;

static hal_led_test_para_t test_para;

static void print_usage(const char *prog)
{
    printf("Usage: %s [OPTION]\n", prog);
    printf(
"  -n <name>\n"
"  -v <value>,  value=[on, off]\n"
    );
    printf("e.g. Turn on led_system_state\n");
    printf("        %s -n led_system_state -v on\n", prog);
}

static int parse_args(int argc, char **argv, hal_led_test_para_t *para)
{
    int option_index;
    int c;
    const char short_options[] = "n:v:h";
    const struct option long_options[] = {
        {"help", 0, 0, 'h'},
        {0, 0, 0, 0}
    };

    /* set the default parameters */
    para->name = NULL;
    para->is_on = FALSE;

    while ((c = getopt_long(argc, argv, short_options, long_options,
                            &option_index)) != -1) {
        switch (c) {
        case 'h':
            return -1;
        case 'n':
            para->name = optarg;
            break;
        case 'v':
            if (strcasecmp(optarg, "on") == 0) {
                para->is_on = TRUE;
            } else if (strcasecmp(optarg, "off") == 0) {
                para->is_on = FALSE;
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

int main(int argc, char **argv)
{
    int value;
    int ret;

    printf("%s %s %s\n", argv[0], __DATE__, __TIME__);

    if (parse_args(argc, argv, &test_para)) {
        print_usage(argv[0]);
        return -1;
    }

    printf("%s: %s\n", test_para.name, test_para.is_on ? "on" : "off");

    ret = hal_led_switch(test_para.name, test_para.is_on);
    if (ret != 0) {
        printf("hal_led_switch failed\n");
    }
    return 0;
}
