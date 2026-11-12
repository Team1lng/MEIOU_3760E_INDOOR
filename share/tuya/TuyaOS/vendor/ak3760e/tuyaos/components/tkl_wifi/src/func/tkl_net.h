#ifndef __TKL_NET_H__
#define __TKL_NET_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifndef WLAN_DEV
#define WLAN_DEV    "wlan0"
#endif

/* 8197设备,开启ap+ez配网时,wlan0做AP,wlan0-vxd做station */
#ifndef WLAN_MONITOR
#define WLAN_MONITOR "wlan0-vxd"
#endif

#ifndef WLAN_AP
#define WLAN_AP    "wlan1"
#endif

#ifndef WIFI_AP_DEFAULT_IP
#define WIFI_AP_DEFAULT_IP  "192.168.31.1"
#endif

#define MAX_AP_SEARCH       (128)
#define MAX_REV_BUFFER      (512)
#define MAX_SSID_LEN        (32)
#define MAX_IP_LEN          (16)
#define MAX_IFNAME_SZ       (16)

#define UDHCPC6                 "udhcpc6"
#define UDHCPC                  "udhcpc"
#define WPA_SUPPLICANT          "wpa_supplicant"
#define WIFI_DHCPD_CONF         "/tuya/data/udhcpd.conf"
#define WPA_CTRL_DIR            "/var/run"
#define WPA_CTRL_INTERFACE      "/var/run/wpa_supplicant"
#ifdef CFG_PATH_TMP
#define WIFI_STATION_CONF       "/tmp/wpa_0_8.conf"
#else
#define WIFI_STATION_CONF       "/tuya/data/wpa_0_8.conf"
#endif
#define WIFI_HOSTAPD_CONF       "/tuya/data/hostapd.conf"
#define CREATE_UDHCPD_DIR       "/bin/mkdir -p /var/lib/misc"
#define CREATE_UDHCPD_CFG_FILE  "/bin/touch /var/lib/misc/udhcpd.leases"

typedef enum {
    WIFI_8188 = 0,
    WIFI_8192,
    WIFI_8723DS,
    WIFI_SSW101B,
    WIFI_HI3881,
    WIFI_RTL8197,
    WIFI_TYPE_NR,
}WIFI_TYPE_E;

typedef struct IP_UDHCPD {
    char ifname[MAX_IFNAME_SZ];
    char ip_start[MAX_IP_LEN];
    char ip_end[MAX_IP_LEN];
    char ip_route[MAX_IP_LEN];
    char ip_mask[MAX_IP_LEN];
    char ip_gw[MAX_IP_LEN];
} UDHCPD_IP_T;

int tkl_net_wifi_enable(char *ifname , char enable);
int tkl_net_get_ipv4(char *ifname, char *ip);
int tkl_net_get_ipv6(char *ifname, NW_IP_TYPE type, NW_IP_S  *ip);
int tkl_net_get_mask_ipv4(char *ifname, char *mask);
int ip_prepare(char *src,char *dst);
int tkl_net_set_ip(const char *ifname, const char *ip);
int tkl_net_get_mac(char *ifname, char *mac);
int tkl_net_set_mac(const char *ifname, const char *mac);
int tkl_net_set_chn(char *ifname, const char channel);
int tkl_net_get_chn(char *ifname, unsigned char *channel);
int tkl_net_get_mode(char *ifname , int *mode);
int tkl_net_set_mode(char *ifname , char *mode);
int tkl_net_wifi_scan(char *ifname , AP_IF_S *aps , int max_ap);
int tkl_net_get_curr_rssi(char *ifname,char *rssi);
int tkl_net_essid_on(char *ifname);
int tkl_net_get_connect_ap_ssid(char *ssid);
WIFI_TYPE_E tkl_net_get_wifi_type(void);
int tkl_net_get_bssid(unsigned char *bssid);
int tkl_net_get_connected_ap_info(char *ssid,char *passwd);
int tkl_net_start_ap_connect(const char *ssid, const char *passwd);
int tkl_net_sta_disconnect(char *ifname);
int tkl_net_ap_start(const WF_AP_CFG_IF_S *cfg);
int tkl_net_ap_stop(void);
int tkl_net_hostapd_conf_prepare(const WF_AP_CFG_IF_S *cfg);
int tkl_net_create_rawsocket(int protocol_to_sniff);
int tkl_net_bind_rawsocket(char *device, int rawsock, int protocol);
int tkl_net_send_rawpacket(int rawsock, unsigned char *pkt, size_t pkt_len);
int tkl_net_recv_rawpacket(int rawsock, unsigned char *pkt, int pkt_len);

int udhcpd_conf_prepare(UDHCPD_IP_T udhcpd_ip_t);
void tkl_net_8197_start_dhcp(char *ifname);
int tkl_net_set_mac_8723( const char *ifname,const char *mac);
int tkl_net_set_country(char *country);
int tkl_net_set_country_rtl8188(char *country);
int tkl_net_set_sniffer_mode(int index);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

