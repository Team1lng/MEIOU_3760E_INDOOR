#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <poll.h>
#include <sys/socket.h>
#include <errno.h>
#include <signal.h>
#include<net/if.h>
#include<net/if_arp.h>
#include<arpa/inet.h>
#include<sys/ioctl.h>
#include <pthread.h>

#include "hostapd_command.h"

struct wifi_mode global_mode;

int get_wifi_current_mode() 
{
    return global_mode.mode;
}

void set_wifi_mode(int mode)
{
    pthread_mutex_lock(&global_mode.mode_mutex);
    global_mode.mode = mode;
    pthread_mutex_unlock(&global_mode.mode_mutex);
}

static int softap_prepare_configure(char *ssid, char *psk)
{
    int fd = 0;
    char buf[1024] = {0};
    int ret = 0;

    unlink("/data/system/hostapd.conf");

    fd = open("/data/system/hostapd.conf", O_RDWR | O_CREAT, 0600);
    if (fd < 0) {
        printf("create configure error %d\n", errno);
        return -1;
    }

    if (strlen(psk) > 0) {

        sprintf(buf, "interface=wlan0\ndriver=nl80211\nctrl_interface=/var/run/hostapd\nssid=%s\nchannel=6\nieee80211n=1\nieee80211ac=1\nhw_mode=g\nignore_broadcast_ssid=0\nwpa=3\nwpa_passphrase=%s\nwpa_key_mgmt=WPA-PSK\nwpa_pairwise=TKIP CCMP\nrsn_pairwise=CCMP\n", ssid, psk);
    } else {
        sprintf(buf, "interface=wlan0\ndriver=nl80211\nctrl_interface=/var/run/hostapd\nssid=%s\nchannel=6\nieee80211n=1\nieee80211ac=1\nhw_mode=g\nignore_broadcast_ssid=0\n#wpa=3\n#wpa_passphrase=\n#wpa_key_mgmt=WPA-PSK\n#wpa_pairwise=TKIP CCMP\n#rsn_pairwise=CCMP\n", ssid);

    }

    ret = write(fd, buf, strlen(buf));
    if (ret < 0) {
        printf("write hostapd buf errno %d\n", errno);
        return ret;
    }

    close(fd);
    return 0;
}

static int set_current_ip(const char *ipaddr)
{
    int sock_set_ip;
    struct sockaddr_in sin_set_ip;
    struct ifreq ifr_set_ip;

    memset(&ifr_set_ip,0,sizeof(ifr_set_ip));
    if (ipaddr == NULL) {
        return -1;
    }

    sock_set_ip = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_set_ip < 0) {
        perror("socket create failse...SetLocalIp!");
        return -1;
    }

    memset(&sin_set_ip, 0, sizeof(sin_set_ip));
    strncpy(ifr_set_ip.ifr_name, "wlan0", sizeof(ifr_set_ip.ifr_name) - 1);

    sin_set_ip.sin_family = AF_INET;
    sin_set_ip.sin_addr.s_addr = inet_addr(ipaddr);
    memcpy( &ifr_set_ip.ifr_addr, &sin_set_ip, sizeof(sin_set_ip));
    printf("ipaddr===%s\n",ipaddr);
    if (ioctl(sock_set_ip, SIOCSIFADDR, &ifr_set_ip) < 0) {
        perror( "Not setup interface");
        return -1;
    }

    ifr_set_ip.ifr_flags |= IFF_UP |IFF_RUNNING;

    if (ioctl(sock_set_ip, SIOCSIFFLAGS, &ifr_set_ip) < 0) {
        perror("SIOCSIFFLAGS");
        return -1;     
    }     
    
    close(sock_set_ip);
    return 0;
}


