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

#include "tkl_wifi.h"
#include "tkl_wifi_log.h"
#include "tkl_gen_net.h"

#include "Amy_log.h"
#include "Amy_sdk.h"

TUYA_WEAK_ATTRIBUTE OPERATE_RET tkl_wifi_init(WIFI_EVENT_CB cb) {
#if 0
    int iret = 0;
#ifdef DEBUG_VERSION
    TKL_LOGD("#####:  func=%s, line=%d\n", __func__, __LINE__);
#endif
    if (cb == NULL) {
        TKL_LOGE("callbask func is null");
        return OPRT_INVALID_PARM;
    }
    iret = tkl_gen_wifi_init(cb);

    if (iret < 0) {
        TKL_LOGE("TKL WiFi : wifi init failed");
        return OPRT_COM_ERROR;
    }
#endif
    AMY_LOG_INFO("<<<-------------------------------\n");
    return OPRT_OK;
}

TUYA_WEAK_ATTRIBUTE OPERATE_RET tkl_wifi_deinit(VOID_T)
{
#if 0
#ifdef DEBUG_VERSION
    TKL_LOGD("#####:  func=%s, line=%d\n", __func__, __LINE__);
#endif
    tkl_gen_wifi_deinit();
#endif
    AMY_LOG_INFO("<<<-------------------------------\n");
    return OPRT_OK;
}


TUYA_WEAK_ATTRIBUTE OPERATE_RET tkl_wifi_scan_ap(CONST SCHAR_T* ssid, AP_IF_S** ap_ary, UINT_T* num) {
#if 0
    int iret = 0;
#ifdef DEBUG_VERSION
    TKL_LOGD("#####:  func=%s, line=%d\n", __func__, __LINE__);
#endif
    TKL_LOGD("#####:  test func=%s, line=%d\n", __func__, __LINE__);
    iret = tkl_gen_ap_scan(ssid, ap_ary, num);
    if (iret < 0) {
        TKL_LOGE("scan ap failed");
        return OPRT_COM_ERROR;
    }
#endif
    AMY_LOG_INFO("<<<-------------------------------\n");
    return OPRT_OK;
}

TUYA_WEAK_ATTRIBUTE OPERATE_RET tkl_wifi_release_ap(AP_IF_S* ap)
{
#if  0
#ifdef DEBUG_VERSION
    TKL_LOGD("#####:  func=%s, line=%d\n", __func__, __LINE__);
#endif
    if (ap == NULL) {
        TKL_LOGE("wrong param");
        return OPRT_INVALID_PARM;
    }

    TKL_LOGD("#####:  func=%s, line=%d, ap=%p\n", __func__, __LINE__, ap);
    free(ap);
    ap = NULL;
#endif
    AMY_LOG_INFO("<<<-------------------------------\n");
    return OPRT_OK;
}

TUYA_WEAK_ATTRIBUTE OPERATE_RET tkl_wifi_set_cur_channel(CONST UCHAR_T chan) {
#if 0
    int iret = 0;

#ifdef DEBUG_VERSION
    TKL_LOGD("#####:  func=%s, line=%d\n", __func__, __LINE__);
#endif
    iret = tkl_gen_net_set_cur_channel(chan);
    if (iret < 0) {
        TKL_LOGE("set channel failed");
        return OPRT_COM_ERROR;
    }
#endif
    AMY_LOG_INFO("<<<-------------------------------\n");
    return OPRT_OK;
}

TUYA_WEAK_ATTRIBUTE OPERATE_RET tkl_wifi_get_cur_channel(UCHAR_T* chan) {
#if 0
    int iret = 0;
#ifdef DEBUG_VERSION
    TKL_LOGD("#####:  func=%s, line=%d\n", __func__, __LINE__);
#endif
    iret = tkl_gen_net_get_cur_channel(chan);
    if (iret < 0) {
        TKL_LOGE("get channel failed");
        return OPRT_COM_ERROR;
    }
#endif
    AMY_LOG_INFO("<<<-------------------------------\n");
    return OPRT_OK;
}

