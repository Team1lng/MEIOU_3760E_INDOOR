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
#include "hal_gpio.h"

typedef struct {
    int num;
    int direction;
    int value;
    bool irq_en;
    gpio_irq_type_e irq_type;
} hal_gpio_test_para_t;

static hal_gpio_test_para_t test_para;

static void print_usage(const char *prog)
{
    printf("Usage: %s [OPTION]\n", prog);
    printf(
"  -n <number>\n"
"  -d <direction>,  direction=[in, out]           out: output, in: input\n"
"  -v <value>,      value=[high, low]             Only required when the direction is OUT\n"
"  --irq <type>,    type=[rising, falling, both]  enable gpio interrupt and set irq type\n"
    );
    printf("e.g. Set gpio16 to OUTPUT with HIGH level\n");
    printf("        %s -n 16 -d out -v high\n", prog);
    printf("e.g. Test gpio16 falling interrupt\n");
    printf("        %s -n 16 --irq falling\n", prog);
}

enum {
    OPT_IRQ = 1,
};

#define HAS_ARG     1

static int parse_args(int argc, char **argv, hal_gpio_test_para_t *para)
{
    int option_index;
    int c;
    const char short_options[] = "n:d:v:h";
    const struct option long_options[] = {
        {"help", 0, 0, 'h'},
        {"irq", HAS_ARG, 0, OPT_IRQ},
        {0, 0, 0, 0}
    };

    /* set the default parameters */
    para->num = -1;
    para->direction = -1;
    para->value = -1;
    para->irq_en = 0;
    para->irq_type = GPIO_IRQ_NONE;

    while ((c = getopt_long(argc, argv, short_options, long_options,
                            &option_index)) != -1) {
        switch (c) {
        case 'h':
            return -1;
        case 'n':   /* num */
            para->num = atoi(optarg);
            break;
        case 'd':   /* direction */
            if (strcasecmp(optarg, "in") == 0) {
                para->direction = GPIO_IN;
            } else if (strcasecmp(optarg, "out") == 0) {
                para->direction = GPIO_OUT;
            } else {
                printf("error: Invalid direction %s\n", optarg);
                return -1;
            }
            break;
        case 'v':   /* value */
            if (strcasecmp(optarg, "high") == 0) {
                para->value = GPIO_HIGH;
            } else if (strcasecmp(optarg, "low") == 0) {
                para->value = GPIO_LOW;
            } else {
                printf("error: Invalid value %s\n", optarg);
                return -1;
            }
            break;
        case OPT_IRQ:   /* interrupt */
            para->direction = GPIO_IN;
            para->irq_en = TRUE;
            if (strcasecmp(optarg, "rising") == 0) {
                para->irq_type = GPIO_IRQ_EDGE_RISING;
            } else if (strcasecmp(optarg, "falling") == 0) {
                para->irq_type = GPIO_IRQ_EDGE_FALLING;
            } else {
                para->irq_type = GPIO_IRQ_EDGE_BOTH;
            }
            break;
        default:
            return -1;
        }
    }

    if ((para->direction == GPIO_OUT) && (para->value == -1)) {
        return -1;
    }

    return 0;
}

int main(int argc, char **argv)
{
    hal_gpio_attr_t attr;
    int value;
    int ret;

    printf("%s %s %s\n", argv[0], __DATE__, __TIME__);

    if (parse_args(argc, argv, &test_para)) {
        print_usage(argv[0]);
        return -1;
    }

    if ((test_para.num == -1) || (test_para.direction == -1)) {
        print_usage(argv[0]);
        return -1;
    }

    printf("para: num=%d, direction=%d, value=%d\n",
           test_para.num, test_para.direction, test_para.value);

    attr.direction = test_para.direction;
    attr.value = test_para.value;
    attr.irq_en = test_para.irq_en;
    attr.irq_type = test_para.irq_type;
    ret = hal_gpio_config(test_para.num, &attr);
    if (ret != 0) {
        printf("hal_gpio_config failed\n");
        return -1;
    }

    /* interrupt test */
    if (test_para.irq_en) {
        int timeout = 5000; // ms
        int cnt = 10;
        int i;

        printf("gpio interrupt test, cnt: %d, timeout: %d ms\n",
               cnt, timeout);

        for (i = 0; i < cnt; i++) {
            printf("waiting for gpio irq...\n");
            ret = hal_gpio_wait_for_irq(test_para.num, timeout);
            if ((ret != 0) && (ret != -ETIMEDOUT)) {
                printf("hal_gpio_wait_for_irq failed\n");
                return -1;
            }

            ret = hal_gpio_get_value(test_para.num, &value);
            if (ret != 0) {
                printf("hal_gpio_get_value failed\n");
                return -1;
            }
            printf("gpio level: %d\n", value);
        }

    } else if (test_para.direction == GPIO_IN) {
        ret = hal_gpio_get_value(test_para.num, &value);
        if (ret != 0) {
            printf("hal_gpio_get_value failed\n");
            return -1;
        }

        printf("gpio level: %d\n", value);
    } else {
        ret = hal_gpio_set_value(test_para.num, test_para.value);
        if (ret != 0) {
            printf("hal_gpio_set_value failed\n");
            return -1;
        }

        printf("gpio write: %d\n", test_para.value);
    }

    return 0;
}
