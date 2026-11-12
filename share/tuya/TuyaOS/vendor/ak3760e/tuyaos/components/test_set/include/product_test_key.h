/* 
 *  @file product_test_key.h
 *  @brief product_test_key module is supply key test interface
 *  @version 0.1
 *  @date 2021-05-07
 *
 *  @copyright Copyright (c) tuya.inc 2021
 *  
 */

#ifndef __PRODUCT_TEST_KEY_H__
#define __PROUDCT_TEST_KEY_H__

#include "ptal_key.h" 

#ifdef __cplusplus
extern "C" {
#endif


/*@brief set main cpu key numbers
 *@param(in) num:main cpu key numbers <= MAX_KEY_NUM
 *@return 0:succeed,-1:fail
 */
extern int product_test_key_set_num(int num);

/*@brief get main cpu key numbers
 *@param(out) num:main cpu key numbers <= MAX_KEY_NUM
 *@return 0:succeed,-1:fail
 */
extern int product_test_key_get_num(int *num);

/*@brief set key physic nums 
 *@param(in) key_num:key index 
 *@param(in) gpio: key physic number
 *@return 0:succeed,-1:fail
 */
extern int product_test_key_set_gpio(int key_num,int gpio);

/*@brief get key physic nums 
 *@param(in) key_num:key index 
 *@param(out) gpio: key physic number
 *@return 0:succeed,-1:fail
 */
extern int product_test_key_get_gpio(int key_num,int *gpio);

/*@brief  main cpu key get  
 *@param(in)  n:key index 
 *@param(out) flag:key detect flag,1:detect 0:undetect
 *@return 0:succeed,-1:fail
 */
extern int product_test_main_cpu_key_get(int n,int *flag);


/*@brief board key get main cpu key +  sub cpu key 
 *@param(in)  n:key index 
 *@param(out) flag:key detect flag,1:detect 0:undetect
 *@return 0:succeed,-1:fail
 */
extern int product_test_key_get(int n,int *flag);

/*@brief set sub cpu key numbers
 *@param(in) num:main cpu key numbers <= MAX_SUB_MCU_KEY_NUM
 *@return 0:succeed,-1:fail
 */
extern int product_test_sub_mcu_set_key_num(int num);

/*@brief get sub cpu key numbers
 *@param(out) num:main cpu key numbers <= MAX_SUB_MCU_KEY_NUM
 *@return 0:succeed,-1:fail
 */
extern int product_test_sub_mcu_get_key_num(int*num);

/*@brief  sub mcu key get  
 *@param(in)  n:key index 
 *@param(out) flag:key detect flag,1:detect 0:undetect
 *@return 0:succeed,-1:fail
 */
extern int product_test_sub_mcu_key_get(int n,int *flag); 


/*@brief   loop wait key input 
 *@return 0:succeed,-1:fail
 */
extern int product_test_key_loop_wait(void);

#ifdef __cplusplus
}
#endif

#endif
