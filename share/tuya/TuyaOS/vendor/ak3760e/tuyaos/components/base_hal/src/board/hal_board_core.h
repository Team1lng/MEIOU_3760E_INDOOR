/*
 * Copyright (C) 2021 Tuya Inc.
 */

#ifndef __HAL_BOARD_CORE_H__
#define __HAL_BOARD_CORE_H__

#include "hal_board.h"

/* HAL interface defination */
typedef struct {
    int (*get_config_file)(char *buf, int size);
} hal_if_board_t;

extern hal_if_board_t hal_if_board;

#endif
