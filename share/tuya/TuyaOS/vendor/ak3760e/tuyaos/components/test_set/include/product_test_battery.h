/* 
 *  @file product_test_battery.h
 *  @brief product_test_battery module is supply battery test interface
 *  @version 0.1
 *  @date 2021-05-07
 *
 *  @copyright Copyright (c) tuya.inc 2021
 *  
 */
#ifndef __PRODUCT_TEST_BATTERY_H__
#define __PRODUCT_TEST_BATTERY_H__

#ifdef __cplusplus
extern "C" {
#endif

/*@brief get curret battery capacity
 *@param(out)value: battery capacity 
 *@return 0:succeed,-1:fail
 */
extern int product_test_battery_get_current_capacity(uint32_t *value);

/*@brief set battery factory
 *@param(in)factory: battery factory
 *@return 0:succeed,-1:fail
 */
extern int product_test_battery_set_factory(char *factory);

/*@brief get battery factory
 *@param(out)factory: battery factory
 *@return 0:succeed,-1:fail
 */
extern int product_test_battery_get_factory(char *factory); 

/*@brief set battery type
 *@param(in)type: battery type
 *@return 0:succeed,-1:fail
 */
extern int product_test_battery_set_type(char *type); 

/*@brief  get battery type
 *@param(out)type: battery type
 *@return 0:succeed,-1:fail
 */
extern int product_test_battery_get_type(char *type); 

/*@brief set battery total capacity
 *@param(in)capacity: battery capacity 
 *@return 0:succeed,-1:fail
 */
extern int product_test_battery_set_capacity(int capacity);

/*@brief get battery total capacity 
 *@param(out)capacity: battery capacity 
 *@return 0:succeed,-1:fail
 */
extern int product_test_battery_get_capacity(int *capacity);

/*@brief set battery information 
 *@param(out)capacity: battery information
 *@return 0:succeed,-1:fail
 */
extern int product_test_battery_set_info(char *info); 

/*@brief set battery information 
 *@param(out)capacity: battery information
 *@return 0:succeed,-1:fail
 */
extern int product_test_battery_get_info(char *info); 

#ifdef __cplusplus
}
#endif

#endif
