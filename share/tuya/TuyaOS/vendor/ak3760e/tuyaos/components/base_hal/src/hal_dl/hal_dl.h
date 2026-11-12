/*
 * Copyright (C) 2021 Tuya Inc.
 */

#ifndef __HAL_DL_H__
#define __HAL_DL_H__

void hal_get_dlsym_for_board(void *handle);
void hal_get_dlsym_for_gpio(void *handle);
void hal_get_dlsym_for_led(void *handle);
void hal_get_dlsym_for_ircut(void *handle);
void hal_get_dlsym_for_pir(void *handle);
void hal_get_dlsym_for_pwm(void *handle);
void hal_get_dlsym_for_adc(void *handle);
void hal_get_dlsym_for_watchdog(void *handle);
void hal_get_dlsym_for_motor(void *handle);

void hal_get_dlsym_for_subg(void *handle);
void hal_get_dlsym_for_modem_4g(void *handle);

void hal_get_dlsym_for_audio(void *handle);
void hal_get_dlsym_for_video(void *handle);

#endif
