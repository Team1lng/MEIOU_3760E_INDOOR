/* 
 *  @file product_test_bt.h
 *  @brief product_test_bt module is supply bluetooth test interface
 *  @version 0.1
 *  @date 2021-05-07
 *
 *  @copyright Copyright (c) tuya.inc 2021
 *  
 */
#ifndef __PRODUCT_TEST_BT_H__
#define __PRODUCT_TEST_BT_H__

#ifdef __cplusplus
extern "C" {
#endif

/*
 * @brief get bluetooth rssi (signal strength)
 *@param(in) name: bluetooth name 
 *@param(out) rssi: bluetooth signal strength
 *@return 0:succeed ,-1:fail
 */
extern int product_test_bt_get_rssi(char *name,  int *rssi);


#ifdef __cplusplus
}
#endif
#endif
