/*
 * Copyright (C) 2021 Tuya Inc.
 */

#ifndef __HAL_MOTOR_CORE_H__
#define __HAL_MOTOR_CORE_H__

#include "hal_motor.h"

/* HAL interface defination */
typedef struct {
    int (*init)(int dev, hal_motor_attr_t *attr);
    int (*deinit)(int dev);
    int (*get_attr)(int dev, hal_motor_attr_t *attr);
    int (*op)(int dev, hal_motor_cmd_t *cmd);
    int (*get_state)(int dev, hal_motor_stat_t *stat);
} hal_if_motor_t;

extern hal_if_motor_t hal_if_motor;

int hal_motor_init_common(int dev, hal_motor_attr_t *attr);
int hal_motor_deinit_common(int dev);
int hal_motor_get_attr_common(int dev, hal_motor_attr_t *attr);
int hal_motor_op_common(int dev, hal_motor_cmd_t *cmd);
int hal_motor_get_state_common(int dev, hal_motor_stat_t *stat);

#endif
