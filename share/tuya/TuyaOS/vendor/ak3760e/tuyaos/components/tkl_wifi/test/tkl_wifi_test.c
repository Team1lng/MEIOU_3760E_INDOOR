/*********************************************************************************
  * Copyright (C) by TY Inc
  * All rights reserved
  * 
  * FileName: main.c
  * Description:
  *     1. tuya wifi_hal Correlation function interface test
  *
  * @Author     : dingyi.ru
  * @Data       : 2021-08-10
  * @brief      : for wifi_hal API test
  * @log        : first implement by dingyi.ru
  * 
**********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <string.h>
#include <getopt.h>

#include "tkl_wifi.h"
#include "sys_cmd.h"

#define DEBUG_STR     "%d, %s: "
#define DEBUG_ARGS    __LINE__,__FUNCTION__

#define P_DEBUG(format,arg ...)                       \
    do{                                                     \
        printf(DEBUG_STR format "\n", DEBUG_ARGS, ##arg);   \
    } while (0)

/* ------------------------------ Test functions declaration --------------------------------- */
int demo_set_mac(void);
int demo_get_mac(void);
//int demo_set_ip(void);
int demo_get_ip(void);
int demo_set_country_code(void);
//int demo_get_cur_country_code(void);
int demo_set_cur_channel(void);
int demo_get_cur_channel(void);

int demo_all_ap_scan(void);
int demo_assign_ap_scan(void);
//int demo_get_assign_ap_rssi(void);
int demo_station_get_conn_ap_rssi(void);
int demo_get_connect_ap_info(void);
int demo_fast_station_connect(void);
int demo_ap_start(void);
int demo_ap_stop(void);
int demo_get_bssid(void);
int demo_set_work_mode(void);
int demo_get_work_mode(void);
int demo_register_recv_mgnt_callback(void);
int demo_send_mgnt(void);
int demo_sniffer_set(void);
int demo_station_connect(void);
int demo_station_disconnect(void);
int demo_station_get_status(void);
//int demo_station_get_local_mac(void);

//int demo_get_assign_ap_rssi_for_product_test(void);

/* --------------------------------------------------------------------------------------------- */

/* ---------------------------------- Data type defination ------------------------------------ */
typedef int (*wifi_hal_func)(void);
//typedef void (*WIFI_REV_MGNT_CB)(unsigned char *buf, unsigned int len);

struct wifi_hal_tbl
{
    const char      *name;
    wifi_hal_func   func;
    int             arg;
};

static struct wifi_hal_tbl wifi_hal_demo[]= {
    /* --------------------- 1.wifi Basic Parameter Configuration test ------------------------ */
    {"set_mac"                  ,demo_set_mac               ,0},
    {"get_mac"                  ,demo_get_mac               ,0},
    //{"set_ip"                   ,demo_set_ip                ,0},
    {"get_ip"                   ,demo_get_ip                ,0},
    {"set_country_code"         ,demo_set_country_code      ,0},
    //{"get_cur_country_code"     ,demo_get_cur_country_code  ,0},
    {"set_cur_channel"          ,demo_set_cur_channel       ,0},
    {"get_cur_channel"          ,demo_get_cur_channel       ,0},

    /* --------------------- 2.wifi network configuration test ------------------------------ */
    {"all_ap_scan"                  ,demo_all_ap_scan,       0},
    {"assign_ap_scan"               ,demo_assign_ap_scan,    0},
    //{"get_assign_ap_rssi"           ,demo_get_assign_ap_rssi,0},
    {"get_connect_ap_info"          ,demo_get_connect_ap_info, 0},
    {"fast_station_connect"         ,demo_fast_station_connect, 0},
    {"station_get_conn_ap_rssi"     ,demo_station_get_conn_ap_rssi,0},
    {"ap_start"                     ,demo_ap_start,          0},
    {"ap_stop"                      ,demo_ap_stop,           0},
    {"get_bssid"                    ,demo_get_bssid,         0},
    {"set_work_mode"                ,demo_set_work_mode,     0},
    {"get_work_mode"                ,demo_get_work_mode,     0},
    {"register_recv_mgnt_callback"  ,demo_register_recv_mgnt_callback,0},
    {"send_mgnt"                    ,demo_send_mgnt,         0},
    {"sniffer_set"                  ,demo_sniffer_set,       0},
    {"station_connect"              ,demo_station_connect,   0},
    {"station_disconnect"           ,demo_station_disconnect,0},
    {"station_get_status"           ,demo_station_get_status,0},
    //{"station_get_local_mac"        ,demo_station_get_local_mac,0},

