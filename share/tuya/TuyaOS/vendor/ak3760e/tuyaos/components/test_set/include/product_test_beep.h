/* 
 *  @file product_test_beep.h
 *  @brief product_test_beep module is supply beep test interface
 *  @version 0.1
 *  @date 2021-05-07
 *
 *  @copyright Copyright (c) tuya.inc 2021
 *  
 */
#ifndef __PRODUCT_TEST_BEEP_H__
#define __PRODUCT_TEST_BEEP_H__

#ifdef __cplusplus
extern "C" {
#endif

/*@brief set beep gpio num 
 *@param(in) gpio_num:gpio number,general gpio_num = 32*(gpio group)+(gpio index)
 *@return 0:succeed,-1:fail
 */
extern int product_test_beep_set_gpio_num(int gpio_num);

/*@brief get beep gpio num 
 *@param(ou) gpio_num:gpio number,general gpio_num = 32*(gpio group)+(gpio index)
 *@return 0:succeed,-1:fail
 */
extern int product_test_beep_get_gpio_num(int*gpio_num); 

/*@brief set beep gpio valid level 1:high;0,low 
 *@param(out) gpio_num:gpio number,general gpio_num = 32*(gpio group)+(gpio index)
 *@return 0:succeed,-1:fail
 */
extern int product_test_beep_set_gpio_level(int level);

/*@brief get beep gpio valid level 1:high;0,low 
 *@param(in) gpio_num:gpio number,general gpio_num = 32*(gpio group)+(gpio index)
 *@return 0:succeed,-1:fail
 */
extern int product_test_beep_get_gpio_level(int*level); 

/*@brief beep test start
 *@param(in) hz:beep frequency
 *@param(in) times:beep test seconds 
 *@return 0:succeed,-1:fail
 */
extern int product_test_beep_start(int hz,int times_s); 

/*@brief beep test stop
 *@param(in) void 
 *@return 0:succeed,-1:fail
 */
extern int product_test_beep_stop(void); 

#ifdef __cplusplus
}
#endif

#endif
