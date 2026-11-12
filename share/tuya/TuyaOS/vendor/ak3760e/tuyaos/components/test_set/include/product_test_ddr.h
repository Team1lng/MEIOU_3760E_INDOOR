/* 
 *  @file product_test_ddr.h
 *  @brief product_test_ddr module is supply ddr test interface
 *  @version 0.1
 *  @date 2021-05-07
 *
 *  @copyright Copyright (c) tuya.inc 2021
 *  
 */
#ifndef __PRODUCT_TEST_DDR__
#define __PRODUCT_TEST_DDR__

#ifdef __cplusplus
extern "C" {
#endif

/*@brief ddr capacity test 
 *@param(in) capacit:ddr capacity
 *@return 0:succeed,-1:fail
 */
extern  int  product_test_ddr_capacity(int  ddr_capacity);

#ifdef __cplusplus
}
#endif

#endif
