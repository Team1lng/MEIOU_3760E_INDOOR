/* 
 *  @file product_test_ircut.h
 *  @brief product_test_ircut module is supply ircut test interface
 *  @version 0.1
 *  @date 2021-05-07
 *
 *  @copyright Copyright (c) tuya.inc 2021
 *  
 */

#ifndef __PRODUCT_TEST_IRCUT_H__
#define __PRODUCT_TEST_IRCUT_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    E_IRCUT_PIN1,
    E_IRCUT_PIN2,
    E_IRCUT_MAX,
}IRCUT_PIN_E;

/*@brief set ircut pin gpio num
 *@param(in) pin:ircut pin enum,see IRCUT_PIN_E
 *@param(in) num:ircut pin,physic gpio num
 *@return 0:succeed,-1:fail
 */
extern int product_test_ircut_set_gpio_num(IRCUT_PIN_E pin,int num);

/*@brief get ircut pin gpio num
 *@param(in) pin:ircut pin enum,see IRCUT_PIN_E
 *@param(out) num:ircut pin,physic gpio num
 *@return 0:succeed, -1:fail
 */
extern int product_test_ircut_get_gpio_num(IRCUT_PIN_E pin,int* num);

/*@brief set ircut pin valid level
 *@param(in) pin:ircut pin enum,see IRCUT_PIN_E
 *@param(in) level:ircut pin,level,1:high,0:low
 *@return 0:succeed, -1:fail
 */
extern int product_test_ircut_set_gpio_level(IRCUT_PIN_E pin,int level); 

/*@brief get ircut pin valid level
 *@param(in) pin:ircut pin enum,see IRCUT_PIN_E
 *@param(out) level:ircut pin,level,1:high,0:low
 *@return 0:succeed, -1:fail
 */
extern int product_test_ircut_get_gpio_level(IRCUT_PIN_E pin,int*level);

/*@brief ircut test
 *@param(in) num:ircut switch counts
 *@return 0:succeed, -1:fail
 */
extern int product_test_ircut_test(int num);


/*@brief ircut switch to day mode
 *@param(in) void
 *@return 0:succeed, -1:fail
 */
extern int product_test_ircut_day(void);


/*@brief ircut switch to nitht mode
 *@param(in) void
 *@return 0:succeed, -1:fail
 */
extern int product_test_ircut_night(void);

/*@brief ircut power off 
 *@param(in) void
 *@return 0:succeed, -1:fail
 */
extern int product_test_ircut_off(void);

#ifdef __cplusplus
}
#endif

#endif