static int set_wifi_adapter_state(int state)
{
    int sock_set;
    struct sockaddr_in sin_set;
    struct ifreq ifr_set;

    memset(&ifr_set,0,sizeof(ifr_set));

    sock_set = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_set < 0) {
        perror("socket create failse...SetLocalIp!");
        return -1;
    }

    memset(&sin_set, 0, sizeof(sin_set));
    strncpy(ifr_set.ifr_name, "wlan0", sizeof(ifr_set.ifr_name) - 1);

    ioctl(sock_set, SIOCGIFFLAGS, &ifr_set);

    if (state) {
        ifr_set.ifr_flags |= (IFF_UP | IFF_RUNNING);
    } else {
        ifr_set.ifr_flags &= ~IFF_UP;
    }

    ioctl(sock_set,SIOCSIFFLAGS,&ifr_set);

    return 0;
}

int hostapd_enable(struct hostapd_network *net)
{
    int ret = 0;
    int mode = get_wifi_current_mode();

    if (mode == WIFI_SOFTAP_MODE) {
        return 1;
    }

    if (!net) {
        return -1;
    }

    ret = system("killall supplicant");
    ret |= system("killall dhcpcd");
    if (ret != 0) {
        printf("kill supplicant & dhcpcd error\n");
    }

    ret = softap_prepare_configure(net->ssid, net->psk);
    if (ret < 0) {
        printf("softap configure error\n");
        return ret;
    }

    ret = set_wifi_adapter_state(0);
    if (ret < 0) {
        printf("stop wifi adapter\n");
        return ret;
    }

    ret = system("hostapd /data/system/hostapd.conf -e /etc/entropy.bin &");
    if (ret) {
        printf("hostapd start  error %d\n", errno);
        return -2;
    }

    ret = softap_set_ip(net->ip);
    if (ret < 0) {
        printf("ip configure  error\n");
        return ret;
    }

    set_wifi_mode(WIFI_SOFTAP_MODE);

    return 0;
}

int hostapd_disable()
{
    int ret = 0;
    int mode = get_wifi_current_mode();

    mode = get_wifi_current_mode();
    if (mode != WIFI_SOFTAP_MODE) {
        return -1;
    }

    ret = system("killall dnsmasq");
    ret |= system("killall hostapd");
    if (ret < 0) {
        printf("kill error\n");
    }

    set_wifi_mode(WIFI_NONE_MODE);
    return 0;
}

int softap_set_ip(const char *ip)
{
    int ret = 0;
    char *p = NULL;
    char cur[36] = {0};
    char cmd[256] = {0};


    ret = set_current_ip(ip);
    if (ret < 0) {
        printf("set current ip error\n");
        return -1;
    }

    p = strrchr(ip, '.');
    if (!p) {
        return -2;
    }

    strncpy(cur, ip, p + 1 - ip);
    printf("host :: %s\n", cur);

    sprintf(cmd, "dnsmasq -iwlan0  --dhcp-option=3,%s --dhcp-range=%s50,%s200,12h -p100 -D", ip, cur, cur);
    printf("cmd :: %s\n", cmd);

    ret = system(cmd);
    if (ret) {
        printf("%s is error\n", cmd);
        return ret;
    }

    return 0;
}

int supplicant_enable()
{
    int ret = 0;
    int mode = get_wifi_current_mode();

    printf("wifi current mode :: %d\n", mode);

    if (mode == WIFI_STATION_MODE) {
        return 1;
    }

    ret = system("killall dnsmasq");
    ret |= system("systemctl stop hostapd");
    if (ret != 0) {
        printf("killall error\n");
    }

    ret = set_wifi_adapter_state(0);
    if (ret < 0) {
        printf("stop wifi adapter\n");
        return ret;
    }

    ret = set_current_ip("0.0.0.0");
    if (ret < 0) {
        printf("clear wlan0 ip\n");
        return ret;
    }
    
    ret = system("systemctl start supplicant");
    if (ret < 0) {
        return -2;
    }

    ret = system("systemctl start dhcpcd");
    if (ret < 0) {
        return -3;
    }

    set_wifi_mode(WIFI_STATION_MODE);

    return 0;
}

int wifi_mode_init(int mode)
{
    pthread_mutex_init(&global_mode.mode_mutex, NULL);

    set_wifi_mode(mode);

    return 0;
}
