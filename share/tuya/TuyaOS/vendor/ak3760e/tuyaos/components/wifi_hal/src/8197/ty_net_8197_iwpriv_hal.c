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
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "ty_net.h"
#include "ty_net_hal.h"
#include "ty_net_log.h"
#include "pcap.h"
#include "iwlib.h"

//8197设备station用wlan0-vxd；ap用wlan0
#ifndef WLAN_DEV
#define WLAN_DEV     	"wlan0"
#endif

#ifndef WLAN_MONITOR
#define WLAN_MONITOR   	"wlan0-vxd"
#endif

#define MAX_AP_SEARCH  (64)
#define MAX_CMD_BUF    (128)
#define MAX_R_BUF      (1024)
#define NETWORK_RESTART "init.sh ap all"

#define IFCONFIG_CMD			"ifconfig %s"
#define UP_CMD					"up"
#define DOWN_CMD				"down"
#define IWCONFIG_CMD			"iwconfig %s"
#define MANAGED_MODE			"managed"
#define MASTER_MODE				"master"
#define COUNTRY_CODE_CMD		"regdomain"
#define CHANNEL_CMD				"channel"
#define OPMODE_CMD				"opmode"
#define BAND_CMD				"band"
#define DENY_LEGACY_CMD			"deny_legacy"
#define USE40M_CMD				"use40M"
#define SSID_CMD				"ssid"
#define S_802_1X_CMD 			"802_1x"
#define AUTHTYPE_CMD			"authtype"
#define ENCMODE_CMD				"encmode"
#define PSK_ENABLE_CMD			"psk_enable"
#define WPA2_CIPHER_CMD			"wpa2_cipher"
#define WPA_CIPHER_CMD			"wpa_cipher"
#define PASSPHRASE_CMD			"passphrase"
#define GET_BASE_CMD			"iwpriv %s get_mib %s | awk '{print $2}'"
#define SET_BASE_CMD			"iwpriv %s set_mib %s"

#define STA_INFO				"sta_info"
#define SS_RESULT				"SS_Result"
#define WLAN_PARM				"cat /proc/%s/%s"
#define UDHCPC_SCRIPT_PATH   		"/tuya/data/8197wlan0_udhcpc.script"
#define UDHCPD_CONF_PATH		"/tuya/data/udhcpd.conf"

#define WF_IP_ETHER_STR			"inet addr:"
#define WF_MAC_ETHER_STR    	"HWaddr "

COUNTRY_CODE_S g_c_code[COUNTRY_NR] = {
	{13, 14, 14},		//China  CN
	{1,  12, 14},		//USA    FCC
	{6,  15, 15},		//Japan  MKK
	{3,  14, 14},		//Europe ETSI
};

enum opmode {
	OP_MANAGED 	= 8,	//clinet mode
	OP_MASTER 	= 16,	//AP mode
	OP_AD_HOC	= 32
};

WIFI_MODE_S g_opmode[WIFI_MODE_NR] = {
	0, OP_AD_HOC, OP_MANAGED, OP_MASTER, 100, 100, 100
};

typedef struct wlan_80211_frame {
	u_char frmae_ctl1;
	u_char frmae_ctl2;
	u_short duration;
	u_char addr1[6];    
	u_char addr2[6];    
	u_char addr3[6];
	u_short seq_ctl;
	u_char addr4[6];
}WLAN_80211_HEAD;

#pragma pack(1)

struct rtl_wifi_header{
	unsigned short 	pkt_len;
	unsigned short 	payload_len;
	unsigned char 	data[252];
}__attribute__((packed));

#pragma pack()

#define DEFAULT_8197_IP "192.168.133.1"

static pcap_t *device = NULL; 
static pthread_t sniffer_thId; 
static volatile int s_enable_sniffer = 0;
static volatile int sniffer_thr_run = 0;
static volatile SNIFFER_CB s_pSnifferCall = NULL;
static pthread_mutex_t s_channel_mutex  = PTHREAD_MUTEX_INITIALIZER;
static int moni = 0;
static int ap_mode = 0;
static int s_current_channel = 1;
static char s_default_ip[32]={0};

static int exec_cmd (const char *format, ...)
{
	va_list vaList;
	char cmd_buf[MAX_CMD_BUF]={0};

	va_start (vaList, format);
	vsnprintf ((char *)cmd_buf, sizeof(cmd_buf), format, vaList);
	va_end (vaList);

	TN_LOGI("run %s\n",cmd_buf);
	return system ((char *)cmd_buf);
}

static int ty_net_8197_set_mode(int apmode)
{
	ap_mode = apmode;
}

static int ty_net_8197_get_mode(int*apmode)
{
	*apmode = ap_mode;
}


int ty_net_8197_send_dhcp(void)
{
	char *wlan		= WLAN_MONITOR;	

	exec_cmd("ps | grep 'udhcpc -i %s' | grep -v grep | awk '{print $1}' |xargs kill -9", wlan);
	exec_cmd("ps | grep 'udhcpc -b -i %s' | grep -v grep | awk '{print $1}' |xargs kill -9", wlan);
	exec_cmd("busybox udhcpc -b -i %s -s %s", wlan, UDHCPC_SCRIPT_PATH);
}

