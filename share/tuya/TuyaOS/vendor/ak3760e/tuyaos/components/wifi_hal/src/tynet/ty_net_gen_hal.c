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

#include "ty_net.h"
#include "ty_net_hal.h"
#include "ty_net_log.h"
#include "wpa_command.h"

#define MAX_REV_BUFFER          (512)
#define MAX_AP_SEARCH           (128)

#ifndef WLAN_DEV
#define WLAN_DEV     	"wlan0"
#endif

#ifndef WLAN_MONITOR
#define WLAN_MONITOR   	"wlan1"
#endif

#ifndef MK_PATH
#define MK_PATH			"/var/run/wpa_supplicant"
#endif

#define WIFI_DHCPD_CONF   "/tuya/data/udhcpd.conf"
#define WIFI_STATION_CONF "/tuya/data/wpa_0_8.conf"
#define WIFI_HOSTAPD_CONF "/tuya/data/hostapd.conf"

#ifndef WIFI_AP_DEFAULT_IP
#define WIFI_AP_DEFAULT_IP  "192.168.175.1"
#endif

#define WPA_CLI_CTRL_PATH  	"/var/run/wpa_supplicant"
#define WIFI_TYPE_SSW101B	"ssw101b_wifi_usb"
#define GET_WIFI_TYPE_M1	"lsmod | grep %s"

#define CREATE_UDHCPD_DIR      "mkdir -p /var/lib/misc"
#define CREATE_UDHCPD_CFG_FILE "touch /var/lib/misc/udhcpd.leases"	

#define MKDIR_CMD "mkdir -p %s"
#define UDHCPC_CMD "udhcpc -i %s &"
#define HOSTAPD_CMD "hostapd %s -dd &"
#define UDHCPD_CMD  "udhcpd -f %s &"

#define WPA_SUPPLICANT_CMD "wpa_supplicant -Dnl80211 -i %s -c "WIFI_STATION_CONF" -B -dd"

COUNTRY_CODE_S g_c_code[COUNTRY_NR] = {
	{0x20, 14, 14},		//China
	{0x00, 12, 14},		//USA
	{0x09, 15, 15},		//Japan
	{0x04, 14, 14},		//Europe
};


COUNTRY_CODE_S g_c_code_8723[COUNTRY_NR] = {
	{0x48, 14, 14},		//China
	{0x76, 12, 14},		//USA
	{0x27, 15, 15},		//Japan
	{0x26, 14, 14},		//Europe
};

const char *g_wifi_name[WIFI_TYPE_NR] = {
	"rtl8188fu",
	"rtl8192fu",
	"rtl8723ds"
};

extern int ty_net_hal_stop_ap(void);

static pthread_t sniffer_thId;
static volatile int s_enable_sniffer = 0;
static volatile SNIFFER_CB s_pSnifferCall = NULL;
static volatile uint8_t s_curret_channel=6;
static pthread_mutex_t s_channel_mutex=PTHREAD_MUTEX_INITIALIZER;
static char s_default_ip[32]={0};

#pragma pack(1)
typedef struct {
	uint8_t it_version;
	uint8_t it_pad;
	uint16_t it_len;
	uint32_t it_present;
}ieee80211_radiotap_header;
#pragma pack()

static WIFI_TYPE_E ty_net_get_wifi_type(void);

static int exec_cmd (const char *format, ...)
{
	char buf[256]= {0};
	va_list v_list;

	va_start (v_list, format);
	vsnprintf ((char *)buf, sizeof(buf), format, v_list);
	va_end (v_list);

	TN_LOGI("run %s\n",buf);
	return system ((char *)buf);
}

static void process_kill(char *proc)
{
    char cmd_buf[128] = {0x0};
    snprintf(cmd_buf,sizeof(cmd_buf),"ps | grep '%s' | grep -v grep | awk '{print $1}' |xargs kill -9",proc);
    exec_cmd(cmd_buf);
}


static int station_prepare_configure(const char *path, const char *ssid,const char *passwd) 
{
    int fd = 0;
    char buf[1024] = {0};

    unlink(WIFI_STATION_CONF);

    fd = open(WIFI_STATION_CONF, O_RDWR | O_CREAT, 0x666);
    if (fd < 0) {
        TN_LOGE("create configure error %d\n", errno);
        return -1;
    }

    if(ssid != NULL && strlen(ssid) != 0) {
        if (passwd != NULL && 0 != strlen(passwd)) {
            sprintf(buf,"ctrl_interface=%s\nnetwork={\nssid=\"%s\"\nscan_ssid=1\npairwise=CCMP TKIP\ngroup=CCMP TKIP WEP104 WEP40\npsk=\"%s\"\n}", path, ssid, passwd);
        } else {
            sprintf(buf,"network={\nssid=\"%s\"\nscan_ssid=1\nkey_mgmt=NONE\n}", ssid);
        }
    } else {
        TN_LOGE("create configure error %d\n", errno);
        return -1;
    }

    TN_LOGD("wpa_cfg:%s\n",buf);
    if(write(fd, buf, strlen(buf)) < 0) {
        TN_LOGE("write buf errno %d\n", errno);
        return -1;
    }

    close(fd);

    return 0;
}