    /* --------------------- 3. wifi product test functions test ----------------------------- */
    //{"get_assign_ap_rssi_for_product_test"  ,demo_get_assign_ap_rssi_for_product_test,0},
};

static struct option long_options[] = {
    /* --------------------- 1.wifi Basic Parameter Configuration test ------------------------ */
    {"set_mac"                      , no_argument, 0, 0},
    {"get_mac"                      , no_argument, 0, 0},
    //{"set_ip"                       , no_argument, 0, 0},
    {"get_ip"                       , no_argument, 0, 0},
    {"set_country_code"             , no_argument, 0, 0},
    //{"get_cur_country_code"         , no_argument, 0, 0},
    {"set_cur_channel"              , no_argument, 0, 0},
    {"get_cur_channel"              , no_argument, 0, 0},

    /* --------------------- 2.wifi network configuration test -------------------------------- */
    {"all_ap_scan"                  , no_argument, 0, 0},
    {"assign_ap_scan"               , no_argument, 0, 0},
    //{"get_assign_ap_rssi"           , no_argument, 0, 0},
    {"get_connect_ap_info"          , no_argument, 0, 0},
    {"fast_station_connect"         , no_argument, 0, 0},
    {"station_get_conn_ap_rssi"     , no_argument, 0, 0},
    {"ap_start"                     , no_argument, 0, 0},
    {"ap_stop"                      , no_argument, 0, 0},
    {"get_bssid"                    , no_argument, 0, 0},
    {"set_work_mode"                , no_argument, 0, 0},
    {"get_work_mode"                , no_argument, 0, 0},
    {"register_recv_mgnt_callback"  , no_argument, 0, 0},
    {"send_mgnt"                    , no_argument, 0, 0},
    {"sniffer_set"                  , no_argument, 0, 0},
    {"station_connect"              , no_argument, 0, 0},
    {"station_disconnect"           , no_argument, 0, 0},
    {"station_get_status"           , no_argument, 0, 0},
    {"set_lp_mode"                  , no_argument, 0, 0},
    {"rf_calibrated"                , no_argument, 0, 0},
    //{"station_get_local_mac"        , no_argument, 0, 0},

    /* --------------------- 3. wifi product test functions test ----------------------------- */
    //{"get_assign_ap_rssi_for_product_test",no_argument,0    ,0},

    {0                                    ,0          ,0    ,0}
};

/* ------------------------------------------------------------------------------------------ */

/* ----------------------------defination of functions--------------------------------------- */
int demo_set_mac(void)
{
    int iret = 0;
    WF_IF_E if_e = 0;   /* 0 -> station; 1 -> AP*/
    char mac[6] ={0x00,0x0c,0x18,0xef,0xff,0xed};
    NW_MAC_S n_mac;

    memset(&n_mac,0,sizeof(NW_MAC_S));
    memcpy(n_mac.mac,mac,MAC_ADDR_LEN);

    iret = tkl_wifi_set_mac(if_e,&n_mac);
    if(iret < 0){
        P_DEBUG("set mac failed");
    }

    return 0;
}

int demo_get_mac(void)
{
    int iret = 0;
    WF_IF_E if_e = 0;   /* 0 -> station;1 -> AP*/
    NW_MAC_S n_mac;

    memset(&n_mac,0,sizeof(NW_MAC_S));
    iret = tkl_wifi_get_mac(if_e,&n_mac);
    if(iret < 0) {
        P_DEBUG("get mac failed");
    } else {
        P_DEBUG("get mac success - %02x:%02x:%02x:%02x:%02x:%02x",n_mac.mac[0],n_mac.mac[1],
                                        n_mac.mac[2],n_mac.mac[3],n_mac.mac[4],n_mac.mac[5]);
    }

    return 0;
}

