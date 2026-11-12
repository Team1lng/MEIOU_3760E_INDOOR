/* 
 *  @copyright Copyright (c) tuya.inc 2021
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include "tb_log.h"
#include "product_test_utils.h"
#include "product_test_led.h"
#include "product_test_pwm.h"
#include "ptal_pwm.h"

int product_test_pwm_init(int chip,int channel) 
{
    return ptal_pwm_init(chip,channel);
}

int product_test_pwm_set_duty_cycle(int chip,int channel,int value) 
{
    return ptal_set_duty_cycle(chip,channel,value); 
}

int product_test_pwm_set_period(int chip,int channel,int value) 
{
    return ptal_pwm_set_period(chip,channel,value);
}

int product_test_pwm_led_on(LED_TYPE_E type) 
{
    return ptal_pwm_led_on(type);
}

int product_test_pwm_led_off(LED_TYPE_E type) 
{
    return ptal_pwm_led_off(type);
}

int product_test_pwm_irled_on(IRLED_TYPE_E type)
{
    return ptal_pwm_irled_on(type);
}

int product_test_pwm_irled_off(IRLED_TYPE_E type)
{
    return ptal_pwm_irled_off(type);
}