int ty_net_hal_start_station_connect(const char *ssid, const char *passwd)
{
    char cmd[128] = {0};
    char *ifname = WLAN_DEV;
    char *ip_d = "0.0.0.0";
    int s_wait = 30;
    WF_HAL_STATION_STAT_E stat;

    if (!ssid)
        return -1;

    /* 环境准备：kill      wpa_supplicant/udhcpc ,清IP */
    ty_net_hal_set_station_mode(0);
    process_kill("wpa_supplicant");
    process_kill("udhcpc");
    ty_net_set_ip(ifname,ip_d);

    if (station_prepare_configure(MK_PATH, ssid, passwd)) {
        TN_LOGE("wpa_supplicant prepare config error %d\n", errno);
        return -1;
    }

    /* 启动wpa_supplicant 连接ap */
    exec_cmd(MKDIR_CMD, MK_PATH);
    exec_cmd(WPA_SUPPLICANT_CMD,WLAN_DEV);

    /* 启动udhcpc 获取ip */
    usleep(500);
    exec_cmd(UDHCPC_CMD,WLAN_DEV);

    TN_LOGI("\n=======wait %d s for connect=======\n",s_wait);

    ty_net_hal_get_status(&stat);
    while(stat != WF_WSS_GOT_IP && --s_wait > 0) {
        ty_net_hal_get_status(&stat);
        sleep(1);
    }

    return 0;
}

int ty_net_hal_scan_all_ap(WF_HAL_AP_IF_S **ap_ary, uint32_t *num)
{
	int index = 0;
	int ret = 0;
	char *wlan = WLAN_DEV;
    WIFI_TYPE_E wf_t;
	static WF_HAL_AP_IF_S s_aps[MAX_AP_SEARCH];

	TN_LOGD();

	memset(s_aps, 0, sizeof(s_aps));
	
	*num = 0;
	*ap_ary = s_aps;

    ty_net_enable(wlan, 1);

    wf_t = ty_net_get_wifi_type();
    if(wf_t == WIFI_HI3881) {
        ret = ty_wifi_scan_wpa(wlan,*ap_ary,MAX_AP_SEARCH);
    } else {
        ret = ty_wifi_scan(wlan,*ap_ary,MAX_AP_SEARCH);
    }

	if(ret == 0)        /* 未扫描到AP */
	{
		return -1;
	}
	*num = ret;
    
	TN_LOGD("fan_debug: print s_aps[MAX_AP_SEARCH] MAX_AP_SEARCH=%d, ret=%d", MAX_AP_SEARCH, ret);
	
	TN_LOGD("fan_debug: print s_aps[MAX_AP_SEARCH]");
	for(index = 0; index < *num; index++) {
		TN_LOGD("index:%d channel:%d bssid:%02X-%02X-%02X-%02X-%02X-%02X RSSI:%d SSID:%s\n",
				index,s_aps[index].channel,s_aps[index].bssid[0],s_aps[index].bssid[1]					
				,s_aps[index].bssid[2],s_aps[index].bssid[3],s_aps[index].bssid[4],
					s_aps[index].bssid[5], s_aps[index].rssi, s_aps[index].ssid);
	}

	TN_LOGD("WIFI Scan AP End");

	return 0;
}

int ty_net_hal_product_test_scan_all_ap(WF_HAL_AP_IF_S **ap_ary, uint32_t *num)
{
	int index = 0;
	char *wlan = WLAN_DEV;
	static WF_HAL_AP_IF_S s_aps[MAX_AP_SEARCH];

	TN_LOGD();

	*num = 0;
	*ap_ary = s_aps;
	memset(s_aps, 0, sizeof(s_aps));

	*num = ty_net_wifi_scan_for_product_test(wlan,*ap_ary,MAX_AP_SEARCH);
	if(*num <= 0){
		return -1;
	}
	
	//print all scanned ap
	for(index = 0; index < *num; index++) {
		TN_LOGD("index:%d channel:%d bssid:%02X-%02X-%02X-%02X-%02X-%02X RSSI:%d SSID:%s\n",
				index,s_aps[index].channel,s_aps[index].bssid[0],s_aps[index].bssid[1]					
				,s_aps[index].bssid[2],s_aps[index].bssid[3],s_aps[index].bssid[4],
					s_aps[index].bssid[5], s_aps[index].rssi, s_aps[index].ssid);
	}
	TN_LOGD("WIFI Scan AP End");

	return 0;
}

int ty_net_hal_scan_assigned_ap(const char *ssid, WF_HAL_AP_IF_S **ap)
{
	int iret = 0;	
	int index = 0;
	WF_HAL_AP_IF_S *p_total_ap = NULL;
	uint32_t total_num = 0;

	*ap = NULL;

	TN_LOGD();

	ty_net_hal_scan_all_ap(&p_total_ap, &total_num);

	for(index = 0; index < total_num; index++) {
		if(memcmp(p_total_ap[index].ssid, ssid, p_total_ap[index].s_len) == 0) {
			*ap = p_total_ap + index;
			break;
		}
	}

	if (index >= total_num)
		iret = -1;	

	return iret;
}

