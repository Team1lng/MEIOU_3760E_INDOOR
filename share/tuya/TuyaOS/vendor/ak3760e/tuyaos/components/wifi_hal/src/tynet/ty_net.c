#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <netinet/in.h>
#include <net/if.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include "iwlib.h"
#include "ty_net.h"
#include "ty_net_log.h"
#include "wpa_command.h"
#include "common.h"

#define WPA_CLI "wpa_cli %s"
#define SIGNAL_POLL "signal_poll"

int ty_net_enable(char *ifname , char enable)
{
    int skfd = 0;
    struct ifreq ifr;

    skfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(skfd < 0) {
        fprintf(stderr,"create socket err : %s\n",strerror(errno));
        return -1;
    }

    strcpy(ifr.ifr_name, ifname);

    if(ioctl(skfd, SIOCGIFFLAGS, &ifr) <0 ) {
        fprintf(stderr,"socket ioctl err:%s\n",strerror(errno));
        close(skfd);
        return -1;
    }

    if(enable) {
        ifr.ifr_flags |= IFF_UP;
    } else {
        ifr.ifr_flags &= ~IFF_UP;
    }

    if(ioctl(skfd, SIOCSIFFLAGS, &ifr) <0 ) {
        fprintf(stderr,"socket ioctl err : %s\n",strerror(errno));
        close(skfd);
        return -1;
    }

    close(skfd);
    return 0;
}

int ty_net_get_ip(char *ifname, char *ip)
{
    int skfd = 0;
    struct ifreq ifr;
    struct sockaddr_in *sin;

    skfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(skfd < 0) {
        fprintf(stderr,"create socket err : %s\n",strerror(errno));
        return -1;
    }

    strcpy(ifr.ifr_name, ifname);

    if(ioctl(skfd, SIOCGIFFLAGS, &ifr) < 0) {
        fprintf(stderr,"socket ioctl err : %s\n",strerror(errno));
        close(skfd);
        return -1;
    }
    if(!(ifr.ifr_flags & IFF_RUNNING)) {
        close(skfd);
        return -1;
    }

    strcpy(ifr.ifr_name, ifname);
    if(ioctl(skfd, SIOCGIFADDR, &ifr) <0 ) {
        fprintf(stderr,"socket ioctl err : %s\n",strerror(errno));
        close(skfd);
        return -1;
    }

    sin = (struct sockaddr_in *)&(ifr.ifr_addr);
    strcpy(ip, inet_ntoa(sin->sin_addr));

    close(skfd);
    return 0;
}


int ty_net_set_ip(char *ifname, char *ip)
{
    int skfd = 0;
    struct ifreq ifr;
    struct sockaddr_in *addr;

    memset(&ifr,0,sizeof(struct ifreq));
    strcpy(ifr.ifr_name,ifname);
    if((skfd=socket(AF_INET, SOCK_STREAM, 0))<0) {
        fprintf(stderr,"socket fail\n");
        return -1;
    }

    addr = (struct sockaddr_in *)&(ifr.ifr_addr);
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = inet_addr(ip);
    if(ioctl(skfd,SIOCSIFADDR, &ifr) < 0) {
        fprintf(stderr,"ioctl fail\n");
        return -1;
    }
    ifr.ifr_flags |= IFF_UP |IFF_RUNNING;

    if (ioctl(skfd,SIOCSIFFLAGS, &ifr) < 0) {
        sprintf(stderr,"ioctl fail\n");
        return -1;
    }

    close(skfd);
    return 0;
}

