#ifndef __TY_NET_H__
#define __TY_NET_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "tuya_os_adapter.h"
#include "tuya_wifi_hal.h" 

int ty_net_enable(char *ifname , char enable);
int ty_net_get_ip(char *ifname , char *ip);
int ty_net_set_ip(char *ifname, char *ip);
int ty_net_get_mac(char *inf, uint8_t *mac);
int ty_net_set_mac(char *inf, uint8_t *mac);
int ty_net_get_ap_addr(char *ifname, uint8_t *mac);

int ty_wifi_scan(char *ifname , WF_HAL_AP_IF_S *aps , int max_ap);
int ty_wifi_set_chn(char *ifname, int channel);
int ty_wifi_get_chn(char *ifname, int *channel);
int ty_wifi_set_mode(char *ifname , char *mode);
int ty_wifi_get_mode(char *ifname , int *mode);
int ty_wifi_get_curr_rssi(char *ifname, int *rssi);
int ty_wifi_get_conn_stat(char *ifname);

#ifdef __cplusplus
}
#endif


#endif
