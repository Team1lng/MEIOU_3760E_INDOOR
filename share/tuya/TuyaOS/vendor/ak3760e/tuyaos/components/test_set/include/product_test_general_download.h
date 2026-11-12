/* 
 *  @file product_test_general_download.h
 *  @brief product_test_general_download module is supply general download test interface
 *  @version 0.1
 *  @date 2021-05-07
 *
 *  @copyright Copyright (c) tuya.inc 2021
 *  
 */
#ifndef __PRODUCT_TEST_GENERAL_DOWNLOAD_H__
#define __PRODUCT_TEST_GENERAL_DOWNLOAD_H__

#ifdef __cplusplus
extern "C" {
#endif

/*@brief general download start 
 *@param(in) name:download file save file name
 *@param(in):md5:download file md5sum value
 *@param(in):size:download file total size
 *@return 0:succeed,-1:fail
 */
extern int product_test_general_download_start(char *name,char *md5,int size,char *type); 

/*@brief general download download packets 
 *@param(in) data:download datas 
 *@param(in):data_len:download data len
 *@return 0:succeed,-1:fail
 */
extern int product_test_general_downloading(char *data,uint32_t data_len);

/*@brief general download  end
 *@param(in) void 
 *@param(in):data_len:download data len
 *@return 0:succeed,-1:fail
 */
extern int product_test_general_download_end(void);

/*@brief general download file analysis 
 *@param(in) file: download file name  
 *@return 0:succeed,-1:fail
 */
extern int product_test_general_download_file_analysis(char*file);

/*@brief set download filename 
 *@param(in) name: download filename  
 *@return 0:succeed,-1:fail
 */
extern int product_test_get_download_filename(char *name);

/*@brief get download filename 
 *@param(in) name: download filename  
 *@return 0:succeed,-1:fail
 */
extern int product_test_set_download_filename(char *name); 

#ifdef __cplusplus
}
#endif

#endif
