/*
 * Copyright (C) 2021 Tuya Inc.
 */

#ifndef __HAL_GPIO_KEY_H__
#define __HAL_GPIO_KEY_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    HAL_GPIO_KEY_DOWN,   /* key pressed */
    HAL_GPIO_KEY_UP,     /* key released */
} hal_gpio_key_state_e;

/**
 * @brief Get key state.
 * @param name key device name.
 * @param state key state.
 *
 * @returns 0 on success, error code on failure.
 */
int hal_gpio_key_get_state(char *name, hal_gpio_key_state_e *state);


#ifdef __cplusplus
}
#endif

#endif
