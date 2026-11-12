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
#include "hal_adc_core.h"

/*
 * Get functions from custom library.
 */
void hal_get_dlsym_for_adc(void *handle)
{
    void *sym = NULL;

    if (NULL == handle) {
        return;
    }

    sym = dlsym(handle, "hal_adc_init");
    if (sym != NULL) {
        hal_if_adc.init = (int (*)(int))sym;
    }

    sym = dlsym(handle, "hal_adc_deinit");
    if (sym != NULL) {
        hal_if_adc.deinit = (int (*)(int))sym;
    }

    sym = dlsym(handle, "hal_adc_get_value");
    if (sym != NULL) {
        hal_if_adc.get_value = (int (*)(int, int, int *))sym;
    }

    sym = dlsym(handle, "hal_adc_get_resolution");
    if (sym != NULL) {
        hal_if_adc.get_resolution = (int (*)(int, int, hal_adc_res_t *))sym;
    }
}
