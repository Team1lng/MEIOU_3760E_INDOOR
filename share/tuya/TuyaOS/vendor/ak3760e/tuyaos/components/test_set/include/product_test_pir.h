/* 
 *  @file product_test_pir.h
 *  @brief product_test_pir module is supply pir test interface
 *  @version 0.1
 *  @date 2021-05-07
 *
 *  @copyright Copyright (c) tuya.inc 2021
 *  
 */

#ifndef __PRODUCT_TEST_PIR_H__
#define __PRODUCT_TEST_PIR_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>

/*@brief get pir awake times
 *@param(out)times:awake times
 *@return 0:succeed,-1:fail
 */
extern int product_test_pir_get_awake_times(uint32_t *times);

/*@brief pir test
 *@param(in)counts:test pir times
 *@return 0:succeed,-1:fail
 */
extern int product_test_pir_test(int counts);

#ifdef __cplusplus
}
#endif

#endif