int ty_net_hal_start_station_connect(const char *ssid, const char *passwd)
{
	int s_wait 		= 15;
	char cmd[100]	= {0x00};
	char *wlan		= WLAN_MONITOR;	
	WF_HAL_STATION_STAT_E stat;

 	if(!ssid) {
		return -1;
	}
	
	moni = 0;
	ty_net_8197_set_mode(0);
	
	exec_cmd("killall udhcpd");
	if (WLAN_MONITOR == wlan) {
		snprintf(cmd, sizeof(cmd), IFCONFIG_CMD" %s", WLAN_DEV, UP_CMD);		//ifconfig wlan0 up
		exec_cmd("%s", cmd);	
	}
		
	snprintf(cmd, sizeof(cmd), IFCONFIG_CMD" %s", wlan, DOWN_CMD);		//ifconfig wlan0 down
	exec_cmd("%s", cmd);	
	snprintf(cmd, sizeof(cmd), IWCONFIG_CMD" mode %s", wlan, MANAGED_MODE);		//iwconfig wlan0 mode managed
	exec_cmd("%s", cmd);
	snprintf(cmd, sizeof(cmd), SET_BASE_CMD, wlan, OPMODE_CMD);		//iwpriv wlan0 set_mib opmode=8
	exec_cmd("%s=%d", cmd, 8);
	snprintf(cmd, sizeof(cmd), SET_BASE_CMD, wlan, BAND_CMD);		//iwpriv wlan0 set_mib band=11
	exec_cmd("%s=%d", cmd, 11);
	snprintf(cmd, sizeof(cmd), SET_BASE_CMD, wlan, DENY_LEGACY_CMD);//iwpriv wlan0 set_mib deny_legacy=0
	exec_cmd("%s=%d", cmd, 0);
	snprintf(cmd, sizeof(cmd), SET_BASE_CMD, wlan, USE40M_CMD);		//iwpriv wlan0 set_mib use40M=1
	exec_cmd("%s=%d", cmd, 1);
	snprintf(cmd, sizeof(cmd), SET_BASE_CMD, wlan, SSID_CMD);		//iwpriv wlan0 set_mib ssid=\"%s\"
	exec_cmd("%s=%s", cmd, ssid);
	snprintf(cmd, sizeof(cmd), SET_BASE_CMD, wlan, S_802_1X_CMD);		//iwpriv wlan0 set_mib 802_1x=0
	exec_cmd("%s=%d", cmd, 0);
	exec_cmd("sleep 1");
   
	if(NULL == passwd || strlen(passwd) == 0) {
		TN_LOGD("*******CONNECT AP NO PASSWORD********");
		snprintf(cmd, sizeof(cmd), SET_BASE_CMD, wlan, AUTHTYPE_CMD);	//iwpriv wlan0 set_mib authtype=0
		exec_cmd("%s=%d", cmd, 0);
		snprintf(cmd, sizeof(cmd), SET_BASE_CMD, wlan, ENCMODE_CMD);	//iwpriv wlan0 set_mib encmode=0
		exec_cmd("%s=%d", cmd, 0);
        snprintf(cmd, sizeof(cmd), SET_BASE_CMD, wlan, PSK_ENABLE_CMD);	//iwpriv wlan0 set_mib psk_enable=0
		exec_cmd("%s=%d", cmd, 0);
	}else {        
		TN_LOGD("*******CONNECT AP HAS PASSWORD********");
		snprintf(cmd, sizeof(cmd), SET_BASE_CMD, wlan, AUTHTYPE_CMD);	//iwpriv wlan0 set_mib authtype=2
		exec_cmd("%s=%d", cmd, 2);
		snprintf(cmd, sizeof(cmd), SET_BASE_CMD, wlan, PSK_ENABLE_CMD);	//iwpriv wlan0 set_mib psk_enable=3
		exec_cmd("%s=%d", cmd, 3);
		snprintf(cmd, sizeof(cmd), SET_BASE_CMD, wlan, ENCMODE_CMD);	//iwpriv wlan0 set_mib encmode=2
		exec_cmd("%s=%d", cmd, 2);
		snprintf(cmd, sizeof(cmd), SET_BASE_CMD, wlan, WPA2_CIPHER_CMD);//iwpriv wlan0 set_mib wpa2_cipher=10
		exec_cmd("%s=%d", cmd, 10);
		snprintf(cmd, sizeof(cmd), SET_BASE_CMD, wlan, WPA_CIPHER_CMD);	//iwpriv wlan0 set_mib wpa_cipher=10
		exec_cmd("%s=%d", cmd, 10);
		snprintf(cmd, sizeof(cmd), SET_BASE_CMD, wlan, PASSPHRASE_CMD);	//iwpriv wlan0 set_mib passphrase=\"%s\"
		exec_cmd("%s=%s", cmd, passwd);
	}

	exec_cmd("sleep 1");
	snprintf(cmd, sizeof(cmd), IFCONFIG_CMD" %s", wlan, UP_CMD);		//ifconfig wlan0 up
	exec_cmd("%s", cmd);
	exec_cmd("sleep 1");

	ty_net_8197_send_dhcp();

	TN_LOGI("\n=======wait %d s for connect=======\n",s_wait);

	ty_net_hal_get_status(&stat);

	while(stat != WF_WSS_GOT_IP && --s_wait > 0) {
		ty_net_hal_get_status(&stat);
		sleep(1);
	}

	return 0;
}

