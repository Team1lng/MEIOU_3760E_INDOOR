/* 
 *  @file product_test_media.h
 *  @brief product_test_media module is supply media test interface
 *  @version 0.1
 *  @date 2021-05-07
 *
 *  @copyright Copyright (c) tuya.inc 2021
 *  
 */

#ifndef __PRODUCT_TEST_MEDIA_H__
#define __PRODUCT_TEST_MEDIA_H__

#include <inttypes.h>
#include "ptal_media.h"

#ifdef __cplusplus
extern "C" {
#endif



/*@brief medio init 
 *@param(in)void 
 *@return 0:succeed,-1:fail
 */
extern int product_test_media_init(void);

/*@brief speaker play wav file  
 *@param(in)file_path: file path
 *@return 0:succeed,-1:fail
 */
extern int product_test_speaker_start_play_wav(char *file_path);

/*@brief speaker play pcm file  
 *@param(in)file_path: file path
 *@return 0:succeed,-1:fail
 */
extern int product_test_speaker_start_play_pcm(char *file_path);

/*@brief speaker play stop file  
 *@param(in)void
 *@return 0:succeed,-1:fail
 */
extern int product_test_speaker_stop(void);

/*@brief load isp day file  
 *@param(in)void
 *@return 0:succeed,-1:fail
 */
extern int product_test_media_load_day_isp_file(void);

/*@brief load isp night file  
 *@param(in)void
 *@return 0:succeed,-1:fail
 */
extern int product_test_media_load_night_isp_file(void);

/*@brief get mic sample rate  
 *@param(out)rate:sample rate
 *@return 0:succeed,-1:fail
 */
extern int product_test_mic_get_sample_rate(int *rate);

/*@brief change g711 datas to pcm datas
 *@param(in)src:g711 datas
 *@param(in)src_len:g711 data len 
 *@param(out)dst:pcm datas 
 *@param(out)dst_len:pcm data len
 *@return 0:succeed,-1:fail
 */
extern int product_test_g711_to_pcm(unsigned short *src,unsigned int src_len, unsigned char *dst, unsigned int *dst_len);

/*@brief change pcm datas to g711 datas
 *@param(in)src:pcm datas
 *@param(in)src_len:pcm data len 
 *@param(out)dst:g711 datas 
 *@param(out)dst_len:g711 data len
 *@return 0:succeed,-1:fail
 */
extern int product_test_pcm_to_g711(unsigned short *src, unsigned int src_len, unsigned char *dst, unsigned int *dst_len);

/*@brief video get frame 
 *@param(out)type:get frame type(0:pb frame,1:I frame,2:ts frame)
 *@param(out)buf:get video frame buff
 *@param(out)plen:get video frame len
 *@param(out)pts:frame pts
 *@return 0:succeed,-1:fail
 */
extern int product_test_video_get_frame(VIDEO_TYPE_E video_type,int *frame_type,char **buf,int *plen,uint64_t *pts);

/*@brief audio get frame 
 *@param(out)buf:get audio frame buff
 *@param(out)plen:get audio frame len
 *@param(out)pts:frame pts
 *@return 0:succeed,-1:fail
 */
extern int product_test_audio_get_frame(char **buf,int *plen,uint64_t *pts);


/*@brief audio put frame 
 *@param(in)buf:put audio frame buff
 *@param(in)plen:put audio frame len
 *@return 0:succeed,-1:fail
 */
extern int product_test_audio_put_frame(char *buf,int plen);


/*@brief start media rtsp test 
 *@param(in)void
 *@return 0:succeed,-1:fail
 */
extern int product_test_media_server_start(void);

/*@brief stop media rtsp test 
 *@param(in)void
 *@return 0:succeed,-1:fail
 */
extern int product_test_media_server_stop(void);

/*@brief audio set volume
 *@param(in) type: audio device type,See Also AUDIO_DEV_TYPE_E
 *@param(in) devid: audio device id
 *@param(in) channel:audio channel
 *@param(in) value:audio volume
 *@return 0:succeed,-1:fail
 */
extern int product_test_audio_set_volume(AUDIO_DEV_TYPE_E type,int devid, int channel, unsigned int value);

/*@brief audio get volume
 *@param(in) type: audio device type,See Also AUDIO_DEV_TYPE_E
 *@param(in) devid: audio device id
 *@param(in) channel:audio channel
 *@param(out) value:audio volume
 *@return 0:succeed,-1:fail
 */
extern int product_test_audio_get_volume(AUDIO_DEV_TYPE_E type,int devid, int channel, unsigned int *value);

/*@brief config media parameters
 *@param(in) config:media config,See Also MEDIA_CONFIG_T
 *@return 0:succeed,-1:fail
 */
extern int product_test_init_media_config(MEDIA_CONFIG_T *confg);

/*@brief media set color to gray 
 *@param(in) mode:0,auto;1,day;2,night
 *@return 0:succeed,-1:fail
 */
extern int proudct_test_media_set_color_to_gray(int mode);


/*@brief media get virtual adc 
 *@param(in) mode:0,auto;1,day;2,night
 *@param(out)val:virtual value
 *@return 0:succeed,-1:fail
 */
extern int product_test_media_get_virtual_adc(int mode, uint32_t *val); 


#ifdef __cplusplus
}
#endif

#endif
