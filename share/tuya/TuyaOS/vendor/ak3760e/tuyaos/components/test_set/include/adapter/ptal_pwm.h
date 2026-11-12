/* 
 *  @file ptal_pwm.h
 *  @brief ptal_pwm module is supply pwm product test abstract layer interface
 *  @version 0.1
 *  @date 2021-05-07
 *
 *  @copyright Copyright (c) tuya.inc 2021
 *  
 */
#ifndef __PTAL_PWM_H__
#define __PTAL_PWM_H__

#include "ptal_led.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_PWM_CHIP 4 
#define MAX_PWM_CHAN 4

#define  MAX_PWMS (MAX_PWM_CHIP*MAX_PWM_CHAN)

typedef struct pwm_channel_param_s {
    int type;    //pwm  device type
    int chip;    //pwm chip index
    int channel; //pwm channel index
    int period;  //pwm period 
    int min_value;//pwm period  min duty
    int max_value;// pwm period max duty
}PWM_CHANNEL_PARAM_T;

typedef struct pwm_param_s
{
    int total;
    PWM_CHANNEL_PARAM_T pwm[MAX_PWMS];
}PWM_PARAM_T;


extern int ptal_pwm_init(int chip,int channel);
extern int ptal_set_duty_cycle(int chip,int channel,int value);
extern int ptal_pwm_set_period(int chip,int channel,int value); 

/*@brief light on one led in pwm mode 
 *@param(in) type: led type see LED_TYPE_E
 *@return  0:succeed,-1:fail
 */
extern int ptal_pwm_led_on(LED_TYPE_E type);

/*@brief light of one led in pwm mode 
 *@param(in) type: led type see LED_TYPE_E
 *@return  0:succeed,-1:fail
 */
extern int ptal_pwm_led_off(LED_TYPE_E type);

/*@brief light on one irled in pwm mode 
 *@param(in) type: led type see IRLED_TYPE_E
 *@return  0:succeed,-1:fail
 */
extern int ptal_pwm_irled_on(IRLED_TYPE_E type);

/*@brief light off one irled in pwm mode 
 *@param(in) type: led type see IRLED_TYPE_E
 *@return  0:succeed,-1:fail
 */
extern int ptal_pwm_irled_off(IRLED_TYPE_E type); 

#ifdef __cplusplus
}
#endif

#endif
