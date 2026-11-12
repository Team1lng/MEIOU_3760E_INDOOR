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
#include "hal_gpio_core.h"

/*
 * Get functions from custom library.
 */
void hal_get_dlsym_for_gpio(void *handle)
{
    void *sym = NULL;

    if (NULL == handle) {
        return;
    }

    sym = dlsym(handle, "hal_gpio_config");
    if (sym != NULL) {
        hal_if_gpio.config = (int (*)(int, hal_gpio_attr_t *))sym;
    }

    sym = dlsym(handle, "hal_gpio_set_value");
    if (sym != NULL) {
        hal_if_gpio.set_value = (int (*)(int, int))sym;
    }

    sym = dlsym(handle, "hal_gpio_get_value");
    if (sym != NULL) {
        hal_if_gpio.get_value = (int (*)(int, int *))sym;
    }

    sym = dlsym(handle, "hal_gpio_wait_for_irq");
    if (sym != NULL) {
        hal_if_gpio.wait_for_irq = (int (*)(int, int))sym;
    }
}
