#ifndef TY_NET_HAL_H_
#define TY_NET_HAL_H_

#include <inttypes.h>
#include "ty_net.h"

typedef enum {
	WIFI_8188 = 0,
	WIFI_8192,
	WIFI_8723DS,
	WIFI_SSW101B,
	WIFI_HI3881,
	WIFI_TYPE_NR,
}WIFI_TYPE_E;

typedef enum {
	CN = 0,
	US,
	JP,
	EU,
	COUNTRY_NR,
}WIFI_COUNTRY_E;

typedef enum {
 WIFI_MODE_AUTO=0,
 WIFI_MODE_AD_HOC,
 WIFI_MODE_MANAGED,
 WIFI_MODE_MASTER,
 WIFI_MODE_REPEATER,
 WIFI_MODE_SECONDARY,
 WIFI_MODE_MONITOR,
 WIFI_MODE_NR,
}WIFI_MODE_E;

typedef struct {
	int		c_code;
	int 	channel_s;
	int 	charnel_e;
}COUNTRY_CODE_S;

typedef struct {
	int 	opmodevalue;
}WIFI_MODE_S;

typedef void (*SNIFFER_CB)(const uint8_t *buf, const uint16_t len);

extern int ty_net_hal_send_mgnt(const uint8_t *buf, const uint32_t len);
extern int ty_net_hal_close_concurrent_ap(void);
extern int ty_net_hal_register_recv_mgnt_cb(int enable,WIFI_REV_MGNT_CB_T recv_cb);
extern int ty_net_hal_disable_wifi_lowpower(void);

extern int ty_net_hal_get_country_code(void);
extern int ty_net_hal_set_country_code(const WF_HAL_COUNTRY_CODE_E ccode);

extern int ty_net_hal_start_ap(int channel,char *ssid,char *passwd,char *ip);
extern int ty_net_hal_stop_ap(void);

extern int ty_net_hal_get_status(WF_HAL_STATION_STAT_E*stat);
extern int ty_net_hal_get_conn_ap_rssi(int8_t *rssi);

extern int ty_net_hal_start_station_connect(const char *ssid, const char *passwd);
extern int ty_net_hal_station_disconnect(void);

extern int ty_net_hal_get_work_mode(int index,WIFI_MODE_E *mode);
extern int ty_net_hal_set_station_mode(int index);
extern int ty_net_hal_set_sniffer_mode(int index);
extern int ty_net_hal_set_softap_mode(int index);

extern int ty_net_hal_set_mac(int index, uint8_t*mac);
extern int ty_net_hal_get_mac(int index,uint8_t*mac);

extern int ty_net_hal_get_ip(int index,char *ip);
extern int ty_net_hal_set_ip(int index,const char *ip);

extern int ty_net_hal_set_sniffer(const int en, const SNIFFER_CB cb);
extern int ty_net_hal_get_cur_channel(uint8_t *channel);
extern int ty_net_hal_set_cur_channel(const uint8_t channel);
extern int ty_net_hal_scan_assigned_ap(const char *ssid, WF_HAL_AP_IF_S **ap);
extern int ty_net_hal_scan_all_ap(WF_HAL_AP_IF_S**ap_ary, uint32_t *num);

extern int ty_net_hal_wifi_station_get_ap_mac(int index, uint8_t* mac);

#endif