#if 0
int ty_net_hal_scan_all_ap(WF_HAL_AP_IF_S **ap_ary, uint32_t *num)
{
	int chan		= 0;
	int x1			= 0;
	int x2			= 0;
	int x3			= 0;
	int x4			= 0;
	int x5			= 0;
	int x6			= 0;
	int8_t value 	= 0;
	int recordIdx 	= -1;	
	char *wlan		= WLAN_DEV;
	FILE *pp		= NULL;	
	char *opptr		= NULL;
	char cmd[100]	= {0x00};
	char strtmp[MAX_R_BUF] 	= {0x00};
	static WF_HAL_AP_IF_S s_aps[MAX_AP_SEARCH];

	memset(s_aps, 0x00, sizeof(s_aps));
	*ap_ary = s_aps;
	*num = 0;

	exec_cmd("iwpriv wlan0 at_ss", NULL, 0);
	exec_cmd("sleep 3",NULL,0);

	snprintf(cmd, sizeof(cmd), WLAN_PARM, wlan, SS_RESULT);

	pp = popen(cmd, "r");
	if(!pp)
		return -1;

	while (fgets(strtmp, sizeof(strtmp), pp) != NULL) {
		opptr = strstr(strtmp, "HwAddr");
		if (opptr) {
			sscanf(opptr + strlen("HwAddr: "), "%02x%02x%02x%02x%02x%02x", &x1, &x2, &x3, &x4, &x5, &x6);
			s_aps[recordIdx].bssid[0] = x1 & 0xFF;
			s_aps[recordIdx].bssid[1] = x2 & 0xFF;
			s_aps[recordIdx].bssid[2] = x3 & 0xFF;
			s_aps[recordIdx].bssid[3] = x4 & 0xFF;
			s_aps[recordIdx].bssid[4] = x5 & 0xFF;
			s_aps[recordIdx].bssid[5] = x6 & 0xFF;
			memset(strtmp, 0x00, MAX_R_BUF);
			recordIdx++;
			continue;
		}

		opptr = strstr(strtmp, "Channel");
		if (opptr) {
			sscanf(opptr + strlen("Channel: "), "%d", &chan);
			s_aps[recordIdx].channel = chan;
			memset(strtmp, 0x00, MAX_R_BUF);
			continue;
		}
			
		opptr = strstr(strtmp, "SSID");
		if(opptr) {
			sscanf(opptr + strlen("SSID: "), "%s", s_aps[recordIdx].ssid);
			s_aps[recordIdx].s_len = strlen(s_aps[recordIdx].ssid);
			memset(strtmp, 0x00, MAX_R_BUF);
			continue;
		}

		opptr = strstr(strtmp, "RSSI");
		if(opptr) {
			sscanf(opptr + strlen("RSSI: "), "%d", &s_aps[recordIdx].rssi);
			memset(strtmp, 0x00, MAX_R_BUF);
			continue;
		}		
	}

	pclose(pp);
	
	if(-1 == recordIdx)
		return -1;
	
	*num = recordIdx + 1;

	return 0;
}
#endif

int ty_net_hal_scan_assigned_ap(const char *ssid, WF_HAL_AP_IF_S **ap)
{
	int i 				= 0;
	uint32_t tatalnum 	= 0;
	WF_HAL_AP_IF_S *ptotalap 	= NULL;

	if (!ssid || !ap)
		return -1;
	
	exec_cmd("ifconfig wlan0 down && sleep 1");
	exec_cmd("ifconfig wlan0 up");

	ty_net_hal_scan_all_ap(&ptotalap, &tatalnum);

	*ap = NULL;

	for(i = 0; i < tatalnum; i++) {
		if(memcmp(ptotalap[i].ssid, ssid, ptotalap[i].s_len) == 0) {
			*ap = ptotalap + i;
			break;
		}
	}
	
	return 0;
}

int ty_net_hal_set_cur_channel(const uint8_t channel)
{
    char cmd[100]   = {0x00};

    snprintf(cmd, sizeof(cmd), SET_BASE_CMD, WLAN_DEV, CHANNEL_CMD);

    exec_cmd("%s=%d", cmd, channel);

    pthread_mutex_lock(&s_channel_mutex);
    s_current_channel = channel;
    pthread_mutex_unlock(&s_channel_mutex);

    return 0;
}

int ty_net_hal_get_cur_channel(uint8_t *channel)
{
	int i				= 0;
	int ret 			= 0;
	int chan 			= 0;
	char cmd[100]		= {0x00};
	FILE *pp			= NULL;
	char strtmp[MAX_R_BUF] 	= {0x00};

	snprintf(cmd, sizeof(cmd), GET_BASE_CMD, WLAN_DEV, CHANNEL_CMD);

	pp = popen(cmd, "r");
	if (!pp)
		return -1;
	while (fgets(strtmp, sizeof(strtmp), pp) != NULL) {
		ret = sscanf(strtmp, "get_mib:%d", &chan);
		if (ret)
			break;
	}
	pclose(pp);

	pthread_mutex_lock(&s_channel_mutex);
	if (!ret) {	//fail
		*channel = s_current_channel;
	}else {
		*channel = chan;
	}
	pthread_mutex_unlock(&s_channel_mutex);

	return 0;
}

static int realuser_parse_80211packet(u_char *buf , u_int len)
{
    WLAN_80211_HEAD *wbuf = (WLAN_80211_HEAD*)buf;
    if((wbuf->frmae_ctl1 & 0x3))
        return -1;
    //type
    switch((wbuf->frmae_ctl1) & 0xc) {
        case 0x8: {
            break;
        }
        default: {
            return -1;
        }
    }

    //dir
    if((wbuf->frmae_ctl2 & 0x3) == 0x2) {
        //ap da bssid sa
        /*
        if(memcmp(wbuf->addr2,junzi_mac,6))
            return -1;
        */
        if(memcmp(wbuf->addr1,"\xff\xff\xff\xff\xff\xff",6) == 0 ) { 
            return 0;
        }else if(memcmp(wbuf->addr1,"\x01\x00\x5e",3) == 0) { 
            return 0;
        }
    }

    return -1;
}

void processPacket(u_char *argu, const struct pcap_pkthdr* pkthdr, const u_char* packet)
{  
	struct rtl_wifi_header *rh 	= (struct rtl_wifi_header *)packet;
	unsigned short pk_len 		= rh->pkt_len;

    if(pk_len > 0) {
        //mimo dont handle
        if(rh->data[0] != 0xff) {
            if(s_pSnifferCall && realuser_parse_80211packet(rh->data, pk_len) == 0) {
	            s_pSnifferCall(rh->data, pk_len);  
	        }
        

        }
    }
}