/*********************************************************************
*函数名		:	ty_net_hal_set_cur_channel
*参数      		:	设置wifi的当前channel
*函数功能	:	channel (1~14)
*返回值		:	0，成功；-1 失败
**********************************************************************/
int ty_net_hal_set_cur_channel(const uint8_t channel)
{
	int ret=0;

	TN_LOGD("%d\n",channel);

	pthread_mutex_lock(&s_channel_mutex);

	if(ty_wifi_set_chn(WLAN_MONITOR,channel))
		ret = -1;
	else
		s_curret_channel = channel;

	pthread_mutex_unlock(&s_channel_mutex);

	return ret;
}

/*********************************************************************
*函数名		:	ty_net_hal_get_cur_channel
*参数      		:	空
*函数功能	:	获取 wifi的当前channel
*返回值		:	0，成功；-1 失败；channel 指针，用于返回通道号
**********************************************************************/
int ty_net_hal_get_cur_channel(uint8_t *channel)
{
	int chan;

	TN_LOGD();

	pthread_mutex_lock(&s_channel_mutex);
	if(ty_wifi_get_chn(WLAN_MONITOR,(int*)&chan)){
		TN_LOGW("ty_wifi_get_chn fail,used save channel:%d\n",s_curret_channel);
		*channel = s_curret_channel;
	}else{
		*channel = chan & 0xff;
	}
	pthread_mutex_unlock(&s_channel_mutex);

	return 0;
}

static void *func_Sniffer(void *arg)
{
	int sock	= 0;
	int skipLen = 18;
	char *wlan 	= WLAN_DEV;
	struct ifreq ifr;
	uint8_t rev_buffer[MAX_REV_BUFFER];

	TN_LOGD("Sniffer Thread Create\n");

	pthread_detach (pthread_self());

	sock = socket(PF_PACKET, SOCK_RAW, htons(0x03));//ETH_P_ALL

	if(sock < 0) {
		TN_LOGE("Sniffer Socket Alloc");
		return (void *)0;
	}

	memset(&ifr, 0x00, sizeof(ifr));
	strncpy(ifr.ifr_name, wlan, strlen(wlan) + 1);
	setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, (char *)&ifr, sizeof(ifr));

	while((s_pSnifferCall != NULL) && (s_enable_sniffer == 1)) {
		int rev_num = recvfrom(sock,rev_buffer,MAX_REV_BUFFER,0,NULL,NULL);
		ieee80211_radiotap_header *pHeader = (ieee80211_radiotap_header *)rev_buffer;
		skipLen = pHeader->it_len;

		if(skipLen >= MAX_REV_BUFFER){
			continue;
		}

		if(0){
			int index = 0;
			printf("skipLen:%d ", skipLen);
			for(index = 0; index < rev_num-skipLen; index++) {
				printf("%02X-", rev_buffer[index+skipLen]);
			}
			printf("\r\n");
		}

		if(rev_num > skipLen){
			s_pSnifferCall(rev_buffer+skipLen, rev_num-skipLen);
		}
	}

	s_pSnifferCall = NULL;
	close(sock);
	TN_LOGD("=====Sniffer Proc Finish=====");
	return (void *)0;
}

int ty_net_hal_set_sniffer(const int en, const SNIFFER_CB cb)
{
	char *wlan = WLAN_DEV;

	TN_LOGD("en:%d",en);

	WIFI_TYPE_E wifi_type = WIFI_TYPE_NR;

	wifi_type = ty_net_get_wifi_type();

	if (WIFI_8723DS == wifi_type) {
		printf("WIFI_8723DS is not support to sniffer mode\n");
		return -1;
	}

	if(en) {
		TN_LOGD("Enable Sniffer");

		if(s_enable_sniffer)
			return 0;

		ty_net_hal_set_sniffer_mode(0);
		exec_cmd("ifconfig %s 0.0.0.0", wlan);
			
		s_pSnifferCall = cb;
		s_enable_sniffer = 1;
		pthread_create(&sniffer_thId, NULL, func_Sniffer, NULL);
	}else{
		TN_LOGD("Disable Sniffer");
		s_enable_sniffer = 0;
		ty_net_hal_set_station_mode(0);
	}
	return 0;
}

int ty_net_hal_get_ip(int index,char *ip)
{
	int ret;
	char *wlan=WLAN_DEV;

	if(index)	
		wlan=WLAN_MONITOR;

	ret =  ty_net_get_ip(wlan,ip);
	TN_LOGD("WIFI Get IP:%s\n", ip);

	return ret;
}

int ty_net_hal_set_ip(int index, const char *ip)
{
	int ret;
	char *wlan=WLAN_DEV;

	if(index)	
		wlan=WLAN_MONITOR;

	ret =  ty_net_set_ip(wlan, ip);
	if (ret < 0)
		TN_LOGD("WIFI Set IP:%s error\n", ip);
	
	return 0;
}