TUYA_WEAK_ATTRIBUTE OPERATE_RET tkl_wifi_set_sniffer(CONST BOOL_T en, CONST SNIFFER_CALLBACK cb) {
#if 0
    int iret = 0;
#ifdef DEBUG_VERSION
    TKL_LOGD("#####:  func=%s, line=%d\n", __func__, __LINE__);
#endif
    iret = tkl_gen_net_set_sniffer((int)en, (const SNIFFER_CB)cb);
    if (iret < 0) {
        TKL_LOGE("set wifi sniffer failed");
        return OPRT_COM_ERROR;
    }
#endif
    AMY_LOG_INFO("<<<-------------------------------\n");
    return OPRT_OK;
}

static OPERATE_RET wifi_get_ipv4(CONST WF_IF_E wf, NW_IP_S* ip)
{
    int iret = 0;

#ifdef DEBUG_VERSION
    TKL_LOGD("#####:  func=%s, line=%d\n", __func__, __LINE__);
#endif
    iret = tkl_gen_wifi_get_ipv4(wf, ip);
    if (iret < 0) {
        TKL_LOGE("get ipv4 failed");
        return OPRT_COM_ERROR;
    }
    return OPRT_OK;
}

static OPERATE_RET wifi_get_ipv6(CONST WF_IF_E wf, NW_IP_TYPE type, NW_IP_S* ip)
{
    int iret = 0;

#ifdef DEBUG_VERSION
    TKL_LOGD("#####:  func=%s, line=%d\n", __func__, __LINE__);
#endif
    iret = tkl_gen_wifi_get_ipv6(wf, type, ip);
    if (iret < 0) {
        TKL_LOGE("get ipv6 failed");
        return OPRT_COM_ERROR;
    }
    AMY_LOG_INFO("<<<-------------------------------\n");
    return OPRT_OK;
}

