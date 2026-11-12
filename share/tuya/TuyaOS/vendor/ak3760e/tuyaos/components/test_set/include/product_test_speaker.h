/* 
 *  @file product_test_speaker.h
 *  @brief product_test_speaker module is supply spkeaker test interface
 *  @version 0.1
 *  @date 2021-05-07
 *
 *  @copyright Copyright (c) tuya.inc 2021
 *  
 */

#ifndef __PRODUCT_TEST_SPEAKER_H__
#define __PRODUCT_TEST_SPEAKER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>

//#include "product_test_media.h"

/*@brief set speaker volume
 *@param(in) devid:speaker device id
 *@param(in)channel:speaker channel id
 *@param(in)value:volume value
 *@return 0:succeed,-1:fail
 */
extern int product_test_speaker_set_volume(int devid,int channel, uint32_t value);

/*@brief set speaker volume
 *@param(in) devid:speaker device id
 *@param(in)channel:speaker channel id
 *@param(out)value:volume value
 *@return 0:succeed,-1:fail
 */
extern int product_test_speaker_get_volume(int devid,int channel,uint32_t *value);

/*@brief enable speaker 
 *@param(in)void
 *@return 0:succeed,-1:fail
 */
extern int product_test_speaker_enable(void);

/*@brief disable speaker 
 *@param(in)void
 *@return 0:succeed,-1:fail
 */
extern int product_test_speaker_disable(void);

/*@brief set speaker enable gpio level
 *@param(in)level: gpio level
 *@return 0:succeed,-1:fail
 */
extern int product_test_speaker_set_gpio_level(int level);

/*@brief get speaker enable gpio level
 *@param(in)level: gpio level
 *@return 0:succeed,-1:fail
 */
extern int product_test_speaker_get_gpio_level(int*level);

/*@brief set speaker enable gpio physic num 32*group+index
 *@param(in)level: gpio level
 *@return 0:succeed,-1:fail
 */
extern int product_test_speaker_set_gpio_num(int gpio_num);

/*@brief get speaker enable gpio physic num 32*group+index
 *@param(out)level: gpio level
 *@return 0:succeed,-1:fail
 */
extern int product_test_speaker_get_gpio_num(int*gpio_num);

/*@brief speaker play pcm format file 
 *@param(in)file_path:pcm format file's filepath
 *@return 0:succeed,-1:fail
 */
extern int product_test_speaker_pcm_play_start(char *file_path);

/*@brief speaker play wav format file 
 *@param(in)file_path:wav format file's filepath
 *@return 0:succeed,-1:fail
 */
extern int product_test_speaker_wav_play_start(char *file_path);

/*@brief speaker play stop 
 *@param(in)void
 *@return 0:succeed,-1:fail
 */
extern int product_test_speaker_play_stop(void); 

/*@brief speaker play file name set 
 *@param(in)name:play file name only,not full path
 *@return 0:succeed,-1:fail
 */
extern int product_test_speaker_set_play_filename(char *name);

/*@brief speaker play file name set 
 *@param(out)name:play file name only,not full path
 *@return 0:succeed,-1:fail
 */
extern int product_test_speaker_get_play_filename(char *name);


/*@brief speaker put pcm frame 
 *@param(out)buf:data  buffer
 *@param(out)plen;data len
 *@return 0:succeed,-1:fail
 */
extern int product_test_speaker_put_frame(char *buf,int plen);

/*@brief audio test
 *@return 0:succeed,-1:fail
 */
extern int product_test_audio_test(void);

/*@brief start swept 
 *@param(num) test num
 *@return 0:succeed,-1:fail
 */
extern int product_test_swept_sine_start(int num);

/*@brief stop swept  
 *@return 0:succeed,-1:fail
 */
extern int product_test_swept_over(void);

/*@brief get swept result
 *@return 0:succeed,-1:fail
 */
extern int product_test_read_swept_sine_result(void);

#ifdef __cplusplus
}
#endif

#endif
