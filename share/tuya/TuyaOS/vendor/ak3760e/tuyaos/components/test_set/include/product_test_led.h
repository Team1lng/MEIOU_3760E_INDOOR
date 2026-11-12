/* 
 *  @file product_test_led.h
 *  @brief product_test_led module is supply led test interface
 *  @version 0.1
 *  @date 2021-05-07
 *
 *  @copyright Copyright (c) tuya.inc 2021
 *  
 */

#ifndef __PRODUCT_TEST_LED_H__
#define __PRODUCT_TEST_LED_H__
#include "ptal_led.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    E_LED_INFACE_PWM = 1, /*led interface in pwm mode*/
    E_LED_INFACE_GPIO,    /*led interface in gpio mode*/
}LED_INFACE_TYPE_E;

typedef enum {
    E_IRLED_INFACE_PWM = 1, /*irled interface in pwm mode */
    E_IRLED_INFACE_GPIO,    /*irled interface in gpio mode */
}IRLED_INFACE_TYPE_E;


/*@brief set led physical gpio number
 *@param(in) type:led type,see LED_TYPE_E 
 *@param(in) gpio number
 *@return 0:succeed,-1:fail
 */
extern int product_test_led_set_gpio_num(LED_TYPE_E type,int gpio_num);

/*@brief get led physical gpio number
 *@param(in) type:led type,see LED_TYPE_E 
 *@param(out) gpio number
 *@return 0:succeed,-1:fail
 */
extern int product_test_led_get_gpio_num(LED_TYPE_E type,int*gpio_num); 

/*@brief set led physical gpio valid level 
 *@param(in) type:led type,see LED_TYPE_E 
 *@param(in) level: 1,high;0,low
 *@return 0:succeed,-1:fail
 */
extern int product_test_led_set_gpio_level(LED_TYPE_E type,int level);

/*@brief set led physical gpio valid level
 *@param(in) type:led type,see LED_TYPE_E 
 *@param(out) level: 1,high;0,low
 *@return 0:succeed,-1:fail
 */
extern int product_test_led_get_gpio_level(LED_TYPE_E type,int*level); 

/*@brief set led in test group 
 *@param(in) type:led type,see LED_TYPE_E 
 *@return 0:succeed,-1:fail
 */
extern int product_test_led_set_in_group(LED_TYPE_E type);

/*@brief set led in test group 
 *@param(out) type:led type,see LED_TYPE_E 
 *@return 0:succeed,-1:fail
 */
extern int product_test_led_is_in_group(LED_TYPE_E type); 

/*@brief light one led on 
 *@param(in) type:led type,see LED_TYPE_E 
 *@return 0:succeed,-1:fail
 */
extern int product_test_one_led_on(LED_TYPE_E type); 

/*@brief light one led off 
 *@param(in) type:led type,see LED_TYPE_E 
 *@return 0:succeed,-1:fail
 */
extern int product_test_one_led_off(LED_TYPE_E type);

/*@brief light one led flash 
 *@param(in) type:led type,see LED_TYPE_E 
 *@return 0:succeed,-1:fail
 */
extern int product_test_one_led_flash(LED_TYPE_E type);

/*@brief light all led on in group
 *@param(in) void
 *@return 0:succeed,-1:fail
 */
extern int product_test_led_on(void);

/*@brief light all led off in group
 *@param(in) void
 *@return 0:succeed,-1:fail
 */
extern int product_test_led_off(void);

/*@brief light all led flash in group
 *@param(in) void
 *@return 0:succeed,-1:fail
 */
extern int product_test_led_flash(void);

/*@brief set irled physical gpio number
 *@param(in) type:irled type,see IRLED_TYPE_E 
 *@param(in) gpio number
 *@return 0:succeed,-1:fail
 */
extern int product_test_irled_set_gpio_num(IRLED_TYPE_E type,int num);