/**
 * @brief get wifi ipv6 info.when wifi works in
 *        ap+station mode, wifi has two ips.
 *
 * @param[in]       wf          wifi function type
 * @param[out]      ip          the ip addr info
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wifi_get_ipv6(CONST WF_IF_E wf, NW_IP_TYPE type, NW_IP_S* ip)
{
    AMY_LOG_INFO("<<<-------------------------------\n");
    return OPRT_NOT_SUPPORTED;
}

TUYA_WEAK_ATTRIBUTE OPERATE_RET tkl_wifi_get_ip(CONST WF_IF_E wf, NW_IP_S* ip)
{
    OPERATE_RET ret = OPRT_COM_ERROR;
    if (ip == NULL) {
        printf("%s %d: invalid param\n", __func__, __LINE__);
        return OPRT_INVALID_PARM;
    }

    if (TY_AF_INET == ip->type) {
        ret = wifi_get_ipv4(wf, ip);
    }
    else if (TY_AF_INET6 == ip->type) {
        ret = wifi_get_ipv6(wf, NW_IPV6, ip);
    }
    else {
        printf("%s %d: invalid param type=%d\n", __func__, __LINE__, ip->type);
    }
//    AMY_LOG_INFO("<<<-------------------------------\n");
    return ret;
}

TUYA_WEAK_ATTRIBUTE OPERATE_RET tkl_wifi_set_mac(CONST WF_IF_E wf, CONST NW_MAC_S* mac) {
#if 0
    int iret = 0;

#ifdef DEBUG_VERSION
    TKL_LOGD("#####:  func=%s, line=%d\n", __func__, __LINE__);
#endif
    iret = tkl_gen_set_mac(wf, mac);
    if (iret < 0) {
        TKL_LOGE("SET MAC failed");
        return OPRT_COM_ERROR;
    }
    else {
        TKL_LOGD("WiFi SET MAC success, MAC:%02x:%02x:%02x:%02x:%02x:%02x\n",
            mac->mac[0], mac->mac[1], mac->mac[2], mac->mac[3], mac->mac[4], mac->mac[5]);
    }
#endif
    AMY_LOG_INFO("<<<-------------------------------\n");
    return OPRT_OK;
}

TUYA_WEAK_ATTRIBUTE OPERATE_RET tkl_wifi_get_mac(CONST WF_IF_E wf, NW_MAC_S* mac)
{
    int iret = 0;

#ifdef DEBUG_VERSION
    TKL_LOGD("#####:  func=%s, line=%d\n", __func__, __LINE__);
#endif
    iret = tkl_gen_get_mac(wf, mac);
    if (iret < 0) {
        TKL_LOGE("GET MAC failed");
        return OPRT_COM_ERROR;
    }
    else {
        TKL_LOGD("GET MAC success, MAC:%02x:%02x:%02x:%02x:%02x:%02x\n",
            mac->mac[0], mac->mac[1], mac->mac[2], mac->mac[3], mac->mac[4], mac->mac[5]);
    }
    AMY_LOG_INFO("<<<-------------------------------\n");
    return OPRT_OK;
}

TUYA_WEAK_ATTRIBUTE OPERATE_RET tkl_wifi_set_work_mode(CONST WF_WK_MD_E mode) {
#if 0
    int iret = 0;
#ifdef DEBUG_VERSION
    TKL_LOGD("#####:  func=%s, line=%d\n", __func__, __LINE__);
#endif
    iret = tkl_gen_set_work_mode(mode);
    if (iret < 0) {
        TKL_LOGE("WIFI SET work mode failed");
        return OPRT_COM_ERROR;
    }
    else {
        switch (mode) {
        case WWM_POWERDOWN:
            TKL_LOGD("WIFI SET work mode success, set mode :wifi power down");
            break;
        case WWM_SNIFFER:
            TKL_LOGD("WIFI SET work mode success, set mode :sniffer mode");
            break;
        case WWM_STATION:
            TKL_LOGD("WIFI SET work mode success, set mode :station mode");
            break;
        case WWM_SOFTAP:
            TKL_LOGD("WIFI SET work mode success, set mode :softap mode");
            break;
        case WWM_STATIONAP:
            TKL_LOGD("WIFI SET work mode success, set mode :station on ap mode");
            break;
        default:
            break;
        }
    }
#endif
    AMY_LOG_INFO("<<<-------------------------------\n");
    return OPRT_OK;
}

TUYA_WEAK_ATTRIBUTE OPERATE_RET tkl_wifi_get_work_mode(WF_WK_MD_E* mode) {
#if 0
    int iret = 0;

#ifdef DEBUG_VERSION
    TKL_LOGD("#####:  func=%s, line=%d\n", __func__, __LINE__);
#endif
    iret = tkl_gen_get_work_mode(mode);
    if (iret < 0) {
        //TKL_LOGE("get work mode failed");
        return OPRT_COM_ERROR;
    }
#endif
    * mode = WWM_STATION;
    AMY_LOG_INFO("<<<-------------------------------\n");
    return OPRT_OK;
}

TUYA_WEAK_ATTRIBUTE OPERATE_RET tkl_wifi_start_ap(CONST WF_AP_CFG_IF_S* cfg) {
#if 0

    int iret = 0;
#ifdef DEBUG_VERSION
    TKL_LOGD("#####:  func=%s, line=%d\n", __func__, __LINE__);
#endif
    if (cfg == NULL) {
        TKL_LOGE("invalid parm");
        return OPRT_INVALID_PARM;
    }

    TKL_LOGD("Start AP SSID:%s ,CHANNEL:%d", cfg->ssid, cfg->chan);

    iret = tkl_gen_ap_start(cfg);
    if (iret < 0) {
        TKL_LOGE(" WIFI start ap failed");
        return OPRT_COM_ERROR;
    }
#endif
    AMY_LOG_INFO("<<<-------------------------------\n");
    return OPRT_OK;//OPRT_OK;
}

TUYA_WEAK_ATTRIBUTE OPERATE_RET tkl_wifi_stop_ap(VOID_T) {
#if 0
    int iret = 0;
#ifdef DEBUG_VERSION
    TKL_LOGD("#####:  func=%s, line=%d\n", __func__, __LINE__);
#endif

    iret = tkl_gen_ap_stop();
    if (iret < 0) {
        TKL_LOGE("WIFI STOP softap failed");
        return OPRT_COM_ERROR;
    }
#endif
    AMY_LOG_INFO("<<<-------------------------------\n");
    return OPRT_OK;
}

TUYA_WEAK_ATTRIBUTE OPERATE_RET tkl_wifi_get_connected_ap_info(FAST_WF_CONNECTED_AP_INFO_T** fast_ap_info) {
#if 0
    int iret = 0;

#ifdef DEBUG_VERSION
    TKL_LOGD("#####:  func=%s, line=%d\n", __func__, __LINE__);
#endif
    if (!fast_ap_info) {
        TKL_LOGE("WiFi : illegal parameter");
        return OPRT_INVALID_PARM;
    }
    iret = tkl_gen_get_connected_ap_info(fast_ap_info);
    if (iret < 0) {
        TKL_LOGE("TKL WIFI get connect ap info failed");
        return OPRT_COM_ERROR;
    }
#endif
    AMY_LOG_INFO("<<<-------------------------------\n");
    return OPRT_OK;
}

TUYA_WEAK_ATTRIBUTE OPERATE_RET tkl_wifi_station_fast_connect(CONST FAST_WF_CONNECTED_AP_INFO_T* fast_ap_info)
{
#if 0 
    int iret = 0;
#ifdef DEBUG_VERSION
    TKL_LOGD("#####:  func=%s, line=%d\n", __func__, __LINE__);
#endif
    return OPRT_COM_ERROR;
    iret = tkl_gen_fast_station_connect(fast_ap_info);
    if (iret < 0) {
        TKL_LOGE("WIFI fast connect failed");
        return OPRT_COM_ERROR;
    }
#endif
    AMY_LOG_INFO("<<<-------------------------------\n");
    return OPRT_OK;
}

TUYA_WEAK_ATTRIBUTE OPERATE_RET tkl_wifi_station_connect(CONST SCHAR_T* ssid, CONST SCHAR_T* passwd) {
#if 0
    int iret = 0;
#ifdef DEBUG_VERSION
    TKL_LOGD("#####:  func=%s, line=%d\n", __func__, __LINE__);
#endif
    iret = tkl_gen_station_connect(ssid, passwd);
    if (iret < 0) {
        TKL_LOGE("WIFI Station connect ap failed");
        return OPRT_COM_ERROR;
    }
#endif
    AMY_LOG_INFO("<<<-------------------------------\n");
    return OPRT_OK;
}

TUYA_WEAK_ATTRIBUTE OPERATE_RET tkl_wifi_station_disconnect(VOID_T) {
#if 0
    int iret = 0;
#ifdef DEBUG_VERSION
    TKL_LOGD("#####:  func=%s, line=%d\n", __func__, __LINE__);
#endif
    iret = tkl_gen_station_disconnect();
    if (iret < 0) {
        TKL_LOGE("TKL WIFI sta disconnect failed");
        return OPRT_COM_ERROR;
    }
#endif
    AMY_LOG_INFO("<<<-------------------------------\n");
    return OPRT_OK;
}

TUYA_WEAK_ATTRIBUTE OPERATE_RET tkl_wifi_station_get_conn_ap_rssi(SCHAR_T* rssi) {
#if  0
    int iret = 0;

#ifdef DEBUG_VERSION
    TKL_LOGD("#####:  func=%s, line=%d\n", __func__, __LINE__);
#endif
    if (!rssi) {
        TKL_LOGE("rssi is null");
        return OPRT_INVALID_PARM;
    }
    *rssi = 0;
    iret = tkl_gen_station_get_conn_ap_rssi(rssi);
    if (iret < 0) {
        TKL_LOGE("get connect ap's rssi failed");
        return OPRT_COM_ERROR;
    }
#endif
    AMY_LOG_INFO("<<<-------------------------------\n");
    return OPRT_OK;
}

TUYA_WEAK_ATTRIBUTE OPERATE_RET tkl_wifi_get_bssid(UCHAR_T* mac) {
#if 0
    int iret = 0;

#ifdef DEBUG_VERSION
    TKL_LOGD("#####:  func=%s, line=%d\n", __func__, __LINE__);
#endif
    iret = tkl_gen_get_bssid(mac);
    if (iret < 0) {
        TKL_LOGE("TKL WIFI Get bssid failed");
        return OPRT_COM_ERROR;
    }
#endif
    AMY_LOG_INFO("<<<-------------------------------\n");
    return OPRT_OK;
}

TUYA_WEAK_ATTRIBUTE OPERATE_RET tkl_wifi_station_get_status(WF_STATION_STAT_E* stat) {
#if 0
    int iret = 0;
#ifdef DEBUG_VERSION
    TKL_LOGD("#####:  func=%s, line=%d\n", __func__, __LINE__);
#endif
    iret = tkl_gen_station_get_status(stat);
    if (iret < 0) {
        TKL_LOGE("TKL WIFI Get status failed");
        return OPRT_COM_ERROR;
    }
#endif
    * stat = WSS_GOT_IP;
    //AMY_LOG_INFO("<<<-------------------------------\n");
    return OPRT_OK;
}

TUYA_WEAK_ATTRIBUTE OPERATE_RET tkl_wifi_set_country_code(CONST COUNTRY_CODE_E ccode) {
#if 0
    int iret = 0;

#ifdef DEBUG_VERSION
    TKL_LOGD("#####:  func=%s, line=%d\n", __func__, __LINE__);
#endif
    iret = tkl_gen_set_country_code(ccode);
    if (iret < 0) {
        TKL_LOGE("set country code failed");
        return OPRT_COM_ERROR;
    }
#endif
    return OPRT_OK;
}

TUYA_WEAK_ATTRIBUTE OPERATE_RET tkl_wifi_send_mgnt(CONST UCHAR_T* buf, CONST UINT_T len) {
#if 0
    int iret = 0;
#ifdef DEBUG_VERSION
    TKL_LOGD("#####:  func=%s, line=%d\n", __func__, __LINE__);
#endif
    iret = tkl_gen_send_mgnt(buf, len);
    if (iret < 0) {
        TKL_LOGE("WIFI send mgnt failed");
        return OPRT_SEND_ERR;
    }
#endif
    //AMY_LOG_INFO("<<<-------------------------------\n");
    return OPRT_OK;
}

TUYA_WEAK_ATTRIBUTE OPERATE_RET tkl_wifi_register_recv_mgnt_callback(CONST BOOL_T enable, CONST WIFI_REV_MGNT_CB recv_cb) {

#if 0
    int iret = 0;

#ifdef DEBUG_VERSION
    TKL_LOGD("#####:  func=%s, line=%d\n", __func__, __LINE__);
#endif
    iret = tkl_gen_register_recv_mgnt_callback(enable, recv_cb);
    if (iret < 0) {
        TKL_LOGE("WIFI recv mgnt failed");
        return OPRT_RECV_ERR;
    }
#endif
    AMY_LOG_INFO("<<<-------------------------------\n");
    return OPRT_OK;
}

TUYA_WEAK_ATTRIBUTE OPERATE_RET tkl_wifi_set_rf_calibrated(VOID_T)
{
#if 0
#ifdef DEBUG_VERSION
    TKL_LOGD("#####:  func=%s, line=%d\n", __func__, __LINE__);
#endif
    TKL_LOGD("set rf calibrated is not support");
#endif
    AMY_LOG_INFO("<<<-------------------------------\n");
    return OPRT_COM_ERROR;
}

TUYA_WEAK_ATTRIBUTE OPERATE_RET tkl_wifi_set_lp_mode(CONST BOOL_T enable, CONST UCHAR_T dtim)
{
#if 0
    int iret = 0;
#ifdef DEBUG_VERSION
    TKL_LOGD("#####:  func=%s, line=%d\n", __func__, __LINE__);
#endif
    iret = tkl_gen_set_lp_mode(enable, dtim);
    if (iret < 0) {
        TKL_LOGE("WiFi Set lp mode failed");
        return OPRT_RECV_ERR;
    }
#endif
    AMY_LOG_INFO("<<<-------------------------------\n");
    return OPRT_OK;
}

TUYA_WEAK_ATTRIBUTE OPERATE_RET tkl_wifi_ioctl(WF_IOCTL_CMD_E cmd, VOID* args)
{
    AMY_LOG_INFO("<<<-------------------------------cmd=%d\n", cmd);
    return OPRT_NOT_SUPPORTED;
}
