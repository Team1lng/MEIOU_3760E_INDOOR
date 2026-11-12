/*
 * Copyright (C) 2021 Tuya Inc.
 */

#ifndef __HAL_PIR_H__
#define __HAL_PIR_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    HAL_PIR_TRIG,       /* triggered */
    HAL_PIR_NON_TRIG    /* non-triggered */
} hal_pir_state_e;

/* PIR sensitivity */
typedef enum {
    HAL_PIR_SENS_DEFAULT,   /* default sensitivity configured by hardware */
    HAL_PIR_SENS_LOW,       /* low level sensitivity */
    HAL_PIR_SENS_MIDDLE,    /* middle level sensitivity */
    HAL_PIR_SENS_HIGH,      /* high level sensitivity */
} hal_pir_sens_e;

/**
 * @brief Enable/Disable PIR device.
 * @param name PIR device name.
 *     If name is NULL, the default device name will be used.
 * @param enable enable or not.
 * @returns 0 on success, error code on failure.
 */
int hal_pir_enable(char *name, bool enable);

/**
 * @brief Set PIR sensitivity.
 * @param name PIR device name.
 *     If name is NULL, the default device name will be used.
 * @param sens sensitivity.
 * @returns 0 on success, error code on failure.
 */
int hal_pir_set_sens(char *name, int sens);

/**
 * @brief Detect PIR state.
 * @param name PIR device name.
 *     If name is NULL, the default device name will be used.
 * @param state PIR state.
 * @param timeout
 *     -1 : block indefinitely
 *     0  : non-block
 *     >0 : timeout in milliseconds
 * @returns 0 on success,
 *          -ETIMEDOUT on timeout,
 *          other error code on failure.
 * @note This function will block for <TIMEOUT> milliseconds
 *       until the PIR event is triggered, and save the current state to <STATE>.
 *       If TIMEOUT is 0, it only gets the state
 *       and does not wait for the PIR event.
 */
int hal_pir_detect_state(char *name, hal_pir_state_e *state, int timeout);


#ifdef __cplusplus
}
#endif

#endif
