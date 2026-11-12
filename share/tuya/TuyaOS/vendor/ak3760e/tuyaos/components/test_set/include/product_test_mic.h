/* 
 *  @file product_test_mic.h
 *  @brief product_test_mic module is supply mic test interface
 *  @version 0.1
 *  @date 2021-05-07
 *
 *  @copyright Copyright (c) tuya.inc 2021
 *  
 */
#ifndef __PRODUCT_TEST_MIC_H__
#define __PRODUCT_TEST_MIC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>

typedef enum {
    E_MIC_INFACE_NORMAL = 0, /* mic interface in  normal mode */
    E_MIC_INFACE_TINY_ALSA,  /* mic interface in tiny alsa mode */
}MIC_INFACE_TYPE_E;

/*@brief mic record  seconds
 *@param(in) file_path:save record datas in file_path
 *@param(in) seconds: record seconds
 *@return 0:succeed,-1:fail
 */
extern int product_test_mic_record_start(char* file_path,int seconds);

/*@brief mic record loop 
 *@param(in) file_path:save record datas in file_path
 *@return 0:succeed,-1:fail
 */
extern int product_test_mic_record_start_no_timeout(char *file_path);

/*@brief stop mic record 
 *@return 0:succeed,-1:fail
 */
extern int product_test_mic_record_stop(void); 

/*@brief set mic volume
 *@param(in) dev_id: mic device id
 *@param(in) channel: mic device channel
 *@param(in) value:mic volume
 *@return 0:succeed,-1:fail
 */
extern int product_test_mic_set_volume(int dev_id,int channel,uint32_t value);

/*@brief get mic volume
 *@param(in) dev_id: mic device id
 *@param(in) channel: mic device channel
 *@param(out) value:mic volume
 *@return 0:succeed,-1:fail
 */
extern int product_test_mic_get_volume(int dev_id,int channel,uint32_t *value);

/*@brief mic tiny alsa mode capature data and upload to pc
 *@param fd:communicate fd to upload datas
 *@return 0:succeed;-1:fail
 */
extern int product_test_mic_tiny_capature_and_upload(int fd);

/*@brief set mic interface
 *@param(in) mode: mic interface mode,see also MIC_INFACE_TYPE_E
 *@param(in) value:mic volume
 *@return 0:succeed,-1:fail
 */
extern int product_test_mic_set_mode(MIC_INFACE_TYPE_E mode);

/*@brief set mic interface
 *@param(in) mode: mic interface mode,see also MIC_INFACE_TYPE_E
 *@param(out) value:mic volume
 *@return 0:succeed,-1:fail
 */
extern int product_test_mic_get_mode(MIC_INFACE_TYPE_E *mode);

/*@brief get mic in pcm foramt
 *@param(out):data buf
 *@param(out):data len
 *@return n:bytes,-1:fail
 */
int product_test_mic_get_frame(char *buf,int plen);


#ifdef __cplusplus
}
#endif

#endif
