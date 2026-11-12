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
#include "hal_ircut.h"

typedef struct {
    char *name;
    hal_ircut_cmd_e cmd;
} hal_ircut_test_para_t;

static hal_ircut_test_para_t test_para;

static void print_usage(const char *prog)
{
    printf("Usage: %s [OPTION]\n", prog);
    printf(
"  -n <name>,     default: ircut\n"
"  -c <command>,  command=[on, off, sleep]\n"
    );
    printf("e.g. Turn on ircut\n");
    printf("        %s -n ircut -c on\n", prog);
    printf("        or\n");
    printf("        %s -c on\n", prog);
}

static int parse_args(int argc, char **argv, hal_ircut_test_para_t *para)
{
    int option_index;
    int c;
    const char short_options[] = "n:c:h";
    const struct option long_options[] = {
        {"help", 0, 0, 'h'},
        {0, 0, 0, 0}
    };

    /* set the default parameters */
    para->name = NULL;
    para->cmd = IRCUT_CMD_NONE;

    while ((c = getopt_long(argc, argv, short_options, long_options,
                            &option_index)) != -1) {
        switch (c) {
        case 'h':
            return -1;
        case 'n':
            para->name = optarg;
            break;
        case 'c':
            if (strcasecmp(optarg, "on") == 0) {
                para->cmd = IRCUT_CMD_ON;
            } else if (strcasecmp(optarg, "off") == 0) {
                para->cmd = IRCUT_CMD_OFF;
            } else if (strcasecmp(optarg, "sleep") == 0) {
                para->cmd = IRCUT_CMD_SLEEP;
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

    ret = hal_ircut_switch(test_para.name, test_para.cmd);
    if (ret != 0) {
        printf("hal_ircut_switch failed\n");
    }
    return 0;
}
