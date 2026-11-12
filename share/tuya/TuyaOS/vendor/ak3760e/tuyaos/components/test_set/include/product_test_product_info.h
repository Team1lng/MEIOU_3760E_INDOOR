/* 
 *  @file product_test_product_info.h
 *  @brief product_test_product_info module is supply product_info test interface
 *  @version 0.1
 *  @date 2021-05-07
 *
 *  @copyright Copyright (c) tuya.inc 2021
 *  
 */

#ifndef __PRODUCT_TEST_PRODUCT_INFO_H__
#define __PRODUCT_TEST_PRODUCT_INFO_H__

#ifdef __cplusplus
extern "C" {
#endif

/*@brief set main firmware name
 *@param(in) firmware_name:main firmware name
 *@return 0:succeed,-1:fail
 */
extern int product_test_set_main_firmware_name(char *firmware_name);

/*@brief get main firmware name
 *@param(out) firmware_name:main firmware name
 *@return 0:succeed,-1:fail
 */
extern int product_test_get_main_firmware_name(char *firmware_name);

/*@brief set main firmware version
 *@param(in) firmware_version:main firmware version 
 *@return 0:succeed,-1:fail
 */
extern int product_test_set_main_firmware_version(char *firmware_verison);

/*@brief get main firmware version
 *@param(out) firmware_version:main firmware version 
 *@return 0:succeed,-1:fail
 */
extern int product_test_get_main_firmware_version(char *firmware_verison);

/*@brief set sub firmware name
 *@param(in) cpu:sub cpu number
 *@param(in) firmware_name:sub firmware name
 *@return 0:succeed,-1:fail
 */
extern int product_test_set_sub_firmware_name(int cpu,char *firmware_name); 

/*@brief get sub firmware name
 *@param(in) cpu:sub cpu number
 *@param(out) firmware_name:sub firmware name
 *@return 0:succeed,-1:fail
 */
extern int product_test_get_sub_firmware_name(int cpu,char *firmware_name); 

/*@brief set sub firmware version 
 *@param(in) cpu:sub cpu number
 *@param(in) firmware_version:sub firmware version 
 *@return 0:succeed,-1:fail
 */
extern int product_test_set_sub_firmware_version(int cpu,char *firmware_version);

/*@brief get sub firmware version 
 *@param(in) cpu:sub cpu number
 *@param(out) firmware_version:sub firmware version 
 *@return 0:succeed,-1:fail
 */
extern int product_test_get_sub_firmware_version(int cpu,char *firmware_version); 

/*@brief set communication protocol version version 
 *@param(in)protocol_version:protocol version
 *@return 0:succeed,-1:fail
 */
extern int product_test_set_protocol_version(char *protocol_verison);

/*@brief set communication protocol version version 
 *@param(out)protocol_version:protocol version
 *@return 0:succeed,-1:fail
 */
extern int product_test_get_protocol_version(char *protocol_verison);

/*@brief set product type,NOTICE:must fit with the work order
 *@param(in)product_type:product type
 *@return 0:succeed,-1:fail
 */
extern int product_test_set_product_type(char *product_type); 

/*@brief set product type,NOTICE:must fit with the work order
 *@param(out)product_type:product type
 *@return 0:succeed,-1:fail
 */
extern int product_test_get_product_type(char *product_type);

/*@brief set mac_write flag 
 *@param(in)mac_write:mac write flag
 *@return 0:succeed,-1:fail
 */
extern int product_test_set_mac_write(int mac_write); 

/*@brief get mac_write flag 
 *@param(out)mac_write:mac write flag
 *@return 0:succeed,-1:fail
 */
extern int product_test_get_mac_write(int *mac_write);

/*@brief set pskkey write flag 
 *@param(in)pskkey_write :pskkey write flag
 *@return 0:succeed,-1:fail
 */
extern int product_test_set_pskkey_write(int pskkey_write); 

/*@brief set pskkey write flag 
 *@param(out)pskkey_write :pskkey write flag
 *@return 0:succeed,-1:fail
 */
extern int product_test_get_pskkey_write(int* pskkey_write); 

/*@brief set himekitType flag 
 *@param(in)himekitType: himekitType flag
 *@return 0:succeed,-1:fail
 */
extern int product_test_set_himekitType(int himekitType);

/*@brief get himekitType flag 
 *@param(out)himekitType: himekitType flag
 *@return 0:succeed,-1:fail
 */
extern int product_test_get_himekitType(int* himekitType);

/*@brief set product test lowpower mode 
 *@param(in)mode:lowpower mode,1,lowpower mode;0,unlowpower mode
 *@return 0:succeed,-1:fail
 */
extern int product_test_set_low_power_mode(int mode);

/*@brief get product test lowpower mode 
 *@param(out)mode:lowpower mode,1,lowpower mode;0,unlowpower mode
 *@return 0:succeed,-1:fail
 */
extern int product_test_get_low_power_mode(int* mode);

/*@brief get product version info
 *@param(out)buf:version info buffer
 *@param(in)size:buffer size
 *@return 0:succeed,-1:fail
 */
extern int product_test_get_version_info(char *buf, int size);

/*@brief set version info path
 *@param(in)version_path:version path
 *@return 0:succeed,-1:fail
 */
extern int product_test_set_version_path(char *version_path); 

/*@brief get version info path
 *@param(out)version_path:version path
 *@return 0:succeed,-1:fail
 */
extern int product_test_get_version_path(char *version_path);

#ifdef __cplusplus
}
#endif
#endif
