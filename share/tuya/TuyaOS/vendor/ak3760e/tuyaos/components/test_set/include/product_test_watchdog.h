/* 
 *  @file product_test_watchdog.h
 *  @brief product_test_watchdog module is supply watchdog test interface
 *  @version 0.1
 *  @date 2021-05-07
 *
 *  @copyright Copyright (c) tuya.inc 2021
 *  
 */

#ifndef __PRODUCT_TEST_WATCHDOG_H__
#define __PRODUCT_TEST_WATCHDOG_H__

#ifdef __cplusplus
extern "C" { 
#endif

/*@brief set watchdog  device name
 *@param(in) name:watchdog device name
 *@return 0:succeed,-1:fail
 */
extern int product_test_watchdog_set_devname(char *name);

/*@brief get watchdog  device name
 *@param(out) name:watchdog device name
 *@return 0:succeed,-1:fail
 */
extern int product_test_watchdog_get_devname(char *name);

/*@brief  watchdog test 
 *@param(in) seconds:wathdog feed dog seconds
 *@return 0:succeed,-1:fail
 */
extern int product_test_watchdog_test(int seconds); 

/*@brief  watchdog test 
 *@param(in) times:wathdog don't feed  dog seconds 
 *@return 0:succeed,-1:fail
 */
extern int product_test_watchdog_timeout(int seconds);


/*@brief  watchdog init 
 *@param(in) ms:wathdog timeout mircoseconds 
 *@return :actual timeout mircoseconds,0,fail
 */
extern int product_test_watchdog_init(int ms);

/*@brief  watchdog uinit
 *@return 0:succeed,-1:fail
 */

extern int product_test_watchdog_uinit(void);

/*@brief  watchdog keep alive 
 *@return 0:succeed,-1:fail
 */
extern int product_test_watchdog_feed(void);

#ifdef __cplusplus
}
#endif

#endif

