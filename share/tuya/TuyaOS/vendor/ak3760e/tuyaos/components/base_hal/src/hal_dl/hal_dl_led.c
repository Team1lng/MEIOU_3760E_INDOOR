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
#include "hal_led_core.h"

/*
 * Get functions from custom library.
 */
void hal_get_dlsym_for_led(void *handle)
{
    void *sym = NULL;

    if (NULL == handle) {
        return;
    }

    sym = dlsym(handle, "hal_led_switch");
    if (sym != NULL) {
        hal_if_led.switch_ctrl = (int (*)(char *, int))sym;
    }

    sym = dlsym(handle, "hal_led_pwm_control");
    if (sym != NULL) {
        hal_if_led.pwm_control = (int (*)(char *, hal_led_pwm_attr_t *))sym;
    }

    sym = dlsym(handle, "hal_led_control");
    if (sym != NULL) {
        hal_if_led.pwm_control = (int (*)(char *, hal_led_attr_t *))sym;
    }
}
