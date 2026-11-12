/*
 * Copyright (C) 2021 Tuya Inc.
 */

#ifndef __HAL_BACKLIGHT_H__
#define __HAL_BACKLIGHT_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifndef DISPLAY_BACKLIGHT_PATH
#define DISPLAY_BACKLIGHT_PATH 		"/sys/class/backlight/backlight/brightness"
#endif

int hal_backlight_set_value(int brightness);
int hal_backlight_get_value(void);

#ifdef __cplusplus
}
#endif

#endif
