/* 
 *  @file product_test_modem.h
 *  @brief product_test_modem module is supply modem test interface
 *  @version 0.1
 *  @date 2021-05-07
 *
 *  @copyright Copyright (c) tuya.inc 2021
 *  
 */

#ifndef __PRODUCT_TEST_MODEM_H__
#define __PRODUCT_TEST_MODEM_H__

#ifdef __cplusplus
extern "C" {
#endif

#define AT_CMD_TEST                     "AT\r\n"
#define AT_CMD_ECHO_OFF                 "ATE0\r\n"
#define AT_CMD_CMEE                     "AT+CMEE=2\r\n"
#define AT_CMD_SUPPORT_HOTPLUG          "AT+MSMPD=1\r\n"
#define AT_CMD_USB_MODE_CHECK           "AT+GTUSBMODE?\r\n"
#define AT_CMD_USB_MODE_ECM             "AT+GTUSBMODE=32\r\n"
#define AT_CMD_NET_SET                  "AT+GTRAT=5,3,3\r\n"
#define AT_CMD_CHECK_REG                "AT+COPS?\r\n"
#define AT_CMD_SET_REG                  "AT+COPS=0\r\n"
#define AT_CMD_FUNC_LEVEL_QUERY         "AT+CFUN?\r\n"
#define AT_CMD_FUNC_LEVEL_SET           "AT+CFUN=1\r\n"
#define AT_CMD_RESET_MODEM              "AT+CFUN=15\r\n"
#define AT_CMD_SIGNAL_QUALITY_CHECK     "AT+CSQ\r\n"
#define AT_CMD_SIM_PIN_CHECK            "AT+CPIN?\r\n"
#define AT_CMD_SIM_CCID_CHECK       "AT+CCID\r\n"
#define AT_CMD_SIM_IMSI_CHECK       "AT+CIMI?\r\n"
#define AT_CMD_LTE_IMEI_CHECK       "AT+CGSN?\r\n"
#define AT_CMD_PS_REG_CHECK             "AT+CGREG?\r\n"
#define AT_CMD_LTE_DATA_CHECK           "AT+CEREG?\r\n"
#define AT_CMD_SET_RNDIS                "AT+GTRNDIS=1,1\r\n"
#define AT_CMD_GETIP_CHECK              "AT+GTRNDIS?\r\n"

#define AT_CMD_RESULT_OK        "OK"
#define MODEM_BUFF_SIZE           (256)
#define MODEM_DATA_SIZE           (32)

typedef struct modem_data_s
{
    char ccid[MODEM_DATA_SIZE];
    char imsi[MODEM_DATA_SIZE];
    char imei[MODEM_DATA_SIZE];
    int rssi;
}MODEM_DATA_T;

/*@brief motem enter test
 *@param(in)void
 *@return 0:succeed,-1:fail
 */
extern int product_test_modem_enter_test(void);

/*@brief modem off
 *@param(in)void
 *@return 0:succeed,-1:fail
 */
extern int product_test_modem_off(void);

/*@brief modem get ip
 *@param(out)ip:modem ip
 *@return 0:succeed,-1:fail
 */
extern int product_test_modem_get_ip(uint32_t *ip);

/*@brief modem get signal type
 *@param(out)type:signal ip
 *@return 0:succeed,-1:fail
 */
extern int product_test_modem_get_signal_type(int* type);

/*@brief modem get ccid type
 *@param(out)ccid:modem ccid
 *@return 0:succeed,-1:fail
 */
extern int product_test_modem_get_ccid(char *ccid);

/*@brief modem get imsi
 *@param(out)imsi:modem imsi
 *@return 0:succeed,-1:fail
 */
extern int product_test_modem_get_imsi(char *imsi);

/*@brief modem get imsi
 *@param(out)imsi:modem imsi
 *@return 0:succeed,-1:fail
 */
extern int product_test_modem_get_imei(char *imei);

/*@brief modem get rssi
 *@param(out)db:modem rssi db value
 *@return 0:succeed,-1:fail
 */
extern int product_test_modem_get_rssi(int *db);

#ifdef __cplusplus
}
#endif

#endif
