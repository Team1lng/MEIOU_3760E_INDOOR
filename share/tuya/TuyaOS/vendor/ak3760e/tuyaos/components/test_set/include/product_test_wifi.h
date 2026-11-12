/* 
 *  @file product_test_wifi.h
 *  @brief product_test_wifi module is supply wifi test interface
 *  @version 0.1
 *  @date 2021-05-07
 *
 *  @copyright Copyright (c) tuya.inc 2021
 *  
 */

#ifndef __PRODUCT_TEST_WIFI_H__
#define __PRODUCT_TEST_WIFI_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct wifi_info_s {
    int rssi;         // signal strength
    uint8_t ssid[65]; // ssid
}WIFI_INFO_T;

typedef enum {
    E_SCAN_MODE_SINGLE_AP = 0, // scan one single ap
    E_SCAN_MODE_ALL,           // scan all ap
    E_SCAN_MODE_MAX            // scan max
}WIFI_SCAN_MODE_E;

/*@brief wifi connect ap
 *@param(in)ssid:ap ssid
 *@param(out)psk:ap passwd
 *@return 0:succeed,-1:fail
 */
extern int product_test_wifi_connect_ap(char *ssid,char *psk);

/*@brief wifi get get rssi
 *@param(in)mode:scan mode ,See Alse WIFI_CAN_MODE_E
 *@param(in)ssid: scan ssid (single scan mode valid)
 *@param(out)：wifi_list:wifi ssid and rssi info group,See Also WIFI_INFO_T 
 *@param(out)：total ssid and rssi info group
 *@return 0:succeed,-1:fail
 */
extern int product_test_wifi_rssi_get(WIFI_SCAN_MODE_E mode, char *ssid,WIFI_INFO_T**wifi_list,int *total); 

/*@brief wifi write country code
 *@param(in) country code
 *@return 0:succeed,-1:fail
 */
extern int product_test_wifi_write_country_code(uint32_t code);

/*@brief wifi read country code
 *@param(out) country code
 *@return 0:succeed,-1:fail
 */
extern int product_test_wifi_read_country_code(uint32_t* code);

/*@brief wifi iperf test
 *@param(in) iperf test result file save path
 *@param(out) iperf test result
 *@return 0:succeed,-1:fail
 */
extern int product_test_wifi_iperf_test(char *path,WIFI_IPERF_T *iperf);

/*@brief wifi type set,Such as "ssw101b","rtl8188" 
 *@param(in)wifi:wifi type;
 *@return 0:succeed,-1:fail
 */
extern int product_test_set_wifi_type(char *wifi);

/*@brief wifi type get,Such as "ssw101b","rtl8188" 
 *@param(in)wifi:wifi type;
 *@return 0:succeed,-1:fail
 */
extern int product_test_get_wifi_type(char *wifi);

/*@brief wifi mac set efuse flag set 
 *@param(in)flag:wifi efuse set flag;
 *@return 0:succeed,-1:fail
 */
extern int product_test_wifi_set_mac_in_efuse_flag(int flag);

/*@brief wifi mac get efuse flag set 
 *@param(out)flag:wifi efuse set flag;
 *@return 0:succeed,-1:fail
 */
extern int product_test_wifi_get_mac_in_efuse_flag(int *flag);

/*@brief wifi mac set 
 *@param(in)mac:wifi mac
 *@return 0:succeed,-1:fail
 */
extern int product_test_wifi_set_mac(char *mac);

/*@brief wifi mac get 
 *@param(out)mac:wifi mac
 *@return 0:succeed,-1:fail
 */
extern int product_test_wifi_get_mac(char*mac);

#ifdef __cplusplus
}
#endif

#endif
