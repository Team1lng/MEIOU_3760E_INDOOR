/**
 * @file tuya_wifi_hal.c
 * @brief WIFI设备操作接口
 * 
 * @copyright Copyright(C),2018-2020, 涂鸦科技 www.tuya.com
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/socket.h>
#include <stdint.h>
#include <stdbool.h>
#include "tuya_os_adapter.h"
#include "ty_net_hal.h"
#include "tuya_wifi_hal.h"
#include "ty_net_log.h"
/**
 * @brief scan current environment and obtain all the ap
 *        infos in current environment
 * 
 * @param[out]      ap_ary      current ap info array
 * @param[out]      num         the num of ar_ary
 * @return  OPRT_OK: success  Other: fail
 */
int tuya_wifi_hal_all_ap_scan(WF_HAL_AP_IF_S **ap_ary, uint32_t *num)
{
    if(ty_net_hal_scan_all_ap(ap_ary,num)){
        TN_LOGE("ty_net_hal_scan_all_ap fail \n");
            return OPRT_COM_ERROR;
    }

    return OPRT_OK;
}

/**
 * @brief scan current environment and obtain the specific
 *        ap info.
 * 
 * @param[in]       ssid        the specific ssid
 * @param[out]      ap          the ap info
 * @return  OPRT_OK: success  Other: fail
 */
int tuya_wifi_hal_assign_ap_scan(const char *ssid, WF_HAL_AP_IF_S **ap)
{
	if(ty_net_hal_scan_assigned_ap(ssid,ap)) {
		TN_LOGE("ty_net_hal_scan_assigned_ap fail\n");
		return OPRT_COM_ERROR;
	}

	return OPRT_OK;	
}

/**
 * @brief set wifi interface work channel
 * 
 * @param[in]       chan        the channel to set
 * @return  OPRT_OK: success  Other: fail
 */
int tuya_wifi_hal_set_cur_channel(const uint8_t chan)
{
	int ret  = OPRT_OK;

	if(ty_net_hal_set_cur_channel(chan))
		ret = OPRT_COM_ERROR;

	return ret;	
}

/**
 * @brief get wifi interface work channel
 * 
 * @param[out]      chan        the channel wifi works
 * @return  OPRT_OK: success  Other: fail
 */
int tuya_wifi_hal_get_cur_channel(uint8_t *chan)
{
	int ret = OPRT_OK;

	if(ty_net_hal_get_cur_channel(chan))
		ret = OPRT_COM_ERROR;

	return ret;	
}

/**
 * @brief enable / disable wifi sniffer mode.
 *        if wifi sniffer mode is enabled, wifi recv from
 *        packages from the air, and user shoud send these
 *        packages to tuya-sdk with callback <cb>.
 * 
 * @param[in]       en          enable or disable
 * @param[in]       cb          notify callback
 * @return  OPRT_OK: success  Other: fail
 */
int tuya_wifi_hal_sniffer_set(const bool en, const WF_SNIFFER_CALLBACK cb)
{
	int ret = OPRT_OK;

	if(ty_net_hal_set_sniffer((int)en,(const SNIFFER_CB)cb))
		ret = OPRT_COM_ERROR;

	return ret;	
}

/**
 * @brief get wifi ip info.when wifi works in
 *        ap+station mode, wifi has two ips.
 * 
 * @param[in]       wf          wifi function type
 * @param[out]      ip          the ip addr info
 * @return  OPRT_OK: success  Other: fail
 */
int tuya_wifi_hal_get_ip(const WF_HAL_IF_E wf, NW_IP_S *ip)
{
	int ret = OPRT_OK;

	if(ty_net_hal_get_ip(0,ip->ip))
		ret = OPRT_COM_ERROR;

	TN_LOGI("WIFI Get IP:%s", ip->ip);

	return ret;
}
/**
 * @brief set wifi ip info.when wifi works in
 *        ap+station mode, wifi has two ips.
 * 
 * @param[in]       wf          wifi function type
 * @param[in]       ip          the ip addr info
 * @return  OPRT_OK: success  Other: fail
 */
int tuya_wifi_hal_set_ip(const WF_HAL_IF_E wf, const NW_IP_S *ip)
{
	int ret = OPRT_OK;

	if(ty_net_hal_set_ip(0,ip->ip))
		ret = OPRT_COM_ERROR;

	TN_LOGI("WIFI Get IP:%s", ip->ip);

	return ret;
}

/**
 * @brief get wifi bssid
 * 
 * @param[out]      mac         uplink mac
 * @return  OPRT_OK: success  Other: fail
 */
int tuya_wifi_hal_get_bssid(uint8_t mac[6])
{
	int ret;
    ret = ty_net_hal_get_bssid(mac);
    if(ret < 0)
    {
        TN_LOGD("Get bssid failed");
        return -1;
    }

    return 0;
}

/**
 * @brief set wifi mac info.when wifi works in
 *        ap+station mode, wifi has two macs.
 * 
 * @param[in]       wf          wifi function type
 * @param[in]       mac         the mac info
 * @return  OPRT_OK: success  Other: fail
 */
