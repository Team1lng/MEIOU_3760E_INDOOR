/* 
 *  @file product_test_cfg_info.h
 *  @brief product_test_cfg_info module is supply product information access interface
 *  @version 0.1
 *  @date 2021-05-07
 *
 *  @copyright Copyright (c) tuya.inc 2021
 *  
 */
#ifndef __PRODUCT_TEST_CFG_INFO_H__
#define __PRODUCT_TEST_CFG_INFO_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tcp_parm_s
{
    int port; /*tcp communication port*/
}TCP_INFO_T;

typedef struct uart_parm_s
{
    char name[64+1];/*uart device path*/
    int baud;     /*uart baudrate*/
}UART_INFO_T;

/*@brief save device pid uuid auzkey
 *@param(in) pid:device pid 
 *@param(in) uuid:device uuid
 *@param(in) auzkey:device auzkey
 *@return 0:succeed,-1:fail
 */

extern int product_test_set_auth_info(char *pid,char *uuid,char *auzkey);

/*@brief get device pid uuid auzkey
 *@param(out) pid:device pid 
 *@param(out) uuid:device uuid
 *@param(out) auzkey:device auzkey
 *@return 0:succeed,-1:fail
 */

extern int product_test_get_auth_info(char *pid,char *uuid,char *auzkey);


/*@brief save wifi ssid 
 *@param(in) ssid:wifi ssid 
 *@return 0:succeed,-1:fail
 */
extern int product_test_set_ssid(char *ssid); 

/*@brief get wifi ssid 
 *@param(out) ssid:wif issid 
 *@return 0:succeed,-1:fail
 */
extern int product_test_get_ssid(char *ssid); 

/*@brief save wifi passwd 
 *@param(in) pwd:wifi passwd 
 *@return 0:succeed,-1:fail
 */
extern int product_test_set_pwd(char *pwd); 

/*@brief get wifi passwd 
 *@param(out) pwd:wifi passwd 
 *@return 0:succeed,-1:fail
 */
extern int product_test_get_pwd(char *pwd); 

/*@brief save device pskkey
 *@param(in) pwd:device pskkey
 *@return 0:succeed,-1:fail
 */
extern int product_test_set_pskkey(char *pskkey); 

/*@brief get device pskkey
 *@param(out) pwd:device pskkey
 *@return 0:succeed,-1:fail
 */
extern int product_test_get_pskkey(char *pskkey); 

/*@brief save master network mac address 
 *@param(in) mac:network mac address
 *@return 0:succeed,-1:fail
 */
extern int product_test_set_mmac(char *mac);

/*@brief get master network mac address 
 *@param(out) mac:network mac address
 *@return 0:succeed,-1:fail
 */
extern int product_test_get_mmac(char *mac); 

/*@brief save slave network mac address 
 *@param(in) cpu: slave mac number
 *@param(in) mac:network mac address
 *@return 0:succeed,-1:fail
 */
extern int product_test_set_smac(int cpu,char *mac); 

/*@brief get slave network mac address 
 *@param(in) cpu: slave mac number
 *@param(out) mac:network mac address
 *@return 0:succeed,-1:fail
 */
extern int product_test_get_smac(int cpu,char *mac); 

/*@brief save device bsn 
 *@param(in) bsn:device bsn
 *@return 0:succeed,-1:fail
 */
extern int product_test_set_bsn(char *bsn); 

/*@brief get device bsn 
 *@param(out) bsn:device bsn
 *@return 0:succeed,-1:fail
 */
extern int product_test_get_bsn(char*bsn); 

/*@brief save device sn 
 *@param(in) sn:device sn
 *@return 0:succeed,-1:fail
 */
extern int product_test_set_sn(char *sn); 

/*@brief get device sn 
 *@param(in) sn:device sn
 *@return 0:succeed,-1:fail
 */
extern int product_test_get_sn(char*sn); 

/*@brief save device country code
 *@param(in) country:country code
 *@return 0:succeed,-1:fail
 */
extern int product_test_set_country(char *country); 

/*@brief get device country code
 *@param(out) country:country code
 *@return 0:succeed,-1:fail
 */
extern int product_test_get_country(char*country); 

/*@brief save prod_test flag 
 *@param(in) prod_test:prod_test flag
 *@return 0:succeed,-1:fail
 */
extern int product_test_set_prod_test(int prod_test); 

/*@brief get prod_test flag 
 *@param(out) prod_test:prod_test flag
 *@return 0:succeed,-1:fail
 */
extern  int product_test_get_prod_test(int *prod_test); 

/*@brief save zigbee enable flag 
 *@param(in) enable:zigbee_enable flag
 *@return 0:succeed,-1:fail
 */
extern int product_test_set_zigbee_enable(int enable);

/*@brief get zigbee enable flag
 *@param(out) enable:zigbee_enable flag
 *@return 0:succeed,-1:fail
 */
extern int product_test_get_zigbee_enable(int *enable) ;

/*@brief save led bright value
 *@param(in) val:bright value 
 *@return 0:succeed,-1:fail
 */
extern int product_test_set_led_brightness(int val) ;

