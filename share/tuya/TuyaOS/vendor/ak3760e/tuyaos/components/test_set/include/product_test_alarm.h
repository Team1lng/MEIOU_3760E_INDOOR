/* 
 *  @file product_test_alarm.h
 *  @brief product_test_alarm module is supply alarm test interface
 *  @version 0.1
 *  @date 2021-05-07
 *
 *  @copyright Copyright (c) tuya.inc 2021
 *  
 */

#ifndef __PRODUCT_TEST_ALARM_H__
#define __PRODUCT_TEST_ALARM_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    E_ALARM_TOOL, /*to output high/low level to alarm in*/
    E_ALARM_IN,   /*alarm input */
    E_ALARM_OUT,  /*alarm output */
}ALARM_TYPE_E;

/*@brief set alarm tool  support flag
 *@param(in) flag:1,support
 *@return 0:succeed,-1:fail
 */
extern int product_test_set_alarm_tool_support(int flag);


/*@brief get alarm tool support flag
 *@param(out) *flag:1,support
 *@return 0:succeed,-1:fail
 */
extern int product_test_get_alarm_tool_support(int *flag);


/*@brief set gpio num corresponding type 
 *@param(in) type:alarm type see ALARM_TYPE_E
 *@param(in) gpio_num: gpip physic number
 *@return 0:succeed,-1:fail
 */
extern int product_test_alarm_set_gpio_num(ALARM_TYPE_E type,int gpio_num);


/*@brief get gpio num corresponding type 
 *@param(in) type:alarm type see ALARM_TYPE_E
 *@param(out) gpio_num: gpip physic number
 *@return 0:succeed,-1:fail
 */
extern int product_test_alarm_get_gpio_num(ALARM_TYPE_E type,int*gpio_num);


/*@brief set gpio level corresponding type 
 *@param(in) type:alarm type see ALARM_TYPE_E
 *@param(in) level:1,high;0,low
 *@return 0:succeed,-1:fail
 */
extern int product_test_alarm_set_gpio_level(ALARM_TYPE_E type,int level);


/*@brief get gpio level corresponding type 
 *@param(in) type:alarm type see ALARM_TYPE_E
 *@param(out) level:1,high;0,low
 *@return 0:succeed,-1:fail
 */
extern int product_test_alarm_get_gpio_level(ALARM_TYPE_E type,int*level);


/*@brief alarm out on 
 *@param(in) void
 *@return 0:succeed,-1:fail
 */
extern int product_test_alarm_out_on(void);


/*@brief alarm out off
 *@param(in) void
 *@return 0:succeed,-1:fail
 */
extern int product_test_alarm_out_off(void);


/*@brief alarm out flash 
 *@param(in) void
 *@return 0:succeed,-1:fail
 */
extern int product_test_alarm_out_flash(void);


/*@brief alarm in test
 *@param(in) void
 *@return 0:succeed,-1:fail
 */
extern int product_test_alarm_in_test(void);

#ifdef __cplusplus
}
#endif

#endif