#if 0
int demo_set_ip(void)
{
    int iret = 0;
    char *ip= "192.168.8.8";
    WF_HAL_IF_E if_e = 0;   /* 0 -> station;1 -> AP*/
    NW_IP_S ip_s;

    memset(&ip_s,0,sizeof(NW_IP_S));
    strcpy(ip_s.ip,ip);

    P_DEBUG("set ip --- %s",ip);

    iret = tuya_wifi_hal_set_ip(if_e,&ip_s);
    if(iret < 0){
        P_DEBUG("set ip failed");
    } else {
        P_DEBUG("set ip : %s",ip);
    }

    return 0;
}
#endif
int demo_get_ip(void)
{
    int iret = 0;
    WF_IF_E if_e = 0;   /* 0 -> station;1 -> AP*/
    NW_IP_S ip_s;
    NW_IP_TYPE type = NW_IPV4;

    memset(&ip_s,0,sizeof(NW_IP_S));

    //get ipv4
    type = NW_IPV4;
    iret = tkl_wifi_get_ip(if_e, type, &ip_s);
    if(iret < 0) {
        P_DEBUG("get ipv4 failed");
    } else {
        P_DEBUG("\tget ipv4 : \nip = %s\nmask = %s\ngw = %s\n",ip_s.addr.ip4.ip,ip_s.addr.ip4.mask,ip_s.addr.ip4.gw);
    }

    //get ipv6
    type = NW_IPV6;
    iret = tkl_wifi_get_ip(if_e, type, &ip_s);
    if(iret < 0) {
        P_DEBUG("get ipv6 failed");
    } else {
        P_DEBUG("\tget ipv6 : \nip = %s\n, islinklocal=%d\n",ip_s.addr.ip6.ip, ip_s.addr.ip6.islinklocal);
    }

    type = NW_IPV6_LL;
    iret = tkl_wifi_get_ip(if_e, type, &ip_s);
    if(iret < 0) {
        P_DEBUG("get ipv6_ll failed");
    } else {
        P_DEBUG("\tget ipv6_ll : \nip = %s\n, islinklocal=%d\n",ip_s.addr.ip6.ip, ip_s.addr.ip6.islinklocal);
    }

    return iret;
}

int demo_set_country_code(void)
{
    int iret = 0;
    COUNTRY_CODE_E country_code = 0; /*0->CN;1->US;2->JP;3->EU;4->NR*/

    iret = tkl_wifi_set_country_code(country_code);
    if(iret < 0) {
        P_DEBUG("set country code failed");
    } else {
        country_code = iret;
        switch(country_code){
            case 0:
                P_DEBUG("set country code : CHINA");
                break;
            case 1:
                P_DEBUG("set country code : US");
                break;
            case 2:
                P_DEBUG("set country code : JP");
                break;
            case 3:
                P_DEBUG("set country code : EU");
                break;
            default:
                P_DEBUG("Warning : wrong country code");
                break;
        }
    }

    return iret;
}
#if 0
int demo_get_cur_country_code(void)
{
    int iret = 0;
    WF_HAL_COUNTRY_CODE_E country_code;

    country_code = tuya_wifi_hal_get_cur_country_code();
    if(-1 == country_code) {
        P_DEBUG("get country failed");
        return -1;
    } else {
        switch(country_code){
            case 0:
                P_DEBUG("get country code : CHINA");
                break;
            case 1:
                P_DEBUG("get country code :US");
                break;
            case 2:
                P_DEBUG("get country code : JP");
                break;
            case 3:
                P_DEBUG("get country code : EU");
                break;
            default:
                P_DEBUG("Warning : get wrong country code");
                break;
        }
    }

    return iret;

}
#endif

int demo_set_cur_channel(void)
{
    int iret = 0;
    int chan = 6;

    iret = tkl_wifi_set_cur_channel(chan);
    if(iret < 0) {
        P_DEBUG("set channel failed");
    } else {
        P_DEBUG("set channel : %d",chan);
    }

    return iret;
}

int demo_get_cur_channel(void)
{
    int iret = 0;
    unsigned char chan = 0;

    iret = tkl_wifi_get_cur_channel(&chan);
    if(iret < 0 || chan < 0 || chan > 14) {
        P_DEBUG("failed to get channel or channel illegal : %d",chan);
    } else {
        P_DEBUG("get channel success,channel : %d",chan);
    }

    return iret;
}

int demo_all_ap_scan(void)
{
    int iret = 0;
    unsigned int num = 0,index = 0;
    AP_IF_S *ap_ary = NULL;

    iret = tkl_wifi_scan_ap(NULL, (AP_IF_S **)&ap_ary,&num);
    if(iret < 0) {
        P_DEBUG("scan all ap failed");
    } else {
        P_DEBUG("all ap scan success");
        for(index = 0; index < num; index++) {
            printf("index: %d SSID: %s RSSI: %d channel: %d bssid: %02X-%02X-%02X-%02X-%02X-%02X \n",
            index,ap_ary[index].ssid,ap_ary[index].rssi,ap_ary[index].channel,ap_ary[index].bssid[0],
            ap_ary[index].bssid[1],ap_ary[index].bssid[2],ap_ary[index].bssid[3],ap_ary[index].bssid[4],
            ap_ary[index].bssid[5]);
        }
        if ( ap_ary )
            free( ap_ary );
    }

    return iret;
}

