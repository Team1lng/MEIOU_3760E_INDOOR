#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <inttypes.h>
#include <sys/ioctl.h>
#include <linux/if_ether.h>
#include <netpacket/packet.h>
//#include <linux/ipv6.h>

#include "tkl_wifi.h"
#include "tkl_net.h"
#include "sys_cmd.h"
#include "priv_ipv6.h"
#include "tkl_wifi_log.h"
#include "tkl_gen_net.h"

#include "Amy_log.h"
#include "Amy_sdk.h"
#ifndef NL80211
#include "pcap.h"
#endif

#define MAX_REV_BUFFER      (512)

static volatile uint8_t g_curret_channel = 6;         /*default channel*/
//static pthread_mutex_t g_channel_mutex = PTHREAD_MUTEX_INITIALIZER;   /* init pthread_mutex */
//static pthread_mutex_t s_channel_mutex = PTHREAD_MUTEX_INITIALIZER;       /* 初始化线程锁 */
static pthread_mutex_t s_channel_mutex;

static volatile int s_enable_sniffer = 0;
static volatile int sniffer_thr_run = 0;

#ifndef NL80211
/* pcap抓包使用 */
static pcap_t* device = NULL;
#endif

static volatile BOOL_T s_enable_recv_frame = FALSE;
static volatile SNIFFER_CB s_pSnifferCall = NULL;
static volatile WIFI_REV_MGNT_CB tkl_recv_cb = NULL;
static pthread_t sniffer_thId;
static pthread_t recv_frame_thId; // 抓包线程ID
static int sg_wifi_event_exit = 0;
static WF_STATION_STAT_E cur_stat = 0;
static WF_STATION_STAT_E last_stat = 0;
static int report_stat = -1;
static int sta_conn_flag = 0;
static int f_get_ap_mac_old = 0; //save old status to prevent jitter

WIFI_EVENT_CB wifi_status_cb = NULL; /* wifi 工作事件上报回调 */

static void* wifi_status_event_cb(void* arg)
{
    WF_EVENT_E wf_event;
    WF_EVENT_E first_event;
    WF_STATION_STAT_E first_stat = 0;
    AMY_LOG_ERROR("---------------------->>\n");
    pthread_detach(pthread_self());

    sg_wifi_event_exit = 0;

    tkl_gen_station_get_status(&first_stat);
    TKL_LOGD("WiFi : first_stat=%d\n", first_stat);
    if (first_stat != WSS_IDLE) {
        if (first_stat == WSS_CONN_FAIL) {
            first_event = WFE_CONNECT_FAILED;
        }
        else if (first_stat == WSS_GOT_IP || first_stat == WSS_CONN_SUCCESS) {
            first_event = WFE_CONNECTED;
        }
        else {
            first_event = WFE_DISCONNECTED;
        }
        TKL_LOGD("WiFi : report event=%d\n", first_event);
        wifi_status_cb(first_event, NULL);
    }

    while (!sg_wifi_event_exit) {
        if (!sta_conn_flag) {
#ifdef DEBUG_VERSION
            TKL_LOGD("WiFi : sta_conn_flag=%d\n", sta_conn_flag);
#endif
            sleep(1);
            continue;
        }

        tkl_gen_station_get_status(&cur_stat);
        if (cur_stat == WSS_IDLE) {
#ifdef DEBUG_VERSION
            TKL_LOGD("WiFi : sta_conn_flag=%d, idel last_state=%d, state=%d \n", sta_conn_flag, last_stat, cur_stat);
#endif
            sleep(1);
            continue;
        }

#ifdef DEBUG_VERSION
        TKL_LOGD("WiFi : last_state=%d, state=%d \n", last_stat, cur_stat);
#endif
        if (cur_stat != WSS_CONN_SUCCESS && cur_stat != WSS_GOT_IP) {
            wf_event = WFE_DISCONNECTED;
#ifdef DEBUG_VERSION
            TKL_LOGD("WiFi : last_state=%d, state=%d, wf_event=%d\n", last_stat, cur_stat, wf_event);
#endif
        }

        if (cur_stat == WSS_CONN_FAIL) {
            wf_event = WFE_CONNECT_FAILED;
        }
        else if (cur_stat == WSS_GOT_IP || cur_stat == WSS_CONN_SUCCESS) {
            wf_event = WFE_CONNECTED;
#ifdef DEBUG_VERSION
            TKL_LOGD("WiFi : report_stat=%d, wf_event=%d\n", report_stat, wf_event);
#endif
            if (last_stat != cur_stat) {
                last_stat = cur_stat;
                report_stat = wf_event;
                wifi_status_cb(wf_event, NULL);
                sleep(1);
                continue;
            }
        }

        if (report_stat == wf_event) {
            sleep(1);
            continue;
        }

        report_stat = wf_event;
#ifdef DEBUG_VERSION
        TKL_LOGD("WiFi : report_stat=%d, wf_event=%d\n", report_stat, wf_event);
#endif
        wifi_status_cb(wf_event, NULL);
        sleep(1);
    }
    return NULL;
}

int tkl_gen_wifi_init(WIFI_EVENT_CB cb)
{
    int iret = 0;
    pthread_t thread;
    wifi_status_cb = cb;
    AMY_LOG_ERROR("---------------------->>\n");
    iret = pthread_create(&thread, NULL, wifi_status_event_cb, NULL);
    if (iret) {
        TKL_LOGE("create status_cs thread failed");
        return -1;
    }

    return 0;
}

int tkl_gen_wifi_deinit(void)
{
    AMY_LOG_ERROR("---------------------->>\n");
    sg_wifi_event_exit = 1;
    return 0;
}


