#ifndef __APP_GPIO_H__
#define __APP_GPIO_H__
#include <stdbool.h>
void speak_enable(int en);
void wifi_power_enable(int en);
void atmosphere_gpio_set(bool enable);

/* Chime（门钟）GPIO 控制 */
void chime_gpio_enable(void);
void chime_gpio_disable(void);

/* 室内机锁 GPIO 控制 */
void lock_gpio_enable(void);
void lock_gpio_disable(void);

#endif