/*@brief get led bright value 
 *@param(in) val:bright value 
 *@return 0:succeed,-1:fail
 */
extern int product_test_get_led_brightness(int *val) ;

/*@brief save ble name 
 *@param(in) name:ble name
 *@return 0:succeed,-1:fail
 */
extern int product_test_set_ble_name(char *name) ;

/*@brief get ble name 
 *@param(out) name:ble name
 *@return 0:succeed,-1:fail
 */
extern int product_test_get_ble_name(char  *name);

/*@brief save uart speed
 *@param(in) val:uart speed
 *@return 0:succeed,-1:fail
 */
extern int product_test_set_uart_speed(int val) ;

/*@brief get uart speed
 *@param(out) val:uart speed
 *@return 0:succeed,-1:fail
 */
extern int product_test_get_uart_speed(int *val) ;

/*@brief save firmware type
 *@param(in) type:firmware type
 *@return 0:succeed,-1:fail
 */
extern int product_test_set_firm_type(char *type) ;

/*@brief get firmware type
 *@param(out) type:firmware type
 *@return 0:succeed,-1:fail
 */
extern int product_test_get_firm_type(char  *type);

/*@brief save max volume
 *@param(in) fval:max volume
 *@return 0:succeed,-1:fail
 */
extern int product_test_set_max_volume(float fval) ;

/*@brief get max volume
 *@param(out) fval:max volume
 *@return 0:succeed,-1:fail
 */
extern int product_test_get_max_volume(float *fval) ;

/*@brief save avs pid 
 *@param(in) pid: avs pid
 *@return 0:succeed,-1:fail
 */
extern int product_test_set_avs_pid(char *pid) ;

/*@brief get avs pid 
 *@param(out) pid: avs pid
 *@return 0:succeed,-1:fail
 */
extern int product_test_get_avs_pid(char  *pid);

/*@brief save  respond language
 *@param(in) language: respond language
 *@return 0:succeed,-1:fail
 */
extern int product_test_set_respond_lanuage(char *lanuage) ;

/*@brief get  respond language
 *@param(out) language: respond language
 *@return 0:succeed,-1:fail
 */
extern int product_test_get_respond_lanuage(char  *lanuage);

/*@brief save wakeup language
 *@param(in) language: wakeup language
 *@return 0:succeed,-1:fail
 */
extern int product_test_set_wakeup_lanuage(char *lanuage);

/*@brief get wakeup language
 *@param(out) language: wakeup language
 *@return 0:succeed,-1:fail
 */
extern int product_test_get_wakeup_lanuage(char  *lanuage);

/*@brief save gateway addr
 *@param(in) address: gateway addr
 *@return 0:succeed,-1:fail
 */
extern int product_test_set_gateway_addr(char *address) ;

/*@brief save gateway addr
 *@param(out) address: gateway addr
 *@return 0:succeed,-1:fail
 */
extern int product_test_get_gateway_addr(char  *address);

/*@brief save device active flag string
 *@param(in) status: active status flag
 *@return 0:succeed,-1:fail
 */
extern int product_test_set_active_status(char *status);

/*@brief get device active flag string
 *@param(out) status: active status flag
 *@return 0:succeed,-1:fail
 */
extern int product_test_get_active_status(char *status);

/*@brief save dynamic passwd support flag 
 *@param(in) flag: dynamic passwd support flag 
 *@return 0:succeed,-1:fail
 */
extern int product_test_get_dynamic_passwd_support(int *flag);

/*@brief get dynamic passwd support flag
 *@param(out) flag: dynamic passwd support flag
 *@return 0:succeed,-1:fail
 */
extern int product_test_set_dynamic_passwd_support(int flag);

/*@brief save persist sdk variety status 
 *@param(in) status: persist sdk variety status  
 *@return 0:succeed,-1:fail
 */
extern int product_test_set_persist_sdk_variety(char *status); 

/*@brief get persist sdk variety status 
 *@param(out) status: persist sdk variety status
 *@return 0:succeed,-1:fail
 */
extern int product_test_get_persist_sdk_variety(char *status);

/*@brief set dir that save config information  
 *@param(in) dir: config information dir 
 *@return 0:succeed,-1:fail
 */
extern int product_test_set_config_dir(char *dir);

/*@brief set dir that save config information  
 *@param(out)dir:config information dir 
 *@return 0:succeed,-1:fail
 */
extern int product_test_get_config_dir(char * dir);

/*@brief  init product test configure 
 *@param(in)file_name:product configure file 
 *@return 0:succeed,-1:fail
 */
extern int product_test_init_config(char *file_name); 

/*@brief  get tcp communication information 
 *@param(out)info:tcp communication information,see TCP_INFO_T
 *@return 0:succeed,-1:fail
 */
extern int product_test_get_tcp_info(TCP_INFO_T *info); 

/*@brief  get uart communication information 
 *@param(out)info:uart communication information,see UART_INFO_T
 *@return 0:succeed,-1:fail
 */
extern int product_test_get_uart_info(UART_INFO_T *info); 

#ifdef __cplusplus
}
#endif

#endif
