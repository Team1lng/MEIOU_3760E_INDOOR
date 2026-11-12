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
#include "hal_board.h"
#include "hal_gpio_key.h"

typedef struct {
    char *name;
} hal_gpio_key_test_para_t;

static hal_gpio_key_test_para_t test_para;

static void print_usage(const char *prog)
{
    printf("Usage: %s [OPTION]\n", prog);
    printf(
"  -n <name>\n"
    );
    printf("key list:\n");
    printf("         %s\n", ITEM_KEY_FACTORY_RESET);
    printf("e.g. Test key_factory_reset\n");
    printf("        %s -n key_factory_reset\n", prog);
}

static int parse_args(int argc, char **argv, hal_gpio_key_test_para_t *para)
{
    int option_index;
    int c;
    const char short_options[] = "n:h";
    const struct option long_options[] = {
        {"help", 0, 0, 'h'},
        {0, 0, 0, 0}
    };

    /* set the default parameters */
    para->name = NULL;

    while ((c = getopt_long(argc, argv, short_options, long_options,
                            &option_index)) != -1) {
        switch (c) {
        case 'h':
            return -1;
        case 'n':
            para->name = optarg;
            break;
        default:
            return -1;
        }
    }

    if (NULL == para->name) {
        return -1;
    }

    return 0;
}

int main(int argc, char **argv)
{
    hal_gpio_key_state_e key_state;
    char *name;
    int ret;

    printf("%s %s %s\n", argv[0], __DATE__, __TIME__);

    if (parse_args(argc, argv, &test_para)) {
        print_usage(argv[0]);
        return -1;
    }

    name = test_para.name;
    ret = hal_gpio_key_get_state(name, &key_state);
    if (key_state == HAL_GPIO_KEY_DOWN) {
        printf("%s down\n", name);
    } else {
        printf("%s up\n", name);
    }

    return 0;
}