int ty_net_hal_get_mac(int index,uint8_t *mac)
{
	int ret;
	char *wlan=WLAN_DEV;

	if(index)
		wlan=WLAN_MONITOR;

	ret = ty_net_get_mac(wlan,mac);

	TN_LOGD("WIFI Get MAC:%02x:%02x:%02x:%02x:%02x:%02x\n",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);

	return ret;
}

int ty_net_hal_get_bssid(UINT8_T *bssid)
{
    int ret;
    ret = wifi_get_current_connect_ap_bssid(bssid);
    if(ret < 0)
    {
        TN_LOGD("get bssid failed");
        return -1;
    }
    
    return 0;
}

int ty_net_hal_set_mac(int index, uint8_t *mac)
{
	int ret;
	char *wlan=WLAN_DEV;

	if(index)	
		wlan=WLAN_MONITOR;


	TN_LOGD("WIFI Get MAC:%02x:%02x:%02x:%02x:%02x:%02x\n",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);

	ret = ty_net_set_mac(wlan,mac);

	return ret;
}


int ty_net_hal_wifi_station_get_ap_mac(int index, uint8_t *mac)
{
	int ret;
	char *wlan=WLAN_DEV;

	if(index)	
		wlan=WLAN_MONITOR;

	ret = ty_net_get_ap_addr(wlan,mac);

	TN_LOGD("WIFI Get MAC:%02x:%02x:%02x:%02x:%02x:%02x\n",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);

	return ret;

}

int ty_net_hal_set_sniffer_mode(int index)
{
	char *wlan = WLAN_DEV;
	WIFI_TYPE_E wifi_type = WIFI_TYPE_NR;

	if(index)	
		wlan=WLAN_MONITOR;

	wifi_type = ty_net_get_wifi_type();
	
	if (WIFI_SSW101B == wifi_type) {
		ty_net_enable(wlan, 0);
		ty_wifi_set_mode(wlan, "Monitor");
		ty_net_enable(wlan, 1);
	}else {
		ty_net_enable(wlan, 0);
		ty_net_enable(wlan, 1);		
		ty_wifi_set_mode(wlan, "Monitor");
	}	

	return 0;
}

int ty_net_hal_set_station_mode(int index)
{
	char *wlan=WLAN_DEV;
	WIFI_TYPE_E wifi_type = WIFI_TYPE_NR;

	if(index)	
		wlan=WLAN_MONITOR;

	wifi_type = ty_net_get_wifi_type();
	
	if (WIFI_SSW101B == wifi_type) {
		ty_net_enable(wlan, 0);
		ty_wifi_set_mode(wlan, "Managed");
		ty_net_enable(wlan, 1);
	}else {
		ty_net_enable(wlan, 0);
		ty_net_enable(wlan, 1);		
		ty_wifi_set_mode(wlan, "Managed");
	}

	return 0;
}

int ty_net_hal_set_softap_mode(int index)
{
	char *wlan=WLAN_DEV;
	WIFI_TYPE_E wifi_type = WIFI_TYPE_NR;	

	if(index)	
		wlan=WLAN_MONITOR;

	wifi_type = ty_net_get_wifi_type();
	
	if (WIFI_SSW101B == wifi_type) {
		ty_net_enable(wlan, 0);
		ty_wifi_set_mode(wlan, "Master");
		ty_net_enable(wlan, 1);
	}else {
		ty_net_enable(wlan, 0);
		ty_net_enable(wlan, 1);		
		ty_wifi_set_mode(wlan, "Master");
	}

	return 0;
}

/*  mode:
    0 --- Auto
    1 --- Ad-Hoc
    2 --- Managed
    3 --- Master
    4 --- Repeater
    5 --- Secondary
    6 --- Monitor
*/

int ty_net_hal_get_work_mode(int index,WIFI_MODE_E *mode)
{
	int wifi_mode;
	char *wlan=WLAN_DEV;

	if(index)	
		wlan=WLAN_MONITOR;

	*mode = WIFI_MODE_MANAGED;

	if(ty_wifi_get_mode(wlan,&wifi_mode)) {
		TN_LOGE("ty wifi get mode fail\n");
		return -1;
	}

	*mode = wifi_mode;	

	TN_LOGD("WIFI Get Mode:%d", *mode);

	return 0;
}

int ty_net_hal_station_disconnect(void)
{
    int iret = 0;
    char *ifname = WLAN_DEV;
    char *ip = "0.0.0.0";
    TN_LOGD();

    iret = wifi_disconnect_network();
    {
        TN_LOGE("Disconnect ap failed");
        return -1;
    }

    /* kill wpa_supplicant and udhcpc */
    process_kill("wpa_supplicant");
    process_kill("udhcpc");

    /* 清除station的 IP 地址 */
    iret = ty_net_set_ip(ifname, ip);
    if(iret < 0)
    {
        TN_LOGE("clear %s Ip failed",ifname);
    }

    return 0;
}

int ty_net_hal_get_conn_ap_rssi(int8_t *rssi)
{
    char *wlan=WLAN_DEV;
    int value=0;

	if(ty_wifi_get_curr_rssi(wlan,&value)) {
		TN_LOGE("get curr rssi fail\n");
		return -1;	
	}

	*rssi = value&0xff;

	TN_LOGD("Get Conn AP RSSI:%d", *rssi);
	return 0;
}