int tkl_gen_ap_scan(CONST SCHAR_T* ssid, AP_IF_S** ap_ary, UINT_T* num)
{
    int iret;
    int index;
    char* ifname = WLAN_DEV;
    AP_IF_S* s_aps = NULL; //upper app free
    s_aps = (AP_IF_S*)malloc(MAX_AP_SEARCH * sizeof(AP_IF_S));
    
    AMY_LOG_ERROR("---------------------->>\n");
    if (!s_aps) {
        TKL_LOGE("faileded to malloc mem");
        return -1;
    }

    memset(s_aps, 0, MAX_AP_SEARCH * sizeof(AP_IF_S));

    iret = tkl_net_wifi_enable(ifname, 1);
    if (iret < 0) {
        TKL_LOGE("enable wifi failed");
    }

    iret = tkl_net_wifi_scan(ifname, s_aps, MAX_AP_SEARCH);
    if (iret == 0) {
        free(s_aps);
        return -1;
    }

    if (ssid == NULL) {
        *num = iret;
        *ap_ary = s_aps;
#if 0
        /* --- print all scanned aps' information --- */
        for (index = 0; index < *num; index++) {
            TKL_LOGD("index:%d SSID:%s seurity : %d channel:%d RSSI:%d bssid:%02X-%02X-%02X-%02X-%02X-%02X",
                index, s_aps[index].ssid, s_aps[index].security, s_aps[index].channel, s_aps[index].rssi,
                s_aps[index].bssid[0], s_aps[index].bssid[1], s_aps[index].bssid[2],
                s_aps[index].bssid[3], s_aps[index].bssid[4], s_aps[index].bssid[5]);
        }
#endif
    }
    else {
        AP_IF_S* ap_s = (AP_IF_S*)malloc(sizeof(AP_IF_S));
        memset(ap_s, 0, sizeof(AP_IF_S));
        *ap_ary = ap_s;
        *num = 1;
        for (index = 0;index < iret;index++) {
            if (strcmp((char*)ssid, (char*)s_aps[index].ssid) == 0) {
                memcpy(ap_s, &s_aps[index], sizeof(AP_IF_S));
                break;
            }
        }
        free(s_aps);
        s_aps = NULL;
    }

    return 0;
}

int tkl_gen_net_set_cur_channel(const char channel)
{
    int iret = 0;
    char* ifname = WLAN_DEV;

    AMY_LOG_ERROR("---------------------->>\n");
    pthread_mutexattr_t attr;
    iret = pthread_mutexattr_init(&attr);
    if (0 != iret) {
        TKL_LOGE("init pthread mutex failed");
        return -1;
    }

    iret = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    if (0 != iret) {
        TKL_LOGE("mutex set type failed");
        return -1;
    }

    iret = pthread_mutex_init(&s_channel_mutex, &attr);
    if (0 != iret) {
        TKL_LOGE("s_channel_mutex init failed");
        return -1;
    }
    pthread_mutex_lock(&s_channel_mutex);

    iret = tkl_net_set_chn(ifname, channel);
    if (iret < 0) {
        TKL_LOGE("set channel failed");
        iret = -1;
    }
    else {
        g_curret_channel = channel;
    }

    pthread_mutexattr_destroy(&attr);

    pthread_mutex_unlock(&s_channel_mutex);

    return iret;
}

int tkl_gen_net_get_cur_channel(unsigned char* channel)
{
    int iret = 0;
    unsigned char chn = 0;
    char* ifname = WLAN_DEV;

    AMY_LOG_ERROR("---------------------->>\n");
    pthread_mutexattr_t attr;
    iret = pthread_mutexattr_init(&attr);
    if (0 != iret) {
        TKL_LOGE("init pthread mutex failed");
        return -1;
    }

    /* 将互斥锁属性设置为递归锁 */
    iret = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    if (0 != iret) {
        TKL_LOGE("mutex set type failed");
        return -1;
    }

    iret = pthread_mutex_init(&s_channel_mutex, &attr);
    if (0 != iret) {
        TKL_LOGE("s_channel_mutex init failed");
        return -1;
    }

    pthread_mutex_lock(&s_channel_mutex);
    iret = tkl_net_get_chn(ifname, &chn);
    if (iret < 0) {
        TKL_LOGE("get channel failed");
        iret = -1;
    }
    else {
        *channel = chn & 0xff;
    }

    pthread_mutexattr_destroy(&attr);

    pthread_mutex_unlock(&s_channel_mutex);

    return iret;
}

int tkl_gen_wifi_get_ipv4(CONST WF_IF_E wf, NW_IP_S* ip)
{
    int iret = 0;
    char* ifname = WLAN_DEV;
    char tmp_ip[MAX_IP_LEN] = { 0x0 };
    char tmp_gw[MAX_IP_LEN] = { 0x0 };
    char tmp_prepre[MAX_IP_LEN - 3] = { 0 };

  //  AMY_LOG_ERROR("---------------------->>\n");
    /* TKl WiFi 目前没有做网络的共存，这段去掉*/
#if 0
    if (wf == WF_STATION)
        ifname = WLAN_DEV;
    else if (wf == WF_AP)
        ifname = WLAN_AP;
#endif

    /* get ip */
    iret = tkl_net_get_ipv4(ifname, tmp_ip);
    if (iret < 0) {
        TKL_LOGE("WiFi : get ip failed");
        return -1;
    }
    strncpy(ip->addr.ip4.ip, tmp_ip, sizeof(ip->addr.ip4.ip));

    /* get mask */
    iret = tkl_net_get_mask_ipv4(ifname, ip->addr.ip4.mask);
    if (iret < 0) {
        TKL_LOGE("WiFi : get mask failed");
    }

    /* get gateway */
    iret = ip_prepare(tmp_ip, tmp_prepre);

    snprintf(tmp_gw, sizeof(tmp_gw), "%s%d", tmp_prepre, 1);
    strncpy(ip->addr.ip4.gw, tmp_gw, MAX_IP_LEN - 1);

    if (iret < 0) {
        TKL_LOGE("WiFi : get gateway failed");
    }

    return 0;
}

