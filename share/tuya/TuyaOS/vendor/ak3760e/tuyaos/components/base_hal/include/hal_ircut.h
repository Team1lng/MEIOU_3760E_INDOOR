/*
 * Copyright (C) 2021 Tuya Inc.
 */

#ifndef __HAL_IRCUT_H__
#define __HAL_IRCUT_H__

#ifdef __cplusplus
extern "C" {
#endif

/* IRCUT command */
typedef enum {
    IRCUT_CMD_NONE,         /* null command */
    IRCUT_CMD_ON,           /* enable ircut function */
    IRCUT_CMD_OFF,          /* disable ircut function */
    IRCUT_CMD_SLEEP,        /* put the ircut device to sleep state */
} hal_ircut_cmd_e;

/** Manipulate an IRCUT device.
 * @param name ircut name.
 *     If name is NULL, the default device name will be used.
 * @param cmd operating command.
 * @returns 0 on success, error code on failure.
 */
int hal_ircut_switch(char *name, hal_ircut_cmd_e cmd);


#ifdef __cplusplus
}
#endif

#endif
