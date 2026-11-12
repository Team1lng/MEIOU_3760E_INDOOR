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

#define BASE_HAL_CUSTOM_LIB "libbase_hal_custom.so"

static void *handle = NULL;

void __attribute__((constructor)) base_hal_load(void)
{
    char *dl_error;

    handle = dlopen(BASE_HAL_CUSTOM_LIB, RTLD_NOW);
    if (NULL == handle) {
        hal_log(HAL_LOG_NOTICE,"%s\n",dlerror());
     // hal_log(HAL_LOG_NOTICE, "%s not found\n", BASE_HAL_CUSTOM_LIB);
        return;
    }

    /* get functions from custom library */

    hal_get_dlsym_for_board(handle);
    hal_get_dlsym_for_gpio(handle);
    hal_get_dlsym_for_led(handle);
    hal_get_dlsym_for_ircut(handle);
    hal_get_dlsym_for_pir(handle);
    hal_get_dlsym_for_pwm(handle);
    hal_get_dlsym_for_adc(handle);
    hal_get_dlsym_for_watchdog(handle);
    hal_get_dlsym_for_motor(handle);

    hal_get_dlsym_for_subg(handle);
    hal_get_dlsym_for_modem_4g(handle);

    hal_get_dlsym_for_audio(handle);
    hal_get_dlsym_for_video(handle);

    if((dl_error = dlerror()) != NULL) {
        hal_log(HAL_LOG_NOTICE,"%s\n",dl_error);
    }
}

void __attribute__((destructor)) base_hal_unload(void)
{
    if ( handle )
        dlclose(handle);
}