int tkl_gen_wifi_get_ipv6(CONST WF_IF_E wf, NW_IP_TYPE type, NW_IP_S* ip)
{
    int iret = 0;
    char* ifname = WLAN_DEV;

    AMY_LOG_ERROR("---------------------->>\n");
    /* get ip */
    iret = tkl_net_get_ipv6(ifname, type, ip);
    if (iret < 0) {
        TKL_LOGE("WiFi : get ip failed");
        return -1;
    }
    return 0;
}


int tkl_gen_set_mac(CONST WF_IF_E wf, CONST NW_MAC_S* mac)
{
    int iret = 0;
    char* ifname = NULL;
    WIFI_TYPE_E wf_t;

    if (wf == WF_STATION)
        ifname = WLAN_DEV;
    else if (wf == WF_AP)
        ifname = WLAN_AP;
    else {
        TKL_LOGE("invalid param:%d\n", wf);
        return -1;
    }

    AMY_LOG_ERROR("---------------------->>\n");
    TKL_LOGD("WIFI Set MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
        mac->mac[0], mac->mac[1], mac->mac[2], mac->mac[3], mac->mac[4], mac->mac[5]);

    wf_t = tkl_net_get_wifi_type();
    if (wf_t == WIFI_8188) {
        tkl_net_wifi_enable(ifname, 0);          /* ifconfig wlan0 down */
        iret = tkl_net_set_mac(ifname, (const char*)mac->mac);
        tkl_net_wifi_enable(ifname, 1);          /* ifconfig wlan0 up */
    }
    else if (wf_t == WIFI_8723DS) {
        iret = tkl_net_set_mac_8723(ifname, (const char*)mac->mac);
    }
    else {
        iret = tkl_net_set_mac(ifname, (const char*)mac->mac);
    }

    if (iret < 0) {
        TKL_LOGE("WIFI Set MAC failed");
        return -1;
    }
    else {
        TKL_LOGD("WIFI Set MAC Success, MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
            mac->mac[0], mac->mac[1], mac->mac[2], mac->mac[3], mac->mac[4], mac->mac[5]);
    }

    return 0;
}

int tkl_gen_get_mac(CONST WF_IF_E wf, NW_MAC_S* mac)
{
    int iret = 0;
    char* ifname = NULL;

    if (wf == WF_STATION)
        ifname = WLAN_DEV;
    else if (wf == WF_AP)
        ifname = WLAN_AP;
    else {
        TKL_LOGE("invalid param:%d\n", wf);
        return -1;
    }


    iret = tkl_net_get_mac(ifname, (char*)mac->mac);
    if (iret < 0) {
        TKL_LOGE("WIFI GET MAC failed");
        return -1;
    }
    else {
        TKL_LOGD("WIFI GET MAC success , MAC:%02x:%02x:%02x:%02x:%02x:%02x",
            mac->mac[0], mac->mac[1], mac->mac[2], mac->mac[3], mac->mac[4], mac->mac[5]);
    }

    return 0;
}

int tkl_gen_set_work_mode(CONST WF_WK_MD_E mode)
{
    char* ifname = WLAN_DEV;    //default wlan0
    char* ifname2 = WLAN_AP;

    AMY_LOG_ERROR("---------------------->>\n");
    switch (mode) {
    case WWM_POWERDOWN:
        tkl_net_wifi_enable(ifname, 0);
        break;
    case WWM_SNIFFER:
        tkl_net_set_mode(ifname, "Monitor");
        break;
    case WWM_STATION:
        cur_stat = WSS_CONN_FAIL;
        last_stat = WSS_CONN_FAIL;
        report_stat = WFE_CONNECT_FAILED;
        tkl_net_set_mode(ifname, "Managed");
        break;
    case WWM_SOFTAP:
        tkl_gen_station_disconnect();
        tkl_net_set_mode(ifname, "Master");
        break;
    case WWM_STATIONAP:     /*sta + ap mode*/
        tkl_net_set_mode(ifname, "Managed");
        tkl_net_set_mode(ifname2, "Master");
        break;
    default:
        break;
    }

    return 0;
}

int tkl_gen_get_work_mode(WF_WK_MD_E* mode)
{
    int iret = 0;
    WIFI_MODE_E wf_mode;
    char* ifname = WLAN_DEV;    /* default */

    AMY_LOG_ERROR("---------------------->>\n");
    iret = tkl_net_get_mode(ifname, (int*)&wf_mode);
    if (iret < 0) {
        //TKL_LOGE("WIFI GET work mode failed");
        return -1;
    }

    if (wf_mode == WIFI_MODE_MANAGED) {
        int iret2 = 0;
        WIFI_MODE_E wf2_mode;
        char* ifname2 = WLAN_AP;
        iret2 = tkl_net_get_mode(ifname2, (int*)&wf2_mode);

        if (iret2 == 0 && wf2_mode == WIFI_MODE_MASTER) {
            *mode = WWM_STATIONAP;
        }
        else {
            *mode = WWM_STATION;
        }
    }    
else if (wf_mode == WIFI_MODE_MASTER) {
        *mode = WWM_SOFTAP;
    }
    else if (wf_mode == WIFI_MODE_MONITOR) {
        *mode = WWM_SNIFFER;
    }
    else {
        return -1;
    }

    return 0;
}