#ifdef WIFI_SCAN_RETRY
int ty_net_hal_get_assign_ap_rssi(char *ssid, int *rssi)
{
	int iret = 0;	
	int index = 0;
	WF_HAL_AP_IF_S *p_total_ap = NULL;
	uint32_t total_num = 0;
	int count = 3;

SCAN:

	ty_net_hal_scan_all_ap(&p_total_ap, &total_num);
	
	count--;
	for(index = 0; index < total_num; index++) {
		if(strcmp(p_total_ap[index].ssid, ssid) == 0) {
			if(p_total_ap[index].rssi > 90 || p_total_ap[index].rssi < 40){
				if(count > 0){
					goto SCAN;
				}else{
					return -1;
				}
			}
			*rssi = p_total_ap[index].rssi -100;
			break;
		}
	}

	if (index >= total_num)
		iret = -1;	

	return iret;
}

int ty_net_hal_product_test_get_assign_ap_rssi(char *ssid, int *rssi)
{
	int iret = 0;
	int index = 0;
	WF_HAL_AP_IF_S *p_total_ap = NULL;
	uint32_t total_num = 0;
	int cnts = 3;
	
	iret = ty_net_hal_product_test_scan_all_ap(&p_total_ap, &total_num);
	if(iret == -1){
		goto SCAN_AGAIN;
	}
	
	for(index = 0; index < total_num; index++) {
		if(strcmp(p_total_ap[index].ssid, ssid) == 0) {
			if(p_total_ap[index].rssi > 90 || p_total_ap[index].rssi < 40){
				TN_LOGD("p_total[%d].rssi : %d",index,p_total_ap[index].rssi);
				TN_LOGD("rssi beyong range [-10,-60], scan again");
				 break;
			}
			 *rssi = p_total_ap[index].rssi - 100;
			 return 0;
		}
	}

SCAN_AGAIN:

	while (cnts >= 0) {
		iret = ty_net_hal_scan_all_ap(&p_total_ap, &total_num);
		if(iret == -1){
			cnts --;
			continue;
		}
		
		for(index = 0; index < total_num; index++) {
			if(strcmp(p_total_ap[index].ssid, ssid) == 0) {
				if(p_total_ap[index].rssi > 90 || p_total_ap[index].rssi < 40){
					TN_LOGD("rssi beyong range [-10,-60], scan again");
					cnts --;
					break;
				}
				*rssi = p_total_ap[index].rssi - 100;
				return 0;
			}
		}
	}

   TN_LOGD("Got Scan result , but can't find assign ap's rssi");
	
	return iret;
}

#else 
int ty_net_hal_get_assign_ap_rssi(char *ssid, int *rssi)
{
	int iret = 0;	
	int index = 0;
	WF_HAL_AP_IF_S *p_total_ap = NULL;
	uint32_t total_num = 0;
	int found = 0;

	ty_net_hal_scan_all_ap(&p_total_ap, &total_num);
	
	for(index = 0; index < total_num; index++) {
		if(strcmp(p_total_ap[index].ssid, ssid) == 0) {
			*rssi = p_total_ap[index].rssi -100;
			found = 1;
			break;
		}
	}

	if ( !found ) {
		TN_LOGD("can't find assign ap's rssi");
		return -1;
	}

	if (index >= total_num)
		iret = -1;	

	return iret;
}

int ty_net_hal_product_test_get_assign_ap_rssi(char *ssid, int *rssi)
{
	int iret = 0;
	int index = 0;
	WF_HAL_AP_IF_S *p_total_ap = NULL;
	uint32_t total_num = 0;
	int found = 0;
	
	iret = ty_net_hal_product_test_scan_all_ap(&p_total_ap, &total_num);
	if(iret == -1){
		goto SCAN_AGAIN;
	}
	
	for(index = 0; index < total_num; index++) {
		if(strcmp(p_total_ap[index].ssid, ssid) == 0) {
			TN_LOGD("p_total[%d].rssi : %d",index,p_total_ap[index].rssi);
			 *rssi = p_total_ap[index].rssi - 100;
			 return 0;
		}
	}

SCAN_AGAIN:
	iret = ty_net_hal_scan_all_ap(&p_total_ap, &total_num);
	if(iret == -1) {
		return -1;
	}

	for(index = 0; index < total_num; index++) {
		if(strcmp(p_total_ap[index].ssid, ssid) == 0) {
			*rssi = p_total_ap[index].rssi - 100;
			found = 1;
			return 0;
		}
	}

	if ( !found ) {
		TN_LOGD("can't find assign ap's rssi");
		return -1;
	}

	return iret;
}


#endif 

int ty_net_hal_get_status(WF_HAL_STATION_STAT_E *stat)
{
	char ip[32]={0};

	if(ty_net_hal_get_ip(0,ip) || strlen(ip) == 0) {
		*stat =	WF_WSS_CONN_FAIL;
	} else if(strcmp(ip,s_default_ip) == 0) {
		*stat =	WF_WSS_CONN_FAIL;
	} else {
		*stat = WF_WSS_GOT_IP;
	} 


	TN_LOGD("Curr WIFI Stat:%d", *stat);
	return 0;
}

