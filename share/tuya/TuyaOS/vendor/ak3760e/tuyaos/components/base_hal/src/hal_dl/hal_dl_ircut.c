/*
 * Copyright (C) 2021 Tuya Inc.
 */

#include <errno.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <dlfcn.h>
#include "hal_misc.h"
#include "hal_log.h"
#include "hal_dl.h"
#include "hal_ircut_core.h"

/*
 * Get functions from custom library.
 */
void hal_get_dlsym_for_ircut(void *handle)
{
    void *sym = NULL;

    if (NULL == handle) {
        return;
    }

    sym = dlsym(handle, "hal_ircut_switch");
    if (sym != NULL) {
        hal_if_ircut.switch_ctrl = (int (*)(char *, hal_ircut_cmd_e))sym;
    }
}
