/* 
 *  @file ptal_led.h
 *  @brief ptal_led module is supply product test abstract layer led interface
 *  @version 0.1
 *  @date 2021-05-07
 *
 *  @copyright Copyright (c) tuya.inc 2021
 *  
 */
#ifndef __PTAL_LED_H__
#define __PTAL_LED_H__

#ifdef __cplusplus__
extern "C" {
#endif

typedef enum {
    E_LED_RED = 0, /*red led */
    E_LED_GREEN,   /*green led */
    E_LED_WHITE,   /*white led */
    E_LED_BLUE,    /*blue led */
    E_LED_IDLE,    /*idle led */
    E_LED_MAX,     /*led max */
}LED_TYPE_E;

typedef enum {
    E_IRLED_RED=0,  /*red irled */
    E_IRLED_GREEN=1,/*green irled */
    E_IRLED_MAX,    /*irled max */
}IRLED_TYPE_E;

/*@brief light on one irled 
 *@param(in) type: irled type see IRLED_TYPE_E
 *@return  0:succeed,-1:fail
 */
extern int ptal_led_one_irled_on(IRLED_TYPE_E type);

/*@brief light off one irled 
 *@param(in) type: irled type see IRLED_TYPE_E
 *@return  0:succeed,-1:fail
 */
extern int ptal_led_one_irled_off(IRLED_TYPE_E type);

/*@brief light on one led
 *@param(in) type: led type see LED_TYPE_E
 *@return  0:succeed,-1:fail
 */
extern int ptal_led_one_led_off(LED_TYPE_E type);

/*@brief light off one led
 *@param(in) type: led type see LED_TYPE_E
 *@return  0:succeed,-1:fail
 */
extern int ptal_led_one_led_on(LED_TYPE_E type); 


#ifdef __cplusplus__
}
#endif

#endif