static int softap_prepare_configure(const char *ssid,const char *psk,int channel) 
{
    int fd = 0;
    char buf[1024] = {0};
    int ret = 0;
    char *ifname = WLAN_DEV;

    TN_LOGD("%s->%d\n",__func__,__LINE__);
    unlink(WIFI_HOSTAPD_CONF);

    fd = open(WIFI_HOSTAPD_CONF, O_RDWR | O_CREAT, 0x666);
    if (fd < 0) {
        TN_LOGE("create configure error %d\n", errno);
        return -1;
    }

    TN_LOGD("%s->%d\n",__func__,__LINE__);
    if (psk != NULL && strlen(psk) > 0) {
        sprintf(buf, "interface=%s\n"
                     "driver=nl80211\n"
                     "ieee80211n=1\n"
                     "ssid=%s\n"
                     "hw_mode=g\n"
                     "channel=%d\n"
                     "macaddr_acl=0\n"
                     "ignore_broadcast_ssid=0\n"
                     "wpa=2\n"
                     "wpa_passphrase=%s\n"
                     "wpa_key_mgmt=WPA-PSK\n"
                     "wpa_pairwise=TKIP CCMP\n"
                     "rsn_pairwise=CCMP TKIP\n",
                     ifname, ssid, channel, psk);
    } else if (psk == NULL || strlen(psk) == 0)
    {
        sprintf(buf, "interface=%s\n"
                     "driver=nl80211\n"
                     "ctrl_interface=/var/run/hostapd\n"
                     "ssid=%s\n"
                     "channel=%d\n"
                     "ieee80211n=1\n"
                     "hw_mode=g\n"
                     "ignore_broadcast_ssid=0\n"
                     "#wpa=3\n"
                     "#wpa_passphrase=\n"
                     "#wpa_key_mgmt=WPA-PSK\n"
                     "#wpa_pairwise=TKIP CCMP\n"
                     "#rsn_pairwise=CCMP\n",
                     WLAN_DEV,ssid, channel);
    }

    TN_LOGD("buf:%s\n",buf);
    ret = write(fd, buf, strlen(buf));
    if (ret < 0) {
        TN_LOGE("write hostapd buf errno %d\n", errno);
        return -1;
    }

    close(fd);

    return 0;
}

static int udhcpd_prepare_configure(char*start, char *end,char *ip_router) 
{
    int fd = 0;
    char buf[256] = {0};
    int ret = 0;

    TN_LOGD();
    unlink("udhcpd.conf");

    fd = open(WIFI_DHCPD_CONF,O_RDWR | O_CREAT, 0x666);
    if (fd < 0) {
        TN_LOGE("create configure error %d\n", errno);
        return -1;
    }

    sprintf(buf, "start\t%s\nend\t%s\ninterface\t%s\nmax_leases\t234\nopt router\t%s\n",start,end,WLAN_DEV,ip_router);

    ret = write(fd, buf, strlen(buf));
    if (ret < 0) {
        TN_LOGE("write hostapd buf errno %d\n", errno);
        return -1;
    }

    close(fd);
    return 0;
}

static int _linux_start_ap(const char *ssid,const char *psk, char *ipaddr,int channel)
{
    char buf[512] = {0};
    int ret = 0;
    int i = 0,dot = 0;
    char ipaddr_pre[20] = {0};
    char start[64] = {0};
    char end[64] = {0};

    TN_LOGI("wifi ap start\n");
    ty_net_hal_stop_ap();    /* 先stop ap */
    process_kill("wpa_supplicant");
    process_kill("udhcpc");

    ret = softap_prepare_configure(ssid, psk,channel);
    if (ret) {
        TN_LOGE("hotapd prepare config error %d\n", errno);
        return -1;
    }

    ret = exec_cmd(HOSTAPD_CMD,WIFI_HOSTAPD_CONF);
    if (ret) {
        TN_LOGE("hostapd start  error %d\n", errno);
        return -1;
    }

    memset(buf, 0, sizeof(buf));
    sprintf(buf, "ifconfig %s %s",WLAN_DEV,ipaddr);
    ret = exec_cmd(buf);
    if (ret) {
        TN_LOGE("ifconfig %s error %d\n",WLAN_DEV,errno);
        return -1;
    }

    exec_cmd(CREATE_UDHCPD_DIR);
    exec_cmd(CREATE_UDHCPD_CFG_FILE);

    snprintf(ipaddr_pre, sizeof(ipaddr_pre), "%s", ipaddr);
    for(i = 0; i < strlen(ipaddr_pre); i++) {
        if(ipaddr_pre[i] == '.')
            dot++;
        if(dot == 3) {
            ipaddr_pre[i + 1] = 0;
            break;
        }
    }
    if(dot != 3){
        TN_LOGE("ipaddr(%s) is not legal", ipaddr);
        return -1;
    }

    snprintf(start,sizeof(start), "%s%d",ipaddr_pre,2);
    snprintf(end,sizeof(end), "%s%d",ipaddr_pre,254);
    if(udhcpd_prepare_configure(start,end,ipaddr)){
        process_kill("udhcpd");
        process_kill("hostapd");
        TN_LOGE("udhcpd start fail\n");
        return -1;
    }

    exec_cmd(UDHCPD_CMD,WIFI_DHCPD_CONF);
    TN_LOGI("wifi ap start sucess\n");
    sleep(1);

    return 0;
}

