/*
 * Copyright (C) 2021 Tuya Inc.
 */

#ifndef __HAL_GPIO_H__
#define __HAL_GPIO_H__

#include "hal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* gpio value */
#define GPIO_HIGH   1
#define GPIO_LOW    0

/* gpio direction */
#define GPIO_OUT    0
#define GPIO_IN     1

/* gpio interrupt trigger type */
typedef enum {
    GPIO_IRQ_NONE,
    GPIO_IRQ_EDGE_RISING,   /* trigger on rising edge */
    GPIO_IRQ_EDGE_FALLING,  /* trigger on falling edge */
    GPIO_IRQ_EDGE_BOTH,     /* trigger on rising and falling edge */
} gpio_irq_type_e;

/* gpio attribute */
typedef struct {
    int direction;              /* GPIO_OUT or GPIO_IN */
    int value;                  /* initial level, only for OUTPUT */
    bool irq_en;                /* enable interrupt, only for INPUT */
    gpio_irq_type_e irq_type;   /* interrupt trigger type */
    int reserved[4];
} hal_gpio_attr_t;

/** Configure a gpio.
 * @param num gpio number.
 * @param attr gpio attribute.
 * @returns 0 on success, error code on failure.
 */
int hal_gpio_config(int num, hal_gpio_attr_t *attr);

/** Set gpio level.
 * @param num gpio number.
 * @param value gpio value. GPIO_HIGH or GPIO_LOW
 * @returns 0 on success, error code on failure.
 */
int hal_gpio_set_value(int num, int value);

/** Get gpio level.
 * @param num gpio number.
 * @param value gpio value.
 * @returns 0 on success, error code on failure.
 */
int hal_gpio_get_value(int num, int *value);

/** Wait for gpio interrupt.
 * @param num gpio number.
 * @param timeout
 *     -1 : block indefinitely
 *     0  : non-block
 *     >0 : timeout in milliseconds
 * @returns 0 on success,
 *          -ETIMEDOUT on timeout,
 *          other error code on failure.
 */
int hal_gpio_wait_for_irq(int num, int timeout);

#ifdef __cplusplus
}
#endif

#endif
