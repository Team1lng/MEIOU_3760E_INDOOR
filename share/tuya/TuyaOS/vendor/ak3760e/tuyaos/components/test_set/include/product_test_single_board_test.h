/*@file product_test_single_board_test.h
 *@brief product_test_single_board_test is supply single board test interface
 *@version 0.1
 *@date 2021-5-07
 *
 *@copyright Copyright (c) tuya.inc 2021
 *
 */

#ifndef __PRODUCT_TEST_SINGLE_BOARD_H__
#define __PRODUCT_TEST_SINGLE_BOARD_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct  sing_board_test_parm_s
{
    int camera_test;   /*enable camera aging test*/
    int ircut_test;    /*enable ircut  aging test*/
    int irled_test;    /*enable irled  aging test*/
    int led_group_test;/*enable led group aging test*/
    int audio_test;    /*enable audio aging test*/
    int white_led_test;/*enable white led aging test*/
    int motor_test;    /*enable motor aging test*/
    int sleep;         /*test interval*/
}SINGLE_BOARD_TEST_PARM_T;

/*@brief single board test items configure
 *@param(in) param: configure parameter,see SINGLE_BOARD_TEST_PARM_T
 *@return 0:succeed,-1 fail
 */
extern int product_test_single_board_test_parm_configure(SINGLE_BOARD_TEST_PARM_T *parm);

/*@brief single board test enter
 *@return 0:succeed,-1 fail
 */
extern int product_test_single_board_test(void);

#ifdef __cplusplus
}
#endif

#endif

