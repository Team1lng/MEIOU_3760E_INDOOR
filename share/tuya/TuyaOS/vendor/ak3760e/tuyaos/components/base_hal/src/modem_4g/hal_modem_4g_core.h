/*
 * Copyright (C) 2021 Tuya Inc.
 */

#ifndef __HAL_MODEM_4G_CORE_H__
#define __HAL_MODEM_4G_CORE_H__

#include "hal_modem_4g.h"

/* General commands */
enum AT_CMD_GEN
{
	AT_CMD_AT = 0,
	AT_CMD_ATE,
	AT_CMD_CGMM,
	AT_CMD_CGSN,
	AT_CMD_CFUN,
	AT_CMD_CMEE,
	AT_CMD_CIMI,
	AT_CMD_CCID,
	AT_CMD_CPIN,
	AT_CMD_COPS,
	AT_CMD_CREG,
	AT_CMD_CSQ,
	AT_CMD_CGDCONT,
	AT_CMD_CGACT,
	AT_CMD_CGPADDR,
	AT_CMD_CGREG,
	AT_CMD_CEREG,
	AT_CMD_CSCS,
	AT_CMD_CMGF,
	AT_CMD_CMGS,
	AT_CMD_COLP,
	AT_CMD_CHUP,
	AT_CMD_ATD,
	AT_CMD_CNUM,
	AT_CMD_CGMR,
	TOTAL_GEN_CMD_MAX
};

/* Special commands */
typedef enum AT_CMD_SPE
{
	AT_CMD_USB_MODE = 0,
	AT_CMD_ECM_MODE,
	AT_CMD_SIM_HOT,
	AT_CMD_RNDIS,
	AT_CMD_DATA_CALL,
	AT_CMD_USER,
	AT_CMD_DTMF,
	AT_CMD_CALL_STATE,
	TOTAL_SPE_CMD_MAX
} CMD_INDEX_E;

typedef struct modem_cmd {
   CMD_INDEX_E index;
   char *at_cmd;
   char *result_flag;
   char *result_format;
   char *reserved;
} MODEM_CMD_T;

typedef struct modem_info_s {
	int off;
	int vid;
	int pid;
	char name[32];
	MODEM_CMD_T *cmd_table;
} MODEM_INFO_T;

#define USB_ECM_MODE	(1)
#define USB_NOECM_MODE	(0)
#define MAX_DEVICE (32)
#define NOT_SUPPORT	(1)
#define RET_OK		(0)
#define RET_ERR		(-1)

/* HAL interface defination */
typedef struct {
    int (*init)(hal_modem_4g_dev_attr_t *attr);
    int (*set_ecm_modem)(hal_modem_4g_dev_attr_t *attr);
    int (*imei_check)(hal_modem_4g_dev_attr_t *attr,char *imei);
    int (*iccid_check)(hal_modem_4g_dev_attr_t *attr,char *iccid);
    int (*sim_card_detect)(hal_modem_4g_dev_attr_t *attr);
    int (*imsi_check)(hal_modem_4g_dev_attr_t *attr,char *imsi);
    int (*signal_quality_check)(hal_modem_4g_dev_attr_t *attr,int *rssi,int *ber);
    int (*apn_set)(hal_modem_4g_dev_attr_t *attr);
    int (*connect_net)(hal_modem_4g_dev_attr_t *attr, int delay_ms);
    int (*disconnect_net)(hal_modem_4g_dev_attr_t *attr);
    int (*dail_up)(hal_modem_4g_dev_attr_t *attr);
	int (*send_message)(hal_modem_4g_dev_attr_t *attr, char *phone_num, char *mesg_buf);
	int (*voice_call)(hal_modem_4g_dev_attr_t *attr, char *phone_num);
	int (*get_call_status)(hal_modem_4g_dev_attr_t *attr, char *status);
	int (*voice_hang_up)(hal_modem_4g_dev_attr_t *attr);
	int (*get_dtmf_value)(hal_modem_4g_dev_attr_t *attr, int *value, int timeout);
	int (*get_phone_number)(hal_modem_4g_dev_attr_t *attr, char *phone_num);
	int (*get_firmware_version)(hal_modem_4g_dev_attr_t *attr, char *version);
	int (*reset)(hal_modem_4g_dev_attr_t *attr);
}hal_if_modem_4g_t;

extern hal_if_modem_4g_t hal_if_modem_4g;

int hal_modem_4g_init_common(hal_modem_4g_dev_attr_t *attr);
int hal_modem_4g_set_ecm_modem_common(hal_modem_4g_dev_attr_t *attr);
int hal_modem_4g_imei_check_common(hal_modem_4g_dev_attr_t *attr,char *imei);
int hal_modem_4g_iccid_check_common(hal_modem_4g_dev_attr_t *attr,char *iccid);
int hal_modem_4g_sim_card_detect_common(hal_modem_4g_dev_attr_t *attr);
int hal_modem_4g_imsi_check_common(hal_modem_4g_dev_attr_t *attr,char *imsi);
int hal_modem_4g_signal_quality_check_common(hal_modem_4g_dev_attr_t *attr,int *rssi,int *ber);
int hal_modem_4g_apn_set_common(hal_modem_4g_dev_attr_t *attr, char *apn);
int hal_modem_4g_connect_net_common(hal_modem_4g_dev_attr_t *attr, int delay_ms);
int hal_modem_4g_disconnect_net_common(hal_modem_4g_dev_attr_t *attr);
int hal_modem_4g_dail_up_common(hal_modem_4g_dev_attr_t *attr);
int hal_modem_4g_set_user_common(hal_modem_4g_dev_attr_t *attr, char *apn, char *username, char *password);
int hal_modem_4g_send_message_common(hal_modem_4g_dev_attr_t *attr, char *phone_num, char *mesg_buf);
int hal_modem_4g_voice_call_common(hal_modem_4g_dev_attr_t *attr, char *phone_num);
int hal_modem_4g_get_call_status_common(hal_modem_4g_dev_attr_t *attr, char *status);
int hal_modem_4g_voice_hang_up_common(hal_modem_4g_dev_attr_t *attr);
int hal_modem_4g_get_dtmf_value_common(hal_modem_4g_dev_attr_t *attr, int *value, int timeout);
int hal_modem_4g_get_phone_number_common(hal_modem_4g_dev_attr_t *attr, char *phone_num);
int hal_modem_4g_get_firmware_version_common(hal_modem_4g_dev_attr_t *attr, char *version);
int hal_modem_4g_reset_common(hal_modem_4g_dev_attr_t *attr);

#endif
