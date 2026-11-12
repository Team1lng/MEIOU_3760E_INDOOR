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
#include "hal_pir.h"
#include "hal_pir_core.h"

int hal_pir_enable(char *name, bool enable)
{
    if (NULL == hal_if_pir.enable)
        return hal_pir_enable_common(name, enable);

    return hal_if_pir.enable(name, enable);
}

int hal_pir_set_sens(char *name, int sens)
{
    if (NULL == hal_if_pir.set_sens)
        return hal_pir_set_sens_common(name, sens);

    return hal_if_pir.set_sens(name, sens);
}

int hal_pir_detect_state(char *name, hal_pir_state_e *state, int timeout)
{
    if (NULL == hal_if_pir.detect_state)
        return hal_pir_detect_state_common(name, state, timeout);

    return hal_if_pir.detect_state(name, state, timeout);
}