int demo_get_connect_ap_info(void)
{
    int iret = 0;
    FAST_WF_CONNECTED_AP_INFO_T *fast_ap_info = NULL;

    iret = tkl_wifi_get_connected_ap_info(&fast_ap_info);
    if(iret < 0){
        P_DEBUG("WiFi get conencted ap info failed");
        free(fast_ap_info);
        return -1;
    }
    free(fast_ap_info);

    return 0;
}

int demo_fast_station_connect(void)
{
    return 0;
}

int demo_assign_ap_scan(void)
{
    int iret = 0;
    unsigned int num = 0,index = 0;
    AP_IF_S *ap_ary = NULL;
    char *assign_ssid = "8F-S-12-10";

    iret = tkl_wifi_scan_ap( (const signed char*)assign_ssid, (AP_IF_S **)&ap_ary,&num);
    if(iret < 0) {
        P_DEBUG("scan all ap failed");
    } else {
        P_DEBUG("all ap scan success");
        for(index = 0; index < num; index++) {
            printf("index: %d SSID: %s RSSI: %d channel: %d bssid: %02X-%02X-%02X-%02X-%02X-%02X \n",
            index,ap_ary[index].ssid,ap_ary[index].rssi,ap_ary[index].channel,ap_ary[index].bssid[0],
            ap_ary[index].bssid[1],ap_ary[index].bssid[2],ap_ary[index].bssid[3],ap_ary[index].bssid[4],
            ap_ary[index].bssid[5]);
        }
        if ( ap_ary )
            free( ap_ary );
    }

    return iret;
}

#if 0
int demo_get_assign_ap_rssi(void)
{
    int iret = 0,rssi = 0;
    char *ssid = "3F-N-7-10-2G";

    iret = tuya_wifi_hal_get_assign_ap_rssi(ssid,&rssi);
    if(iret < 0 || rssi == 0) {
        P_DEBUG("get assign ap's rssi failed");
    } else {
        P_DEBUG("get assign ap's rssi success , AP %s rssi : %d",ssid,rssi);
    }

    return iret;
}
#endif

int demo_station_get_conn_ap_rssi(void)
{
    int iret = 0;
    char rssi = 0;

    iret = tkl_wifi_station_get_conn_ap_rssi((signed char *)&rssi);
    if(iret < 0 || rssi == 0) {
        P_DEBUG("get connect ap's rssi failed");
    } else {
        P_DEBUG("get connect ap's rssi success, rssi : %d",rssi);
    }

    return iret;
}

/* -------------stat a softap-------------- */
int demo_ap_start(void)
{
    int iret = 0;
    char *ssid = "rufan16888888888888";
    char *password = "12345678";
    char *ip = "192.168.8.1";
    WF_AP_CFG_IF_S ap_cfg;

    memset(&ap_cfg,0,sizeof(WF_AP_CFG_IF_S));

    strcpy((char *)ap_cfg.ssid,ssid);
    ap_cfg.s_len = strlen(ssid);
    strcpy((char *)ap_cfg.passwd,password);
    ap_cfg.p_len = strlen(password);
    ap_cfg.chan = 6;               /* channel */
    ap_cfg.md = 0;
    /* encryption mode : 0->open;1->wep;2->wpa-psk;3->wpa2->psk;4->wpa/wpa2 */
    ap_cfg.ssid_hidden = 0;        /* hidden ssid or not,0->not hidden;1->hidden,default:0*/
    ap_cfg.max_conn = 8;           /* max number of stations that can be connected to the ap*/
    ap_cfg.ms_interval = 0;        /* broadcast interval default ,default 0*/
    strcpy(ap_cfg.ip.addr.ip4.ip,ip);      /* the AP's ip*/

    iret = tkl_wifi_start_ap(&ap_cfg);
    if(iret < 0) {
        P_DEBUG("start ap failed");
    } else {
        P_DEBUG("start ap success,start ap: %s passwd: %s",ap_cfg.ssid,ap_cfg.passwd);
    }

    return iret;
}