int tuya_wifi_hal_set_mac(const WF_HAL_IF_E wf, const NW_MAC_S *mac)
{
	int ret = OPRT_OK;

	if(ty_net_hal_set_mac(0,(char*)mac->mac))
		ret = OPRT_COM_ERROR;

	return ret;	
}

/**
 * @brief get wifi mac info.when wifi works in
 *        ap+station mode, wifi has two macs.
 * 
 * @param[in]       wf          wifi function type
 * @param[out]      mac         the mac info
 * @return  OPRT_OS_ADAPTER_OK: success  Other: fail
 */
int tuya_wifi_hal_get_mac(const WF_HAL_IF_E wf, NW_MAC_S *mac)
{
    int ret = OPRT_OK;

    if(ty_net_hal_get_mac(0,(char*)mac->mac))
        ret = OPRT_COM_ERROR;

    TN_LOGI("WIFI Get MAC:%02x:%02x:%02x:%02x:%02x:%02x\n",mac->mac[0],mac->mac[1],mac->mac[2],mac->mac[3],mac->mac[4],mac->mac[5]);

    return ret;
}

/**
 * @brief set wifi work mode
 * 
 * @param[in]       mode        wifi work mode
 * @return  OPRT_OK: success  Other: fail
 */
int tuya_wifi_hal_set_work_mode(const WF_HAL_WK_MD_E mode)
{
	switch (mode) {
		case WF_WWM_LOWPOWER:
			break;
		case WF_WWM_SNIFFER:
			ty_net_hal_set_sniffer_mode(0);
			break;
		case WF_WWM_STATION:
			ty_net_hal_set_station_mode(0);
			break;
		case WF_WWM_SOFTAP:
			ty_net_hal_set_softap_mode(0);
			break;
		case WF_WWM_STATIONAP:
			break;
		default:
			break;
	}

	TN_LOGD("WIFI Set Mode:%d", mode);
	return OPRT_OK;
}

/**
 * @brief get wifi work mode
 * 
 * @param[out]      mode        wifi work mode
 * @return  OPRT_OK: success  Other: fail
 */
int tuya_wifi_hal_get_work_mode(WF_HAL_WK_MD_E *mode)
{
	WIFI_MODE_E wifi_mode;
	*mode = WF_WWM_STATION;

	if(ty_net_hal_get_work_mode(0,&wifi_mode)) {
		TN_LOGE("ty wifi get mode fail\n");
		return OPRT_COM_ERROR;
	}

	if(wifi_mode == WIFI_MODE_MANAGED) {
		*mode = WF_WWM_STATION;	
	}else if (wifi_mode == WIFI_MODE_MASTER) {
		*mode = WF_WWM_SOFTAP;
	}else if(wifi_mode == WIFI_MODE_MONITOR) {
		*mode = WF_WWM_SNIFFER;	
	}

	TN_LOGI("WIFI Get Mode:%d", *mode);

	return OPRT_OK;	
}

/**
 * @brief connect wifi with ssid and passwd
 * 
 * @param[in]       ssid
 * @param[in]       passwd
 * @return  OPRT_OK: success  Other: fail
 */
int tuya_wifi_hal_station_connect(const char *ssid, const char *passwd)
{
    if (ty_net_hal_start_station_connect(ssid, passwd)) {
        TN_LOGE("ty_net_hal_start_station_connect fail \n");
        return OPRT_COM_ERROR;
    }

    return OPRT_OK;
}

/**
 * @brief disconnect wifi from connect ap
 * 
 * @return  OPRT_OK: success  Other: fail
 */
int tuya_wifi_hal_station_disconnect(void)
{
    int ret = OPRT_OK;

    if(ty_net_hal_station_disconnect())
        ret = OPRT_COM_ERROR;

    return ret;
}

/**
 * @brief get wifi connect rssi
 * 
 * @param[out]      rssi        the return rssi
 * @return  OPRT_OK: success  Other: fail
 */
int tuya_wifi_hal_station_get_conn_ap_rssi(int8_t *rssi)
{
    if(ty_net_hal_get_conn_ap_rssi(rssi)) {
        TN_LOGE("get curr rssi fail\n");
        return OPRT_COM_ERROR;
    }

    TN_LOGD("Get Conn AP RSSI:%d", *rssi);

    return OPRT_OK;
}

/**
 * @brief get assign ap rssi
 * 
 * @param[in]      ssid        the assign ap's ssid
 * @param[out]      rssi        the return rssi
 * @return  OPRT_OK: success  Other: fail
 */
int tuya_wifi_hal_get_assign_ap_rssi(char *ssid, int *rssi)
{
	if(ty_net_hal_get_assign_ap_rssi(ssid,rssi))
	{
		TN_LOGE("get assign_ap_rssi fail\n");
		return OPRT_COM_ERROR;
	}
	
	TN_LOGD("Get assigned AP RSSI:%d", *rssi);

	return OPRT_OK;
}