int ty_net_hal_stop_ap(void)
{
    int iret = 0;
    char *ifname = WLAN_DEV;
    char *ip = "0.0.0.0";
    char cmd_buf[128] = {0x0};
    char *proc1 = "hostapd";
    char *proc2 = "udhcpd";

    process_kill(proc1);
    process_kill(proc2);

    ty_net_enable(ifname, 0);

    /* 清IP */
    iret = ty_net_set_ip(ifname, ip);
    if(iret < 0) {
        TN_LOGE("clean IP failed");
    }

    return 0;
}

int ty_net_hal_start_ap(int channel,char *ssid,char *passwd,char *ip)
{
    int iret    = -1;
    char ap_ip[16] = {0x0};

//	ty_net_hal_stop_ap();

    if(ssid == NULL) {
        return -1;
    }

    if (ip == NULL)
        strcpy(ap_ip,WIFI_AP_DEFAULT_IP);
    else
        strcpy(ap_ip,ip);

    TN_LOGD("Start AP SSID:%s,PSK:%s,CHANNEL:%d,IP:%s\n",ssid,passwd,channel,ap_ip);

    iret = _linux_start_ap((const char *)ssid,(const char *)passwd,ap_ip,channel);
    if(iret == 0)
        strncpy(s_default_ip,ip,32);

    return iret;
}

#define DIR_NAME_MAX 60
#define CMD_NAME_MAX 60

#define C_CODE_NAME_MAX 60
#define BUF_MAX 100

static WIFI_TYPE_E ty_net_get_wifi_type(void)
{
	FILE *pp				= NULL;
	char cmd[BUF_MAX] 		= {0x00};
	char strtmp[BUF_MAX] 	= {0x00};
	
	if (0 == access("/proc/net/rtl8188fu", F_OK)) {
		return WIFI_8188;
	}else if (0 == access("/proc/net/rtl8189fs", F_OK)) {
		return WIFI_8192;
	}else if (0 == access("/proc/net/rtl8723ds", F_OK)) {
		return WIFI_8723DS;
	}else if (0 == access("/sys/hisys/hipriv", F_OK)) {
		return WIFI_HI3881;
	}else {
		snprintf(cmd, sizeof(cmd), GET_WIFI_TYPE_M1, WIFI_TYPE_SSW101B);
		pp = popen(cmd, "r");
		if (!pp)
			return -1;
		while (fgets(strtmp, sizeof(strtmp), pp) != NULL) {
			if (strstr(strtmp, WIFI_TYPE_SSW101B)) {
				pclose(pp);
				return WIFI_SSW101B;
			}
			memset(strtmp, 0x00, sizeof(strtmp));
		}
		pclose(pp);
		
		return WIFI_TYPE_NR;
	}
}

int ty_net_hal_get_country_code(void)
{
	TN_LOGD();
	char path[DIR_NAME_MAX] = {0x00};
	char buffer[BUF_MAX] = {0x00};
	FILE *fd = 0;
	int c_code_num = -1;
	int i = 0;
	COUNTRY_CODE_S c_code[COUNTRY_NR];
	
	WIFI_TYPE_E wifi_type = WIFI_TYPE_NR;

	wifi_type = ty_net_get_wifi_type();

	if (WIFI_SSW101B == wifi_type) {
		printf("%s is not support to change country code in the running mode\n", WIFI_TYPE_SSW101B);
		return 0x00;
	}

	if (WIFI_8723DS == wifi_type) {
		memcpy(c_code, g_c_code_8723, sizeof(COUNTRY_CODE_S) * COUNTRY_NR);
	}else {
		memcpy(c_code, g_c_code, sizeof(COUNTRY_CODE_S) * COUNTRY_NR);
	}
	
	sprintf(path, "/proc/net/%s/%s/chan_plan", g_wifi_name[wifi_type], WLAN_DEV);
	fd = fopen(path, "r+");
	if (!fd) {
		printf("open %s error, ret=%d.\n", path, fd);
		
		return -1;
	}

	while (fgets(buffer, BUF_MAX, fd)) {
		if (strstr(buffer, "chplan:")) {
			sscanf(buffer+strlen("chplan:"), "%x", &c_code_num);
			break;
		}

		memset(buffer, 0x00, BUF_MAX);
	}
	
	fclose(fd);

	if (c_code_num < 0)
		goto end;

	for (i = 0; i < COUNTRY_NR; i++) {
		if (c_code_num == c_code[i].c_code) {
			return c_code_num;
		}
	}

	if (i >= COUNTRY_NR)
		goto end;
	
end:
	printf("country code null\n");
	
	return -1;	
}

