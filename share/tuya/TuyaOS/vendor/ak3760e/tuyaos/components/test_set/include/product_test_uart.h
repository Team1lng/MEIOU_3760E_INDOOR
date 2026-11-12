/* 
 *  @file product_test_uart.h
 *  @brief product_test_uart module is supply pwm test interface
 *  @version 0.1
 *  @date 2021-05-07
 *
 *  @copyright Copyright (c) tuya.inc 2021
 *  
 */

#ifndef __PRODUCT_TEST_UART_H__
#define __PRODUCT_TEST_UART_H__

#ifdef __cplusplus
extern "C" {
#endif

/*@brief uart loop test
 *@return 0:succeed,-1:fail
 */

extern int product_test_uart_loop_test(void);

#ifdef __cplusplus
}
#endif

#endif
