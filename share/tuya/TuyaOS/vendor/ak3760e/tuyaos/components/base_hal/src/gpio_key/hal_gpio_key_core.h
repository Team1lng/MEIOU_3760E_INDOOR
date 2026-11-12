/*
 * Copyright (C) 2021 Tuya Inc.
 */

#ifndef __HAL_GPIO_KEY_CORE_H__
#define __HAL_GPIO_KEY_CORE_H__

#include "hal_gpio_key.h"


int hal_gpio_key_get_state_common(char *name, hal_gpio_key_state_e *state);

#endif
