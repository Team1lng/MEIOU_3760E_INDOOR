/*
 * Copyright (C) 2021 Tuya Inc.
 */

#ifndef __HAL_MOTOR_H__
#define __HAL_MOTOR_H__

#include "hal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* motor device attribute */
typedef struct {
    int pan_step_max;   /* maximum number of horizontal steps */
    int tilt_step_max;  /* maximum number of vertical steps */
    int pan_roll_step;  /* horizontal rollback position (in steps) */
    bool infi_rot_en;   /* infinity rotation enable */

} __attribute__((packed, aligned(64))) hal_motor_attr_t;

/* motor command ID */
typedef enum {
    HAL_MOTOR_CMD_NONE,

    /* move towards the specified direction */
    HAL_MOTOR_CMD_GO_UP,
    HAL_MOTOR_CMD_GO_DOWN,
    HAL_MOTOR_CMD_GO_LEFT,
    HAL_MOTOR_CMD_GO_RIGHT,
    HAL_MOTOR_CMD_GO_LEFT_UP,
    HAL_MOTOR_CMD_GO_LEFT_DOWN,
    HAL_MOTOR_CMD_GO_RIGHT_UP,
    HAL_MOTOR_CMD_GO_RIGHT_DOWN,

    /* move to the specified position */
    HAL_MOTOR_CMD_GO_POS = 99,

    /* stop */
    HAL_MOTOR_CMD_STOP_ALL = 100,   /* stop horizontal and vertical movement */
    HAL_MOTOR_CMD_STOP_PAN,         /* stop horizontal movement */
    HAL_MOTOR_CMD_STOP_TILT,        /* stop vertical movement */

    /* self check */
    HAL_MOTOR_CMD_SELF_CHECK_ALL = 128, /* horizontal and vertical self-check */
    HAL_MOTOR_CMD_SELF_CHECK_PAN,       /* horizontal self-check */
    HAL_MOTOR_CMD_SELF_CHECK_TILT,      /* vertical self-check */

    HAL_MOTOR_CMD_CLEAR_OPTO_FLAG,

} hal_motor_cmd_id_e;

/* motor command parameter */
typedef struct {
    /* for HAL_MOTOR_CMD_GO_xxxxxx */
    int pan_step_period;    /* horizontal step period (in milliseconds) */
    int tilt_step_period;   /* vertical step period (in milliseconds) */

    /* for HAL_MOTOR_CMD_GO_POS */
    int pan_pos;    /* horizontal position (in steps) */
    int tilt_pos;   /* vertical position (in steps) */

} __attribute__((packed, aligned(64))) hal_motor_cmd_param_t;

typedef struct {
    hal_motor_cmd_id_e id;
    hal_motor_cmd_param_t param;
} hal_motor_cmd_t;

/* motor device status */
typedef struct {
    int pan_pos;    /* horizontal position (in steps) */
    int tilt_pos;   /* vertical position (in steps) */
    int right_opto_flag;/*right limit switch flag */
    int left_opto_flag; /*left limit switch flag */

} __attribute__((packed, aligned(128))) hal_motor_stat_t;

/** Initialize a motor device.
 * @param dev device number.
 * @param attr device attribute.
 *     If `attr` is NULL, default attributes will be used.
 * @returns 0 on success, error code on failure.
 */
int hal_motor_init(int dev, hal_motor_attr_t *attr);

/** Deinitialize a motor device.
 * @param dev device number.
 * @returns 0 on success, error code on failure.
 */
int hal_motor_deinit(int dev);

/** Get motor device attributes.
 * @param dev device number.
 * @param attr device attribute.
 * @returns 0 on success, error code on failure.
 */
int hal_motor_get_attr(int dev, hal_motor_attr_t *attr);

/** Perform a motor operation command.
 * @param dev device number.
 * @param cmd operation command.
 * @returns 0 on success, error code on failure.
 */
int hal_motor_op(int dev, hal_motor_cmd_t *cmd);

/** Get motor device status.
 * @param dev device number.
 * @param stat device status.
 * @returns 0 on success, error code on failure.
 */
int hal_motor_get_state(int dev, hal_motor_stat_t *stat);


#ifdef __cplusplus
}
#endif

#endif