int demo_ap_stop(void)
{
    int iret = 0;

    iret = tkl_wifi_stop_ap();
    if(iret < 0) {
        P_DEBUG("stop ap failed");
    } else {
        P_DEBUG("stop ap success");
    }

    return iret;
}

/* ----------- get the device connected ap's bssid ---------------*/
int demo_get_bssid(void)
{
    int iret = 0;
    unsigned char bssid[6] = {0x0};
    
    iret = tkl_wifi_get_bssid(bssid);
    if(iret < 0) {
        P_DEBUG("get bssid failed");
    } else {
        P_DEBUG("get bssid success,BSSID: %02X:%02X:%02X:%02X:%02X:%02X",
                    bssid[0],bssid[1],bssid[2],bssid[3],bssid[4],bssid[5]);
    }

    return iret;
}

int demo_set_work_mode(void)
{
    int iret = 0;
    WF_WK_MD_E mode = WWM_STATION;  /* station mode */

    iret = tkl_wifi_set_work_mode(mode);
    if(iret < 0) {
        P_DEBUG("set wifi work mode failed");
    } else {
        P_DEBUG("set wifi work mode success");
        switch(mode){
            case WWM_POWERDOWN:
                P_DEBUG("wifi work mode : WWM_POWERDOWN mode");
                break;
            case WWM_SNIFFER:
                P_DEBUG("wifi work mode : Sniffer mode");
                break;
            case WWM_STATION:
                P_DEBUG("wifi work mode : Station mode");
                break;
            case WWM_SOFTAP:
                P_DEBUG("wifi work mode : softap mode");
                break;
            case WWM_STATIONAP:
                P_DEBUG("wifi work mode : AP+Station mode");
                break;
            default:
                P_DEBUG("Warning : wrong mode setting");
        }
    }

    return iret;
}

int demo_get_work_mode(void)
{
    int iret = 0;
    WF_WK_MD_E mode = 0;

    iret = tkl_wifi_get_work_mode(&mode);
    if(iret < 0 || mode < WWM_POWERDOWN || mode > WWM_STATIONAP) {
        P_DEBUG("get wifi work mode failed");
    } else {
        P_DEBUG("get wifi work mode success");
        switch(mode){
            case WWM_POWERDOWN:
                P_DEBUG("wifi work mode : WWM_POWERDOWN mode");
                break;
            case WWM_SNIFFER:
                P_DEBUG("wifi work mode : Sniffer mode");
                break;
            case WWM_STATION:
                P_DEBUG("wifi work mode : Station mode");
                break;
            case WWM_SOFTAP:
                P_DEBUG("wifi work mode : softap mode");
                break;
            case WWM_STATIONAP:
                P_DEBUG("wifi work mode : AP+Station mode");
                break;
            default:
                P_DEBUG("Warning : wrong mode setting");
        }
    }
    
    return iret;
}

void wifi_recv_cb(UCHAR_T *buf, UINT_T len)
{
    int i;

    printf("sniffer buffer dump:\n");
    for(i=0;i < len;i++){
        if((i+1)%16 == 0)
            printf("\n");
        printf("%02x ",buf[i]);
    }
    printf("\n");
}

int demo_register_recv_mgnt_callback(void)
{
    int iret = 0;

    iret = tkl_wifi_register_recv_mgnt_callback(1,wifi_recv_cb);
    if(iret < 0) {
        P_DEBUG("recv mgnt failed");
    } else {
        P_DEBUG("recv mgnt success");
    }

    return iret;
}

int demo_send_mgnt(void)
{
    int iret = 0,i;
    char buf[128];

    for(i = 0;i < 128;i++){
        buf[i] = '8';
    }

    iret = tkl_wifi_send_mgnt((unsigned char *)buf,128);
    if(iret < 0) {
        P_DEBUG("send mgnt failed");
    } else {
        P_DEBUG("send mgnt success");
    }

    return iret;
}

void sniffer_cb(const unsigned char *buf, const unsigned short len, const signed char rssi)
{
    int i;

    printf("sniffer buffer dump:\n");
    for(i=0;i<len;i++){
        if((i+1)%16 == 0)
            printf("\n");
        printf("%02x ",buf[i]);
    }
    printf("\n");
}

int demo_sniffer_set(void)
{
    int iret = 0;

    iret = tkl_wifi_set_sniffer(1,sniffer_cb);
    if(iret < 0) {
        P_DEBUG("sniffer_set failed");
    } else {
        P_DEBUG("sniffer_set success");
    }

    return iret;
}