int ty_net_hal_set_country_code(const WF_HAL_COUNTRY_CODE_E ccode)
{
	TN_LOGD();
	char cmd_s_c_code[CMD_NAME_MAX] = {0x00};
	int c_code = 0;
	WIFI_TYPE_E wifi_type = WIFI_TYPE_NR;

	wifi_type = ty_net_get_wifi_type();
	if (WIFI_SSW101B == wifi_type) {
		printf("%s is not support to change country code in the running mode\n", WIFI_TYPE_SSW101B);
		return 0x00;
	}

	if (WIFI_8723DS== wifi_type) {
		c_code = g_c_code_8723[ccode].c_code;
	}else {
		c_code = g_c_code[ccode].c_code;
	}
	sprintf(cmd_s_c_code, "echo %d > /proc/net/%s/%s/chan_plan", c_code, g_wifi_name[wifi_type], WLAN_DEV);

	exec_cmd(cmd_s_c_code);

	return 0;
}

int ty_net_hal_enable_wifi_lowpower(void)
{
	TN_LOGD();
	return 0;
}

int ty_net_hal_disable_wifi_lowpower(void)
{
	TN_LOGD();
	return 0;
}

/*********************************************************************
*函数名		:	ty_net_hal_register_recv_mgnt_cb
*参数      		:	enable:1,注册；0，注销 recv_cb 回调函数指针
*函数功能	:	注册和注销接受函数的回调函数
*返回值		:	0，成功；-1 失败
**********************************************************************/
int ty_net_hal_register_recv_mgnt_cb(int enable,WIFI_REV_MGNT_CB_T recv_cb)
{
	TN_LOGD();
	return 0;
}

/*********************************************************************
*函数名		:	ty_net_hal_close_concurrent_ap
*参数      		:	空
*函数功能	:	关闭 concurrent_ap
*返回值		:	0，成功；-1 失败
**********************************************************************/
int ty_net_hal_close_concurrent_ap(void)
{
	TN_LOGD();
	return 0;
}

/*********************************************************************
*函数名		:	ty_net_hal_send_mgnt
*参数      		:	buf:数据包缓冲,len:数据包大小
*函数功能	:	通过原始套接字发送网络数据包
*返回值		:	0，成功；-1 失败
**********************************************************************/
int ty_net_hal_send_mgnt(const uint8_t *buf, const uint32_t len)
{
	int i =0;
	int t_size;
	uint8_t* tmpBuf;
	struct ifreq t_ifr;
	struct packet_mreq t_mr;
	struct sockaddr_ll t_sll;
	uint8_t* t_radiotap=(uint8_t*)"\x00\x00\x0c\x00\x04\x80\x02\x00\x02\x00\x00\x00";

	int t_socket=socket(PF_PACKET,SOCK_RAW,htons(ETH_P_ALL));

	if(t_socket<0) {
		TN_LOGE("socket failed\n");
		return -1;
	}

	memset(&t_ifr,0,sizeof(t_ifr));
	strncpy(t_ifr.ifr_name,WLAN_DEV,sizeof(t_ifr.ifr_name)-1);

	if(ioctl(t_socket,SIOCGIFINDEX,&t_ifr)<0) {
		TN_LOGE("ioctl failed\n");
		close(t_socket);
		return -1;
	}

	memset(&t_sll,0,sizeof(t_sll));
	t_sll.sll_family=AF_PACKET;
	t_sll.sll_ifindex=t_ifr.ifr_ifindex;
	t_sll.sll_protocol=htons(ETH_P_ALL);

	if(bind(t_socket,(struct sockaddr*)&t_sll,sizeof(t_sll))<0) {
        	TN_LOGE("bind failed\n");
        	close(t_socket);
		return -1;
	}

	memset(&t_mr,0,sizeof(t_mr));
	t_mr.mr_ifindex=t_sll.sll_ifindex;
	t_mr.mr_type=PACKET_MR_PROMISC;

	if(setsockopt(t_socket,SOL_PACKET,PACKET_ADD_MEMBERSHIP,&t_mr,sizeof(t_mr))<0)
	{
		TN_LOGE("setsockoptfailed\n");
		close(t_socket);
		return -1;
	}

   	tmpBuf = malloc(12+len);

	if(tmpBuf==NULL) {
		TN_LOGE("malloc failed\n");
		return 0;
	}
	memcpy(tmpBuf,t_radiotap,12);
	memcpy(tmpBuf+12,buf,len);

//	printf("mgnt data start ");
//	for(i=0;i<len+12;i++) {
//		printf("%d ",tmpBuf[i]);
//	}
//	printf(" mgnt data end\n ");

	t_size=write(t_socket,tmpBuf,len+12);

	if(t_size<0) {
		TN_LOGE("write() failed\n");
    		free(tmpBuf);
        	close(t_socket);
        	return -1;
    	}
    	free(tmpBuf);
    	TN_LOGD("tuya_hwl_send_mgnt is end\n");
	close(t_socket);
	return 0;
}

