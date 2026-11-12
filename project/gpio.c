#include "gpio.h"
#include "Amy_log.h"
#include "Amy_sdk.h"
#include "gpio_ctrl.h"

// 功放
#define AMP_GPIO_PIN (37)
void speak_enable(int en)
{
    Amy_gpio_set(AMP_GPIO_PIN, en ? 1 : 0);
}

// WIFI
#define WIFI_PWR_EN (83)
void wifi_power_enable(int en)
{
    return;
    Amy_gpio_set(WIFI_PWR_EN, en ? 0 : 0);
}

// 氛围灯
#define ATMOSPHERE_CTRL_PIN (25)
void atmosphere_gpio_set(bool enable)
{
    Amy_gpio_set(ATMOSPHERE_CTRL_PIN, enable ? 1 : 0);
}

// Chime 门钟
#define CHIME_EN_PIN (52)
void chime_gpio_enable(void)
{
    Amy_gpio_set(CHIME_EN_PIN, 1);
}

void chime_gpio_disable(void)
{
    Amy_gpio_set(CHIME_EN_PIN, 0);
}

// 室内机锁
#define LOCK_GPIO_PIN (51)
void lock_gpio_enable(void)
{
    Amy_gpio_set(LOCK_GPIO_PIN, 1);
}

void lock_gpio_disable(void)
{
    Amy_gpio_set(LOCK_GPIO_PIN, 0);
}

void Amy_gpio_ctr_init(void)
{
    // 功放
    Amy_gpio_init(AMP_GPIO_PIN, GPIO_DIR_OUT_LOW);
    // 氛围灯
    Amy_gpio_init(ATMOSPHERE_CTRL_PIN, GPIO_DIR_OUT_LOW);
    // Chime 门钟
    Amy_gpio_init(CHIME_EN_PIN, GPIO_DIR_OUT_LOW);
    // 室内机锁
    Amy_gpio_init(LOCK_GPIO_PIN, GPIO_DIR_OUT_LOW);
    // Amy_gpio_init(WIFI_PWR_EN, GPIO_DIR_OUT_LOW);
}
