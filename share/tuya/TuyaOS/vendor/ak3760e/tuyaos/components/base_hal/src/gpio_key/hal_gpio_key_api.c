/*
 * Copyright (C) 2021 Tuya Inc.
 */

#include <errno.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "hal_misc.h"
#include "hal_log.h"
#include "hal_gpio_key.h"
#include "hal_gpio_key_core.h"

int hal_gpio_key_get_state(char *name, hal_gpio_key_state_e *state)
{
    return hal_gpio_key_get_state_common(name, state);
}
