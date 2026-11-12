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
#include "hal_motor_core.h"

/*
 * Get functions from custom library.
 */
void hal_get_dlsym_for_motor(void *handle)
{
    void *sym = NULL;

    if (NULL == handle) {
        return;
    }

    sym = dlsym(handle, "hal_motor_init");
    if (sym != NULL) {
        hal_if_motor.init = (int (*)(int, hal_motor_attr_t *))sym;
    }

    sym = dlsym(handle, "hal_motor_deinit");
    if (sym != NULL) {
        hal_if_motor.deinit = (int (*)(int))sym;
    }

    sym = dlsym(handle, "hal_motor_get_attr");
    if (sym != NULL) {
        hal_if_motor.get_attr = (int (*)(int, hal_motor_attr_t *))sym;
    }

    sym = dlsym(handle, "hal_motor_op");
    if (sym != NULL) {
        hal_if_motor.op = (int (*)(int, hal_motor_cmd_t *))sym;
    }

    sym = dlsym(handle, "hal_motor_get_state");
    if (sym != NULL) {
        hal_if_motor.get_state = (int (*)(int, hal_motor_stat_t *))sym;
    }
}
