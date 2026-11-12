/* 
 *  @file product_test_reboot.h
 *  @brief product_test_reboot module is supply reboot test interface
 *  @version 0.1
 *  @date 2021-05-07
 *
 *  @copyright Copyright (c) tuya.inc 2021
 *  
 */
#ifndef __PRODUCT_TEST_REBOOT_H__
#define __PRODUCT_TEST_REBOOT_H__

#ifdef __cplusplus 
extern "C" {
#endif

/*@brief reboot system
 *@param(in) _sync:whether sync read/write ,or not;1,sync;0,unsync
 *@return 0:succeed,-1:fail
 */
extern int product_test_reboot(int _sync);

#ifdef __cplusplus
}
#endif

#endif
