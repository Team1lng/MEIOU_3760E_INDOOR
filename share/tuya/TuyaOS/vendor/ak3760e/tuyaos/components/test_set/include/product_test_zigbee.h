/* 
 *  @file product_test_zigbee.h
 *  @brief product_test_zigbee module is supply zigbee test interface
 *  @version 0.1
 *  @date 2021-05-07
 *
 *  @copyright Copyright (c) tuya.inc 2021
 *  
 */
#ifndef __PRODUCT_TEST_ZIGBEE_H__
#define __PRODUCT_TEST_ZIGBEE_H__

#ifdef __cplusplus
extern "C" {
#endif

/*
 * @brief get zigbee firmware info
 *@param(in) in:  buffer  size
 *@param(out) buf:buffer firmware info
 *@return 0:succeed ,-1:fail
 */

extern int product_test_zigbee_get_firmware_info(char *buf,int size);

/*
 * @brief get zigbee version info
 *@param(in) in:  buffer  size
 *@param(out) buf:buffer version info
 *@return 0:succeed ,-1:fail
 */

extern int product_test_zigbee_get_version_info(char *buf, int size);

/*
 * @brief test zigbee
 *@param(in) channel: zigbee channel 
 *@param(in) send_pkts: zigbee  send packets
 *@param(out) recv_pkts: zigbee receive packets
 *@return 0:succeed ,-1:fail
 */
extern int product_test_zigbee_test(int channel,int send_pkts,int *recv_pkts);


#ifdef __cplusplus
}
#endif

#endif
