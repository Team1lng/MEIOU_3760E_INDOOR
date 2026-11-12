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
#include <signal.h>
#include "hal_motor.h"

typedef struct {
    int dev;    /* motor device number */
} hal_motor_test_para_t;

static hal_motor_test_para_t test_para;

static void print_usage(const char *prog)
{
    printf("Usage: %s [OPTION]\n", prog);
    printf(
"  -d <device number>  default 0\n");
    printf("e.g. %s -d 0\n", prog);
}

static int parse_args(int argc, char **argv, hal_motor_test_para_t *para)
{
    int option_index;
    int c;
    const char short_options[] = "d:h";
    const struct option long_options[] = {
        {"help", 0, 0, 'h'},
        {0, 0, 0, 0}
    };

    /* set the default parameters */
    para->dev = 0;

    while ((c = getopt_long(argc, argv, short_options, long_options,
                            &option_index)) != -1) {
        switch (c) {
        case 'h':
            return -1;
        case 'd':
            para->dev = atoi(optarg);
            break;
        default:
            return -1;
        }
    }

    return 0;
}

static int sg_flag = 0;
void signal_handle(int sig)
{
    printf("get signal: %d\n", sig);

    switch(sig) {
    case SIGINT:
    case SIGKILL:
    case SIGTERM:
        sg_flag = 1;
        break;

    default:
        break;
    }

    return;
}

static void motor_func_example(int dev)
{
    hal_motor_cmd_t cmd;
    hal_motor_stat_t motor_stat;
    int pan_step_period;    /* horizontal step period (in milliseconds) */
    int tilt_step_period;   /* vertical step period (in milliseconds) */
    int pan_pos;            /* horizontal position (in steps) */
    int tilt_pos;           /* vertical position (in steps) */
    int func_num = 0;
    int ret;

    ret = hal_motor_init(dev, NULL);
    if (ret != 0) {
        printf("error: hal_motor_init failed\n");
        return;
    }

    cmd.param.pan_step_period = 5;
    cmd.param.tilt_step_period = 4;

    while (!sg_flag) {
        printf("0  go up\n");
        printf("1  go down\n");
        printf("2  go left\n");
        printf("3  go right\n");
        printf("4  go left_up\n");
        printf("5  go left_down\n");
        printf("6  go right_up\n");
        printf("7  go right_down\n");
        printf("8  go specific position\n");
        printf("----------\n");

        printf("9  all stop\n");
        printf("10 pan stop\n");
        printf("11 tilt stop\n");
        printf("----------\n");

        printf("12 all self check\n");
        printf("13 pan self check\n");
        printf("14 tilt self check\n");
        printf("----------\n");

        printf("15 set pan-tilt speed\n");
        printf("16 get pan-tilt position\n");
        printf("17 exit\n");
        printf("please enter the function number: \n");

        ret = scanf("%d", &func_num);
        if (ret != 1) {
            printf("scanf error %d\n", ret);
        }
        getchar();

        switch(func_num) {
        case 0:
            cmd.id = HAL_MOTOR_CMD_GO_UP;
            ret = hal_motor_op(dev, &cmd);
            break;
        case 1:
            cmd.id = HAL_MOTOR_CMD_GO_DOWN;
            ret = hal_motor_op(dev, &cmd);
            break;
        case 2:
            cmd.id = HAL_MOTOR_CMD_GO_LEFT;
            ret = hal_motor_op(dev, &cmd);
            break;
        case 3:
            cmd.id = HAL_MOTOR_CMD_GO_RIGHT;
            ret = hal_motor_op(dev, &cmd);
            break;
        case 4:
            cmd.id = HAL_MOTOR_CMD_GO_LEFT_UP;
            ret = hal_motor_op(dev, &cmd);
            break;
        case 5:
            cmd.id = HAL_MOTOR_CMD_GO_LEFT_DOWN;
            ret = hal_motor_op(dev, &cmd);
            break;
        case 6:
            cmd.id = HAL_MOTOR_CMD_GO_RIGHT_UP;
            ret = hal_motor_op(dev, &cmd);
            break;
        case 7:
            cmd.id = HAL_MOTOR_CMD_GO_RIGHT_DOWN;
            ret = hal_motor_op(dev, &cmd);
            break;

        case 8:
            printf("please input the pan position:");
            ret = scanf("%d", &pan_pos);
            getchar();
            if (ret != 1) {
                printf("scanf error %d\n", ret);
                break;
            }

            printf("please input the tilt position:");
            ret = scanf("%d", &tilt_pos);
            getchar();
            if (ret != 1) {
                printf("scanf error %d\n", ret);
                break;
            }

            cmd.id = HAL_MOTOR_CMD_GO_POS;
            cmd.param.pan_pos = pan_pos;
            cmd.param.tilt_pos = tilt_pos;
            ret = hal_motor_op(dev, &cmd);
            break;

        case 9:
            cmd.id = HAL_MOTOR_CMD_STOP_ALL;
            ret = hal_motor_op(dev, &cmd);
            break;
        case 10:
            cmd.id = HAL_MOTOR_CMD_STOP_PAN;
            ret = hal_motor_op(dev, &cmd);
            break;
        case 11:
            cmd.id = HAL_MOTOR_CMD_STOP_TILT;
            ret = hal_motor_op(dev, &cmd);
            break;

        case 12:
            cmd.id = HAL_MOTOR_CMD_SELF_CHECK_ALL;
            ret = hal_motor_op(dev, &cmd);
            break;
        case 13:
            cmd.id = HAL_MOTOR_CMD_SELF_CHECK_PAN;
            ret = hal_motor_op(dev, &cmd);
            break;
        case 14:
            cmd.id = HAL_MOTOR_CMD_SELF_CHECK_TILT;
            ret = hal_motor_op(dev, &cmd);
            break;

        case 15:
            printf("please input the pan_step_period:");
            ret = scanf("%d", &pan_step_period);
            getchar();
            if (ret != 1) {
                printf("scanf pan speed erro %d\n", ret);
                break;
            }

            printf("please input the tilt_step_period:");
            ret = scanf("%d", &tilt_step_period);
            getchar();
            if (ret != 1) {
                printf("scanf tilt speed erro %d\n", ret);
                break;
            }

            cmd.param.pan_step_period = pan_step_period;
            cmd.param.tilt_step_period = tilt_step_period;
            printf("new pan_step_period: %d\n", pan_step_period);
            printf("new tilt_step_period: %d\n", tilt_step_period);
            break;

        case 16:
            ret = hal_motor_get_state(dev, &motor_stat);
            if (ret != 0) {
                printf("hal_motor_get_state failed\n");
                break;
            }

            printf("pan pos: %d\n", motor_stat.pan_pos);
            printf("tilt pos: %d\n", motor_stat.tilt_pos);
            break;

        case 17:
        default:
            sg_flag = 1;
            break;
        }
    }

    hal_motor_deinit(dev);
    return;
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

    printf("para: dev=%d\n", test_para.dev);

    signal(SIGINT, signal_handle);

    motor_func_example(test_para.dev);
    return 0;
}
