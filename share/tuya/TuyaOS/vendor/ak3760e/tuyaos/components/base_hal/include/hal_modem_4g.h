/*
 * Copyright (C) 2021 Tuya Inc.
 */

#ifndef __HAL_MODEM_4G_H__
#define __HAL_MODEM_4G_H__

#include "hal_serial.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MODEM_4G_CMD_DHCP  "busybox udhcpc -i usb0"
enum AT_INIT_STATE{
	AT_PORT_INIT_SUCCESS = 0,
	AT_UART_INIT_FAILED = -1,
	AT_TEST_FAILED = -2,
	AT_SET_ECHO_OFF_FAILED = -3,
	AT_SET_CMEE_FAILED = -4,
	AT_GET_NAME_FAILED = -5,
	AT_SET_HOT_PLUG_FAILED = -6,
};

enum AT_ECM_STATE{
	AT_USB_RESET_SUCCESS = 1,
	AT_USB_IN_ECM_MODE = 0,
	AT_USB_MODE_CHECK_FAILED = -1,
	AT_USB_SET_ECM_FAILED = -2,
};

enum AT_NET_STATE{

	AT_REGISTER_NET_SUCCESS = 0,
	AT_FUNC_ENABLE_FAILED = -1,
	AT_REG_MODE_SET_FAILED = -2,
	AT_REGISTER_NET_FAILED = -3,
	AT_LTE_DATA_CHECK_FAILED = -4,
	AT_SET_RNDIS_FAILED = -5,
	AT_NET_STA_CHECK_FAILED = -6,
};

enum AT_DAIL_UP_STATE{
	AT_DAIL_UP_SUCCESS = 0,
	AT_PDP_IP_QUERY_FAILED = -1,
	AT_DHCP_GET_IP_FAILED = -2,
	AT_PING_TEST_FAILED = -3,
};

#define AT_CMD_TABLE_SIZE	(40)
/* 4G modem device attribute */
typedef struct {
    char net_dev[64];                   	/* network device name, e.g. USB0 */
	char module_name[32];                   /* module name, e.g. EC21 */
    char at_port[64];                   	/* AT command port, e.g. /dev/ttyUSB0 */
    hal_serial_attr_t at_port_attr;     	/* AT command port attribute */
    int at_uart_fd;				/* AT command port file descriptor */
    char at_cmd_table[AT_CMD_TABLE_SIZE][32];	/* AT command table */
    char at_recv_buf[256];			/* AT command recv buffer */
} __attribute__((packed, aligned(4))) hal_modem_4g_dev_attr_t;

/** Get 4g modem device attributes.
 * @param attr device attribute.
 * @returns 0 on success, error code on failure.
 */
int hal_modem_4g_init(hal_modem_4g_dev_attr_t *attr);
int hal_modem_4g_set_ecm_modem(hal_modem_4g_dev_attr_t *attr);
int hal_modem_4g_imei_check(hal_modem_4g_dev_attr_t *attr,char *imei);
int hal_modem_4g_sim_card_detect(hal_modem_4g_dev_attr_t *attr);
int hal_modem_4g_imsi_check(hal_modem_4g_dev_attr_t *attr,char *imsi);
int hal_modem_4g_iccid_check(hal_modem_4g_dev_attr_t *attr,char *iccid);
int hal_modem_4g_signal_quality_check(hal_modem_4g_dev_attr_t *attr,int *rssi,int *ber);
int hal_modem_4g_apn_set(hal_modem_4g_dev_attr_t *attr, char *apn);
int hal_modem_4g_connect_net(hal_modem_4g_dev_attr_t *attr, int delay_ms);
int hal_modem_4g_disconnect_net(hal_modem_4g_dev_attr_t *attr);
int hal_modem_4g_dail_up(hal_modem_4g_dev_attr_t *attr);
int hal_modem_4g_user_set(hal_modem_4g_dev_attr_t *attr, char *apn, char *username, char *password);
int hal_modem_4g_send_message(hal_modem_4g_dev_attr_t *attr, char *phone_num, char *mesg_buf);
int hal_modem_4g_voice_call(hal_modem_4g_dev_attr_t *attr, char *phone_num);
int hal_modem_4g_get_call_status(hal_modem_4g_dev_attr_t *attr, char *status);
int hal_modem_4g_voice_hang_up(hal_modem_4g_dev_attr_t *attr);
int hal_modem_4g_get_dtmf_value(hal_modem_4g_dev_attr_t *attr, int *value, int timeout);
int hal_modem_4g_get_phone_number(hal_modem_4g_dev_attr_t *attr, char *phone_num);
int hal_modem_4g_get_firmware_version(hal_modem_4g_dev_attr_t *attr, char *version);
int hal_modem_4g_reset(hal_modem_4g_dev_attr_t *attr);

#ifdef __cplusplus
}
#endif

#endif