int tkl_gen_ap_start(CONST WF_AP_CFG_IF_S* cfg)
{
    int iret = 0;
    WF_AP_CFG_IF_S tmp_cfg;

    AMY_LOG_ERROR("---------------------->>\n");
    memcpy(&tmp_cfg, cfg, sizeof(WF_AP_CFG_IF_S));

    if (cfg->chan <= 0 || cfg->chan > 14) {
        TKL_LOGE("WIFI : Set a wrong channel out of range");
        return -1;
    }
    else if (cfg->ip.addr.ip4.ip[0] == 0x0 || strlen(cfg->ip.addr.ip4.ip) == 0) {
        TKL_LOGE("WIFI:Set a wrong ip address");
        strcpy(tmp_cfg.ip.addr.ip4.ip, WIFI_AP_DEFAULT_IP);
    }

    iret = tkl_net_ap_start(&tmp_cfg);
    if (iret < 0) {
        TKL_LOGE("start ap failed");
        return -1;
    }

    return 0;
}

int tkl_gen_ap_stop(void)
{
    int iret = 0;

    AMY_LOG_ERROR("---------------------->>\n");
    iret = tkl_net_ap_stop();
    if (iret < 0) {
        return -1;
    }

    return 0;
}

int tkl_gen_get_connected_ap_info(FAST_WF_CONNECTED_AP_INFO_T** fast_ap_info)
{
    int iret = 0;
    FAST_WF_CONNECTED_AP_INFO_T* ap_info_t = NULL;
    FAST_CONNECTED_AP_INFO_S* ap_info_s;

    AMY_LOG_ERROR("---------------------->>\n");
    ap_info_t = (FAST_WF_CONNECTED_AP_INFO_T*)malloc(sizeof(FAST_WF_CONNECTED_AP_INFO_T) + sizeof(FAST_CONNECTED_AP_INFO_S));
    if (NULL == ap_info_t)
    {
        TKL_LOGE("WiFi : malloc memory failed");
        return -1;
    }

    ap_info_t->len = sizeof(FAST_CONNECTED_AP_INFO_S);

    ap_info_s = (FAST_CONNECTED_AP_INFO_S*)&(ap_info_t->data[0]);
    memset(ap_info_s, 0x0, sizeof(FAST_CONNECTED_AP_INFO_S));

    iret = tkl_gen_net_get_cur_channel(&(ap_info_s->chan));
    if (iret < 0) {
        TKL_LOGE("WiFi : get current work channel failed\n");
        return -1;
    }

    iret = tkl_gen_get_bssid(&(ap_info_s->bssid[0]));
    if (iret < 0) {
        TKL_LOGE("WiFi : get bssid failed\n");
        return -1;
    }

    iret = tkl_net_get_connected_ap_info((char*)ap_info_s->ssid, (char*)ap_info_s->passwd);
    if (iret < 0) {
        TKL_LOGE("WiFi : get passwd and passwd failed\n");
        return -1;
    }

    *fast_ap_info = ap_info_t;

    return 0;
}

int tkl_gen_fast_station_connect(CONST FAST_WF_CONNECTED_AP_INFO_T* fast_ap_info)
{
    int iret = 0;
    FAST_CONNECTED_AP_INFO_S* ap_info_s = NULL;
    ap_info_s = (FAST_CONNECTED_AP_INFO_S*)&(fast_ap_info->data[0]);

    AMY_LOG_ERROR("---------------------->>\n");
    iret = tkl_net_start_ap_connect((char*)ap_info_s->ssid, (char*)ap_info_s->passwd);
    if (iret < 0) {
        TKL_LOGE("WiFi : connected to wifi failed");
        return -1;
    }

    return 0;
}

int tkl_gen_station_connect(CONST SCHAR_T* ssid, CONST SCHAR_T* passwd)
{
    int iret = 0;

    TKL_LOGD("WiFi : connect to ap \n");

    iret = tkl_net_start_ap_connect((char*)ssid, (char*)passwd);
    if (iret < 0) {
        TKL_LOGE("WiFi : connect to ap failed\n");
        return -1;
    }

    sta_conn_flag = 1;
    return 0;
}

int tkl_gen_station_disconnect(void)
{
    
    AMY_LOG_ERROR("---------------------->>\n");
    char* ifname = WLAN_DEV;
    sta_conn_flag = 0;
    tkl_net_sta_disconnect(ifname);

    return 0;
}

int tkl_gen_station_get_conn_ap_rssi(SCHAR_T* rssi)
{
    int iret = 0;
    char* ifname = WLAN_DEV;

    iret = tkl_net_get_curr_rssi(ifname, (char*)rssi);
    if (iret < 0) {
        TKL_LOGE("get connect ap's rssi failed");
        return -1;
    }
    else {
        TKL_LOGD("get connect ap's rssi : %d", *rssi);
    }

    return 0;
}