int demo_station_connect(void)
{
    int iret = 0;
    char *ssid = "8F-S-12-10";
    char *passwd = "bsp123456";

    iret = tkl_wifi_station_connect((signed char *)ssid,(signed char *)passwd);
    if(iret < 0) {
        P_DEBUG("station connect failed");
    } else {
        P_DEBUG("station connect success,connect to AP: %s",ssid);
        sys_cmd_run( "/sbin/iwconfig wlan0", NULL, NULL );
    }

    return iret;
}

int demo_station_disconnect(void)
{
    int iret = 0;

    iret = tkl_wifi_station_disconnect();
    if(iret < 0) {
        P_DEBUG("station disconnect failed");
    } else {
        P_DEBUG("station disconnect success");
    }

    return iret;
}

int demo_station_get_status(void)
{
    int iret = 0;
    WF_STATION_STAT_E stat = 0;

    iret = tkl_wifi_station_get_status(&stat);
    if(iret < 0) {
        P_DEBUG("station get status failed");
    } else {
        P_DEBUG("station disconnect success");
        switch(stat){
            case WSS_IDLE:
                P_DEBUG("get wifi status : IDLE");
                break;
            case WSS_CONNECTING:
                P_DEBUG("get wifi status : CONNECTING");
                break;
            case WSS_PASSWD_WRONG:
                P_DEBUG("get wifi status : PASSWD_WRONG");
                break;
            case WSS_NO_AP_FOUND:
                P_DEBUG("get wifi status : NO_AP_FOUND");
                break;
            case WSS_CONN_FAIL:
                P_DEBUG("get wifi status : CONN_FAIL");
                break;
            case WSS_CONN_SUCCESS:
                P_DEBUG("get wifi status : CONN_SUCCESS");
                break;
            case WSS_GOT_IP:
                P_DEBUG("get wifi status : GOT_IP");
                break;
            default:
                P_DEBUG("Warning : Wrong wifi status Got");
        }
    }

    return iret;
}

#if 0
int demo_station_get_local_mac(void)
{
    int iret = 0;
    NW_MAC_S mac_s;

    memset(&mac_s,0,sizeof(NW_MAC_S));

    iret = tuya_wifi_hal_station_get_local_mac(&mac_s);
    if(iret < 0) {
        P_DEBUG("station get local mac failed");
    } else {
        P_DEBUG("station get local mac success, get mac: %02X:%02X:%02X:%02X:%02X:%02X",
        mac_s.mac[0],mac_s.mac[1],mac_s.mac[2],mac_s.mac[3],mac_s.mac[4],mac_s.mac[5]);
    }

    return iret;
}
#endif

#if 0
int demo_get_assign_ap_rssi_for_product_test(void)
{
    int iret = 0,rssi = 0;
    char *ssid = "3F-N-7-10-2G";

    iret = tuya_wifi_hal_get_assign_ap_rssi_for_product_test(ssid,&rssi);
    if(iret < 0) {
        P_DEBUG("get assign ap's rssi for product API failed");
    } else {
        P_DEBUG("get assign ap's rssi for product API success,AP : %s RSSI : %d",ssid,rssi);
    }

    return iret;
}
#endif
/* ------------------------------------------------------------------------------------------ */

int help_usage(void)
{
    int i = 0;

    printf("Demo Usage:\n");
    printf("\tuse -%s to get help information \n","hH");
    printf("\n---------------------------------wifi_hal test Items--------------------------------\n");

    while(long_options[i].name) {
        printf("\t --%s\n",long_options[i].name);
        i++;
    }

    return 0;
}

int wifi_hal_test(int argc,char *argv[])
{
    int choice;
    int iret = -1;
    int option_index = 0;

    if(argc <= 1)
        help_usage();

    while(1) {
        choice = getopt_long(argc, argv,"hH",long_options, &option_index);
        if(choice == -1) {
            return 0;
        }

        switch (choice) {
            case 0:
                iret = wifi_hal_demo[option_index].func();
                break;
            case 'h':
            case 'H':
                iret = help_usage();
                break;
            case '?':
                break;
            default:
                printf("?? getopt returned character code 0%o ??\n", choice);
        }
    }

    return iret;
}

int main(int argc,char * argv[])
{
    printf("\n\t\t wifi_hal test begin.\n");

    wifi_hal_test(argc,argv);

    return 0;
}

