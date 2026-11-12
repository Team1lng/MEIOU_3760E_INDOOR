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
#include "hal_pir.h"

typedef struct {
    hal_pir_sens_e sens;
} hal_pir_test_para_t;

static hal_pir_test_para_t test_para;

static void print_usage(const char *prog)
{
    printf("Usage: %s [OPTION]\n", prog);
    printf(
"  -s <sens>    sens=[default, low, middle, high]\n"
    );
    printf("e.g. Test PIR (sensitivity: default)\n");
    printf("        %s -s default\n", prog);
    printf("        or\n");
    printf("        %s\n", prog);
}

static int parse_args(int argc, char **argv, hal_pir_test_para_t *para)
{
    int option_index;
    int c;
    const char short_options[] = "s:h";
    const struct option long_options[] = {
        {"help", 0, 0, 'h'},
        {0, 0, 0, 0}
    };

    /* set the default parameters */
    para->sens = HAL_PIR_SENS_DEFAULT;

    while ((c = getopt_long(argc, argv, short_options, long_options,
                            &option_index)) != -1) {
        switch (c) {
        case 'h':
            return -1;
        case 's':
            if (strcasecmp(optarg, "low") == 0) {
                para->sens = HAL_PIR_SENS_LOW;
            } else if (strcasecmp(optarg, "middle") == 0) {
                para->sens = HAL_PIR_SENS_MIDDLE;
            } else if (strcasecmp(optarg, "high") == 0) {
                para->sens = HAL_PIR_SENS_HIGH;
            } else {
                para->sens = HAL_PIR_SENS_DEFAULT;
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
    hal_pir_state_e pir_state;
    int timeout = 5000; // ms
    int ret;
    int i;

    printf("%s %s %s\n", argv[0], __DATE__, __TIME__);

    if (parse_args(argc, argv, &test_para)) {
        print_usage(argv[0]);
        return -1;
    }

    printf("para: sensitivity=%d\n", test_para.sens);

    ret = hal_pir_enable(NULL, TRUE);
    if (ret != 0) {
        printf("hal_pir_enable failed\n");
        return -1;
    }

    ret = hal_pir_set_sens(NULL, test_para.sens);
    if (ret != 0) {
        printf("hal_pir_set_sens failed\n");
        return -1;
    }

    for (i = 0; i < 10; i++) {
        printf("waiting for pir event...\n");

        ret = hal_pir_detect_state(NULL, &pir_state, timeout);
        if ((ret != 0) && (ret != -ETIMEDOUT)) {
            printf("hal_pir_detect_state failed\n");
            return -1;
        }

        ret = hal_pir_detect_state(NULL, &pir_state, 0);
        if (pir_state == HAL_PIR_TRIG) {
            printf("pir triggered\n");
        } else {
            printf("pir not triggered\n");
        }
    }

    return 0;
}
