/*
 * Copyright (C) 2021 Tuya Inc.
 */

#ifndef __HAL_SUBG_CORE_H__
#define __HAL_SUBG_CORE_H__

#include "hal_subg.h"

/* HAL interface defination */
typedef struct {
    int (*send)(void *buf, int size);
    int (*receive)(void *buf, int size);
} hal_if_subg_t;

extern hal_if_subg_t hal_if_subg;


int hal_subg_send_common(void *buf, int size);
int hal_subg_receive_common(void *buf, int size);

#endif
