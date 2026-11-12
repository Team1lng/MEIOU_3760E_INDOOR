/*
 * Copyright (C) 2021 Tuya Inc.
 */

#ifndef __HAL_INPUT_EVENT_H__
#define __HAL_INPUT_EVENT_H__

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_DEVICES 32

/**
 * @brief Initialize input event device.
 * @param void
 * @return 0 on success, error code on failure.
 * @note This function will open and listen all input event devices.
 */
int hal_input_event_init(void);

/**
 * @brief Receive input events
 * @param ev input event array
 * @param count the number of input events received
 * @param timeout
 *     -1 : block indefinitely
 *     0  : non-block
 *     >0 : timeout in milliseconds
 * @returns 0 on success,
 *          -ETIMEDOUT on timeout,
 *          other error code on failure.
 * @pre hal_input_event_init must be called first
 * @note Multiple events can be received in one call
 */
int hal_input_event_recv(struct input_event *ev, int *count, int timeout);

/**
 * @brief De-Initialize input event device.
 * @param void
 * @return void
 */
void hal_input_event_exit(void);


#ifdef __cplusplus
}
#endif

#endif
