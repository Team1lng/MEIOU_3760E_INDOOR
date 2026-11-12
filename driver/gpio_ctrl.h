#ifndef _AMY_GPIO_H_
#define _AMY_GPIO_H_

typedef enum {
    GPIO_DIR_IN,
    GPIO_DIR_OUT,
    GPIO_DIR_OUT_HIGH,
    GPIO_DIR_OUT_LOW
}gpio_direction_t;

typedef enum {
    GPIO_EDGE_NONE,
    GPIO_EDGE_RISING,
    GPIO_EDGE_FALLING,
    GPIO_EDGE_BOTH
}gpio_edge_t;

typedef enum {
    GPIO_VALUE_LOW = 0,
    GPIO_VALUE_HIGH = 1
} gpio_value_t;

int Amy_gpio_init(int pin, int dir);

int Amy_gpio_set(int pin, int val);

int Amy_gpio_get_value_open(int pin);
int Amy_gpio_get(int fd);
int Amy_gpio_get_value_close(int fd);


void Amy_gpio_ctr_init(void);
#endif