/**
 * @brief get assign ap's rssi [only for product test]
 * 
 * @param[in]      ssid        the assign ap's ssid
 * @param[out]      rssi        the return rssi
 * @return  OPRT_OK: success  Other: fail
 */
int tuya_wifi_hal_get_assign_ap_rssi_for_product_test(char *ssid, int *rssi)
{
	if(ty_net_hal_product_test_get_assign_ap_rssi(ssid,rssi))
	{
		TN_LOGE("get assign_ap_rssi fail\n");
		return OPRT_COM_ERROR;
	}
	
	TN_LOGD("Get assigned AP RSSI:%d", *rssi);

	return OPRT_OK;
}

/**
 * @brief get wifi local mac
 * 
 * @param[in,out]   mac    the return local mac
 * @return  OPRT_OK: success  Other: fail
 */
int tuya_wifi_hal_station_get_local_mac(NW_MAC_S *mac)
{
	int ret = OPRT_OK;

	if(ty_net_hal_get_mac(0,(char*)mac->mac))
		ret = OPRT_COM_ERROR;

	TN_LOGI("WIFI Get MAC:%02x:%02x:%02x:%02x:%02x:%02x\n",mac->mac[0],mac->mac[1],mac->mac[2],mac->mac[3],mac->mac[4],mac->mac[5]);

	return ret;
}

/**
 * @brief get wifi station work status
 * 
 * @param[out]      stat        the wifi station work status
 * @return  OPRT_OK: success  Other: fail
 */
int tuya_wifi_hal_station_get_status(WF_HAL_STATION_STAT_E *stat)
{
	int ret = OPRT_OK;

	if(ty_net_hal_get_status(stat))
		ret = OPRT_COM_ERROR;

	return ret;	
}

/**
 * @brief start a soft ap
 * 
 * @param[in]       cfg         the soft ap config
 * @return  OPRT_OK: success  Other: fail
 */
int tuya_wifi_hal_ap_start(const WF_HAL_AP_CFG_IF_S *cfg)
{
	int channel;
	int ret = OPRT_OK;

	ty_net_hal_stop_ap();

	if(NULL == cfg) {
		return OPRT_OS_ADAPTER_INVALID_PARM;
	}

	if(cfg->chan <=0 || cfg->chan >= 14)
		channel = 6;
	else
		channel = cfg->chan;

	TN_LOGD("Start AP SSID:%s,PSK:%s,CHANNEL:%d\n", cfg->ssid,cfg->passwd,channel);

	if(ty_net_hal_start_ap(channel,(char *)cfg->ssid,(char *)cfg->passwd,(char*)cfg->ip.ip))
		ret = OPRT_COM_ERROR;

	return ret;	
}

/**
 * @brief stop a soft ap
 * 
 * @return  OPRT_OK: success  Other: fail
 */
int tuya_wifi_hal_ap_stop(void)
{
	int ret = OPRT_OK;

	TN_LOGD("Stop Ap Mode");

	if(ty_net_hal_stop_ap())
		ret = OPRT_COM_ERROR;

	return ret;
}

/**
 * @brief set wifi country code
 * 
 * @param[in]       p_country_code  country code
 * @return  OPRT_OK: success  Other: fail
 */
int tuya_wifi_hal_set_country_code(const WF_HAL_COUNTRY_CODE_E ccode)
{
	int ret = OPRT_OK;

	if(ty_net_hal_set_country_code(ccode))
		ret = OPRT_COM_ERROR;

	return ret;		
}


/**
 * @brief get wifi country code
 * 
 * @return  WF_HAL_COUNTRY_CODE_E: country code
 */
WF_HAL_COUNTRY_CODE_E tuya_wifi_hal_get_cur_country_code(void)
{
	int ccode = OPRT_OK;

	ccode = ty_net_hal_get_country_code();
	if(-1 == ccode)
		ccode = OPRT_COM_ERROR;

	return ccode;		
}

/**
 * @brief send wifi management
 * 
 * @param[in]       buf         pointer to buffer
 * @param[in]       len         length of buffer
 * @return  OPRT_OK: success  Other: fail
 */
int tuya_wifi_hal_send_mgnt(const uint8_t *buf, const uint32_t len)
{
	int ret = OPRT_OK;

	if(ty_net_hal_send_mgnt(buf,len))
		ret = OPRT_COM_ERROR;

	return ret;	
}

/**
 * @brief register receive wifi management callback
 * 
 * @param[in]       enable
 * @param[in]       recv_cb     receive callback
 * @return  OPRT_OK: success  Other: fail
 */
int tuya_wifi_hal_register_recv_mgnt_callback(bool enable, WIFI_REV_MGNT_CB_T recv_cb)
{
	int ret = OPRT_OK;

	if(ty_net_hal_register_recv_mgnt_cb(enable,recv_cb))
		ret = OPRT_COM_ERROR;

	return ret;	
}

