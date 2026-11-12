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
#include "hal_pir_core.h"

/*
 * Get functions from custom library.
 */
void hal_get_dlsym_for_pir(void *handle)
{
    void *sym = NULL;

    if (NULL == handle) {
        return;
    }

    sym = dlsym(handle, "hal_pir_enable");
    if (sym != NULL) {
        hal_if_pir.enable = (int (*)(char *, bool))sym;
    }

    sym = dlsym(handle, "hal_pir_set_sens");
    if (sym != NULL) {
        hal_if_pir.set_sens = (int (*)(char *, int))sym;
    }

    sym = dlsym(handle, "hal_pir_detect_state");
    if (sym != NULL) {
        hal_if_pir.detect_state = (int (*)(char *, hal_pir_state_e *, int))sym;
    }
}
