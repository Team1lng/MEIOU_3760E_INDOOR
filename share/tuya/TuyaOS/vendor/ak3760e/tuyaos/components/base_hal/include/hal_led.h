/*
 * Copyright (C) 2021 Tuya Inc.
 */

#ifndef __HAL_LED_H__
#define __HAL_LED_H__

#ifdef __cplusplus
extern "C" {
#endif

/* LED value */
#define LED_ON      1
#define LED_OFF     0

/** Turn on/off the switch type LED.
 * @param name led name.
 * @param on LED_ON or LED_OFF.
 * @returns 0 on success, error code on failure.
 */
int hal_led_switch(char *name, int on);

typedef struct {
    int freq;   /* frequency, in Hz */
    /*
     * duty cycle : range [0, 10000], e.g. 1234 means 12.34%
     *      0     - lowest  brightness
     *      10000 - highest brightness
     */
    int duty_cycle;
    int reserved[2];
} hal_led_pwm_attr_t;

/** Control a brightness gradient light/LED.
 * @param name led name.
 * @param attr led attribute.
 * @returns 0 on success, error code on failure.
 */
int hal_led_pwm_control(char *name, hal_led_pwm_attr_t *attr);


typedef struct {
    /* frequency, in Hz
     * For gpio led, set it to any value.
     * For pwm led,  set it to pwm frequency.
     */
    int freq;
    /* range [0, 10000], e.g. 1234 means 12.34%
     * For gpio led:
     *         0                     - Turn off led
     *         Non-zero (e.g. 10000) - Turn on  led
     * For pwm led:
     *         set it to pwm duty cycle.
     */
    int duty_cycle;
    int reserved[2];
} hal_led_attr_t;

/** Control a switch type led (gpio led) or brightness gradient led (pwm led).
 * @param name led name.
 * @param attr led attribute.
 * @returns 0 on success, error code on failure.
 */
int hal_led_control(char *name, hal_led_attr_t *attr);

/** Control a brightness gradient light/LED.
 * @param name led name.
 * @param brightness led brightness.
 * @returns 0 on success, error code on failure.
 */
int hal_led_brightness_set(char *name, int brightness);

#ifdef __cplusplus
}
#endif

#endif
