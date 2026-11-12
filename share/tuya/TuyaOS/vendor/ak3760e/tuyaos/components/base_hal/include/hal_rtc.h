/*
 * Copyright (C) 2021 Tuya Inc.
 */

#ifndef __HAL_RTC_H__
#define __HAL_RTC_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int second;
    int minute;
    int hour;
    int day;
    int month;
    int year;
} hal_rtc_time_t;

/** Initialize rtc device.
 * @param void
 * @returns 0 on success, error code on failure.
 */
int hal_rtc_init(void);

/** Deinitialize rtc device.
 * @param void
 * @returns 0 on success, error code on failure.
 */
int hal_rtc_deinit(void);

/** Set rtc time.
 * @param time rtc time
 * @returns 0 on success, error code on failure.
 */
int hal_rtc_set_time(hal_rtc_time_t *time);

/**  Readrtc time.
 * @param time rtc time
 * @returns 0 on success, error code on failure.
 */
int hal_rtc_get_time(hal_rtc_time_t *time);

#ifdef __cplusplus
}
#endif

#endif
