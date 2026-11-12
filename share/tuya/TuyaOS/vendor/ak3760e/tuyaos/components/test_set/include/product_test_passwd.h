/* 
 *  @file product_test_passwd.h
 *  @brief product_test_passwd module is supply dynamc passwd set interface
 *  according to uuid and authkey
 *  @version 0.1
 *  @date 2021-05-07
 *
 *  @copyright Copyright (c) tuya.inc 2021
 *  
 */

#ifndef __PRODUCT_TEST_PASSWD_H__
#define __PRODUCT_TEST_PASSED_H__

#ifdef __cplusplus
extern "C" {
#endif

/*@brief set passwd path 
 *@param(in) path:set passwd path,default /etc/shadow
 *@return 0:succeed,-1:fail
 */
extern int product_test_set_passwd_path(char *path); 

/*@brief get passwd path 
 *@param(out) path:set passwd path,default /etc/shadow
 *@return 0:succeed,-1:fail
 */
extern int product_test_get_passwd_path(char *path);

/*@brief set new passwd according to uuid,authkey
 *@param(in) uuid:device's uuid
 *@param(in) authkey:device's authkey
 *@return 0:succeed,-1:fail
 */
extern int product_test_passwd_change(char *uuid,char *authkey);

/*@brief reset to original passwd 
 *@param(in)void
 *@return 0:succeed,-1:fail
 */
extern int product_test_passwd_reset(void);

#ifdef __cplusplus
}
#endif

#endif
