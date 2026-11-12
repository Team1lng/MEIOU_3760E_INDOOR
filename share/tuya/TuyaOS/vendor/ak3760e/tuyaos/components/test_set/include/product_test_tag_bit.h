/* 
 *  @file product_test_tag.h
 *  @brief product_test_tag module is supply tag test interface
 *  @version 0.1
 *  @date 2021-05-07
 *
 *  @copyright Copyright (c) tuya.inc 2021
 *  
 */
#ifndef __PRODUCT_TEST_TAG_BIT_H__
#define __PRODUCT_TEST_TAG_BIT_H__

#ifdef __cplusplus
extern "C" {
#endif

/*@brief set tag bit
 *@param(out) val: tag bit value
 *@return 0:succeed,-1:fail
 */
extern int  product_test_write_tag_bit(int val);

/*@brief get tag bit
 *@param(out) val: tag bit value
 *@return 0:succeed,-1:fail
 */
extern int  product_test_read_tag_bit(int *val);


#ifdef __cplusplus
}
#endif

#endif
