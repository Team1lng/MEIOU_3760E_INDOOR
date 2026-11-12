/* 
 *  @file product_test_gpio.h
 *  @brief product_test_gpio module is supply gpio test interface
 *  @version 0.1
 *  @date 2021-05-07
 *
 *  @copyright Copyright (c) tuya.inc 2021
 *  
 */
#ifndef __PRODUCT_TEST_GPIO_H__
#define __PRODUCT_TEST_GPIO_H__

#include "ptal_gpio.h"

#ifdef __cplusplus
extern "C" {
#endif


/*@brief gpio test 
 *@param(in) gpio test group
 *@return 0:succeed,-1:fail
 */
extern int product_test_gpio(GPIO_GROUP_T *group); 

/*@brief gpio init 
 *@param(in) num: gpio physic num 
 *@param(in) direction: 0,out ,1 in  
 *@param(in) init:1:high,0:low
 *@return 0:succeed,-1:fail
 */
extern int product_test_gpio_init(int num,int direction,int init);

/*@brief set gpio num level
 *@param(in) num: gpio physic num 
 *@param(in) value: 1,high level ,0 low level
 *@return 0:succeed,-1:fail
 */
extern int product_test_gpio_set_value(int num, int value);

/*@brief get gpio num level
 *@param(in) num: gpio physic num 
 *@param(in) value: 1,high level ,0 low level
 *@return 0:succeed,-1:fail
 */
extern int product_test_gpio_get_value(int num,int *value); 

/*@brief set  gpio direction and value 
 *@param(in) num: gpio physic num 
 *@param(in) inti: 0,high level 1, low level 0,
 *@return 0:succeed,-1:fail
 */
extern int product_test_gpio_set_direction(int num,int direction,int init);

#ifdef __cplusplus
}
#endif

#endif
