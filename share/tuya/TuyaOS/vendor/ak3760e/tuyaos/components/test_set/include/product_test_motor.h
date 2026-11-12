/* 
 *  @file product_test_motor.h
 *  @brief product_test_motor module is supply motor test interface
 *  @version 0.1
 *  @date 2021-05-07
 *
 *  @copyright Copyright (c) tuya.inc 2021
 *  
 */
#ifndef __PRODUCT_TEST_MOTOR_H__
#define __PRODUCT_TEST_MOTOR_H__

#ifdef __cpulsplus
extern "C" {
#endif

/*@move motor up
 *@param(in) speed: motor move speed
 *@param(in) steps: motor move steps
 *@param(in) sync: 1,sync mode,0 async mode  
 *@return 0:succeed,-1:fail
 */
extern int product_test_motor_move_up(int speed,int steps,int sync);

/*@move motor down 
 *@param(in) speed: motor move speed
 *@param(in) steps: motor move steps
 *@param(in) sync: 1,sync mode,0 async mode  
 *@return 0:succeed,-1:fail
 */
extern int product_test_motor_move_down(int speed,int steps,int sync);

/*@move motor left 
 *@param(in) speed: motor move speed
 *@param(in) steps: motor move steps
 *@param(in) sync: 1,sync mode,0 async mode  
 *@return 0:succeed,-1:fail
 */
extern int product_test_motor_move_left(int speed,int steps,int sync);

/*@move motor right 
 *@param(in) speed: motor move speed
 *@param(in) steps: motor move steps
 *@param(in) sync: 1,sync mode,0 async mode  
 *@return 0:succeed,-1:fail
 */
extern int product_test_motor_move_right(int speed,int steps,int sync);

/*@motor set motor device name  
 *@param(in) name: motor device name 
 *@return 0:succeed,-1:fail
 */
extern int product_test_motor_set_devname(char *name);

/*@motor get motor device name  
 *@param(out) name: motor device name 
 *@return 0:succeed,-1:fail
 */
extern int product_test_motor_get_devname(char *name);

/*@motor  self check 
 *@return 0:succeed,-1:fail
 */
extern int product_test_motor_self_check(void);

/*@motor set limitswitch support  
 *@param(in) flag:1,support;0,unsupport  
 *@return 0:succeed,-1:fail
 */
extern int product_test_motor_set_limitswitch_support(int flag);

/*@motor get limitswitch support  
 *@param(out) flag:1,support;0,unsupport  
 *@return 0:succeed,-1:fail
 */
extern int product_test_motor_get_limitswitch_support(int* flag); 

/*@motor get left limitswitch touch flag  
 *@param(out) flag:1, limitswitch touched valid;0, limitswitch touched invalid
 *@return 0:succeed,-1:fail
 */
extern int product_test_get_left_limitswitch_flag(int *flag);

/*@motor get right limitswitch touch flag  
 *@param(out) flag:1, limitswitch touched valid;0, limitswitch touched invalid
 *@return 0:succeed,-1:fail
 */
extern int product_test_get_right_limitswitch_flag(int *flag); 

/*@set pan motor max steps 
 *@param(in) max_steps: max motor steps
 *@return 0:succeed,-1:fail
 */
extern int product_test_set_pan_max_step(unsigned int max_steps); 

/*@set tilt motor max steps 
 *@param(in) max_steps: max motor steps
 *@return 0:succeed,-1:fail
 */
extern int product_test_set_tilt_max_step(int max_steps);

/*@set pan motor speed 
 *@param(in) speed: pan speed 
 *@return 0:succeed,-1:fail
 */
extern int product_test_set_pan_speed(int speed);

/*@set tilt motor speed 
 *@param(in) speed: pan speed 
 *@return 0:succeed,-1:fail
 */
extern int product_test_set_tilt_speed(int speed);


/*@get motor current location 
 *@param(out) pan_cur: current pan location
 *@param(out) tilt_cur: current tilt location
 *@return 0:succeed,-1:fail
 */
extern int product_test_motor_get_cur_pos(int* pan_cur, int *tilt_cur);

#ifdef __cplusplus
}
#endif

#endif
