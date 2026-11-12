/*
 *  @file product_test_camera.h
 *  @brief product_test_camera is support camera test 
 *  @version 0.1
 *  @date 2021-05-07
 *
 * @copyright Copyright (c) tuya inc 2021
 *
 */

#ifndef __PRODUCT_TEST_CAMERA_H__
#define __PRODUCT_TEST_CAMERA_H__

#ifdef __cplusplus
extern "C" {
#endif


typedef enum {
    E_CAMERA_MF_NONE = 0, //camera mirror flip none
    E_CAMERA_MF_MIRROR = 1,//camera mirror
    E_CAMERA_MF_FLIP = 2,  //camera flip
    E_CAMERA_MF_BOTH = 3,  //camera mirror and flip
    E_CAMERA_MF_MAX,       //camera max
}CAMERA_MIRROR_FLIP_TYPE_T;

/*
 *@brief camera test start
 *@param(in) void
 *@return 0:succeed ,-1 fail
 */
extern int product_test_camera_test_start(void);

/*
 *@brief camera test top
 *@param(in) void
 *@return 0:succeed ,-1 fail
 */
extern int product_test_camera_test_stop(void);

/*
 *@brief camera black test start
 *@param(in) void
 *@return 0:succeed ,-1 fail
 */
extern int product_test_black_camera_test_start(void);

/*
 *@brief camera black test stop 
 *@param(in) void
 *@return 0:succeed ,-1 fail
 */
extern int product_test_black_camera_test_stop(void);

/*
 *@brief set camera type
 *@param(in) type:camera type "jf37" 
 *@return 0:succeed ,-1 fail
 */
extern int product_test_camera_set_type(char *type);

/*
 *@brief get camera type
 *@param(in) type:camera type buffer 
 *@param(in) len:camera type buffer len 
 *@return 0:succeed ,-1 fail
 */
extern int product_test_camera_get_type(char *type,int len);

/*
 *@brief set camera  mirror and flip
 *@param(in) mirror_flip:camera mirror and flip,See Also  CAMERA_MIRROR_FLIP_TYPE_T
 *@return 0:succeed ,-1 fail
 */
extern int product_test_camera_set_mirror_flip(CAMERA_MIRROR_FLIP_TYPE_T mirror_flip);

/*
 *@brief get camera  mirror and flip
 *@param(out) mirror_flip:camera mirror and flip,See Also  CAMERA_MIRROR_FLIP_TYPE_T
 *@return 0:succeed ,-1 fail
 */
extern int product_test_camera_get_mirror_flip(CAMERA_MIRROR_FLIP_TYPE_T *mirror_flip); 


/*
 *@brief get camera frame 
 *@param(in)chan_num:channel number 0: main channel,1:sub channel
 *@param(out)buf:frame data 
 *@param(out)len:frame len 
 *@return 0:succeed ,-1 fail
 */
extern int product_test_camera_get_frame(int chan_num,char **buf,int *len);


/*
 *@brief set camera version 
 *@param(in)version:camera  version string
 *@return 0:succeed ,-1 fail
 */
extern int product_test_camera_set_version(char *version);

/*
 *@brief get  camera version 
 *@param(out)version:camera  version buffer 
 *@param(in)len:camera  version buffer len
 *@return 0:succeed ,-1 fail
 */
extern int product_test_camera_get_version(char *version,int len);

/*
 *@brief set camera config 
 *@param(in)config:camera  config string
 *@return 0:succeed ,-1 fail
 */
extern int product_test_camera_set_config(char *config);

/*
 *@brief get camera config 
 *@param(out)config:camera config buffer 
 *@param(in)len:camera config buffer len
 *@return 0:succeed ,-1 fail
 */
extern int product_test_camera_get_config(char *config,int len);

#ifdef __cplusplus
}
#endif

#endif
