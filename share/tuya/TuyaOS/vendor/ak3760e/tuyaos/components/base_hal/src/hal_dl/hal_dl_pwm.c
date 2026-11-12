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
#include "hal_pwm_core.h"

/*
 * Get functions from custom library.
 */
void hal_get_dlsym_for_pwm(void *handle)
{
    void *sym = NULL;

    if (NULL == handle) {
        return;
    }

    sym = dlsym(handle, "hal_pwm_config");
    if (sym != NULL) {
        hal_if_pwm.config = (int (*)(int, int, hal_pwm_chn_attr_t *))sym;
    }

    sym = dlsym(handle, "hal_pwm_enable");
    if (sym != NULL) {
        hal_if_pwm.enable = (int (*)(int, int))sym;
    }

    sym = dlsym(handle, "hal_pwm_disable");
    if (sym != NULL) {
        hal_if_pwm.disable = (int (*)(int, int))sym;
    }
}
