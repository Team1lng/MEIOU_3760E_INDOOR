/* 
 *  @file product_test_subg.h
 *  @brief product_test_sub module is supply subg test interface
 *  @version 0.1
 *  @date 2021-05-07
 *
 *  @copyright Copyright (c) tuya.inc 2021
 *  
 */

#ifndef __PRODUCT_TEST_SUBG_H__
#define __PRODUCT_TEST_SUBG_H__

#ifdef __cplusplus
extern "C" {
#endif

/*@brief set subg device name 
 *@param(in) name:device name
 *@return 0:succeed,-1:fail
 */
extern int product_test_subg_set_devname(char *name);

/*@brief get subg device name 
 *@param(in) name:device name
 *@return 0:succeed,-1:fail
 */
extern int product_test_subg_get_devname(char *name);

/*@brief subg test 
 *@param(out) num:test succeed times or packets
 *@return 0:succeed,-1:fail
 */
extern int product_test_subg_test(int *num);

/*@brief subg receive packet test
 *@param(in)timeout:seconds wait for receive subg packets
 *@return 0:succeed,-1:fail
 */
extern int product_test_subg_receive_test(int timeout);

#ifdef __cplusplus
}
#endif

#endif
