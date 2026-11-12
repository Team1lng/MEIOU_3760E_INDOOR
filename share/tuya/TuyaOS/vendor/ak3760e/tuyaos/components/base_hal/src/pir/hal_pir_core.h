/*
 * Copyright (C) 2021 Tuya Inc.
 */

#ifndef __HAL_PIR_CORE_H__
#define __HAL_PIR_CORE_H__

#include "hal_pir.h"

/* HAL interface defination */
typedef struct {
    int (*enable)(char *name, bool enable);
    int (*set_sens)(char *name, int sens);
    int (*detect_state)(char *name, hal_pir_state_e *state, int timeout);
} hal_if_pir_t;

extern hal_if_pir_t hal_if_pir;


int hal_pir_enable_common(char *name, bool enable);
int hal_pir_set_sens_common(char *name, int sens);
int hal_pir_detect_state_common(char *name, hal_pir_state_e *state, int timeout);

#endif