static void *func_Sniffer(void *arg)
{
	pthread_detach (pthread_self());

	sniffer_thr_run = 1;
    TN_LOGD("#################enter pcap loop#######################"); 

    if(device)
	    pcap_loop(device, -1, processPacket, NULL);

    TN_LOGD("#################exit pcap loop#######################"); 
	sniffer_thr_run = 0;	    
 
	return (void *)0;
}


int ty_net_hal_set_sniffer(const int en, const SNIFFER_CB cb)
{
	char *wlan 	= WLAN_DEV;
	char cmd[100]	= {0x00};

	TN_LOGD("en:%d",en);

	if(en) {
		TN_LOGD("Enable Sniffer");

		if(s_enable_sniffer)
			return 0;

		snprintf(cmd, sizeof(cmd), IFCONFIG_CMD" %s", wlan, "0.0.0.0");  	//ifconfig wlan0 0.0.0.0
		exec_cmd("%s", cmd);
		snprintf(cmd, sizeof(cmd), SET_BASE_CMD, wlan, OPMODE_CMD);          	//iwpriv wlan0 set_mib opmode=16
		exec_cmd("%s=%d", cmd, 16);
		snprintf(cmd, sizeof(cmd), IFCONFIG_CMD" %s", wlan, UP_CMD);         	//ifconfig wlan0 up
		exec_cmd("%s", cmd);

		ty_net_hal_set_sniffer_mode(0);
		s_pSnifferCall = cb;
		s_enable_sniffer = 1;
		pthread_create(&sniffer_thId, NULL, func_Sniffer, NULL);
	}else{
		TN_LOGD("Disable Sniffer11");
		snprintf(cmd, sizeof(cmd), IFCONFIG_CMD" %s", wlan, DOWN_CMD);          //ifconfig wlan0 down
		exec_cmd("%s", cmd);
		while (1) {
			if(s_enable_sniffer && (!sniffer_thr_run)) {
				pcap_close(device);
				s_enable_sniffer = 0;
				TN_LOGD("sniffer thread exit");
				return 0;
			}
			usleep(50*1000);
		}	
	}

	return 0;
}

int ty_net_hal_get_ip(int index, char *ip)
{
	int ret		= 0;
	char *wlan	= WLAN_MONITOR;
	FILE *pp			= NULL;	
	char *pipstart		= 0;
	char cmd[100]		= {0x00};
	char strtmp[MAX_R_BUF] 	= {0x00};

	if(index)	
		wlan = WLAN_MONITOR;	

	snprintf(cmd, sizeof(cmd), IFCONFIG_CMD, wlan);

	pp = popen(cmd, "r");
	if (!pp)
		return -1;
	
	while (fgets(strtmp, sizeof(strtmp), pp) != NULL) {
		pipstart = strstr(strtmp, WF_IP_ETHER_STR);
		if (pipstart)
			ret = sscanf(pipstart + strlen(WF_IP_ETHER_STR), "%[^ ]", ip);

		if (ret)
			break;
	}
	pclose(pp);

	if (!ret)
		return -1;
	else
		return 0;
}

int ty_net_hal_set_ip(int index, const char *ip)
{
	char *wlan			= WLAN_DEV;
	char cmd[100]		= {0x00};

	if(index)	
		wlan = WLAN_MONITOR;	

	snprintf(cmd, sizeof(cmd), IFCONFIG_CMD" %s", wlan, ip);

	exec_cmd("%s", cmd);

	return 0;
}

