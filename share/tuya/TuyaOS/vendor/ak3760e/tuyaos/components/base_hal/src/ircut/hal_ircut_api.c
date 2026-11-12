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
#include "hal_ircut.h"
#include "hal_ircut_core.h"

int hal_ircut_switch(char *name, hal_ircut_cmd_e cmd)
{
    if (NULL == hal_if_ircut.switch_ctrl)
        return hal_ircut_switch_common(name, cmd);

    return hal_if_ircut.switch_ctrl(name, cmd);
}
