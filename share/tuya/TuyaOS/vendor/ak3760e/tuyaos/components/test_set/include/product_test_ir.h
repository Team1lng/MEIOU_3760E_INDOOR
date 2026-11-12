/* 
 *  @file product_test_adc.h
 *  @brief product_test_adc module is supply adc test interface
 *  @version 0.1
 *  @date 2021-05-07
 *  
 *  @copyright Copyright (c) tuya.inc 2021
 *   
 */

#ifndef __PRODUCT_TEST_IR_H__
#define __PRODUCT_TEST_IR_H__

#ifdef __cplusplus
extern "C" {
#endif

/*@brief ir send datas
 *@param(in) buf: send data buffer
 *@param(in) len: send data len 
 *@return 0:succeed,-1:fail
 */
int product_test_ir_send(char *buf,int len) ;

/*@brief ir receive datas
 *@param(out) buf: receive data buffer
 *@param(out) len: receive data len 
 *@return 0:succeed,-1:fail
 */
int product_test_ir_receive(char *buf, int *len);

#ifdef __cplusplus
}
#endif

#endif