int tkl_gen_get_bssid(unsigned char* mac)
{
    int iret = 0;

    AMY_LOG_ERROR("---------------------->>\n");
    iret = tkl_net_get_bssid(mac);
    if (iret < 0) {
        TKL_LOGE("WIFI GET BSSID failed");
        return -1;
    }
    else {
        TKL_LOGD("WiFi Get AP BSSID %02x:%02x:%02x:%02x:%02x:%02x",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }

    return 0;
}

static int is_valid_mac(unsigned char* mac)
{
    unsigned char cmp_mac[6] = { 0 };

    AMY_LOG_ERROR("---------------------->>\n");
    if (mac == NULL) {
        //TKL_LOGE("invalid param\n"); 
        return 0;
    }

    if (mac[0] & 0x1) {
        //TKL_LOGE("mac is multi broadcast mac\n");
        return 0;
    }

    memset(cmp_mac, 0x0, sizeof(cmp_mac));
    if (memcmp(cmp_mac, mac, 6) == 0) {
        //TKL_LOGE("mac is all zero,not valid\n");
        return 0;
    }

    memset(cmp_mac, 0xff, 6);

    if (memcpy(cmp_mac, mac, 6) == 0) {
        //TKL_LOGE("mac is all 0xff,not valid\n");
        return 0;
    }

    return 1;

}

static int clear_ipv6_ipaddr(const char* ifname)
{
    printf("ERROR!!! compiler error , need adapter\n");
#if 0   //platform(T31) compiler error
#define _PATH_PROCNET_IFINET6           "/proc/net/if_inet6"
    FILE* f;
    char addr6[40], devname[21];
    struct sockaddr_in6 sap;
    int plen, scope, dad_status, if_idx;
    char addr6p[8][5];
    int sockfd6;
    struct in6_ifreq ifr6;
    int found = 0;

    f = fopen(_PATH_PROCNET_IFINET6, "r");
    if (f == NULL)
        return -1;

    while (fscanf
    (f, "%4s%4s%4s%4s%4s%4s%4s%4s %08x %02x %02x %02x %20s\n",
        addr6p[0], addr6p[1], addr6p[2], addr6p[3], addr6p[4],
        addr6p[5], addr6p[6], addr6p[7], &if_idx, &plen, &scope,
        &dad_status, devname) != EOF
        ) {
        if (strcmp(devname, ifname) == 0) {
            sprintf(addr6, "%s:%s:%s:%s:%s:%s:%s:%s",
                addr6p[0], addr6p[1], addr6p[2], addr6p[3],
                addr6p[4], addr6p[5], addr6p[6], addr6p[7]);
            memset(&sap, 0, sizeof(sap));
            inet_pton(AF_INET6, addr6,
                (struct sockaddr*)&sap.sin6_addr);
            sap.sin6_family = AF_INET6;
            if ((scope & IPV6_ADDR_SCOPE_MASK) == IPV6_ADDR_ANY) {
                TKL_LOGW("ip6 addr=%s/%d, scope=0x%02x\n", addr6, plen, scope);
                found = 1;
                break;
            }

            /*
               switch (scope & IPV6_ADDR_SCOPE_MASK) {
               case 0:
               TKL_LOGW("Global");
               break;
               case IPV6_ADDR_LINKLOCAL:
               TKL_LOGW("Link");
               break;
               case IPV6_ADDR_SITELOCAL:
               TKL_LOGW("Site");
               break;
               case IPV6_ADDR_COMPATv4:
               TKL_LOGW("Compat");
               break;
               case IPV6_ADDR_LOOPBACK:
               TKL_LOGW("Host");
               break;
               default:
               TKL_LOGW("Unknown");
               }
               */

        }
    }
    fclose(f);

    if (!found) {
        return -1;
    }

    sockfd6 = socket(AF_INET6, SOCK_DGRAM, 0);
    if (sockfd6 < 0) {
        return -1;
    }

    memset(&ifr6, 0x0, sizeof(struct in6_ifreq));
    ifr6.ifr6_ifindex = if_idx;
    ifr6.ifr6_prefixlen = plen;
    if (ioctl(sockfd6, SIOCDIFADDR, &ifr6) < 0) {
        TKL_LOGW("error=%d,%s\n", errno, strerror(errno));
        close(sockfd6);
        return -1;
    }
    close(sockfd6);
#endif
    return 0;
}

int tkl_gen_station_get_status(WF_STATION_STAT_E* stat)
{
    AMY_LOG_ERROR("---------------------->>\n");
    int iret = 0;
    char* ifname = WLAN_DEV;    /* default: wlan0*/
    unsigned char mac[6] = { 0x0 };
    char ip[16] = { 0x0 };
    int f_get_ap_mac = 0, f_get_ipv4 = 0, f_essid_on = 0;
    WIFI_MODE_E mode = WIFI_MODE_AUTO;
    static int ipv4_status = 0;
    static int ipv6_status = 0;
    int f_get_ipv6 = 0;
    int skfd = 0;
    struct ifreq ifr;
    struct sockaddr_in* addr;
    char* clear_ip = "0.0.0.0";
    NW_IP_S ip_info;

    tkl_net_get_mode(ifname, (int*)&mode);

    if (mode != WIFI_MODE_MANAGED) {
        //TKL_LOGE("get wifi work status only in station mode");
        *stat = WSS_IDLE;
        f_get_ap_mac_old = 0;
        return 0;
    }

    iret = tkl_net_get_bssid(mac);
    if (!iret && is_valid_mac(mac)) {
        f_get_ap_mac = 1;
    }

    //ipv4 or ipv6
    iret = tkl_net_get_ipv4(ifname, ip);
    if (!iret && strlen(ip) != 0) {
        f_get_ipv4 = 1;
    }

    /* get ipv6 */
    memset(&ip_info, 0x0, sizeof(NW_IP_S));
    iret = tkl_net_get_ipv6(ifname, NW_IPV6, &ip_info);
    if (!iret && strlen(ip_info.addr.ip6.ip) != 0) {
        f_get_ipv6 = 1;
    }

    f_essid_on = tkl_net_essid_on(ifname);
    //    if(f_get_ap_mac == 1 && f_get_ipv4 == 1) {
    //        f_netwok = NetIsOk();
    //    }

    if ((f_get_ap_mac_old == 0 && f_get_ap_mac == 0) || f_essid_on == 0) {
        *stat = WSS_CONN_FAIL;
        if (ipv4_status) {
            memset(&ifr, 0, sizeof(struct ifreq));
            strcpy(ifr.ifr_name, ifname);
            if ((skfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                //fprintf(stderr,"socket fail\n");
            }
            else {
                addr = (struct sockaddr_in*)&(ifr.ifr_addr);
                addr->sin_family = AF_INET;
                addr->sin_addr.s_addr = inet_addr(clear_ip);
                if (ioctl(skfd, SIOCSIFADDR, &ifr) < 0) {
                    //fprintf(stderr,"ioctl fail\n");
                    if (ioctl(skfd, SIOCSIFADDR, &ifr) < 0) {
                        //fprintf(stderr,"clear ip  fail\n");
                    }
                    else {
                        ipv4_status = 0;
                    }
                }
                else {
                    ipv4_status = 0;
                }
                close(skfd);
            }


        }

        if (ipv6_status) {
            if (!clear_ipv6_ipaddr(ifname)) {
                ipv6_status = 0;
            }
        }

    }

    if (f_get_ap_mac == 1 && f_essid_on == 1 && (f_get_ipv4 == 0 || f_get_ipv6 == 0)) {
        *stat = WSS_CONN_SUCCESS;
    }

    if (f_get_ap_mac == 1 && f_essid_on == 1 && (f_get_ipv4 == 1 || f_get_ipv6 == 1)) {
        *stat = WSS_GOT_IP;
        if (f_get_ipv4 == 1) {
            ipv4_status = 1;
        }
        if (f_get_ipv6 == 1) {
            ipv6_status = 1;
        }
    }

    f_get_ap_mac_old = f_get_ap_mac;

#ifdef DEBUG_VERSION
    TKL_LOGD("WiFi : f_get_ap_mac=%d, f_essid_on=%d, f_get_ipv4=%d, f_get_ipv6=%d\n", f_get_ap_mac, f_essid_on, f_get_ipv4, f_get_ipv6);
#endif

    return 0;
}

int tkl_gen_set_country_code(CONST COUNTRY_CODE_E ccode)
{
    int iret = 0;
    WIFI_TYPE_E wf_type;
    char* country = NULL;

    AMY_LOG_ERROR("---------------------->>\n");
    wf_type = tkl_net_get_wifi_type();

    if (ccode == COUNTRY_CODE_CN) {
        country = "CN";
    }
    else if (ccode == COUNTRY_CODE_US) {
        country = "US";
    }
    else if (ccode == COUNTRY_CODE_JP) {
        country = "JP";
    }
    else if (ccode == COUNTRY_CODE_EU) {
        country = "EU";
    }
    else {
        TKL_LOGE("invalid param:%d\n", ccode);
        return -1;
    }

    switch (wf_type) {
    case WIFI_8192:     /* 8197 */
    case WIFI_RTL8197:
        TKL_LOGD("wifi_type=%d, set country %s\n", wf_type, country);
        iret = tkl_net_set_country(country);
        if (iret < 0) {
            return -1;
        }
        break;
#ifdef NL80211
    case WIFI_8188:
        iret = tkl_net_set_country_rtl8188(country);
        if (iret < 0) {
            return -1;
        }
        break;
#endif
    case WIFI_8723DS:
        iret = tkl_net_set_country(country);
        if (iret < 0) {
            return -1;
        }
        break;
    default:
        break;
    }

    return 0;
}

#ifdef NL80211
static void* func_Sniffer(void* arg)
{
    int sock = 0;
    //int skipLen = 18;
    char* wlan = WLAN_DEV;     /* default wlan0 */
    struct ifreq ifr;

    char rev_buffer[MAX_REV_BUFFER];

    TKL_LOGD("Sniffer Thread Create\n");

    pthread_detach(pthread_self());

    sock = socket(PF_PACKET, SOCK_RAW, htons(0x03));//ETH_P_ALL
    if (sock < 0) {
        TKL_LOGE("Sniffer Socket Alloc");
        return (void*)0;
    }

    memset(&ifr, 0x00, sizeof(ifr));
    strncpy(ifr.ifr_name, wlan, strlen(wlan) + 1);
    setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, (char*)&ifr, sizeof(ifr));

    while ((s_pSnifferCall != NULL) && (s_enable_sniffer == 1)) {
        int rev_num = recvfrom(sock, rev_buffer, MAX_REV_BUFFER, 0, NULL, NULL);
        //ieee80211_radiotap_header *pHeader = (ieee80211_radiotap_header *)rev_buffer;
        //skipLen = pHeader->it_len;

        if (rev_num > 0) {
            //s_pSnifferCall(rev_buffer+skipLen, rev_num-skipLen);
            s_pSnifferCall((const uint8_t*)rev_buffer, rev_num);
        }
    }

    s_pSnifferCall = NULL;
    close(sock);

    return (void*)0;
}

int tkl_gen_net_set_sniffer(const int en, const SNIFFER_CB cb)
{
    char* ifname = WLAN_DEV;
    WIFI_TYPE_E wf_type;

    wf_type = tkl_net_get_wifi_type();
    if (wf_type == WIFI_8723DS) {
        TKL_LOGE("rtl8723ds doesn't support monitor mode");
        return -1;
    }

    if (en == TRUE) {
        TKL_LOGD("WIFI enable Sniffer");
        tkl_net_wifi_enable(ifname, 1);
        tkl_net_set_mode(ifname, "monitor");
        s_pSnifferCall = cb;
        s_enable_sniffer = 1;
        pthread_create(&sniffer_thId, NULL, func_Sniffer, NULL);
    }
    else {
        TKL_LOGD("Disable Sniffer");
        s_enable_sniffer = 0;
        pthread_join(sniffer_thId, NULL);
        tkl_net_set_mode(ifname, "managed");     /* 重新设置为station mode */
    }

    return 0;
}

#else
/* not NL80211 for 8197 device */

static int realuser_parse_80211packet(u_char* buf, u_int len)
{
    WLAN_80211_HEAD* wbuf = (WLAN_80211_HEAD*)buf;
    
    AMY_LOG_ERROR("---------------------->>\n");
    if ((wbuf->frmae_ctl1 & 0x3))
        return -1;
    //type
    switch ((wbuf->frmae_ctl1) & 0xc) {
    case 0x8: {
        break;
    }
    default: {
        return -1;
    }
    }

    //dir
    if ((wbuf->frmae_ctl2 & 0x3) == 0x2) {
        //ap da bssid sa
        /*
        if(memcmp(wbuf->addr2,junzi_mac,6))
            return -1;
        */
        if (memcmp(wbuf->addr1, "\xff\xff\xff\xff\xff\xff", 6) == 0) {
            return 0;
        }
        else if (memcmp(wbuf->addr1, "\x01\x00\x5e", 3) == 0) {
            return 0;
        }
    }

    return -1;
}

void processPacket(u_char* argu, const struct pcap_pkthdr* pkthdr, const u_char* packet)
{
    struct rtl_wifi_header* rh = (struct rtl_wifi_header*)packet;
    unsigned short pk_len = rh->pkt_len;

    AMY_LOG_ERROR("---------------------->>\n");
    if (pk_len > 0) {
        /* 对抓到的包进行处理 */
        if (rh->data[0] != 0xff) {
            if (s_pSnifferCall && realuser_parse_80211packet(rh->data, pk_len) == 0) {
                s_pSnifferCall(rh->data, pk_len);       /* 将满足需求的包送给上层进行处理 */
            }
        }
    }
}

static void* rtl8197_func_Sniffer(void* arg)
{
    pthread_detach(pthread_self());

    sniffer_thr_run = 1;
    TKL_LOGD("################# enter pcap loop #######################");

    if (device)
        pcap_loop(device, -1, processPacket, NULL);

    TKL_LOGD("################# exit pcap loop #######################");
    sniffer_thr_run = 0;

    return (void*)0;
}

int tkl_gen_net_set_sniffer(const int en, const SNIFFER_CB cb)
{
    char* ifname = WLAN_DEV;
    char cmd[100] = { 0x0 };

    if (en) {
        TKL_LOGD("TKL WiFi : Enable Sniffer");

        if (s_enable_sniffer)    /* 加判断，防止发生特殊情况 */
            return 0;

        snprintf(cmd, sizeof(cmd), "/sbin/ifconfig %s 0.0.0.0", ifname);     //ifconfig wlan0 0.0.0.0
        sys_cmd_run(cmd, NULL, NULL);
        snprintf(cmd, sizeof(cmd), "/sbin/iwpriv %s set_mib %s", ifname, "opmode=16");    //iwpriv wlan0 set_mib opmode=16
        sys_cmd_run(cmd, NULL, NULL);
        snprintf(cmd, sizeof(cmd), "/sbin/ifconfig %s up", ifname);        //ifconfig wlan0 up
        sys_cmd_run(cmd, NULL, NULL);

        tkl_net_set_sniffer_mode(0);
        s_pSnifferCall = cb;
        s_enable_sniffer = 1;
        pthread_create(&sniffer_thId, NULL, rtl8197_func_Sniffer, NULL);

    }
    else {
        TKL_LOGD("Disable Sniffer11");
        snprintf(cmd, sizeof(cmd), "/sbin/ifconfig %s down", ifname);          //ifconfig wlan0 down
        sys_cmd_run(cmd, NULL, NULL);
        while (1) {
            if (s_enable_sniffer && (!sniffer_thr_run)) {
                pcap_close(device);
                s_enable_sniffer = 0;
                TKL_LOGD("sniffer thread exit");
                return 0;
            }
            usleep(50 * 1000);
        }
    }

    return 0;
}

#endif

static void* recev_frame(void* arg)
{
#if 1
    int sockraw;
    int ret = 0;
    unsigned char pkt[8192] = { 0 };
    char* if_name = WLAN_DEV;

    if ((sockraw = tkl_net_create_rawsocket(ETH_P_ALL)) == -1) {
        TKL_LOGE("WIFI Create socket failed ret:%d", sockraw);
        return NULL;
    }

    if ((ret = tkl_net_bind_rawsocket(if_name, sockraw, ETH_P_ALL)) == -1) {
        TKL_LOGE("WIFI Bind socket failed ret:%d", ret);
        return NULL;
    }

    while (s_enable_recv_frame) {
        if ((ret = tkl_net_recv_rawpacket(sockraw, pkt, sizeof(pkt))) > -1) {
            tkl_recv_cb(pkt, ret);
            //fan_dump(pkt+18, ret);
            //TKL_LOGD("WIFI recv data length : %d", ret);
        }
        else {
            TKL_LOGD("WIFI recv data failed");
        }
    }

    return NULL;
#else
    int sock = socket(PF_PACKET, SOCK_RAW, htons(0x03));//ETH_P_ALL
    if (sock < 0) {
        TYLOGD("Sniffer Socket Alloc Fails %d \r\n", sock);
        perror("Sniffer Socket Alloc");
        return (void*)0;
    }

    if (1) {/* 强制绑定到wlan0 上。后续可以考虑去掉 */
        struct ifreq ifr;
        memset(&ifr, 0x00, sizeof(ifr));
        //strncpy(ifr.ifr_name, WLAN_DEV , strlen(WLAN_DEV) + 1);
        strncpy(ifr.ifr_name, "wlan1", strlen(WLAN_DEV) + 1);
        setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, (char*)&ifr, sizeof(ifr));
    }

#define MAX_REV_BUFFER 512
    BYTE_T rev_buffer[MAX_REV_BUFFER];

    while (s_enable_recv_frame) {
        int rev_num = recvfrom(sock, rev_buffer, MAX_REV_BUFFER, 0, NULL, NULL);
        if (rev_num > 0) {
            TYLOGD("Sniffer Proc recv_len: %d\r\n", rev_num);
            s_pSnifferCall(rev_buffer, rev_num, 99);
            fan_dump(rev_buffer, rev_num);
        }
    }
    s_pSnifferCall = NULL;
    close(sock);
    TYLOGD("Sniffer Proc Finish\r\n");
#endif
}

