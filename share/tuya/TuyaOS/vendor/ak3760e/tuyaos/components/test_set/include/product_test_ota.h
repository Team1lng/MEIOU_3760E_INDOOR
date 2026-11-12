/* 
 *  @file product_test_ota.h
 *  @brief product_test_ota module is supply ota test interface
 *  @version 0.1
 *  @date 2021-05-07
 *
 *  @copyright Copyright (c) tuya.inc 2021
 *  
 */
#ifndef __PRODUCT_TEST_OTA_H__
#define __PRODUCT_TEST_OTA_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    E_ZIGBEE_IMG,/*zigbee image*/
    E_BT_IMG,    /*bt image*/
    E_SUBG_IMG, /*sub1g image */
    E_IMG_MAX,  /*image max */
}SUB_IMAGE_TYPE_E;

/*@brief get ota file path
 *@param(out)file:ota file full path buffer
 *@param(in)len:buffer len
 *@return 0:succeed,-1:fail
 */
extern int product_test_get_ota_file(char *file, int len);

/*@brief set ota file path
 *@param(in)file:ota file full path
 *@return 0:succeed,-1:fail
 */
extern int product_test_set_ota_file(char *file);

/*@brief ota start
 *@param(out)packet_size:ota packet size
 *@return 0:succeed,-1:fail
 */
extern int product_test_ota_start(unsigned long * packet_size);

/*@brief ota data receive  
 *@param(in) data:ota datas buffer
 *@param(in) data_len:ota datas lens
 *@return 0:succeed,-1:fail
 */
extern int product_test_ota_recv(char *data,unsigned int data_len);

/*@brief ota end process 
 *@param(in)void 
 *@return 0:succeed,-1:fail
 */
extern int product_test_ota_end(void); 

/*@brief ota slave image 
 *@param(in)image_type:slave image type,see also E_SUB_IMAGE_TYPE 
 *@return 0:succeed,-1:fail
 */
extern int product_test_slave_upgrade(SUB_IMAGE_TYPE_E image_type);


/*@brief ota init 
 *@param(in)baud:ota buadrate,when used uart transfer
 *@param(in)crc32:ota firmware's crc32
 *@param(in)size:ota firmware's total size
 *@return 0:succeed,-1:fail
 */
extern int product_test_ota_init (unsigned long baud,unsigned long crc32,unsigned long size);

/*@brief get ota packet size
 *@return pack_size 
 */
extern unsigned long product_test_get_ota_packet_size(void);

/*@brief get ota packet size
 *param(in) set ota packet size 
 *@return 0:succeed,-1:fail
 */
extern int product_test_set_ota_packet_size(unsigned long size);

#ifdef __cplusplus
}
#endif

#endif
