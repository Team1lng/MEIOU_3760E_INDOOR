/*
 * Copyright (C) 2021 Tuya Inc.
 */

#ifndef __HAL_IRCUT_CORE_H__
#define __HAL_IRCUT_CORE_H__

#include "hal_ircut.h"

/* HAL interface defination */
typedef struct {
    int (*switch_ctrl)(char *name, hal_ircut_cmd_e cmd);
} hal_if_ircut_t;

extern hal_if_ircut_t hal_if_ircut;


int hal_ircut_switch_common(char *name, hal_ircut_cmd_e cmd);

#endif