int ty_net_get_mac(char *inf, uint8_t *mac)
{
    int fd = -1;
    struct ifreq ifr;

    bzero(&ifr, sizeof(struct ifreq));

    if(mac == NULL) {
        fprintf(stderr,"invalid parm\n");
        return -1;
    }

    if( (fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr,"get %s mac address socket creat error\n", inf);
        return -1;
    }

    strncpy(ifr.ifr_name, inf, sizeof(ifr.ifr_name) - 1);
    if(ioctl(fd, SIOCGIFHWADDR, &ifr) < 0) {
        fprintf(stderr,"get %s mac address error\n", inf);
        close(fd);
        return -1;
    }
    memcpy(mac,(unsigned char*)ifr.ifr_hwaddr.sa_data,6);

    close(fd);
    return 0;
}

int ty_net_set_mac(char *inf, uint8_t *mac)
{
    int fd = -1;
    struct ifreq ifr;

    if (!inf || !mac) {
        fprintf(stderr,"invalid parm\n");
        return -1;
    }

    bzero(&ifr, sizeof(struct ifreq));

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr,"get %s mac address socket creat error\n", inf);
        return -1;
    }
    ifr.ifr_addr.sa_family = ARPHRD_ETHER;
    strncpy(ifr.ifr_name, inf, sizeof(ifr.ifr_name) - 1);
    memcpy((unsigned char *)ifr.ifr_hwaddr.sa_data, mac, 6);

    if(ioctl(fd, SIOCSIFHWADDR, &ifr) < 0) {
        fprintf(stderr,"get %s mac address error\n", inf);
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

int ty_net_get_ap_addr(char *ifname, uint8_t *mac)
{
    int skfd;
    struct iwreq wrq;
    int ret = 0;
    char buffer[32]= {0};

    if((skfd = iw_sockets_open()) < 0){
        perror("socket fail\n");
        return -1;
    }

    ret = iw_get_ext(skfd,ifname,SIOCGIWAP, &wrq);
    if (ret  < 0) {
        fprintf(stderr,"socket ioctl err : %s\n",strerror(errno));
    } else {
        const struct ether_addr * eth = (const struct ether_addr *)(&wrq.u.ap_addr);
        mac[0]= eth->ether_addr_octet[0];
        mac[1]= eth->ether_addr_octet[1];
        mac[2]= eth->ether_addr_octet[2];
        mac[3]= eth->ether_addr_octet[3];
        mac[4]= eth->ether_addr_octet[4];
        mac[5]= eth->ether_addr_octet[5];
    }
    iw_sockets_close(skfd);

    return ret;
}

int ty_wifi_scan(char *ifname , WF_HAL_AP_IF_S *aps , int max_ap)
{
    int ret;
    int skfd;   
    struct iw_range   range;
    int has_range;
    wireless_scan_head ctx;
    char buffer[128];
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
            //tmp->rssi = node->stats.qual.qual ;
            tmp->rssi = node->stats.qual.level ;

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

int ty_wifi_scan_wpa(char *ifname , WF_HAL_AP_IF_S *aps , int max_ap)
{
    int i;
    int x1,x2,x3,x4,x5,x0;
    int iret = 0,ap_nums = 0;
    WF_HAL_AP_IF_S *tmp;
    struct wifi_scan_list s_list;

    iret = wifi_get_block_scan_result(&s_list,0);
    if(iret < 0) {
        TN_LOGE("WiFi Get scan result fialed\n");
        return -1;
    }

    TN_LOGD();
    ap_nums = s_list.num;
    for(i = 0; i < s_list.num ;i++) {
        tmp = &aps[i];
        tmp->channel = ((s_list.ssid[i].freq - 2412) / 5) + 1;      /* 将频率转换成对应的channel */
        tmp->rssi = s_list.ssid[i].sig;
        tmp->s_len = strlen(s_list.ssid[1].ssid);
        strcpy(tmp->ssid,s_list.ssid[i].ssid);
        //strcpy(tmp->bssid,s_list.ssid[i].mac_addr);
        sscanf(s_list.ssid[i].mac_addr,MACSTR,&x0,&x1,&x2,&x3,&x4,&x5);
            tmp->bssid[0] = 0xff & x0;
            tmp->bssid[1] = 0xff & x1;
            tmp->bssid[2] = 0xff & x2;
            tmp->bssid[3] = 0xff & x3;
            tmp->bssid[4] = 0xff & x4;
            tmp->bssid[5] = 0xff & x5;
    }

    return ap_nums;     /* 返回扫描到的AP的数量 */
}

int ty_net_wifi_scan_for_product_test(char *ifname , WF_HAL_AP_IF_S *aps , int max_ap)
{
    int ret,skfd;
    struct iw_range   range;
    int has_range;
    wireless_scan_head ctx;
    char buffer[128];
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
    if (ret < 0) {
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

int ty_wifi_set_chn(char *ifname, int channel)
{
    int ret = 0;
    int skfd;
    struct iw_range range;
    int has_range;
    struct iwreq wrq;
    double freq;

    // printf("set Channel is %d\n",channel);

    if((skfd = iw_sockets_open()) < 0) {
        perror("socket fail\n");
        return -1;
    }

    has_range = (iw_get_range_info(skfd, ifname, &range) >= 0);
    if((!has_range) || (range.we_version_compiled < 14)) {
        fprintf(stderr, "%-8.16s  Interface doesn't support scanning.\n\n",ifname);
        iw_sockets_close(skfd);
        return(-1);
    }

    iw_channel_to_freq(channel, &freq , &range);
    iw_float2freq(freq, &(wrq.u.freq));

    wrq.u.freq.flags = IW_FREQ_FIXED;
    ret = iw_set_ext(skfd, ifname, SIOCSIWFREQ, &wrq);

    if(ret < 0){
        fprintf(stderr,"socket ioctl err : %s\n",strerror(errno));
    }

    iw_sockets_close(skfd);

    return ret;
}

int ty_wifi_get_chn(char *ifname, int *channel)
{
    int skfd;
    struct iw_range range;
    int has_range;
    struct iwreq wrq;
    int ret;
    double freq;

    if((skfd = iw_sockets_open()) < 0){
        perror("socket fail\n");
        return -1;
    }

    has_range = (iw_get_range_info(skfd, ifname, &range) >= 0);

    if((!has_range) || (range.we_version_compiled < 14)) {
        fprintf(stderr, "%-8.16s  Interface doesn't support scanning.\n\n",ifname);
        iw_sockets_close(skfd);
        return(-1);
    }

    ret = iw_get_ext(skfd, ifname, SIOCGIWFREQ, &wrq);
    if (ret < 0) {
        fprintf(stderr,"socket ioctl err : %s\n",strerror(errno));
        return -1;
    } else {
        freq = iw_freq2float(&wrq.u.freq);
        *channel = iw_freq_to_channel( freq, &range);
    }
    iw_sockets_close(skfd);

    return 0;
}

int ty_wifi_set_mode(char *ifname , char *mode)
{
    int k,skfd = 0;
    struct iwreq wrq;
    int NUM_OPER_MODE = 7;

    for(k = 0; k < NUM_OPER_MODE; k++) {
        if(strncasecmp(mode, iw_operation_mode[k], 3) == 0)
            break;
    }

    if(k >= NUM_OPER_MODE || k == 1 || k == 4 || k == 5) {
        fprintf(stderr,"[%s:%d] set invalid mode : %s\n",__FUNCTION__,__LINE__,mode);
        return -1;
    }

    skfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(skfd < 0) {
        fprintf(stderr,"create socket err : %s\n",strerror(errno));
        return -1;
    }

    strcpy(wrq.ifr_name, ifname);
    wrq.u.mode = k;
    if(ioctl(skfd,SIOCSIWMODE, &wrq) < 0) {
        fprintf(stderr,"socket ioctl err : %s\n",strerror(errno));
        close(skfd);
        return -1;
    }

    close(skfd);
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
int ty_wifi_get_mode(char *ifname , int *mode)
{
    int skfd = 0;
    struct iwreq wrq;

    skfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(skfd < 0) {
        fprintf(stderr,"create socket err : %s\n",strerror(errno));
        return -1;
    }

    strcpy(wrq.ifr_name, ifname);
    if(ioctl(skfd,SIOCGIWMODE, &wrq) < 0) {
        fprintf(stderr,"socket ioctl err : %s\n",strerror(errno));
        close(skfd);
        return -1;
    }
    close(skfd);
    *mode = wrq.u.mode;

    return 0;
}

#if 0
int ty_wifi_get_curr_rssi(char *ifname, int *rssi)
{
	int skfd;
	struct iwreq      wrq;
	struct iw_range   range;
	int has_range;
	iwstats stats;
	int ret;

	if((skfd = iw_sockets_open()) < 0) {
		perror("socket fail\n");
		return -1;
	}

	has_range = (iw_get_range_info(skfd, ifname, &range) >= 0);

	if((!has_range) || (range.we_version_compiled < 14)) {
		fprintf(stderr,"%-8.16s  Interface doesn't support iw_get_range_info.\n\n",ifname);
		iw_sockets_close(skfd);
		return(-1);
	}
    
	wrq.u.data.pointer = (caddr_t)(&stats);
	wrq.u.data.length = sizeof(struct iw_statistics);
	wrq.u.data.flags = 1;     /* Clear updated flag */
	strcpy(wrq.ifr_name, ifname);

	ret = iw_get_ext(skfd, ifname, SIOCGIWSTATS, &wrq);
	if(ret < 0){
		fprintf(stderr,"socket ioctl err : %s\n",strerror(errno));
	}else{
		//printf("stats : rssi %d , max %d\n",stats.qual.qual,range.max_qual.qual);
		*rssi = (stats.qual.qual * 100) / ( range.max_qual.qual + 1);
	}

	iw_sockets_close(skfd);
	return 0;
}
#endif

#if 0
int ty_wifi_get_curr_rssi(char *ifname, int *rssi)
{
	int8_t value 	= 0;
	char *opptr		= NULL;
	char cmd[100]	= {0x00};
	FILE *pp		= NULL;
	char strtmp[1024] 	= {0x00};

	snprintf(cmd, sizeof(cmd), WPA_CLI,SIGNAL_POLL);
	pp = popen(cmd, "r");

	while (fgets(strtmp, sizeof(strtmp), pp) != NULL) {
		opptr = strstr(strtmp, "RSSI=");
		if(NULL == opptr) {
			continue;
		}else {
			sscanf(opptr + strlen("RSSI="),"%d", &value);
			break;
		}
	}

	pclose(pp);

	if(rssi)
	*rssi = value;    

	TN_LOGD("Get Conn AP RSSI:%d", *rssi);

	return 0;	
}?
#endif

int ty_wifi_get_curr_rssi(char *ifname,int *rssi)
{
    int ret;
    int tmp;

    ret = wifi_get_signal(&tmp);
    if (ret) {
        fprintf(stderr,"get rssi fail : %s\n",strerror(errno));
        return -1;
    }
    if (rssi) {
        *rssi = tmp;
    } else {
        return -1;
    }

    return 0;
}

int ty_get_conn_stat(char *ifname)
{
    int skfd;
    int ret;
    struct wireless_config conf; 

    if((skfd = iw_sockets_open()) < 0) {
        perror("socket fail\n");
        return -1;
    }

    ret = iw_get_basic_config(skfd,ifname,&conf);
    if(ret < 0 ) {
        fprintf(stderr,"%-8.16s  Interface doesn't support iw_get_basic_config.\n\n",ifname);
        iw_sockets_close(skfd);
        return(-1);
    }
    //printf("essid on is %d, esssid : %s\n",conf.essid_on,conf.essid);
    iw_sockets_close(skfd);
    ret = conf.essid_on;

    return ret;
}