int tkl_gen_register_recv_mgnt_callback(CONST BOOL_T enable, CONST WIFI_REV_MGNT_CB recv_cb)
{
    char* ifname = WLAN_DEV;

    AMY_LOG_ERROR("---------------------->>\n");
    if (enable == TRUE) {
        TKL_LOGD("Enable WIFI recv mgnt frame");
        tkl_net_wifi_enable(ifname, 1);
        tkl_net_set_mode(ifname, "Monitor");
        tkl_recv_cb = recv_cb;
        s_enable_recv_frame = 1;
        pthread_create(&recv_frame_thId, NULL, recev_frame, NULL);
    }
    else {
        TKL_LOGD("Disable recv frame");
        s_enable_recv_frame = 0;
        pthread_join(recv_frame_thId, NULL);
        tkl_recv_cb = NULL;
        tkl_net_set_mode(ifname, "Managed");
    }

    return 0;
}

#ifdef NL80211
int tkl_gen_send_mgnt(CONST UCHAR_T* buf, CONST UINT_T len)
{
    char* ifname = WLAN_DEV;
    struct ifreq t_ifr;
    struct packet_mreq t_mr;
    struct sockaddr_ll t_sll;
    int socket_raw;

    socket_raw = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (socket_raw < 0) {
        TKL_LOGE("socket failed\n");
        return -1;
    }

    memset(&t_ifr, 0, sizeof(t_ifr));
    strncpy(t_ifr.ifr_name, ifname, sizeof(t_ifr.ifr_name) - 1);
    if (ioctl(socket_raw, SIOCGIFINDEX, &t_ifr) < 0) {
        TKL_LOGE("ioctl failed\n");
        close(socket_raw);
        return -1;
    }

    memset(&t_sll, 0, sizeof(t_sll));
    t_sll.sll_family = AF_PACKET;
    t_sll.sll_ifindex = t_ifr.ifr_ifindex;
    t_sll.sll_protocol = htons(ETH_P_ALL);
    //t_sll.sll_protocol=htons(ETH_P_PAE);
    //t_sll.sll_halen = ETH_ALEN;
    //memcpy(t_sll.sll_addr,dstmac,ETH_ALEN);
    if (bind(socket_raw, (struct sockaddr*)&t_sll, sizeof(t_sll)) < 0) {
        perror("bind socket failed\n");
        close(socket_raw);
        return -1;
    }

    memset(&t_mr, 0, sizeof(t_mr));
    t_mr.mr_ifindex = t_sll.sll_ifindex;
    //t_mr.mr_type = PACKET_MR_PROMISC;
    t_mr.mr_type = PACKET_HOST | PACKET_MULTICAST;//| PACKET_MR_PROMISC;
    if (setsockopt(socket_raw, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &t_mr, sizeof(t_mr)) < 0) {
        perror("setsockopt failed\n");
        close(socket_raw);
        return -1;
    }

    //char tmp_buf[256] = {0};
    //strcpy(tmp_buf,t_radiotap);
    //memcpy(tmp_buf + strlen(t_radiotap),buf,len);

    //int t_size = write(socket_raw, tmp_buf, len + strlen(t_radiotap));
    int t_size = tkl_net_send_rawpacket(socket_raw, (unsigned char*)buf, len);
    if (t_size < 0) {
        TKL_LOGE("write() failed : seng packet failed\n");
        close(socket_raw);
        return -1;
    }

    close(socket_raw);

    return 0;
}

#else /* not NL80211 -> rtl8197 */
int tkl_gen_send_mgnt(CONST UCHAR_T* buf, CONST UINT_T len)
{
    AMY_LOG_ERROR("---------------------->>\n");
    TKL_LOGD("send mgnt is not support\n");
    return -1;
}
#endif

#ifdef NL80211
int tkl_gen_set_lp_mode(CONST BOOL_T en, CONST UINT_T dtim)
{
    TKL_LOGD("set lp mode is not support\n");
    return -1;
}

#else
int tkl_gen_set_lp_mode(CONST BOOL_T en, CONST UINT_T dtim)
{
    AMY_LOG_ERROR("---------------------->>\n");
    char* ifname = WLAN_DEV;
    char cmd[128] = { 0x0 };

    if (en == TRUE) {
        snprintf(cmd, sizeof(cmd), "/sbin/iwpriv %s set_mib dtimperiod=%d", ifname, dtim);
        sys_cmd_run(cmd, NULL, NULL);
    }

    return 0;
}
#endif

