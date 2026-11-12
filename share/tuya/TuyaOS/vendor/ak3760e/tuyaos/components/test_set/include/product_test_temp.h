/* 
 *  @file product_test_temp.h
 *  @brief product_test_temp module is supply tempture test interface
 *  @version 0.1
 *  @date 2021-05-07
 *
 *  @copyright Copyright (c) tuya.inc 2021
 *  
 */
#ifndef _PRODUCT_TEST_TEMP_H__
#define _PRODUCT_TEST_TEMP_H__

#ifdef __cplusplus
extern "C" {
#endif

/*@brief get tempture sensor value
 *@param(in) temp:tempture sensor value
 *@return 0:succeed,-1:fail
 */
extern int product_test_temp_get_tempture(int *temp);

#ifdef __cplusplus
}
#endif
#endif