int ty_net_hal_get_mac(int index, uint8_t *mac)
{
	int ret		= 0;
	char *wlan	= WLAN_DEV;
	FILE *pp			= NULL;	
	char *pmacstart		= NULL;
	char cmd[100]		= {0x00};
	char strtmp[MAX_R_BUF] 	= {0x00};

	if (index)
		wlan = WLAN_MONITOR;

	snprintf(cmd, sizeof(cmd), IFCONFIG_CMD, wlan);
	
	pp = popen(cmd, "r");
	if(!pp)
		return -1;

	while (fgets(strtmp, sizeof(strtmp), pp) != NULL) {
		pmacstart = strstr(strtmp, WF_MAC_ETHER_STR);

		if(pmacstart) {
			int x1,x2,x3,x4,x5,x6;
			sscanf(pmacstart + strlen(WF_MAC_ETHER_STR), "%02x%02x%02x%02x%02x%02x",&x1,&x2,&x3,&x4,&x5,&x6);
			mac[0] = x1 & 0xFF;
			mac[1] = x2 & 0xFF;
			mac[2] = x3 & 0xFF;
			mac[3] = x4 & 0xFF;
			mac[4] = x5 & 0xFF;
			mac[5] = x6 & 0xFF;
			break;
		}
	}

	pclose(pp);

	TN_LOGD("WIFI Get MAC %02x-%02x-%02x-%02x-%02x-%02x",mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

	return 0;
}

int ty_net_hal_set_mac(int index, uint8_t *mac)
{
	char *wlan			= WLAN_DEV;
	char cmd[100]		= {0x00};
	
	if (!mac)
		return -1;

	if (index)
		wlan = WLAN_MONITOR;

	snprintf(cmd, sizeof(cmd), IFCONFIG_CMD" hw ether %d:%d:%d:%d:%d:%d", wlan, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

	exec_cmd("%s", cmd);
	
	return 0;
}

int ty_net_hal_wifi_station_get_ap_mac(int index, uint8_t *mac)
{
	char *wlan	= WLAN_DEV;
	FILE *pp			= NULL;	
	char *opptr 		= NULL;
	char cmd[100]		= {0x00};
	char strtmp[MAX_R_BUF] 	= {0x00};

	if (index)
		wlan = WLAN_MONITOR;

	snprintf(cmd, sizeof(cmd), WLAN_PARM, wlan, STA_INFO);
	
	pp = popen(cmd, "r");
	if(!pp)
		return -1;

	while (fgets(strtmp, sizeof(strtmp), pp) != NULL) {
		opptr = strstr(strtmp, "hwaddr");
		if(NULL == opptr) {
			TN_LOGE("get wifi connect hwaddr failed\n");
			continue;
		}else {
			sscanf(opptr + strlen("hwaddr: "),"%x%x%x%x%x%x", &mac[0],&mac[1],&mac[2],&mac[3],&mac[4],&mac[5]);

	    	TN_LOGD("AP MAC %02X-%02X-%02X-%02X-%02X-%02X",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
			break;
		}
	}

	pclose(pp);

	return 0;
}

int ty_net_hal_set_sniffer_mode(int index)
{
	char *wlan	= WLAN_DEV;
	int dl		= 0;
	int status	= 0;
	char errbuf[PCAP_ERRBUF_SIZE] = {0x00};

	if (index)
		wlan = WLAN_MONITOR;
	
	device = pcap_create(wlan, errbuf); 
	if (!device) {
		TN_LOGE("pcap_create failed\n");
		return -1; 
	}

	if(pcap_set_rfmon(device, 1) == 0)
		TN_LOGI("monitor mode enabled\n");

	pcap_set_snaplen(device, 2048);  	
	pcap_set_promisc(device, 1);      	 
    
	status = pcap_activate(device);

	if(status < 0)
		TN_LOGD("WLAN status = %d\n", status);

	dl = pcap_datalink(device);

	TN_LOGD("The Data Link Type = %s\n", pcap_datalink_val_to_name(dl));

	moni = 1;
	
	return 0;
}

int ty_net_hal_set_station_mode(int index)
{
	char *wlan			= WLAN_DEV;
	char cmd[100]		= {0x00};

	if (index)
		wlan = WLAN_MONITOR;

	snprintf(cmd, sizeof(cmd), SET_BASE_CMD, wlan, OPMODE_CMD);

	exec_cmd("%s=%d", cmd, OP_MANAGED);

	moni = 0;
//    exec_cmd("ifconfig wlan0 up");
//	sleep(1);
//	printf("========================sjm ifconfig wlan0 up ================\n");
	return 0;
}

int ty_net_hal_set_softap_mode(int index)
{
    char *wlan          = WLAN_DEV;
    char cmd[100]       = {0x00};

    if (index)
        wlan = WLAN_MONITOR;

    snprintf(cmd, sizeof(cmd), SET_BASE_CMD, wlan, OPMODE_CMD);

    exec_cmd("%s=%d", cmd, OP_MASTER);

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

int ty_net_hal_get_work_mode(int index, WIFI_MODE_E *mode)
{
	int i		= 0;
	int ret 	= 0;
	char *wlan	= WLAN_DEV;
	FILE *pp			= NULL; 
	int opmode 			= 0;
	char cmd[100]		= {0x00};
	char strtmp[MAX_R_BUF]	= {0x00};

	if (!mode)
		return -1;
	
	if(index)	
		wlan = WLAN_MONITOR;	
	
	snprintf(cmd, sizeof(cmd), GET_BASE_CMD, wlan, OPMODE_CMD);
	
	pp = popen(cmd, "r");
	if (!pp)
		return -1;
	
	while (fgets(strtmp, sizeof(strtmp), pp) != NULL) {
		ret = sscanf(strtmp, "get_mib:%d", &opmode);
		if (ret)
			break;
	}

	pclose(pp);

	for (i = 0; i < WIFI_MODE_NR; i++) {
		if (opmode == g_opmode[i].opmodevalue) {
			*mode = opmode;
			break;
		}
	}

	if (i >= WIFI_MODE_NR)
		return -1;

	TN_LOGD("WIFI Get Mode:%d", *mode);

	return 0;
}

int ty_net_hal_station_disconnect(void)
{
    exec_cmd("killall -9 $(ps | grep busybox | grep udhcpc | awk '{print$1}')");
    TN_LOGD("Disconnect WIFI Conn");

    return 0;
}

int ty_net_wifi_scan(char *ifname , WF_HAL_AP_IF_S *aps , int max_ap)
{
    int ret;
    int skfd;   
    struct iw_range   range;
    int has_range;
    wireless_scan_head ctx;
    char      buffer[128];
    int count = 0;
    wireless_scan *node;
    struct wireless_config *pconf;

    if((skfd = iw_sockets_open()) < 0) {
        perror("socket fail\n");
        return 0;
    }

    /* Get range stuff */
    has_range = (iw_get_range_info(skfd, ifname, &range) >= 0);
    /* Check if the interface could support scanning. */
    if((!has_range) || (range.we_version_compiled < 14)) {
        fprintf(stderr, "%-8.16s  Interface doesn't support scanning.\n\n",ifname);
        iw_sockets_close(skfd);
        return 0;
    }

    memset(&ctx,0 ,sizeof(wireless_scan_head));
    
    ret = iw_scan(skfd,ifname,range.we_version_compiled , &ctx);

    if(ret < 0){
        fprintf(stderr,"iw scan error:%d\n",ret);
        iw_sockets_close(skfd);
        return 0;
    }

    node = ctx.result;

    while(node != NULL) {
        int length;
        WF_HAL_AP_IF_S *tmp; 
        double freq; 
        int x0,x1,x2,x3,x4,x5;

        if(node->has_ap_addr == 0)
            continue;

        if(count >= max_ap)
            break;

        tmp = &aps[count++];

        pconf = &(node->b);
        //printf("essid len is %d ,ESSID : %s\n",pconf->essid_len,pconf->essid);
        length = strlen(pconf->essid);
        tmp->s_len = length > WF_HAL_WIFI_SSID_LEN ? WF_HAL_WIFI_SSID_LEN:length;
        memcpy(tmp->ssid , pconf->essid, tmp->s_len);
        //printf("len is %d ,SSID : %s\n",tmp->s_len,tmp->ssid);
        freq = pconf->freq;    /* Frequency/channel */
        tmp->channel = iw_freq_to_channel( freq, &range);

        memset(buffer,0,sizeof(buffer));
        iw_sawap_ntop(&node->ap_addr, buffer);
        sscanf(buffer,"%02x:%02x:%02x:%02x:%02x:%02x\n",&x0,&x1,&x2,&x3,&x4,&x5);
        tmp->bssid[0] = 0xff & x0;
        tmp->bssid[1] = 0xff & x1;
        tmp->bssid[2] = 0xff & x2;
        tmp->bssid[3] = 0xff & x3;
        tmp->bssid[4] = 0xff & x4;
        tmp->bssid[5] = 0xff & x5;
        if(node->has_stats)
            tmp->rssi = node->stats.qual.qual ;

        node = node->next;
    }

    node = ctx.result;
    while(node != NULL) {
        wireless_scan *tmp = node;
        node = node->next;
        free(tmp);
    }

    iw_sockets_close(skfd);

    return count;
}

int ty_net_hal_scan_all_ap(WF_HAL_AP_IF_S **ap_ary, uint32_t *num)
{
    int index = 0;
    char *wlan = WLAN_DEV;
    static WF_HAL_AP_IF_S s_aps[MAX_AP_SEARCH];

    TN_LOGD();

    *num = 0;
    *ap_ary = s_aps;
    memset(s_aps, 0, sizeof(s_aps));

    *num = ty_net_wifi_scan(wlan,*ap_ary,MAX_AP_SEARCH);

    //print all scanned ap
    for(index = 0; index < *num; index++) {
        TN_LOGD("index:%d channel:%d bssid:%02X-%02X-%02X-%02X-%02X-%02X RSSI:%d SSID:%s\n",
                index,s_aps[index].channel,s_aps[index].bssid[0],s_aps[index].bssid[1],
                s_aps[index].bssid[2],s_aps[index].bssid[3],s_aps[index].bssid[4],
                s_aps[index].bssid[5], s_aps[index].rssi, s_aps[index].ssid);
    }
    TN_LOGD("WIFI Scan AP End");

    return 0;
}

int ty_net_hal_get_assign_ap_rssi(char *ssid, int *rssi)
{
    int iret = 0;	
    int index = 0;
    WF_HAL_AP_IF_S *p_total_ap = NULL;
    uint32_t total_num = 0;

    ty_net_hal_scan_all_ap(&p_total_ap, &total_num);

    for(index = 0; index < total_num; index++) {
        if(strcmp(p_total_ap[index].ssid, ssid) == 0) {
            *rssi = p_total_ap[index].rssi - 100;
            break;
        }
    }

    if (index >= total_num)
        iret = -1;

    return iret;
}
#if 0
int ty_net_hal_get_conn_ap_rssi(int8_t *rssi)
{
    int8_t value    = 0;
    char *wlan      = WLAN_DEV;
    FILE *pp        = NULL;	
    char *opptr     = NULL;
    char cmd[100]   = {0x00};
    char strtmp[MAX_R_BUF]  = {0x00};

    exec_cmd("iwpriv %s at_ss", wlan);
    exec_cmd("sleep 3");

    snprintf(cmd, sizeof(cmd), WLAN_PARM, wlan, SS_RESULT);

    pp = popen(cmd, "r");
    if(!pp)
        return -1;

    while (fgets(strtmp, sizeof(strtmp), pp) != NULL) {
        opptr = strstr(strtmp, "RSSI:");
        if(NULL == opptr) {
            continue;
        } else {
            sscanf(opptr + strlen("RSSI: "),"%d", &value);
            break;
        }
    }

    pclose(pp);

    if(rssi)
        *rssi = value - 100;

    TN_LOGD("Get Conn AP RSSI:%d", *rssi);

    return 0;
}
#else
int ty_net_hal_get_conn_ap_rssi(int8_t *rssi)
{
    int skfd;
    int has_range;
    struct iwreq wrq;
    struct iw_range range;
    struct iw_statistics stats;
    char *ifname = WLAN_DEV;

    if((skfd = iw_sockets_open()) < 0){
        perror("socket");
        exit(-1);
    }
    memset(&stats,0,sizeof(struct iw_statistics));
    memset(&wrq,0,sizeof(struct iwreq));

    has_range = iw_get_range_info(skfd, ifname, &range);     //get the WIRELESE_EXT-version
    /* Check if the interface could support get rssi. */
    if(has_range || (range.we_version_compiled < 14)) {
        printf("%-8.16s  Interface doesn't get connected ap's rssi.\n\n",ifname);
        iw_sockets_close(skfd);
        return -1;
    }

    wrq.u.data.pointer = (caddr_t) (&stats);
    wrq.u.data.length = sizeof(struct iw_statistics);
    wrq.u.data.flags = 1;       /* Clear updated flag */
    strncpy(wrq.ifr_name, ifname, IFNAMSIZ);
    if (iw_get_ext(skfd, ifname, SIOCGIWSTATS, &wrq) < 0) {
        printf("get connect ap's rssi failed : %s",strerror(errno));
        TN_LOGD();
        return (-1);
    }

    /* Deal with signal level in RCPI */
    /* RCPI = int{(Power in dBm +110)*2} for 0dbm > Power > -110dBm */
    TN_LOGD("[rufan]:connect ap's rssi %d",stats.qual.level);
    if (stats.qual.updated & IW_QUAL_RCPI) {
        if(!(stats.qual.updated & IW_QUAL_LEVEL_INVALID)){
            int rcpilevel = (stats.qual.level / 2) - 110;
            *rssi = rcpilevel & 0xFF;
            iw_sockets_close(skfd);
        } else {
            /* Check if the statistics are in dBm */
            if((stats.qual.updated & IW_QUAL_DBM) || (stats.qual.level > range.max_qual.level)){
                /* Deal with signal level in dBm  (absolute power measurement) */
                if(!(stats.qual.updated & IW_QUAL_LEVEL_INVALID)){
                    int dblevel = stats.qual.level;
                    /* Implement a range for dBm [-192; 63] */
                    if(stats.qual.level >= 64)
                        dblevel -= 0x100;
                        *rssi = dblevel;
                }
            } else {
                /* Deal with signal level as relative value (0 -> max) */
                if(!(stats.qual.updated & IW_QUAL_LEVEL_INVALID)){
                    *rssi = stats.qual.level;
                }
            }
        }
    } else {
            *rssi = stats.qual.level;
        }

    return 0;
}
#endif

int ty_net_hal_get_status(WF_HAL_STATION_STAT_E *stat)
{
    char ip[32] = {0};

    if(ty_net_hal_get_ip(0, ip) || strlen(ip) == 0) {
        *stat = WF_WSS_CONN_FAIL;
    } else if(strcmp(ip,s_default_ip) == 0 || strcmp(ip,DEFAULT_8197_IP)==0) {
        *stat = WF_WSS_CONN_FAIL;
    } else {
        *stat = WF_WSS_GOT_IP;
    }

    TN_LOGD("Curr WIFI Stat:%d", *stat);

    return 0;
}


int ty_net_hal_stop_ap(void)
{
	char cmd[100]	= {0x00};
	char *wlan		= WLAN_DEV;
	
	exec_cmd("killall udhcpd");
	snprintf(cmd, sizeof(cmd), IFCONFIG_CMD" %s", wlan, DOWN_CMD);		//ifconfig wlan0 down
	exec_cmd("%s", cmd);

	return 0;
}

int ty_net_hal_start_ap(int channel,char *ssid,char *passwd,char *ip)
{
	char cmd[100]	= {0x00};
	char *wlan		= WLAN_DEV;

	if(ssid == NULL || ip == NULL) {
		TN_LOGE("invalid Parm\n");
		return -1;	
	}

	TN_LOGD("Start AP SSID:%s", ssid);
	
	moni = 0;

	exec_cmd("killall udhcpd");
	snprintf(cmd, sizeof(cmd), IFCONFIG_CMD" %s", wlan, DEFAULT_8197_IP);		//ifconfig wlan0 ip
	exec_cmd("%s", cmd);
	snprintf(cmd, sizeof(cmd), IFCONFIG_CMD" %s", wlan, DOWN_CMD);				//ifconfig wlan0 down
	exec_cmd("%s", cmd);
	snprintf(cmd, sizeof(cmd), IWCONFIG_CMD" mode %s", wlan, MASTER_MODE);		//iwconfig wlan0 mode master
	exec_cmd("%s", cmd);
	snprintf(cmd, sizeof(cmd), SET_BASE_CMD, wlan, OPMODE_CMD);		//iwpriv wlan0 set_mib opmode=16
	exec_cmd("%s=%d", cmd, 16);
	snprintf(cmd, sizeof(cmd), SET_BASE_CMD, wlan, BAND_CMD);		//iwpriv wlan0 set_mib band=11
	exec_cmd("%s=%d", cmd, 11);
	snprintf(cmd, sizeof(cmd), SET_BASE_CMD, wlan, DENY_LEGACY_CMD);//iwpriv wlan0 set_mib deny_legacy=0
	exec_cmd("%s=%d", cmd, 0);
	snprintf(cmd, sizeof(cmd), SET_BASE_CMD, wlan, S_802_1X_CMD);	//iwpriv wlan0 set_mib 802_1x=0
	exec_cmd("%s=%d", cmd, 0);
	snprintf(cmd, sizeof(cmd), SET_BASE_CMD, wlan, SSID_CMD);		//iwpriv wlan0 set_mib ssid=\"%s\"
	exec_cmd("%s=%s", cmd, ssid);

 	if (passwd != NULL && 0 != strlen(passwd)) {
		snprintf(cmd, sizeof(cmd), SET_BASE_CMD, wlan, AUTHTYPE_CMD);	//iwpriv wlan0 set_mib authtype=2
		exec_cmd("%s=%d", cmd, 2);
		snprintf(cmd, sizeof(cmd), SET_BASE_CMD, wlan, PSK_ENABLE_CMD);	//iwpriv wlan0 set_mib psk_enable=3
		exec_cmd("%s=%d", cmd, 3);		
		snprintf(cmd, sizeof(cmd), SET_BASE_CMD, wlan, ENCMODE_CMD);	//iwpriv wlan0 set_mib encmode=2
		exec_cmd("%s=%d", cmd, 2);
		snprintf(cmd, sizeof(cmd), SET_BASE_CMD, wlan, WPA2_CIPHER_CMD);//iwpriv wlan0 set_mib wpa2_cipher=10
		exec_cmd("%s=%d", cmd, 10);
		snprintf(cmd, sizeof(cmd), SET_BASE_CMD, wlan, WPA_CIPHER_CMD);	//iwpriv wlan0 set_mib wpa_cipher=10
		exec_cmd("%s=%d", cmd, 10);
		snprintf(cmd, sizeof(cmd), SET_BASE_CMD, wlan, PASSPHRASE_CMD);	//iwpriv wlan0 set_mib passphrase=\"%s\"
		exec_cmd("%s=%s", cmd, passwd);			
	}else {
		snprintf(cmd, sizeof(cmd), SET_BASE_CMD, wlan, AUTHTYPE_CMD);	//iwpriv wlan0 set_mib authtype=0
		exec_cmd("%s=%d", cmd, 0);
		snprintf(cmd, sizeof(cmd), SET_BASE_CMD, wlan, ENCMODE_CMD);	//iwpriv wlan0 set_mib encmode=0
		exec_cmd("%s=%d", cmd, 0);		
	}

	snprintf(cmd, sizeof(cmd), IFCONFIG_CMD" %s", wlan, UP_CMD);		//ifconfig wlan0 up
	exec_cmd("%s", cmd); 
	exec_cmd("udhcpd %s", UDHCPD_CONF_PATH);    

	ty_net_8197_set_mode(1);
	TN_LOGD("************** Start AP SSID:%s *************************",ssid);

	return 0;
}

int ty_net_wifi_scan_for_product_test(char *ifname , WF_HAL_AP_IF_S *aps , int max_ap)
{
	int ret;
	int skfd;   
	struct iw_range   range;
	int has_range;
	wireless_scan_head ctx;
	char      buffer[128];
	int count = 0;
	wireless_scan *node;
	struct wireless_config *pconf;

	if((skfd = iw_sockets_open()) < 0) {
		perror("socket fail\n");
		return 0;
	}

	/* Get range stuff */
	has_range = (iw_get_range_info(skfd, ifname, &range) >= 0);
	/* Check if the interface could support scanning. */
	if((!has_range) || (range.we_version_compiled < 14)) {
		fprintf(stderr, "%-8.16s  Interface doesn't support scanning.\n\n",ifname);
		iw_sockets_close(skfd);
		return 0;
	}

	memset(&ctx,0 ,sizeof(wireless_scan_head));
    
	ret = iw_scan_for_product_test(skfd,ifname,range.we_version_compiled , &ctx);

	if(ret < 0){
		fprintf(stderr,"iw scan error:%d\n",ret);
		iw_sockets_close(skfd);
		return 0;
	}

	node = ctx.result;

	while(node != NULL) {
		int length;
		WF_HAL_AP_IF_S *tmp; 
		double freq; 
		int x0,x1,x2,x3,x4,x5;
		
		if(node->has_ap_addr == 0)
			continue;

		if(count >= max_ap)
			break;
        
		tmp = &aps[count++];

		pconf = &(node->b);
		//printf("essid len is %d ,ESSID : %s\n",pconf->essid_len,pconf->essid);
		length = strlen(pconf->essid);
		tmp->s_len = length > WF_HAL_WIFI_SSID_LEN ? WF_HAL_WIFI_SSID_LEN:length;
		memcpy(tmp->ssid , pconf->essid, tmp->s_len);
		//printf("len is %d ,SSID : %s\n",tmp->s_len,tmp->ssid);
		freq = pconf->freq;    /* Frequency/channel */
		tmp->channel = iw_freq_to_channel( freq, &range);

		memset(buffer,0,sizeof(buffer));
		iw_sawap_ntop(&node->ap_addr, buffer);
		sscanf(buffer,"%02x:%02x:%02x:%02x:%02x:%02x\n",&x0,&x1,&x2,&x3,&x4,&x5);
		tmp->bssid[0] = 0xff & x0;
		tmp->bssid[1] = 0xff & x1;
		tmp->bssid[2] = 0xff & x2;
		tmp->bssid[3] = 0xff & x3;
		tmp->bssid[4] = 0xff & x4;
		tmp->bssid[5] = 0xff & x5;
		if(node->has_stats)
			tmp->rssi = node->stats.qual.qual ;

		node = node->next;
	}

	node = ctx.result;
	while(node != NULL) {
		wireless_scan *tmp = node;
		node = node->next;
		free(tmp);
    }

	iw_sockets_close(skfd);

	return count;
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
	TN_LOGD("WIFI Scan AP End , %d APs has been scaned",*num);

	return 0;
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

int ty_net_hal_get_country_code(void)
{
	int i				= 0;
	int ret 			= 0;
	int c_code 			= 0;
	char cmd[100]		= {0x00};
	char strtmp[100] 	= {0x00};
	FILE *pp			= NULL;

	snprintf(cmd, sizeof(cmd), GET_BASE_CMD, WLAN_DEV, COUNTRY_CODE_CMD);

	pp = popen(cmd, "r");
	if (!pp)
		return -1;
	while (fgets(strtmp, sizeof(strtmp), pp) != NULL) {
		ret = sscanf(strtmp, "get_mib:%d", &c_code);
		if (ret)
			break;
	}
	pclose(pp);

	for (i = 0; i < COUNTRY_NR; i++) {
		if (c_code == g_c_code[i].c_code) {
			return i;
		}
	}

	if (i >= COUNTRY_NR)
		return -1;
}

int ty_net_hal_set_country_code(const WF_HAL_COUNTRY_CODE_E ccode)
{
	char cmd[100]		= {0x00};

	if (ccode > WF_COUNTRY_CODE_NR)
		return -1;

	snprintf(cmd, sizeof(cmd), SET_BASE_CMD, WLAN_DEV, COUNTRY_CODE_CMD);
	exec_cmd("%s=%d", cmd, g_c_code[ccode]);

	return 0;
}

int ty_net_hal_disable_wifi_lowpower(void)
{
	TN_LOGD();
	return 0;
}

int ty_net_hal_register_recv_mgnt_cb(int enable,WIFI_REV_MGNT_CB_T recv_cb)
{
	TN_LOGD();
	return 0;
}

int ty_net_hal_close_concurrent_ap(void)
{
	TN_LOGD();
	return 0;
}

int ty_net_hal_send_mgnt(const uint8_t *buf, const uint32_t len)
{
	TN_LOGD();
	return 0;
}

int ty_net_hal_get_bssid(UINT8_T *bssid)
{
	TN_LOGD();

	return 0;
}
