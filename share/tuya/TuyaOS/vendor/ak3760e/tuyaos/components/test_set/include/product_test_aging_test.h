/*@file product_test_aging_test.h
 *@brief product_test_aging_test is supply aging test interface
 *@version 0.1
 *@date 2021-5-07
 *
 *@copyright Copyright (c) tuya.inc 2021
 *
 */
#ifndef __PRODUCT_TEST_AGING_TEST_H__
#define __PRODUCT_TEST_AGING_TEST_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct  aging_test_parm_s
{
    int camera_test;   /*enable camera aging test*/
    int ircut_test;    /*enable ircut  aging test*/
    int irled_test;    /*enable irled  aging test*/
    int led_group_test;/*enable led group aging test*/
    int audio_test;    /*enable audio aging test*/
    int white_led_test;/*enable white led aging test*/
    int motor_test;    /*enable motor aging test*/
    int sleep;         /*test interval*/
}AGING_TEST_PARM_T;

/*@brief aging test items configure
 *@param(in) param: configure parameter,see AGING_TEST_PARM_T
 *@return 0:succeed,-1 fail
 */
extern int product_test_aging_test_parm_configure(AGING_TEST_PARM_T*param);

/*@brief aging test enter
 *@param(in) time: agint test times (hours)
 *@return 0:succeed,-1 fail
 */
extern int product_test_aging_test(int time);

#ifdef __cplusplus
}
#endif

#endif

