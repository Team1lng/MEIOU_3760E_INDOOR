/* 
 *  @file product_test_adc.h
 *  @brief product_test_adc module is supply adc test interface
 *  @version 0.1
 *  @date 2021-05-07
 *
 *  @copyright Copyright (c) tuya.inc 2021
 *  
 */
#ifndef __PRODUCT_TEST_ADC_H__
#define __PRODUCT_TEST_ADC_H__

#ifdef __cpulsplus
extern "C" {
#endif

#include <inttypes.h>

/*@brief get adc value
 *@param(in) n:virtual channel number
 *@param(out)value: adc value
 *@return 0:succeed,-1:fail
 */
extern int product_test_adc_get_value(int n,uint32_t *value);

/*@brief set actual physical channel by virtual channel n
 *@param(in) n:virtual channel 
 *@param(in) channel: actual physical channel
 *@return 0:succuud,-1:fail
 */
extern int product_test_adc_set_channel(int n,int channel);


/*@brief get actual physical channel by virtual channel n
 *@param(in) n:virtual channel 
 *@param(out) channel: actual physical channel
 *@return 0:succuud,-1:fail
 */
extern int product_test_adc_get_chanel(int n,int* channel);

/*@brief set adc day night switch threashold 
 *@param(in) n:virtual channel 
 *@param(in) d2n:day to night threashold 
 *@param(in) n2d:night to day threashold 
 *@return 0:succuud,-1:fail
 */
extern int product_test_adc_set_threashold(int n,int d2n,int n2d);

/*@brief get adc day night switch threashold
 *@param(in) n:virtual channel
 *@param(out) d2n:day to night threashold
 *@param(out) n2d:night to day threashold
 *@return 0:succuud,-1:fail
 */
extern int product_test_adc_get_threshold(int n,int* d2n, int *n2d);

/*@brief set adc type,physic or not
 *@param(in) n:virtual channel
 *@param(in) type:adc type,physic or not
 *@return 0:succuud,-1:fail
 */
extern int product_test_adc_set_type(int n,int type);

/*@brief get adc type,physic or not
 *@param(in) n:virtual channel
 *@param(in) type:adc type,physic or not
 *@return 0:succuud,-1:fail
 */
extern int product_test_adc_get_type(int n,int *type);

/*@brief set adc value reverse or swith mode
 *@param(in) n:virtual channel
 *@param(in) mode: switch mode
 *@param(in) reverse:adc value reverse
 *@return 0:succuud,-1:fail
 */
extern int product_test_adc_set_switch_param(int n,int mode,int reverse);


/*@brief get adc value reverse or swith mode
 *@param(in) n:virtual channel
 *@param(out) mode: switch mode
 *@param(out) reverse:adc value reverse
 *@return 0:succuud,-1:fail
 */
extern int product_test_adc_get_switch_param(int n,int* mode,int *reverse);


#ifdef __cpulsplus
}
#endif

#endif
