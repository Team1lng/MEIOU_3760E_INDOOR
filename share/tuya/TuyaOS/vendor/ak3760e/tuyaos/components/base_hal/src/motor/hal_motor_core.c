/*
 * Copyright (C) 2021 Tuya Inc.
 */

#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/reboot.h>
#include <pthread.h>
#include "bcmnvram.h"
#include "hal_log.h"
#include "hal_misc.h"
#include "hal_motor.h"
#include "hal_motor_core.h"
#include "ty_motor_usr.h"

/*
 * For generic instance, set all the members to NULL
 */
hal_if_motor_t hal_if_motor = { 0 };


#define HAL_MOTOR_MAX_NUM   1
#define HAL_MOTOR_DEV_NAME  "/dev/tymotor"

typedef struct {
    int fd;
} hal_motor_priv_data_t;

static hal_motor_priv_data_t motor_priv_data[HAL_MOTOR_MAX_NUM];

static inline bool motor_dev_valid(int dev)
{
    return ((dev >= 0) && (dev < HAL_MOTOR_MAX_NUM));
}

int hal_motor_init_common(int dev, hal_motor_attr_t *attr)
{
    int fd = -1;
    int ret;

    if (! motor_dev_valid(dev)) {
        hal_log(HAL_LOG_ERR, "%s: invalid dev %d\n", __FUNCTION__, dev);
        return -EINVAL;
    }

    fd = open(HAL_MOTOR_DEV_NAME, O_RDWR);
    if (fd < 0) {
        hal_log(HAL_LOG_ERR, "%s: Failed to open %s\n",
                __FUNCTION__, HAL_MOTOR_DEV_NAME);
        return -EINVAL;
    }

    /* Set the motor properties (if specified by the user) */
    if (attr != NULL) {
        struct ty_motor_param_init_s init_param;
        init_param._pan_step_max = attr->pan_step_max;
        init_param._tilt_step_max = attr->tilt_step_max;
        init_param._pan_roll_step = attr->pan_roll_step;
        init_param._infi_rot_en = attr->infi_rot_en;
        ret = ioctl(fd, TY_MOTOR_IOCTL_PARA_INIT_SET, &init_param);
        if (ret != 0) {
            hal_log(HAL_LOG_ERR, "%s: Failed to set motor param\n", __FUNCTION__);
            close(fd);
            return ret;
        }
    }

    /* Save private data */
    motor_priv_data[dev].fd = fd;
    return 0;
}

int hal_motor_deinit_common(int dev)
{
    if (! motor_dev_valid(dev)) {
        hal_log(HAL_LOG_ERR, "%s: invalid dev %d\n", __FUNCTION__, dev);
        return -EINVAL;
    }

    if (motor_priv_data[dev].fd > 0) {
        close(motor_priv_data[dev].fd);
        motor_priv_data[dev].fd = -1;
    }
    return 0;
}

int hal_motor_get_attr_common(int dev, hal_motor_attr_t *attr)
{
    // TODO: 'get_attr' not supported by motor driver
    return -EPERM;
}

