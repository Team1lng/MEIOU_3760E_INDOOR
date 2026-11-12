/* 
 *  @file product_test_rf.h
 *  @brief product_test_rf module is supply rf test interface
 *  @version 0.1
 *  @date 2021-05-07
 *
 *  @copyright Copyright (c) tuya.inc 2021
 *  
 */
#ifndef __PRODUCT_TEST_RF_H__
#define __PRODUCT_TEST_RF_H__

#ifdef __cplusplus 
extern "C" {
#endif

/*@brief set wifi rf flag
 *@param(in)flag: rf flag ,1:rf tested 0:,untested
 *@return 0:succeed,-1:fail
 */
extern int product_test_wifi_set_rf_detect_flag(int flag); 

/*@brief get wifi rf flag
 *@param(out)flag: rf flag ,1:rf tested 0:,untested
 *@return 0:succeed,-1:fail
 */
extern int product_test_wifi_get_rf_detect_flag(int *flag); 

/*@brief set bt rf flag
 *@param(in)flag: rf flag ,1:rf tested 0:,untested
 *@return 0:succeed,-1:fail
 */
extern int product_test_bt_set_rf_detect_flag(int flag); 

/*@brief get  bt rf flag
 *@param(in)flag: rf flag ,1:rf tested 0:,untested
 *@return 0:succeed,-1:fail
 */
extern int product_test_bt_get_rf_detect_flag(int *flag); 

/*@brief check all rf status
 *@return 0:succeed,-1:fail
 */
extern int product_test_check_all_rf_status(void);

#ifdef __cplusplus
}
#endif

#endif
