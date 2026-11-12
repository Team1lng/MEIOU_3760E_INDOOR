#ifndef __TKL_GEN_NET_H__
#define __TKL_GEN_NET_H__

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_REV_BUFFER      (512)

/* wifi work mode */
typedef enum {
    WIFI_MODE_AUTO = 0,
    WIFI_MODE_AD_HOC,
    WIFI_MODE_MANAGED,
    WIFI_MODE_MASTER,
    WIFI_MODE_REPEATER,
    WIFI_MODE_SECONDARY,
    WIFI_MODE_MONITOR,
    WIFI_MODE_NR,
}WIFI_MODE_E;

typedef struct {
    UCHAR_T ssid[WIFI_SSID_LEN+1];
    UCHAR_T passwd[WIFI_PASSWD_LEN+1];
    UCHAR_T bssid[6];
    UINT_T security;
    UCHAR_T chan; 
    UCHAR_T psk_set;
}FAST_CONNECTED_AP_INFO_S;
/* realtek wifi 特有的头部 */
struct rtl_wifi_header{
    unsigned short  pkt_len;
    unsigned short  payload_len;
    unsigned char   data[252];
}__attribute__((packed));

#pragma pack(1)
typedef struct wlan_80211_frame {
    unsigned char frmae_ctl1;
    unsigned char frmae_ctl2;
    unsigned short duration;
    unsigned char addr1[6];
    unsigned char addr2[6];
    unsigned char addr3[6];
    unsigned short seq_ctl;
    unsigned char addr4[6];
}WLAN_80211_HEAD;
#pragma pack()

typedef void (*SNIFFER_CB)(const uint8_t *buf, const uint16_t len);

int tkl_gen_wifi_init(WIFI_EVENT_CB cb);
int tkl_gen_wifi_deinit(void);

int tkl_gen_ap_scan(CONST SCHAR_T *ssid, AP_IF_S **ap_ary, UINT_T *num);
int tkl_gen_net_set_cur_channel(const char channel);
int tkl_gen_net_get_cur_channel(unsigned char *channel);
int tkl_gen_wifi_get_ipv4(CONST WF_IF_E wf, NW_IP_S *ip);
int tkl_gen_wifi_get_ipv6(CONST WF_IF_E wf, NW_IP_TYPE type, NW_IP_S *ip);
int tkl_gen_set_mac(CONST WF_IF_E wf, CONST NW_MAC_S *mac);
int tkl_gen_get_mac(CONST WF_IF_E wf, NW_MAC_S *mac);
int tkl_gen_set_work_mode(CONST WF_WK_MD_E mode);
int tkl_gen_get_work_mode(WF_WK_MD_E *mode);
int tkl_gen_ap_start(CONST WF_AP_CFG_IF_S *cfg);
int tkl_gen_ap_stop(void);
int tkl_gen_get_connected_ap_info(FAST_WF_CONNECTED_AP_INFO_T **fast_ap_info);
int tkl_gen_fast_station_connect(CONST FAST_WF_CONNECTED_AP_INFO_T *fast_ap_info);
int tkl_gen_station_connect(CONST SCHAR_T *ssid, CONST SCHAR_T *passwd);
int tkl_gen_station_disconnect(VOID_T);
int tkl_gen_station_get_conn_ap_rssi(SCHAR_T *rssi);
int tkl_gen_get_bssid(unsigned char *mac);
int tkl_gen_station_get_status(WF_STATION_STAT_E *stat);
int tkl_gen_set_country_code(CONST COUNTRY_CODE_E ccode);
int tkl_gen_net_set_sniffer(const int en, const SNIFFER_CB cb);
int tkl_gen_send_mgnt(CONST UCHAR_T *buf, CONST UINT_T len);
int tkl_gen_register_recv_mgnt_callback(CONST BOOL_T enable, CONST WIFI_REV_MGNT_CB recv_cb);
int tkl_gen_set_lp_mode(CONST BOOL_T en, CONST UINT_T dtim);
BOOL_T tkl_gen_rf_calibrated(VOID_T);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

