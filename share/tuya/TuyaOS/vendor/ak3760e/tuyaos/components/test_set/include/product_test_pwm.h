/* 
 *  @file product_test_pwm.h
 *  @brief product_test_pwm module is supply pwm test interface
 *  @version 0.1
 *  @date 2021-05-07
 *
 *  @copyright Copyright (c) tuya.inc 2021
 *  
 */

#ifndef __PRODUCT_TEST_PWM_H__
#define __PRODUCT_TEST_PWM_H__

#include "product_test_led.h" 

#ifdef __cplusplus
extern "C" {
#endif


/*@brief pwm init
 *@param(in) chip:pwm chip number
 *@param(in)channel:pwm chip channel
 *@return 0:succeed,-1:fail
 */
extern int product_test_pwm_init(int chip,int channel); 

/*@brief pwm set duty cycle,0~100,
 *@param(in) chip:pwm chip number
 *@param(in)channel:pwm chip channel
 *@return 0:succeed,-1:fail
 */
extern int product_test_pwm_set_duty_cycle(int chip,int channel,int value); 


/*@brief pwm set peirid (ns)
 *@param(in) chip:pwm chip number
 *@param(in)channel:pwm chip channel
 *@return 0:succeed,-1:fail
 */
extern int product_test_pwm_set_period(int chip,int channel,int value); 

/*@brief pwm led set on
 *@param(in) type:led type,see LED_TYPE_E
 *@return 0:succeed,-1:fail
 */
extern int product_test_pwm_led_on(LED_TYPE_E type); 

/*@brief pwm led set off
 *@param(in) type:led type,see LED_TYPE_E
 *@return 0:succeed,-1:fail
 */
extern int product_test_pwm_led_off(LED_TYPE_E type); 

/*@brief pwm irled set on
 *@param(in) type:led type,see IRLED_TYPE_E
 *@return 0:succeed,-1:fail
 */
extern int product_test_pwm_irled_on(IRLED_TYPE_E type); 

/*@brief pwm irled set off
 *@param(in) type:led type,see IRLED_TYPE_E
 *@return 0:succeed,-1:fail
 */
extern int product_test_pwm_irled_off(IRLED_TYPE_E type); 

#ifdef __cplusplus
}
#endif

#endif