/*@brief get irled physical gpio number
 *@param(in) type:irled type,see IRLED_TYPE_E 
 *@param(out) gpio number
 *@return 0:succeed,-1:fail
 */
extern int product_test_irled_get_gpio_num(IRLED_TYPE_E type,int *num);

/*@brief set irled physical gpio level 
 *@param(in) type:irled type,see IRLED_TYPE_E 
 *@param(in) gpio number
 *@return 0:succeed,-1:fail
 */
extern int product_test_irled_set_gpio_level(IRLED_TYPE_E type,int level);

/*@brief get irled physical gpio level 
 *@param(in) type:irled type,see IRLED_TYPE_E 
 *@param(out) gpio number
 *@return 0:succeed,-1:fail
 */
extern int product_test_irled_get_gpio_level(IRLED_TYPE_E type,int* level);

/*@brief set irled in test group 
 *@param(in) type:irled type,see IRLED_TYPE_E 
 *@return 0:succeed,-1:fail
 */
extern int product_test_irled_set_in_group(IRLED_TYPE_E type);

/*@brief get irled in test group 
 *@param(out) type:irled type,see IRLED_TYPE_E 
 *@return 0:succeed,-1:fail
 */
extern int product_test_irled_is_in_group(IRLED_TYPE_E type);

/*@brief light one irled on 
 *@param(in) type:irled type,see IRLED_TYPE_E 
 *@return 0:succeed,-1:fail
 */
extern int product_test_one_irled_on(IRLED_TYPE_E type);

/*@brief light one irled off 
 *@param(in) type:irled type,see IRLED_TYPE_E 
 *@return 0:succeed,-1:fail
 */
extern int product_test_one_irled_off(IRLED_TYPE_E type);

/*@brief light group irled on 
 *@param(in) type:irled type,see IRLED_TYPE_E 
 *@return 0:succeed,-1:fail
 */
extern int product_test_irled_on(void);

/*@brief light group irled off 
 *@param(in) type:irled type,see IRLED_TYPE_E 
 *@return 0:succeed,-1:fail
 */
extern int product_test_irled_off(void);

/*@brief set led interface type 
 *@param(in) type:led type,see LED_TYPE_E 
 *@param(in) inface_type:interface type,see LED_INFACE_TYPE_E
 *@return 0:succeed,-1:fail
 */
extern int product_test_led_set_interfece_type(LED_TYPE_E type,LED_INFACE_TYPE_E inface_type);

/*@brief get led interface type 
 *@param(in) type:led type,see LED_TYPE_E 
 *@param(out) inface_type:interface type,see LED_INFACE_TYPE_E
 *@return 0:succeed,-1:fail
 */
extern int product_test_led_get_interfece_type(LED_TYPE_E type,LED_INFACE_TYPE_E *inface_type);

/*@brief set irled interface type 
 *@param(in) type:led type,see IRLED_TYPE_E 
 *@param(in) inface_type:interface type,see IRLED_INFACE_TYPE_E
 *@return 0:succeed,-1:fail
 */
extern int product_test_irled_set_interfece_type(IRLED_TYPE_E type,IRLED_INFACE_TYPE_E inface_type);

/*@brief set irled interface type 
 *@param(in) type:led type,see IRLED_TYPE_E 
 *@param(out) inface_type:interface type,see IRLED_INFACE_TYPE_E
 *@return 0:succeed,-1:fail
 */
extern int product_test_irled_get_interfece_type(IRLED_TYPE_E type,IRLED_INFACE_TYPE_E *inface_type);

/*@brief light color test
 *@param(in) no
 *@return 0:succeed,-1:fail
 */
extern int product_test_auto_light_color(void);

/*@brief red led on
 *@return 0:succeed,-1:fail
 */
extern int product_test_auto_red_on(void);

/*@brief green led on
 *@return 0:succeed,-1:fail
 */
extern int product_test_auto_green_on(void);

#ifdef __cplusplus
}
#endif

#endif
