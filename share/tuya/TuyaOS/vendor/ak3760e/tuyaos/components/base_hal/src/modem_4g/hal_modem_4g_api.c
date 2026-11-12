/*
 * Copyright (C) 2021 Tuya Inc.
 */

#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/reboot.h>
#include "bcmnvram.h"
#include "hal_log.h"
#include "hal_misc.h"
#include "hal_modem_4g.h"
#include "hal_modem_4g_core.h"

int hal_modem_4g_init(hal_modem_4g_dev_attr_t *attr)
{
    if (NULL == hal_if_modem_4g.init)
	return hal_modem_4g_init_common(attr);

    return hal_if_modem_4g.init(attr);
}

int hal_modem_4g_set_ecm_modem(hal_modem_4g_dev_attr_t *attr)
{
    if (NULL == hal_if_modem_4g.set_ecm_modem)
	return hal_modem_4g_set_ecm_modem_common(attr);

    return hal_if_modem_4g.set_ecm_modem(attr);
}

int hal_modem_4g_imei_check(hal_modem_4g_dev_attr_t *attr,char *imei)
{
    if (NULL == hal_if_modem_4g.imei_check)
	return hal_modem_4g_imei_check_common(attr,imei);

    return hal_if_modem_4g.imei_check(attr,imei);
}

int hal_modem_4g_iccid_check(hal_modem_4g_dev_attr_t *attr,char *iccid)
{
    if (NULL == hal_if_modem_4g.iccid_check)
	return hal_modem_4g_iccid_check_common(attr,iccid);

    return hal_if_modem_4g.iccid_check(attr,iccid);
}

int hal_modem_4g_sim_card_detect(hal_modem_4g_dev_attr_t *attr)
{
    if (NULL == hal_if_modem_4g.sim_card_detect)
	return hal_modem_4g_sim_card_detect_common(attr);

    return hal_if_modem_4g.sim_card_detect(attr);
}

int hal_modem_4g_imsi_check(hal_modem_4g_dev_attr_t *attr,char *imsi)
{
    if (NULL == hal_if_modem_4g.imsi_check)
	return hal_modem_4g_imsi_check_common(attr,imsi);

    return hal_if_modem_4g.imsi_check(attr,imsi);
}

int hal_modem_4g_signal_quality_check(hal_modem_4g_dev_attr_t *attr,int *rssi,int *ber)
{
    if (NULL == hal_if_modem_4g.signal_quality_check)
	return hal_modem_4g_signal_quality_check_common(attr,rssi,ber);
	
    return hal_if_modem_4g.signal_quality_check(attr,rssi,ber);
}

int hal_modem_4g_apn_set(hal_modem_4g_dev_attr_t *attr, char *apn)
{
   if (NULL == hal_if_modem_4g.apn_set)
	return hal_modem_4g_apn_set_common(attr, apn);
           
   return hal_if_modem_4g.apn_set(attr);
}

int hal_modem_4g_user_set(hal_modem_4g_dev_attr_t *attr, char *apn, char *username, char *password)
{
	return hal_modem_4g_set_user_common(attr, apn, username, password);
}

int hal_modem_4g_reset(hal_modem_4g_dev_attr_t *attr)
{
   if (NULL == hal_if_modem_4g.reset)
      return hal_modem_4g_reset_common(attr);

   return hal_if_modem_4g.apn_set(attr);
}

int hal_modem_4g_send_message(hal_modem_4g_dev_attr_t *attr, char *phone_num, char *mesg_buf)
{
	if (NULL == hal_if_modem_4g.send_message)
		return hal_modem_4g_send_message_common(attr, phone_num, mesg_buf);
           
	return hal_if_modem_4g.send_message(attr, phone_num, mesg_buf);
}

int hal_modem_4g_voice_call(hal_modem_4g_dev_attr_t *attr, char *phone_num)
{
	if (NULL == hal_if_modem_4g.voice_call)
		return hal_modem_4g_voice_call_common(attr, phone_num);
           
	return hal_if_modem_4g.voice_call(attr, phone_num);
}

int hal_modem_4g_get_call_status(hal_modem_4g_dev_attr_t *attr, char *status)
{
	if (NULL == hal_if_modem_4g.get_call_status)
		return hal_modem_4g_get_call_status_common(attr, status);

	return hal_if_modem_4g.get_call_status(attr, status);
}

int hal_modem_4g_voice_hang_up(hal_modem_4g_dev_attr_t *attr)
{
	if (NULL == hal_if_modem_4g.voice_hang_up)
		return hal_modem_4g_voice_hang_up_common(attr);
           
	return hal_if_modem_4g.voice_hang_up(attr);
}

int hal_modem_4g_get_dtmf_value(hal_modem_4g_dev_attr_t *attr, int *value, int timeout)
{
	if (NULL == hal_if_modem_4g.get_dtmf_value)
		return hal_modem_4g_get_dtmf_value_common(attr, value, timeout);

	return hal_if_modem_4g.get_dtmf_value(attr, value, timeout);
}

int hal_modem_4g_get_phone_number(hal_modem_4g_dev_attr_t *attr, char *phone_num)
{
	if (NULL == hal_if_modem_4g.get_phone_number)
		return hal_modem_4g_get_phone_number_common(attr, phone_num);

	return hal_if_modem_4g.get_phone_number(attr, phone_num);
}

int hal_modem_4g_get_firmware_version(hal_modem_4g_dev_attr_t *attr, char *version)
{
	if (NULL == hal_if_modem_4g.get_firmware_version)
		return hal_modem_4g_get_firmware_version_common(attr, version);

	return hal_if_modem_4g.get_firmware_version(attr, version);
}

int hal_modem_4g_connect_net(hal_modem_4g_dev_attr_t *attr, int delay_ms)
{
   if (NULL == hal_if_modem_4g.connect_net)
	return hal_modem_4g_connect_net_common(attr, delay_ms);
   return hal_if_modem_4g.connect_net(attr, delay_ms);
}

int hal_modem_4g_disconnect_net(hal_modem_4g_dev_attr_t *attr)
{
   if (NULL == hal_if_modem_4g.disconnect_net)
	return hal_modem_4g_disconnect_net_common(attr);
   return hal_if_modem_4g.disconnect_net(attr);
}

int hal_modem_4g_dail_up(hal_modem_4g_dev_attr_t *attr)
{
   if (NULL == hal_if_modem_4g.dail_up)
	return hal_modem_4g_dail_up_common(attr);
   return hal_if_modem_4g.dail_up(attr);
}
