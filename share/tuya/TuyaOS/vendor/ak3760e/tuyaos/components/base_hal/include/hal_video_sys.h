/*
 * Copyright (C) 2021 Tuya Inc.
 */

#ifndef HAL_VIDEO_SYS_H
#define HAL_VIDEO_SYS_H

#include "hal_video_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Initialize video system.
 * @param void
 * @returns 0 on success, error code on failure.
 */
int hal_video_sys_init(void);

/** Deinitialize video system.
 * @param void
 * @returns 0 on success, error code on failure.
 */
int hal_video_sys_deinit(void);

#ifdef __cplusplus
}
#endif

#endif
