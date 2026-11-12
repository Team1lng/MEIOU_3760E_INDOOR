#include "key.h"
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include "Amy_audio_play.h"
#include "user_data.h"
#include "Amy_sdk.h"
#include "gpio_ctrl.h"

/* 机械按键 GPIO（低电平触发），ADC 触摸按键复用 EMOS 的 SARADC 通道 */
#define MECHANICAL_KEY_PIN 55
#define ADC_RAW_PATH "/sys/bus/iio/devices/iio:device0/in_voltage0_raw"
#define ADC_KEY_THRESHOLD 500
#define ADC_KEY_VALUE 2
#define ADC_DEBOUNCE_MS 500
#define KEY_DETECT_INTERVAL_MS 50

void key_event_push(key_type_t type, unsigned long arg1, unsigned long arg2)
{
    if (type == KEY_TYPE_MECHANICAL)
        Amy_mechanical_key_event_put(arg1, arg2);
    else if (type == KEY_TYPE_ADC)
        Amy_adc_key_event_put(arg1, arg2);
}

/* =========================== 机械按键（GPIO）检测 =========================== */

static int mechanical_key_level_read(void)
{
    int fd = Amy_gpio_get_value_open(MECHANICAL_KEY_PIN);
    if (fd < 0)
        return -1;
    int level = Amy_gpio_get(fd);
    Amy_gpio_get_value_close(fd);
    return level;
}

static void mechanical_key_detect(void)
{
    static int prev_level = GPIO_VALUE_HIGH;
    static bool detect_beating = false;
    int level = mechanical_key_level_read();
    if (level < 0)
        return;

    if (prev_level != level)
    {
        if (detect_beating == false)
        {
            detect_beating = true;
        }
        else
        {
            detect_beating = false;
            prev_level = level;
            if (level == GPIO_VALUE_LOW)
            {
                key_event_push(KEY_TYPE_MECHANICAL, 0, 0);
            }
        }
    }
    else
    {
        detect_beating = false;
    }
}

/* =========================== ADC 按键检测 =========================== */

static int adc_key_value_read(void)
{
    int fd = open(ADC_RAW_PATH, O_RDONLY);
    if (fd < 0)
        return -1;
    char buf[32] = {0};
    int n = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (n <= 0)
        return -1;
    buf[n] = '\0';
    return atoi(buf);
}

static unsigned long long diff_time_ms(unsigned long long *last_ms, unsigned long long interval)
{
    unsigned long long now;
    Amy_timestamp(now);
    unsigned long long diff = now - *last_ms;
    if (interval < diff)
        *last_ms = now;
    return diff;
}

static void adc_key_detect(void)
{
    static int prev_value = 2999;
    static int flag = -1;
    static unsigned long long last_ms = 0;
    int value = adc_key_value_read();
    if (value < 0)
        return;

    if (value < ADC_KEY_THRESHOLD && prev_value < ADC_KEY_THRESHOLD)
    {
        if (flag == -1)
        {
            flag = 0;
            if (diff_time_ms(&last_ms, ADC_DEBOUNCE_MS) > ADC_DEBOUNCE_MS)
                key_event_push(KEY_TYPE_ADC, ADC_KEY_VALUE, 0);
        }
    }
    else if (abs(prev_value - value) < 100)
    {
        flag = -1;
    }
    prev_value = value;
}

/* =========================== 检测任务 =========================== */

static void *key_detect_task(void *arg)
{
    (void)arg;
    while (1)
    {
        mechanical_key_detect();
        adc_key_detect();
        Amy_msleep(KEY_DETECT_INTERVAL_MS);
    }
    return NULL;
}

void key_module_init(void)
{
    Amy_gpio_init(MECHANICAL_KEY_PIN, GPIO_DIR_IN);

    pthread_t tid;
    Amy_pthread_create(&tid, key_detect_task, NULL, 32 * 1024, 0);
    Amy_pthread_detach(tid);
}