int hal_motor_op_common(int dev, hal_motor_cmd_t *cmd)
{
    struct ty_motor_param_s param;
    struct ty_motor_param_g gparam= {0};
    int fd;
    int ret = 0;

    if (! motor_dev_valid(dev)) {
        hal_log(HAL_LOG_ERR, "%s: invalid dev %d\n", __FUNCTION__, dev);
        return -EINVAL;
    }

    if (NULL == cmd) {
        hal_log(HAL_LOG_ERR, "%s: invalid cmd\n", __FUNCTION__);
        return -EINVAL;
    }

    memset(&param, 0, sizeof(param));
    param.goal_pan_speed = cmd->param.pan_step_period;
    param.goal_tilt_speed = cmd->param.tilt_step_period;
    param.goal_pan_pos = cmd->param.pan_pos;
    param.goal_tilt_pos = cmd->param.tilt_pos;

    fd = motor_priv_data[dev].fd;
    switch (cmd->id) {
    /* move towards the specified direction */
    case HAL_MOTOR_CMD_GO_UP:
        param.motor_func = GO_UP;
        ret = ioctl(fd, TY_MOTOR_IOCTL_SET, &param);
        break;
    case HAL_MOTOR_CMD_GO_DOWN:
        param.motor_func = GO_DOWN;
        ret = ioctl(fd, TY_MOTOR_IOCTL_SET, &param);
        break;
    case HAL_MOTOR_CMD_GO_LEFT:
        param.motor_func = GO_LEFT;
        ret = ioctl(fd, TY_MOTOR_IOCTL_SET, &param);
        break;
    case HAL_MOTOR_CMD_GO_RIGHT:
        param.motor_func = GO_RIGHT;
        ret = ioctl(fd, TY_MOTOR_IOCTL_SET, &param);
        break;
    case HAL_MOTOR_CMD_GO_LEFT_UP:
        param.motor_func = LEFT_UP;
        ret = ioctl(fd, TY_MOTOR_IOCTL_SET, &param);
        break;
    case HAL_MOTOR_CMD_GO_RIGHT_UP:
        param.motor_func = RIGHT_UP;
        ret = ioctl(fd, TY_MOTOR_IOCTL_SET, &param);
        break;
    case HAL_MOTOR_CMD_GO_LEFT_DOWN:
        param.motor_func = LEFT_DOWN;
        ret = ioctl(fd, TY_MOTOR_IOCTL_SET, &param);
        break;
    case HAL_MOTOR_CMD_GO_RIGHT_DOWN:
        param.motor_func = RIGHT_DOWN;
        ret = ioctl(fd, TY_MOTOR_IOCTL_SET, &param);
        break;

    /* move to the specified position */
    case HAL_MOTOR_CMD_GO_POS:
        param.motor_func = GO_POS;
        ret = ioctl(fd, TY_MOTOR_IOCTL_SET, &param);
        break;

    /* stop */
    case HAL_MOTOR_CMD_STOP_ALL:
        ret = ioctl(fd, TY_MOTOR_IOCTL_STOP, NULL);
        break;
    case HAL_MOTOR_CMD_STOP_PAN:
        param.motor_func = PAN_STOP;
        ret = ioctl(fd, TY_MOTOR_IOCTL_SET, &param);
        break;
    case HAL_MOTOR_CMD_STOP_TILT:
        param.motor_func = TILT_STOP;
        ret = ioctl(fd, TY_MOTOR_IOCTL_SET, &param);
        break;

    /* self check */
    case HAL_MOTOR_CMD_SELF_CHECK_ALL:
        param.motor_func = PAN_SELF_CHECK;
        ret = ioctl(fd, TY_MOTOR_IOCTL_SET, &param);
        param.motor_func = TILT_SELF_CHECK;
        ret |= ioctl(fd, TY_MOTOR_IOCTL_SET, &param);
        break;
    case HAL_MOTOR_CMD_SELF_CHECK_PAN:
        param.motor_func = PAN_SELF_CHECK;
        ret = ioctl(fd, TY_MOTOR_IOCTL_SET, &param);
        break;
    case HAL_MOTOR_CMD_SELF_CHECK_TILT:
        param.motor_func = TILT_SELF_CHECK;
        ret = ioctl(fd, TY_MOTOR_IOCTL_SET, &param);
        break;
    case HAL_MOTOR_CMD_CLEAR_OPTO_FLAG:
	gparam.state_cmd = SET_CLEAR_OPTO_FLAG;
	ret = ioctl(fd,TY_MOTOR_IOCTL_GET,&param);
	break;
    default:
        hal_log(HAL_LOG_ERR, "%s: cmd %d NOT supported\n", __FUNCTION__, cmd->id);
        return -EINVAL;
    }

    return ret;
}

int hal_motor_get_state_common(int dev, hal_motor_stat_t *stat)
{
    struct ty_motor_param_g gparam;
    int pan_pos;
    int tilt_pos;
    int fd;
    int ret = 0;
    int right_opto_flag;
    int left_opto_flag;

    if (! motor_dev_valid(dev)) {
        hal_log(HAL_LOG_ERR, "%s: invalid dev %d\n", __FUNCTION__, dev);
        return -EINVAL;
    }

    if (NULL == stat) {
        hal_log(HAL_LOG_ERR, "%s: invalid stat\n", __FUNCTION__);
        return -EINVAL;
    }

    fd = motor_priv_data[dev].fd;

    memset(&gparam, 0, sizeof(gparam));

    gparam.state_cmd = GET_PAN_POS;
    ret = ioctl(fd, TY_MOTOR_IOCTL_GET, &gparam);
    if (ret != 0) {
        hal_log(HAL_LOG_ERR, "%s: GET_PAN_POS failed\n", __FUNCTION__);
        return -EINVAL;
    }
    pan_pos = gparam.state_value;

    gparam.state_cmd = GET_TILT_POS;
    ret = ioctl(fd, TY_MOTOR_IOCTL_GET, &gparam);
    if (ret != 0) {
        hal_log(HAL_LOG_ERR, "%s: GET_TILT_POS failed\n", __FUNCTION__);
        return -EINVAL;
    }
    tilt_pos = gparam.state_value;

    gparam.state_cmd =  GET_LEFT_OPTO_FLAG;;
    ret = ioctl(fd, TY_MOTOR_IOCTL_GET, &gparam);
    if (ret != 0) {
        hal_log(HAL_LOG_ERR, "%s: GET_TILT_POS failed\n", __FUNCTION__);
        return -EINVAL;
    }

    left_opto_flag = gparam.state_value;


    gparam.state_cmd =  GET_RIGHT_OPTO_FLAG;;
    ret = ioctl(fd, TY_MOTOR_IOCTL_GET, &gparam);
    if (ret != 0) {
        hal_log(HAL_LOG_ERR, "%s: GET_TILT_POS failed\n", __FUNCTION__);
        return -EINVAL;
    }

    right_opto_flag = gparam.state_value;

    stat->pan_pos = pan_pos;
    stat->tilt_pos = tilt_pos;
    stat->left_opto_flag = left_opto_flag;
    stat->right_opto_flag = right_opto_flag;

    return 0;
}
