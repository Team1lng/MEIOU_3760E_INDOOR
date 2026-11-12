#include <arpa/inet.h>
#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <linux/sockios.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <netinet/in.h>
#include <net/if.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <signal.h>
#include <ifaddrs.h>
#include <stddef.h>
#include <linux/types.h>

#include "iwlib.h"
#include "tkl_wifi.h"
#include "tkl_net.h"
#include "tkl_gen_net.h"
#include "tkl_wifi_log.h"
#include "wpa_command.h"
#include "sys_cmd.h"
#include "priv_ipv6.h"
#ifndef NL80211
#include "pcap.h"
#endif

#ifndef NL80211
/* pcapץ��ʹ�� */
static pcap_t *device = NULL;
#endif

#ifndef TKL_SSID_LEN
#define TKL_SSID_LEN 32
#endif

/* for 8197 */
#ifndef UDHCPC_SCRIPT_PATH
// #define UDHCPC_SCRIPT_PATH      "/tuya/data/8197wlan0_udhcpc.script"
#define UDHCPC_SCRIPT_PATH "/usr/share/udhcpc/default.script"
#endif

typedef enum
{
    CN = 0,
    US,
    JP,
    EU,
    COUNTRY_NR,
} WIFI_COUNTRY_E;

typedef struct
{
    WIFI_COUNTRY_E cc;
    int regd;
} G_C_COUNTRY;

G_C_COUNTRY g_c_country[COUNTRY_NR] = {
    {CN, 13}, /* China  CN */
    {US, 1},  /* USA  FCC */
    {JP, 6},  /* Japan  MKK*/
    {EU, 3},  /* Europe ETSI*/
};

int tkl_net_wifi_enable(char *ifname, char enable)
{
    int skfd = 0;
    struct ifreq ifr;
    struct sockaddr_in *sin = NULL;
    char *default_ip = "0.0.0.0";

    memset(&ifr, 0, sizeof(ifr));
    skfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (skfd < 0)
    {
        fprintf(stderr, "create socket err : %s\n", strerror(errno));
        return -1;
    }

    strcpy(ifr.ifr_name, ifname);

    if (ioctl(skfd, SIOCGIFFLAGS, &ifr) < 0)
    {
        // fprintf(stderr,"socket ioctl err:%s\n",strerror(errno));
        close(skfd);
        return -1;
    }

    if (enable)
    {
        ifr.ifr_flags |= IFF_UP;
    }
    else
    {
        // clear ip
        sin = (struct sockaddr_in *)&ifr.ifr_addr;
        sin->sin_family = AF_INET;
        if (inet_aton(default_ip, &(sin->sin_addr)) != 0)
        {
            if (ioctl(skfd, SIOCSIFADDR, &ifr) < 0)
            {
                close(skfd);
                return -1;
            }
        }
        ifr.ifr_flags &= ~IFF_UP;
    }

    if (ioctl(skfd, SIOCSIFFLAGS, &ifr) < 0)
    {
        // fprintf(stderr,"socket ioctl err : %s\n",strerror(errno));
        close(skfd);
        return -1;
    }

    close(skfd);
    return 0;
}

int tkl_net_get_ipv4(char *ifname, char *ip)
{
    int skfd = 0;
    struct ifreq ifr;
    struct sockaddr_in *sin;

    skfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (skfd < 0)
    {
        fprintf(stderr, "create socket err : %s\n", strerror(errno));
        return -1;
    }

    strcpy(ifr.ifr_name, ifname);
    if (ioctl(skfd, SIOCGIFFLAGS, &ifr) < 0)
    {
        // fprintf(stderr,"socket ioctl err : %s\n",strerror(errno));
        close(skfd);
        return -1;
    }

    if (!(ifr.ifr_flags & IFF_RUNNING))
    {
        close(skfd);
        return -1;
    }

    /* get ip */
    strcpy(ifr.ifr_name, ifname);
    if (ioctl(skfd, SIOCGIFADDR, &ifr) < 0)
    {
        // fprintf(stderr,"socket ioctl err : %s\n",strerror(errno));
        close(skfd);
        return -1;
    }
    sin = (struct sockaddr_in *)&(ifr.ifr_addr);
    strcpy(ip, inet_ntoa(sin->sin_addr));

    close(skfd);
    return 0;
}

static inline unsigned int ipv6_addr_scope2type(unsigned int scope)
{
    switch (scope)
    {
    case IPV6_ADDR_SCOPE_NODELOCAL:
        return (IPV6_ADDR_SCOPE_TYPE(IPV6_ADDR_SCOPE_NODELOCAL) |
                IPV6_ADDR_LOOPBACK);
    case IPV6_ADDR_SCOPE_LINKLOCAL:
        return (IPV6_ADDR_SCOPE_TYPE(IPV6_ADDR_SCOPE_LINKLOCAL) |
                IPV6_ADDR_LINKLOCAL);
    case IPV6_ADDR_SCOPE_SITELOCAL:
        return (IPV6_ADDR_SCOPE_TYPE(IPV6_ADDR_SCOPE_SITELOCAL) |
                IPV6_ADDR_SITELOCAL);
    }
    return IPV6_ADDR_SCOPE_TYPE(scope);
}

static inline int __ipv6_addr_type(const struct in6_addr *addr)
{
    __be32 st;
    st = addr->s6_addr32[0];

    /* Consider all addresses with the first three bits different of
       000 and 111 as unicasts.
       */
    if ((st & htonl(0xE0000000)) != htonl(0x00000000) && (st & htonl(0xE0000000)) != htonl(0xE0000000))
        return (IPV6_ADDR_UNICAST |
                IPV6_ADDR_SCOPE_TYPE(IPV6_ADDR_SCOPE_GLOBAL));

    if ((st & htonl(0xFF000000)) == htonl(0xFF000000))
    {
        /* multicast */
        /* addr-select 3.1 */
        return (IPV6_ADDR_MULTICAST |
                ipv6_addr_scope2type(IPV6_ADDR_MC_SCOPE(addr)));
    }

    if ((st & htonl(0xFFC00000)) == htonl(0xFE800000))
        return (IPV6_ADDR_LINKLOCAL | IPV6_ADDR_UNICAST |
                IPV6_ADDR_SCOPE_TYPE(IPV6_ADDR_SCOPE_LINKLOCAL)); /* addr-select 3.1 */
    if ((st & htonl(0xFFC00000)) == htonl(0xFEC00000))
        return (IPV6_ADDR_SITELOCAL | IPV6_ADDR_UNICAST |
                IPV6_ADDR_SCOPE_TYPE(IPV6_ADDR_SCOPE_SITELOCAL)); /* addr-select 3.1 */
    if ((st & htonl(0xFE000000)) == htonl(0xFC000000))
        return (IPV6_ADDR_UNICAST |
                IPV6_ADDR_SCOPE_TYPE(IPV6_ADDR_SCOPE_GLOBAL)); /* RFC 4193 */

    if ((addr->s6_addr32[0] | addr->s6_addr32[1]) == 0)
    {
        if (addr->s6_addr32[2] == 0)
        {
            if (addr->s6_addr32[3] == 0)
                return IPV6_ADDR_ANY;

            if (addr->s6_addr32[3] == htonl(0x00000001))
                return (IPV6_ADDR_LOOPBACK | IPV6_ADDR_UNICAST |
                        IPV6_ADDR_SCOPE_TYPE(IPV6_ADDR_SCOPE_LINKLOCAL)); /* addr-select 3.4 */

            return (IPV6_ADDR_COMPATv4 | IPV6_ADDR_UNICAST |
                    IPV6_ADDR_SCOPE_TYPE(IPV6_ADDR_SCOPE_GLOBAL)); /* addr-select 3.3 */
        }

        if (addr->s6_addr32[2] == htonl(0x0000ffff))
            return (IPV6_ADDR_MAPPED |
                    IPV6_ADDR_SCOPE_TYPE(IPV6_ADDR_SCOPE_GLOBAL)); /* addr-select 3.3 */
    }

    return (IPV6_ADDR_UNICAST |
            IPV6_ADDR_SCOPE_TYPE(IPV6_ADDR_SCOPE_GLOBAL)); /* addr-select 3.4 */
}

static inline int ipv6_addr_scope(const struct in6_addr *addr)
{
    return __ipv6_addr_type(addr) & IPV6_ADDR_SCOPE_MASK;
}

int tkl_net_get_ipv6(char *ifname, NW_IP_TYPE type, NW_IP_S *ip)
{
    int family = 0;
    int scope = 0;
    struct ifaddrs *ifaddr = NULL;
    struct ifaddrs *ifa = NULL;
    char *addr = NULL;
    int size = 0;
    int ret = -1;

    if (getifaddrs(&ifaddr) == 0)
    {
        for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
        {
            if (ifa->ifa_addr == NULL)
                continue;

            if (strncmp(ifname, ifa->ifa_name, IFNAMSIZ))
                continue;

            family = ifa->ifa_addr->sa_family;

            switch (family)
            {
            case AF_INET:
                continue;
            case AF_INET6:
                addr = ip->addr.ip6.ip;
                size = sizeof(ip->addr.ip6.ip);

                scope = ipv6_addr_scope((const struct in6_addr *)&((struct sockaddr_in6 *)ifa->ifa_addr)->sin6_addr);
                if (type == NW_IPV6 && (scope == IPV6_ADDR_UNICAST || scope == IPV6_ADDR_ANY))
                {
                    inet_ntop(family, (char *)ifa->ifa_addr + offsetof(struct sockaddr_in6, sin6_addr), addr, size);
                    printf("%s: %s, scope=0x%04x\n", ifa->ifa_name, addr, scope);
                    ret = 0;
                    break;
                }
                else if (type == NW_IPV6_LL && scope == IPV6_ADDR_LINKLOCAL)
                {
                    inet_ntop(family, (char *)ifa->ifa_addr + offsetof(struct sockaddr_in6, sin6_addr), addr, size);
                    printf("%s: %s, scope=0x%04x\n", ifa->ifa_name, addr, scope);
                    ip->addr.ip6.islinklocal = 1;
                    ret = 0;
                    break;
                }

            default:
                continue;
            }
        }
        freeifaddrs(ifaddr);
    }
    else
    {
        return -1;
    }

    return ret;
}

int tkl_net_get_mask_ipv4(char *ifname, char *mask)
{
    int skfd = 0;
    struct ifreq ifr;
    struct sockaddr_in *sin;

    if (mask == NULL)
    {
        TKL_LOGE("invalid param");
        return -1;
    }

    skfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (skfd < 0)
    {
        fprintf(stderr, "create socket err : %s\n", strerror(errno));
        return -1;
    }

    /* get mask*/
    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, ifname);
    if (ioctl(skfd, SIOCGIFNETMASK, &ifr) < 0)
    {
        // printf("ioctl error\n");
        close(skfd);
    }
    sin = (struct sockaddr_in *)&(ifr.ifr_addr);
    strncpy(mask, inet_ntoa(sin->sin_addr), MAX_IP_LEN);

    close(skfd);
    return 0;
}

int tkl_net_set_ip(const char *ifname, const char *ip)
{
    int skfd = 0;
    struct ifreq ifr;
    struct sockaddr_in *addr = NULL;

    memset(&ifr, 0, sizeof(struct ifreq));
    strcpy(ifr.ifr_name, ifname);
    if ((skfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        fprintf(stderr, "socket fail\n");
        return -1;
    }

    addr = (struct sockaddr_in *)&(ifr.ifr_addr);
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = inet_addr(ip);
    if (ioctl(skfd, SIOCSIFADDR, &ifr) < 0)
    {
        fprintf(stderr, "ioctl fail\n");
        close(skfd);
        return -1;
    }

    ifr.ifr_flags |= IFF_UP | IFF_RUNNING;
    if (ioctl(skfd, SIOCSIFFLAGS, &ifr) < 0)
    {
        fprintf(stderr, "ioctl fail:%s\n", strerror(errno));
        close(skfd);
        return -1;
    }
    close(skfd);

    return 0;
}

int tkl_net_get_mac(char *ifname, char *mac)
{
    int fd = -1;
    struct ifreq ifr;

    bzero(&ifr, sizeof(struct ifreq));

    if (mac == NULL)
    {
        fprintf(stderr, "invalid parm\n");
        return -1;
    }

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        fprintf(stderr, "get %s mac address socket creat error\n", ifname);
        return -1;
    }

    strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name) - 1);
    if (ioctl(fd, SIOCGIFHWADDR, &ifr) < 0)
    {
        // fprintf(stderr,"get %s mac address error\n", ifname);
        close(fd);
        return -1;
    }
    memcpy(mac, (unsigned char *)ifr.ifr_hwaddr.sa_data, 6);

    close(fd);

    return 0;
}

#ifdef NL80211
int tkl_net_get_connect_ap_ssid(char *ssid)
{
    int iret = 0;

    iret = wifi_get_current_ssid(ssid);
    if (iret < 0)
    {
        TKL_LOGE("WiFi : get connect ap ssid failed");
        return -1;
    }

    return 0;
}

#else
int tkl_net_get_connect_ap_ssid(char *ssid)
{
    int iret = 0;
    int skfd;
    char *ifname = WLAN_DEV; // default
    struct iwreq wrq;
    char tmp_ssid[MAX_SSID_LEN + 2] = {0x0};

    if ((skfd = iw_sockets_open()) < 0)
    {
        perror("socket");
        exit(-1);
    }

    wrq.u.essid.pointer = (caddr_t)tmp_ssid;
    wrq.u.essid.length = MAX_SSID_LEN + 2;
    wrq.u.essid.flags = 0;
    iret = iw_get_ext(skfd, ifname, SIOCGIWESSID, &wrq);
    if (iret < 0)
    {
        TKL_LOGE("WiFI : Get connect ap ssid failed");
        close(skfd);
        return -1;
    }
    strncpy(ssid, tmp_ssid, sizeof(tmp_ssid));

    close(skfd);

    return 0;
}
#endif

int tkl_net_set_mac(const char *ifname, const char *mac)
{
    int fd = -1;
    struct ifreq ifr;

    if (!ifname || !mac)
    {
        fprintf(stderr, "invalid parm\n");
        return -1;
    }

    bzero(&ifr, sizeof(struct ifreq));

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        fprintf(stderr, "get %s mac address socket creat error\n", ifname);
        return -1;
    }

    ifr.ifr_addr.sa_family = ARPHRD_ETHER;
    strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name) - 1);
    memcpy((unsigned char *)ifr.ifr_hwaddr.sa_data, mac, 6);

    if (ioctl(fd, SIOCSIFHWADDR, &ifr) < 0)
    {
        fprintf(stderr, "set %s mac address error\n", ifname);
        close(fd);
        return -1;
    }
    close(fd);

    return 0;
}
/* **************************************************************
 * Function : tkl_net_set_mac_8723
 * Describe : 8723���治��֧��ʹ��ͨ�ýӿ�����MAC��ַ�����Բ���ר�ýӿڡ�
 *            8723���趨MAC��ַ��Ҫ��Ч��Ҫ�Ƚ�wlan0 down��up
 */
int tkl_net_set_mac_8723(const char *ifname, const char *mac)
{
    int i;
    char *if_name = (char *)ifname;
    char tmp_mac[2 * MAC_ADDR_LEN + 1] = {0x0};
    char buf[128] = {0x0};

    for (i = 0; i < MAC_ADDR_LEN; i++)
    {
        sprintf(&tmp_mac[2 * i], "%02x", mac[i]);
    }

    tkl_net_wifi_enable(if_name, 0); // ifconfig wlan0 down
    snprintf(buf, sizeof(buf), "vendor_storage -w \"VENDOR_WIFI_MAC_ID %s\"", tmp_mac);
    sys_cmd_run(buf, NULL, NULL);
    tkl_net_wifi_enable(if_name, 1);

    return 0;
}

int tkl_net_set_chn(char *ifname, const char channel)
{
    int ret;
    int skfd;
    struct iw_range range;
    int has_range;
    struct iwreq wrq;
    double freq;

    if ((skfd = iw_sockets_open()) < 0)
    {
        perror("socket fail\n");
        return -1;
    }

    has_range = (iw_get_range_info(skfd, ifname, &range) >= 0);
    if ((!has_range) || (range.we_version_compiled < 14))
    {
        fprintf(stderr, "%-8.16s  Interface doesn't support channel setting.\n\n", ifname);
        iw_sockets_close(skfd);
        return (-1);
    }

    WIFI_TYPE_E wifi_type = tkl_net_get_wifi_type();
    if (wifi_type < 0 || wifi_type > WIFI_TYPE_NR)
    {
        TKL_LOGE("WiFi : can't get wifi type");
        return -1;
    }
    else if (wifi_type == WIFI_RTL8197)
    {
        wrq.u.freq.m = channel;
        wrq.u.freq.e = 0;
        wrq.u.freq.i = 0;
    }
    else
    {
        ret = iw_channel_to_freq(channel, &freq, &range);

        if (ret != channel)
        {
            TKL_LOGE("change_to_freq fail:%d\n", ret);
            return -1;
        }
        // TKL_LOGD("freq = %lf",freq);
        iw_float2freq(freq, &(wrq.u.freq));
    }

    //    iw_channel_to_freq(channel, &freq , &range);
    //    TKL_LOGD("freq = %lf",freq);
    //    iw_float2freq(freq, &(wrq.u.freq));

    wrq.u.freq.flags = IW_FREQ_FIXED;
    ret = iw_set_ext(skfd, ifname, SIOCSIWFREQ, &wrq);
    if (ret < 0)
    {
        // fprintf(stderr,"socket ioctl err : %s\n",strerror(errno));
        return -1;
    }

    iw_sockets_close(skfd);

    return ret;
}

int tkl_net_get_chn(char *ifname, unsigned char *channel)
{
    int skfd;
    struct iw_range range;
    int has_range;
    struct iwreq wrq;
    int ret = 0;
    double freq;

    if ((skfd = iw_sockets_open()) < 0)
    {
        perror("socket fail\n");
        return -1;
    }

    has_range = (iw_get_range_info(skfd, ifname, &range) >= 0);
    if ((!has_range) || (range.we_version_compiled < 14))
    {
        fprintf(stderr, "%-8.16s  Interface doesn't support channel get.\n\n", ifname);
        iw_sockets_close(skfd);
        return (-1);
    }

    ret = iw_get_ext(skfd, ifname, SIOCGIWFREQ, &wrq);
    if (ret < 0)
    {
        // fprintf(stderr,"socket ioctl err : %s\n",strerror(errno));
        iw_sockets_close(skfd);
        return -1;
    }
    else
    {
        freq = iw_freq2float(&wrq.u.freq);
        *channel = iw_freq_to_channel(freq, &range);
    }

    iw_sockets_close(skfd);

    return ret;
}

/*  mode:
 *    0 --- Auto
 *    1 --- Ad-Hoc
 *    2 --- Managed
 *    3 --- Master
 *    4 --- Repeater
 *    5 --- Secondary
 *    6 --- Monitor
 */
int tkl_net_get_mode(char *ifname, int *mode)
{
    int skfd = 0;
    struct iwreq wrq;

    skfd = iw_sockets_open();
    if (skfd < 0)
    {
        fprintf(stderr, "create socket err : %s\n", strerror(errno));
        return -1;
    }

    if (iw_get_ext(skfd, ifname, SIOCGIWMODE, &wrq) < 0)
    {
        // fprintf(stderr,"socket ioctl err : %s\n",strerror(errno));
        iw_sockets_close(skfd);
        return -1;
    }
    *mode = wrq.u.mode;

    iw_sockets_close(skfd);

    return 0;
}
#ifdef NL80211
int tkl_net_set_mode(char *ifname, char *mode)
{
    int k;
    int skfd = 0;
    struct iwreq wrq;
    int NUM_OPER_MODE = 7;

    for (k = 0; k < NUM_OPER_MODE; k++)
    {
        if (strncasecmp(mode, iw_operation_mode[k], 3) == 0)
            break;
    }

    if (k >= NUM_OPER_MODE || k == 1 || k == 4 || k == 5)
    {
        fprintf(stderr, "[%s:%d] set invalid mode : %s\n", __FUNCTION__, __LINE__, mode);
        return -1;
    }

    skfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (skfd < 0)
    {
        fprintf(stderr, "create socket err : %s\n", strerror(errno));
        return -1;
    }

    strcpy(wrq.ifr_name, ifname);
    wrq.u.mode = k;
    if (ioctl(skfd, SIOCSIWMODE, &wrq) < 0)
    {
        // fprintf(stderr,"socket ioctl err : %s\n",strerror(errno));
        close(skfd);
        return -1;
    }

    close(skfd);
    return 0;
}

#else /* for 8197 */
int tkl_net_set_mode(char *ifname, char *mode)
{
    WIFI_MODE_E mode_e = 0;
    char cmd_buf[128] = {0x0};
    tkl_net_get_mode(ifname, (int *)&mode_e);

    if (strncasecmp(mode, "Managed", 6) == 0 && mode_e != WIFI_MODE_MANAGED)
    { /* 8197 ��֧���ظ��趨����ģʽ���ظ����ù���ģʽ����� */
        snprintf(cmd_buf, sizeof(cmd_buf), "/sbin/iwpriv %s set_mib opmode=%d", ifname, 8);
        sys_cmd_run(cmd_buf, NULL, NULL);
    }

    if (strncasecmp(mode, "Master", 5) == 0 && mode_e != WIFI_MODE_MASTER)
    {
        snprintf(cmd_buf, sizeof(cmd_buf), "/sbin/iwpriv %s set_mib opmode=%d", ifname, 16);
        sys_cmd_run(cmd_buf, NULL, NULL);
    }

    return 0;
}
#endif

static int process_kill(char *ifname, char *proc)
{
    char cmd_buf[128] = {0};
    char pidbuf[32];
    int pidbuf_len = sizeof(pidbuf);
    int killpid = 0;
    if (!ifname || !proc)
    {
        TKL_LOGE("args is null");
        return -1;
    }

    /* ��ȡ��ָ��process��PID */
    snprintf(cmd_buf, sizeof(cmd_buf), "/bin/cat /tmp/%s-%s.pid", proc, ifname);

    if (sys_cmd_run(cmd_buf, pidbuf, &pidbuf_len) < 0)
    {
        TKL_LOGE("popen failed");
        return -1;
    }

    if (strlen(pidbuf) == 0)
    {
        TKL_LOGE("failed to get pid");
        return -1;
    }
    sscanf(pidbuf, "%d", &killpid);
    if (killpid <= 0)
    {
        TKL_LOGE("pid is error, killpid=%d\n", killpid);
        return -1;
    }

    snprintf(cmd_buf, sizeof(cmd_buf), "/tmp/%s-%s.pid", proc, ifname);
    if (kill(killpid, SIGKILL) < 0)
    {
        TKL_LOGE("kill 1 times failed, pid=%d\n", killpid);
        if (kill(killpid, SIGKILL) < 0)
        {
            TKL_LOGE("kill 2 times failed, pid=%d\n", killpid);
            return -1;
        }
        else
        {
            unlink(cmd_buf);
        }
    }
    else
    {
        unlink(cmd_buf);
    }

    return 0;
}

#ifdef NL80211
static int process_alive(char *process)
{
    FILE *fp;
    char buf[100];
    char command[150];
    int procpid = 0;

    snprintf(command, sizeof(command), "/tmp/%s-wlan0.pid", process);
    if (0 != access(command, F_OK))
    {
        return 0;
    }

    if ((fp = fopen(command, "r")) == NULL)
    {
        TKL_LOGE("fopen failed");
        return 0;
    }

    memset(buf, 0x0, sizeof(buf));
    if (!fgets(buf, sizeof(buf), fp))
    {
        fclose(fp);
        return 0;
    }

    fclose(fp);
    if (strlen(buf) == 0)
    {
        TKL_LOGE("failed to get pid");
        return 0;
    }

    sscanf(buf, "%d", &procpid);
    if (procpid <= 0)
    {
        TKL_LOGE("pid is error, procpid=%d", procpid);
        return 0;
    }

    snprintf(command, sizeof(command), "/proc/%d/status", procpid);
    if (0 != access(command, F_OK))
    {
        return 0;
    }

    if ((fp = fopen(command, "r")) == NULL)
    {
        TKL_LOGE("fopen failed");
        return 0;
    }

    memset(buf, 0x0, sizeof(buf));
    if (!fgets(buf, sizeof(buf), fp))
    {
        fclose(fp);
        return 0;
    }
    fclose(fp);
    if (strstr(buf, process))
    {
        return 1;
    }

    return 0;
}

#endif
int ip_prepare(char *src, char *dst)
{
    int i, dot = 0;
    char tmp_ip[MAX_IP_LEN] = {0x0};

    snprintf(tmp_ip, sizeof(tmp_ip), "%s", src);
    for (i = 0; i < strlen(tmp_ip); i++)
    {
        if (tmp_ip[i] == '.')
            dot++;
        if (dot == 3)
        {
            tmp_ip[i + 1] = 0;
            break;
        }
    }

    if (dot != 3)
    {
        TKL_LOGE("WIFI ipaddr(%s) is not legal", src);
        return -1;
    }

    strcpy(dst, tmp_ip);

    return 0;
}

int tkl_net_wifi_scan(char *ifname, AP_IF_S *aps, int max_ap)
{
    int ret;
    int skfd;
    struct iw_range range;
    int has_range;
    wireless_scan_head ctx;
    char buffer[128];
    int count = 0;
    wireless_scan *node;
    struct wireless_config *pconf;

    if ((skfd = iw_sockets_open()) < 0)
    {
        perror("socket fail\n");
        return 0;
    }

    /* Get range stuff */
    has_range = (iw_get_range_info(skfd, ifname, &range) >= 0);
    /* Check if the interface could support scanning. */
    if ((!has_range) || (range.we_version_compiled < 14))
    {
        fprintf(stderr, "%-8.16s  Interface doesn't support scanning.\n\n", ifname);
        iw_sockets_close(skfd);
        return 0;
    }

    memset(&ctx, 0, sizeof(wireless_scan_head));
    ret = iw_scan(skfd, ifname, range.we_version_compiled, &ctx);
    if (ret < 0)
    {
        fprintf(stderr, "iw scan error:%d\n", ret);
        iw_sockets_close(skfd);
        return 0;
    }

    node = ctx.result;

    while (node != NULL)
    {
        int length;
        AP_IF_S *tmp;
        double freq;
        int x0, x1, x2, x3, x4, x5;

        if (node->has_ap_addr == 0)
            continue;

        if (count >= max_ap)
            break;

        tmp = &aps[count++];
        pconf = &(node->b);
        // printf("essid len is %d ,ESSID : %s\n",pconf->essid_len,pconf->essid);
        length = strlen(pconf->essid);
        tmp->s_len = length > TKL_SSID_LEN ? TKL_SSID_LEN : length;
        memcpy(tmp->ssid, pconf->essid, tmp->s_len);
        // printf("len is %d ,SSID : %s\n",tmp->s_len,tmp->ssid);
        freq = pconf->freq; /* Frequency/channel */
        tmp->channel = iw_freq_to_channel(freq, &range);

        memset(buffer, 0, sizeof(buffer));
        iw_sawap_ntop(&node->ap_addr, buffer);
        sscanf(buffer, "%02x:%02x:%02x:%02x:%02x:%02x\n", &x0, &x1, &x2, &x3, &x4, &x5);
        tmp->bssid[0] = 0xff & x0;
        tmp->bssid[1] = 0xff & x1;
        tmp->bssid[2] = 0xff & x2;
        tmp->bssid[3] = 0xff & x3;
        tmp->bssid[4] = 0xff & x4;
        tmp->bssid[5] = 0xff & x5;
        if (node->has_stats)
            tmp->rssi = node->stats.qual.qual;
        // tmp->rssi = node->stats.qual.qual - 100;  /* rssi ת�� */

        // TKL_LOGD("SSID : %s key : %s key_size : %d key_flags : %d",pconf->essid,pconf->key,pconf->key_size,pconf->key_flags);

        if (pconf->key_flags & 0x8000)
        {                      /* �ж��Ƿ���� */
            tmp->security = 0; // open
        }
        else
        {
            if (pconf->key_flags & 0x2000)
            {
                tmp->security = 0; // open
            }
            else if (pconf->key_flags & 0x800)
            {
                tmp->security = 4;
            }
        }
        node = node->next;
    }

    node = ctx.result;
    while (node != NULL)
    {
        wireless_scan *tmp = node;
        node = node->next;
        free(tmp);
    }

    iw_sockets_close(skfd);

    return count;
}

// current is not used
#if 0
static int softap_prepare_configure(const char *ssid,const char *psk,int channel) 
{
    int fd = 0,ret = 0;
    char buf[1024] = {0};
    char *ifname = WLAN_DEV;

    unlink(WIFI_HOSTAPD_CONF);

    fd = open(WIFI_HOSTAPD_CONF, O_RDWR | O_CREAT, 0x666);
    if (fd < 0) {
        TKL_LOGE("create configure error %d\n", errno);
        return -1;
    }

    TKL_LOGD("%s->%d\n",__func__,__LINE__);
    if (psk != NULL && strlen(psk) > 0) {
//      sprintf(buf, "interface=wlan0\n#driver=nl80211\nctrl_interface=/var/run/hostapd\nssid=%s\nchannel=%d\nieee80211n=1\n#ieee80211ac=1\nhw_mode=g\nignore_broadcast_ssid=0\nwpa=3\nwpa_passphrase=%s\nwpa_key_mgmt=WPA-PSK\nwpa_pairwise=TKIP CCMP\nrsn_pairwise=CCMP\n", ssid, channel, psk);
        sprintf(buf, "interface=%s\nctrl_interface_group=0\ndriver=nl80211\nieee80211n=1\nssid=%s\nhw_mode=g\nchannel=%d\nmacaddr_acl=0\nauth_algs=1\nignore_broadcast_ssid=0\nwpa=3\nwpa_passphrase=%s\nwpa_key_mgmt=WPA-PSK\nwpa_pairwise=TKIP CCMP\nrsn_pairwise=CCMP\n",ifname, ssid, channel, psk);
    } else {
//      sprintf(buf, "interface=wlan0\n#driver=nl80211\nctrl_interface=/var/run/hostapd\nssid=%s\nchannel=%d\nieee80211n=1\n#ieee80211ac=1\nhw_mode=g\nignore_broadcast_ssid=0\n#wpa=3\n#wpa_passphrase=\n#wpa_key_mgmt=WPA-PSK\n#wpa_pairwise=TKIP CCMP\n#rsn_pairwise=CCMP\n", ssid, channel);
        sprintf(buf, "interface=%s\n#driver=nl80211\nctrl_interface=/var/run/hostapd\nssid=%s\nchannel=%d\nieee80211n=1\n#ieee80211ac=1\nhw_mode=g\nignore_broadcast_ssid=0\n#wpa=3\n#wpa_passphrase=\n#wpa_key_mgmt=WPA-PSK\n#wpa_pairwise=TKIP CCMP\n#rsn_pairwise=CCMP\n",ifname,ssid, channel);
    }

    ret = write(fd, buf, strlen(buf));
    if (ret < 0) {
        TKL_LOGE("write hostapd buf errno %d\n", errno);
        close(fd);
        return -1;
    }

    close(fd);

    return 0;
}
#endif

#ifdef NL80211
static int wpa_config_prepare(const char *ssid, const char *psk)
{
    int fd = 0;
    int iret = 0;
    char buf[1024] = {0};

    unlink(WIFI_STATION_CONF);

    fd = open(WIFI_STATION_CONF, O_RDWR | O_CREAT, 0x666);
    if (fd < 0)
    {
        TKL_LOGE("create configure error %d\n", errno);
        return -1;
    }

#if 0 /* ���ַ�ʽ�ᴥ������wpa_supplicant���̣����ǲ��ᴥ�����ӣ����Բ����������ʧ�ܵ����� */
    sprintf(buf,"ctrl_interface=%s\n"
        "update_config=1\n", 
    WPA_CTRL_INTERFACE, ssid, psk);
#endif

    if (psk == NULL || 0 == strlen(psk))
    {
        sprintf(buf, "ctrl_interface=%s\n"
                     "update_config=1\n"
                     "network={\n"
                     "scan_ssid=1\n"
                     "ssid=\"%s\"\n"
                     "key_mgmt=NONE\n"
                     "}\n",
                WPA_CTRL_INTERFACE, ssid);
    }
    else
    {
        sprintf(buf, "ctrl_interface=%s\n"
                     "update_config=1\n"
                     "network={\n"
                     "scan_ssid=1\n"
                     "ssid=\"%s\"\n"
                     "key_mgmt=WPA-PSK\n"
                     "psk=\"%s\"\n"
                     "}\n",
                WPA_CTRL_INTERFACE, ssid, psk);
    }

    iret = write(fd, buf, strlen(buf));
    if (iret < 0)
    {
        TKL_LOGE("write hostapd buf errno %d\n", errno);
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

static int start_wpa(void)
{
    char *ifname = WLAN_DEV;
    char cmd_buf[128] = {0x0};
    DIR *dir = NULL;

    /* ȷ�����ƽӿ�/va/run�Ǵ��ڵ�,���������򴴽���Ŀ¼ */
    dir = opendir(WPA_CTRL_DIR);
    if (dir == NULL)
    {
        if (mkdir(WPA_CTRL_DIR, 0666) < 0)
        { /* wpa_supplicant ctrl dir mode 0666 */
            TKL_LOGE("WiFi: Create '%s' fail", WPA_CTRL_DIR);
            return -1;
        }
    }
    else
    {
        closedir(dir);
    }

    /* ���������ӿڵĲ�ͬ��ѡ��ͬ */
#ifdef NL80211
    snprintf(cmd_buf, sizeof(cmd_buf), "/usr/sbin/" WPA_SUPPLICANT " -P /tmp/wpa_supplicant-%s.pid -i %s -Dnl80211 -c %s -B", ifname, ifname, WIFI_STATION_CONF);
#else
    snprintf(cmd_buf, sizeof(cmd_buf), "/usr/sbin/" WPA_SUPPLICANT " -P /tmp/wpa_supplicant-%s.pid -i %s -Dwext -c %s -B", ifname, ifname, WIFI_STATION_CONF);
#endif
    if (sys_cmd_run(cmd_buf, NULL, NULL) < 0)
    {
        return -1;
    }
    return 0;
}

static int get_passwd_and_ssid_from_wpa_file(char *ssid, char *passwd)
{
    FILE *fp = NULL;
    char str[1024] = {0x0};
    char *opstr = NULL;
    char buffer[1024] = {0x0};
    char tmp_ssid[32 + 1] = {0x0};
    char tmp_pwd[64 + 1] = {0x0};

    fp = fopen(WIFI_STATION_CONF, "r");
    if (fp == NULL)
    {
        TKL_LOGE("WiFi : can'r open %s", WIFI_STATION_CONF);
        return -1;
    }

    while ((fgets(buffer, sizeof(buffer), fp)) != NULL)
    {
        strcat(str, buffer);
    }

    opstr = strstr(str, "ssid=");
    if (opstr)
    {
        sscanf(opstr + strlen("ssid="), "%s", tmp_ssid);
    }
    else
    {
        TKL_LOGE("get ssid failed");
        fclose(fp);
        return -1;
    }

    opstr = strstr(str, "psk=");
    if (opstr)
    {
        sscanf(opstr + strlen("psk="), "%s", tmp_pwd);
    }
    else
    {
        TKL_LOGE("get psk failed");
        fclose(fp);
        return -1;
    }

    strncpy(ssid, tmp_ssid, 32);
    strncpy(passwd, tmp_pwd, 64);

    fclose(fp);

    return 0;
}

#ifdef OWN_UDHCPC
static void start_udhcpc(void)
{
    char *ifname = WLAN_DEV;
    char cmd_buf[128] = {0x0};

    sys_cmd_run("/usr/bin/killall udhcpc", NULL, NULL);

    snprintf(cmd_buf, sizeof(cmd_buf), "/sbin/" UDHCPC " -p  /tmp/udhcpc-%s.pid -i %s &", ifname, ifname);
    // sys_cmd_run( cmd_buf, NULL, NULL );
    system(cmd_buf);
}

// before start udhcpc6,  must set linklocal addr
static int priv_set_ipv6_linklocal_addr(const char *ifname)
{
    int sock = -1;
    struct ifreq ifr;
    struct sockaddr *addr;
    unsigned char mac[6];
    struct in6_addr ll_addr;
    char ipv6_str[40];
    char cmd_buf[128] = {0x0};

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        TKL_LOGE("%s %d: socket create fail\n", __func__, __LINE__);
        return -1;
    }

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name) - 1);
    addr = (struct sockaddr *)&ifr.ifr_hwaddr;
    addr->sa_family = 1;

    /* get mac addr */
    if (ioctl(sock, SIOCGIFHWADDR, &ifr) < 0)
    {
        TKL_LOGE("%s %d: get mac, error=%d,%s\n", __func__, __LINE__, errno, strerror(errno));
        close(sock);
        return -1;
    }

    memset(mac, 0x0, sizeof(mac));
    memcpy(mac, addr->sa_data, sizeof(mac));
    close(sock);

    memset(&ll_addr, 0x0, sizeof(ll_addr));
    ll_addr.s6_addr32[0] = htonl(0xFE800000);
    memcpy(ll_addr.s6_addr + 8, mac, 3);
    memcpy(ll_addr.s6_addr + 13, mac + 3, 3);
    ll_addr.s6_addr[11] = 0xFF;
    ll_addr.s6_addr[12] = 0xFE;
    ll_addr.s6_addr[8] ^= 2;

    inet_ntop(AF_INET6, &ll_addr, ipv6_str, sizeof(ipv6_str));
    TKL_LOGD("ll addr: %s\n", ipv6_str);

    snprintf(cmd_buf, sizeof(cmd_buf), "/sbin/ifconfig %s %s/64", ifname, ipv6_str);
    // TKL_LOGD( "cmd_buf=%s\n", cmd_buf );
    sys_cmd_run(cmd_buf, NULL, NULL);

    return 0;
}

static void start_udhcpc6(void)
{
    char *ifname = WLAN_DEV;
    char cmd_buf[128] = {0x0};

    priv_set_ipv6_linklocal_addr(ifname);

    sys_cmd_run("/usr/bin/killall udhcpc6", NULL, NULL);

    snprintf(cmd_buf, sizeof(cmd_buf), "/usr/bin/" UDHCPC6 " -p /tmp/udhcpc6-%s.pid -i %s -s /usr/share/udhcpc/default6.script &", ifname, ifname);
    // sys_cmd_run( cmd_buf, NULL, NULL );
    system(cmd_buf);
}

#endif

int tkl_net_get_curr_rssi(char *ifname, char *rssi)
{
    int ret;
    char tmp = 0;

    ret = wifi_get_signal(&tmp);
    if (ret)
    {
        fprintf(stderr, "get rssi fail : %s\n", strerror(errno));
        return -1;
    }

    if (rssi)
    {
        *rssi = (char)tmp;
    }
    else
    {
        return -1;
    }

    return 0;
}

#else /* no define NL80211*/
#if 0 /* 8197 not support */
int tkl_net_get_curr_rssi(char *ifname,int *rssi)
{
    int skfd;
    int has_range;
    struct iwreq wrq;
    struct iw_range range;
    struct iw_statistics stats;

    if((skfd = iw_sockets_open()) < 0){
        perror("socket");
        exit(-1);
    }
    memset(&stats,0,sizeof(struct iw_statistics));
    memset(&wrq,0,sizeof(struct iwreq));

    has_range = iw_get_range_info(skfd, ifname, &range);     //get the WIRELESE_EXT-version
    /* Check if the interface could support get rssi. */
    if(has_range || (range.we_version_compiled < 14)) {
        printf("%-8.16s Interface doesn't support.\n\n",ifname);
        iw_sockets_close(skfd);
        return -1;
    }

    wrq.u.data.pointer = (caddr_t) (&stats);
    wrq.u.data.length = sizeof(struct iw_statistics);
    wrq.u.data.flags = 1;       /* Clear updated flag */
    strncpy(wrq.ifr_name, ifname, IFNAMSIZ);
    if (iw_get_ext(skfd, ifname, SIOCGIWSTATS, &wrq) < 0) {
        printf("get connected ap's rssi failed : %s\n",strerror(errno));
        //return (-1);
    }

    /* Deal with signal level in RCPI */
    /* RCPI = int{(Power in dBm +110)*2} for 0dbm > Power > -110dBm */
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
#else

#if 0
int tkl_net_get_curr_rssi(char *ifname,int *rssi)
{
    int iret = 0;
    int i;
    int max_ap = MAX_AP_SEARCH;
    char ssid[MAX_SSID_LEN + 2] = {0x0};
    AP_IF_S *s_aps = (AP_IF_S *)malloc(MAX_AP_SEARCH * sizeof(AP_IF_S));
    memset(s_aps, 0, sizeof(s_aps));

    iret = tkl_net_get_connect_ap_ssid(ssid);
    if(iret < 0) {
        TKL_LOGE("WiFi : Get connect ap ssid failed");
        return -1;
    }
    iret = tkl_net_wifi_scan(ifname, s_aps, max_ap);
    if(iret < 0) {
        TKL_LOGE("WiFi : scan failed");
        return -1;
    }
    for(i = 0;i < max_ap;i++) {
        if(strcmp(ssid,s_aps[i].ssid) == 0) {
            *rssi = s_aps[i].rssi;
        }
    }

    free(s_aps);
    s_aps = NULL;

    return 0;
}

#else /* for 8197*/
int tkl_net_get_curr_rssi(char *ifname, char *rssi)
{
    int skfd; /* generic raw socket desc. */
    struct iw_quality *qual;
    iwrange range;
    struct iwreq wrq;
    char buffer[(sizeof(struct iw_quality) + sizeof(struct sockaddr)) * IW_MAX_AP];

    /* Create a channel to the NET kernel. */
    if ((skfd = iw_sockets_open()) < 0)
    {
        perror("socket");
        return -1;
    }

    wrq.u.data.pointer = (caddr_t)buffer;
    wrq.u.data.length = IW_MAX_AP;
    wrq.u.data.flags = 0;
    if (iw_get_ext(skfd, ifname, SIOCGIWAPLIST, &wrq) < 0)
    {
        fprintf(stderr, "%-8.16s  Interface doesn't have a list of Peers/Access-Points\n\n", ifname);
        iw_sockets_close(skfd);
        return (-1);
    }

    qual = (struct iw_quality *)(buffer + (sizeof(struct sockaddr) * wrq.u.data.length));
    if (wrq.u.data.flags)
    { /* has qual */
        if (iw_get_range_info(skfd, ifname, &(range)) >= 0)
        {
            *rssi = qual->qual - range.max_qual.qual;
        }
        else
        {
            *rssi = qual->qual - 100;
        }
    }

    iw_sockets_close(skfd);
    return 0;
}

#endif

#endif
#endif

int tkl_net_essid_on(char *ifname)
{
    int skfd;
    int ret;
    struct wireless_config conf;

    if ((skfd = iw_sockets_open()) < 0)
    {
        perror("socket fail\n");
        return -1;
    }

    ret = iw_get_basic_config(skfd, ifname, &conf);
    if (ret < 0)
    {
        fprintf(stderr, "%-8.16s  Interface doesn't support iw_get_basic_config.\n\n", ifname);
        iw_sockets_close(skfd);
        return (-1);
    }
    iw_sockets_close(skfd);

    if (conf.essid_on && conf.has_essid == 1)
    {
        ret = 1;
    }

    return ret;
}

WIFI_TYPE_E tkl_net_get_wifi_type(void)
{
    FILE *pp = NULL;
    char *opter = NULL;
    char str_tmp[1024] = {0x0};
    char *path = NULL;

    if (0 == access("/proc/net/rtl8188fu", F_OK))
    {
        return WIFI_8188;
    }
    else if (0 == access("/proc/net/rtl8189fs", F_OK))
    {
        return WIFI_8192;
    }
    else if (0 == access("/proc/net/rtl8723ds", F_OK))
    {
        return WIFI_8723DS;
    }

    path = "/proc/cpuinfo";
    pp = fopen(path, "r");
    if (!pp)
    {
        perror("popen failed");
        return -1;
    }
    while (fgets(str_tmp, sizeof(str_tmp), pp) != NULL)
    {
        opter = strstr(str_tmp, "RTL8197F");
        if (NULL == opter)
        {
            continue;
        }
        else
        {
            pclose(pp);
            return WIFI_RTL8197;
        }
    }
    pclose(pp);

    TKL_LOGE("WIFI chip type doesn't support");

    return -1;
}

#ifdef NL80211
int tkl_net_get_connected_ap_info(char *ssid, char *passwd)
{
    int iret = 0;
    WF_CONNECTED_AP_INFO_S ap_info_s;
    memset(&ap_info_s, 0, sizeof(WF_CONNECTED_AP_INFO_S));

    iret = wifi_get_ap_info(&ap_info_s);
    if (iret < 0)
    {
        TKL_LOGE("WiFi : Get conected ap info faied");
        return -1;
    }
    else
    {
        strcpy(ssid, (char *)ap_info_s.ssid);
        strcpy(passwd, (char *)ap_info_s.passwd);
    }

    if (strlen((char *)ap_info_s.ssid) == 0 || strlen((char *)ap_info_s.passwd) == 0)
    {
        iret = get_passwd_and_ssid_from_wpa_file(ssid, passwd);
        if (iret < 0)
        {
            TKL_LOGE("get ssid & passwd failed\n");
            return -1;
        }
    }

    return 0;
}
#else
int tkl_net_get_connected_ap_info(char *ssid, char *passwd)
{
    return 0;
}
#endif

#ifdef NL80211
int tkl_net_get_bssid(unsigned char *bssid)
{
    int iret;
    iret = wifi_get_current_connect_ap_bssid((char *)bssid);
    if (iret < 0)
    {
        // TKL_LOGE("wifi get bssid failed");
        return -1;
    }

    return 0;
}

#else
int tkl_net_get_bssid(unsigned char *bssid)
{
    int skfd;
    char *ifname = WLAN_DEV;
    struct iwreq wrq;
    const struct ether_addr *ether_wap;

    if ((skfd = iw_sockets_open()) < 0)
    {
        perror("socket");
        return -1;
    }

    if (iw_get_ext(skfd, ifname, SIOCGIWAP, &wrq) < 0)
    {
        TKL_LOGD("get AP MAC failed");
        iw_sockets_close(skfd);
        return -1;
    }

    ether_wap = (const struct ether_addr *)wrq.u.ap_addr.sa_data;
    memcpy(bssid, ether_wap->ether_addr_octet, ETH_ALEN);

    iw_sockets_close(skfd);
    return 0;
}
#endif

#ifdef NL80211
int tkl_net_start_ap_connect(const char *ssid, const char *passwd)
{
    int iret = 0;
    struct wifi_network Net_wk;

    memset(&Net_wk, 0, sizeof(struct wifi_network));
    strcpy(Net_wk.ssid, ssid);
    if (passwd == NULL || 0 == strlen(passwd))
    {
        Net_wk.key_mgmt = WIFI_KEY_NONE;
    }
    else
    {
        strcpy(Net_wk.psk, passwd);
        Net_wk.key_mgmt = WIFI_KEY_WPA2PSK; // default key_mgnt
    }

    iret = process_alive(WPA_SUPPLICANT); /* �ж�wpa_supplicant�����Ƿ������� */
    if (iret == 0)
    { /* wpa_supplicant ����û������,�ֶ���wpa_supplicant���̽������� */
        if (wpa_config_prepare(ssid, passwd))
        {
            TKL_LOGE("WiFi : configure wpa_supplicant config file failed");
            return -1;
        }
        if (start_wpa() < 0)
        { /* ����wpa_supplicant */
            TKL_LOGE("failed to start wap_supplicant");
            return -1;
        }
        usleep(1500);
#ifdef OWN_UDHCPC
        start_udhcpc();
        usleep(1500);
        start_udhcpc6();
#endif
    }
    else if (iret == 1)
    {                                      /* wpa_supplicant ������,�������صĽӿڽ������� */
        iret = wifi_join_network(&Net_wk); /* connect to network */
        if (iret < 0)
        {
            TKL_LOGE("connect network:%s failed", ssid);
            return -1;
        }
#ifdef OWN_UDHCPC
        if (process_alive(UDHCPC) == 0)
        { /* ����ʱ�ж�udhcpc�Ƿ�������δ���������ֶ����� */
            usleep(500);
            start_udhcpc();
        }

        if (process_alive(UDHCPC6) == 0)
        { /* ����ʱ�ж�udhcpc�Ƿ�������δ���������ֶ����� */
            usleep(500);
            start_udhcpc6();
        }
#endif
    }
    else
    {
        TKL_LOGE("wpa_suppplicant error");
        return -1;
    }

    return 0;
}

#else
int tkl_net_start_ap_connect(const char *ssid, const char *passwd)
{
    int iret = 0;
    char *ifname = WLAN_DEV;
    char cmd[128] = {0x0};
    char *ip = "0.0.0.0";

    iret = tkl_net_set_mode(ifname, "Managed"); /* ȷ���豸������station ģʽ�� */
    if (iret < 0)
    {
        TKL_LOGE("WIIF set mode failed");
        return -1;
    }
    process_kill(ifname, UDHCPC);

    /* ��wlan0 IP */
    iret = tkl_net_set_ip(ifname, ip);
    if (iret < 0)
    {
        tkl_net_set_ip(ifname, ip); /* ʧ��������һ�� */
    }

    snprintf(cmd, sizeof(cmd), "/sbin/ifconfig %s down", ifname);
    sys_cmd_run(cmd, NULL, NULL);
    snprintf(cmd, sizeof(cmd), "/sbin/iwpriv %s set_mib opmode=8", ifname);
    sys_cmd_run(cmd, NULL, NULL);
    snprintf(cmd, sizeof(cmd), "/sbin/iwpriv %s set_mib band=11", ifname);
    sys_cmd_run(cmd, NULL, NULL);
    snprintf(cmd, sizeof(cmd), "/sbin/iwpriv %s set_mib deny_legacy=0", ifname);
    sys_cmd_run(cmd, NULL, NULL);
    snprintf(cmd, sizeof(cmd), "/sbin/iwpriv %s set_mib use40M=0", ifname);
    sys_cmd_run(cmd, NULL, NULL);
    snprintf(cmd, sizeof(cmd), "/sbin/iwpriv %s set_mib ssid=%s", ifname, ssid);
    sys_cmd_run(cmd, NULL, NULL);
    snprintf(cmd, sizeof(cmd), "/sbin/iwpriv %s set_mib 802_1x=0", ifname);
    sys_cmd_run(cmd, NULL, NULL);

    if (NULL == passwd || strlen(passwd) == 0)
    {
        TKL_LOGD("************CONNECT AP NO PASSWORD**************");
        snprintf(cmd, sizeof(cmd), "/sbin/iwpriv %s set_mib authtype=0", ifname);
        sys_cmd_run(cmd, NULL, NULL);
        snprintf(cmd, sizeof(cmd), "/sbin/iwpriv %s set_mib encmode=0", ifname);
        sys_cmd_run(cmd, NULL, NULL);
        snprintf(cmd, sizeof(cmd), "/sbin/iwpriv %s set_mib psk_enable=0", ifname);
        sys_cmd_run(cmd, NULL, NULL);
        snprintf(cmd, sizeof(cmd), "/sbin/ifconfig %s up", ifname);
        sys_cmd_run(cmd, NULL, NULL);
    }
    else
    {
        TKL_LOGD("**************CONNECT AP WITH PASSWORD************");
        snprintf(cmd, sizeof(cmd), "/sbin/iwpriv %s set_mib authtype=2", ifname);
        sys_cmd_run(cmd, NULL, NULL);
        snprintf(cmd, sizeof(cmd), "/sbin/iwpriv %s set_mib psk_enable=3", ifname);
        sys_cmd_run(cmd, NULL, NULL);
        snprintf(cmd, sizeof(cmd), "/sbin/iwpriv %s set_mib encmode=2", ifname);
        sys_cmd_run(cmd, NULL, NULL);
        snprintf(cmd, sizeof(cmd), "/sbin/iwpriv %s set_mib wpa2_cipher=10", ifname);
        sys_cmd_run(cmd, NULL, NULL);
        snprintf(cmd, sizeof(cmd), "/sbin/iwpriv %s set_mib wpa_cipher=10", ifname);
        sys_cmd_run(cmd, NULL, NULL);
        snprintf(cmd, sizeof(cmd), "/sbin/iwpriv %s set_mib passphrase=%s", ifname, passwd);
        sys_cmd_run(cmd, NULL, NULL);
        snprintf(cmd, sizeof(cmd), "/sbin/ifconfig %s up", ifname);
        sys_cmd_run(cmd, NULL, NULL);
    }

#if 0
    iret = tkl_net_wifi_enable(ifname,1);
    if(iret < 0){
        TKL_LOGE("enable wifi again");
        snprintf(cmd,sizeof(cmd),"/sbin/ifconfig %s up",ifname);
        sys_cmd_run( cmd, NULL, NULL );
    }

    tkl_net_8197_start_dhcp(ifname);
    iret = process_alive(UDHCPC);       /* ���udhcpc����ʧ����������һ�� */
    if(iret == 0) {
        tkl_net_8197_start_dhcp(ifname);
    }
#endif

    return 0;
}
#endif

#ifdef NL80211
int tkl_net_sta_disconnect(char *ifname)
{
    int iret = 0;

    iret = wifi_disconnect_network();
    if (iret < 0)
    {
        TKL_LOGE("station disconnect failed");
        return -1;
    }

    // ???? need  when change to ap or monitor wpa_supplicant still exist?
#ifdef OWN_UDHCPC
    process_kill(ifname, UDHCPC);
    process_kill(ifname, UDHCPC6);
#endif
    process_kill(ifname, WPA_SUPPLICANT);
    return 0;
}

#else
int tkl_net_sta_disconnect(char *ifname)
{
    char buf[64] = {0x0};
    process_kill(ifname, UDHCPC);
    snprintf(buf, sizeof(buf), "/sbin/ifconfig %s down", ifname);
    sys_cmd_run(buf, NULL, NULL);

    TKL_LOGD("%s", buf);

    tkl_net_set_mode(ifname, "Master");

    return 0;
}
#endif

#ifdef NL80211
int tkl_net_set_country_rtl8188(char *country)
{
    char *cpath = "/proc/net/rtl8188fu/wlan0/country_code";
    FILE *fp = NULL;

    if (0 != access(cpath, F_OK) || !country || strlen(country) == 0)
    {
        return -1;
    }

    fp = fopen(cpath, "r+");
    if (fp == NULL)
    {
        TKL_LOGE("WiFi : can't open %s", cpath);
        return -1;
    }

    if (fwrite(country, sizeof(country), 1, fp) != 1)
    {
        TKL_LOGE("WiFi : write error %s", cpath);
        fclose(fp);
        return -1;
    }

    fclose(fp);
    TKL_LOGD("country: %s", country);
    return 0;
}

int tkl_net_set_country(char *country)
{
    int iret = 0;

    iret = wifi_set_country(country);
    if (iret < 0)
    {
        return -1;
    }

    return 0;
}

#else
int tkl_net_set_country(char *country)
{
    char *ifname = WLAN_DEV;
    char cmd[128] = {0x0};
    WIFI_COUNTRY_E cc_code = COUNTRY_NR;

    if (strcmp("CN", country) == 0)
    {
        cc_code = CN;
    }
    else if (strcmp("US", country) == 0)
    {
        cc_code = US;
    }
    else if (strcmp("JP", country) == 0)
    {
        cc_code = JP;
    }
    else if (strcmp("EU", country) == 0)
    {
        cc_code = EU;
    }

    snprintf(cmd, sizeof(cmd), "/sbin/iwpriv %s set_mib regdomain=%d", ifname, g_c_country[cc_code].regd);
    sys_cmd_run(cmd, NULL, NULL);

    return 0;
}
#endif

#ifdef NL80211
int tkl_net_ap_start(const WF_AP_CFG_IF_S *cfg)
{
    int iret = 0;
    char *ifname = WLAN_DEV;
    char cmd[128] = {0x0};
    char ipaddr_pre[13] = {0x0};
    UDHCPD_IP_T udhcpd_ip_t = {{0}};

    tkl_net_ap_stop(); /* stop udhcpd ,wlan down*/

    iret = tkl_net_hostapd_conf_prepare(cfg);
    if (iret < 0)
    {
        TKL_LOGE("WIFI prepare hostapd.conf failed");
        return -1;
    }
    snprintf(cmd, sizeof(cmd), "/usr/sbin/hostapd %s -P /tmp/hostapd-%s.pid -dd -B", WIFI_HOSTAPD_CONF, ifname);
    sys_cmd_run(cmd, NULL, NULL);

    sleep(3);

    iret = tkl_net_set_ip(ifname, cfg->ip.addr.ip4.ip);
    if (iret < 0)
    {
        TKL_LOGE("WIFI set ip failed");
        return -1;
    }

    sys_cmd_run(CREATE_UDHCPD_DIR, NULL, NULL);
    sys_cmd_run(CREATE_UDHCPD_CFG_FILE, NULL, NULL);

    if (ip_prepare((char *)cfg->ip.addr.ip4.ip, ipaddr_pre))
    {
        TKL_LOGE("ip_pepare fail\n");
        return -1;
    }

    snprintf(udhcpd_ip_t.ip_start, sizeof(udhcpd_ip_t.ip_start), "%s%d", ipaddr_pre, 2);
    snprintf(udhcpd_ip_t.ip_end, sizeof(udhcpd_ip_t.ip_end), "%s%d", ipaddr_pre, 254);

    strcpy(udhcpd_ip_t.ifname, ifname);
    strcpy(udhcpd_ip_t.ip_route, cfg->ip.addr.ip4.ip);

    iret = udhcpd_conf_prepare(udhcpd_ip_t);
    if (iret < 0)
    {
        TKL_LOGE("udhcpd_conf_prepare\n");
        sys_cmd_run("/usr/bin/killall -9 hostapd", NULL, NULL);
        return -1;
    }

    memset(cmd, 0, sizeof(cmd));
    snprintf(cmd, sizeof(cmd), "/usr/sbin/udhcpd %s ", WIFI_DHCPD_CONF);
    sys_cmd_run(cmd, NULL, NULL);

    return 0;
}

#else /* for 8197 */
/*
 * 8197 softap ������ز���˵��
 *   authtype;           0->opsystem; 1->shared key; 2->auto
 *   psk_enable;         0->disable; 1->WPA; 2->WPA2; 3->WPA/WPA2 mixed
 *   int encmode;        0->disabled; 1->WEP64; 2->TKIP; 4->AES(CCMP); 5->WEP128
 *   wpa2_cipher;        2->TKIP; 8->AES(CCMP); 10->TKIP/AES mixed
 *   int wpa_cipher;     2->TKIP; 8->AES(CCMP); 10->TKIP/AES mixed
 *   int w802_1x;        0->disable; 1->enable
 */
int tkl_net_ap_start(const WF_AP_CFG_IF_S *cfg)
{
#if 0
    int iret = 0;
    char *ifname = WLAN_DEV;    /* default wlan0 */
    char cmd[128] = {0x0};
    char ipaddr_pre[20] = {0x0};
    char *mode = "master";
    UDHCPD_IP_T udhcpd_ip_t;
    memset( &udhcpd_ip_t, 0x0, sizeof( UDHCPD_IP_T ) );

    tkl_net_ap_stop();              /* stop udhcpd ,wlan down*/

    /* -------------------start a softap------------------------ */
    snprintf(cmd,sizeof(cmd),"/sbin/ifconfig %s down",ifname);
    sys_cmd_run( cmd, NULL, NULL );
    snprintf(cmd,sizeof(cmd),"/sbin/iwconfig %s mode %s",ifname,mode);
    sys_cmd_run( cmd, NULL, NULL );

    snprintf(cmd,sizeof(cmd),"/sbin/iwpriv %s set_mib %s",ifname,"opmode=16");
    sys_cmd_run( cmd, NULL, NULL );
    snprintf(cmd,sizeof(cmd),"/sbin/iwpriv %s set_mib band=11",ifname);
    sys_cmd_run( cmd, NULL, NULL );
    snprintf(cmd,sizeof(cmd),"/sbin/iwpriv %s set_mib %s",ifname,"deny_legacy=0");
    sys_cmd_run( cmd, NULL, NULL );
    snprintf(cmd,sizeof(cmd),"/sbin/iwpriv %s set_mib %s",ifname,"802_1x=0");
    sys_cmd_run( cmd, NULL, NULL );
    snprintf(cmd,sizeof(cmd),"/sbin/iwpriv %s set_mib %s%d",ifname,"channel=",cfg->chan);
    sys_cmd_run( cmd, NULL, NULL );
    snprintf(cmd,sizeof(cmd),"/sbin/iwpriv %s set_mib ssid=%s",ifname,cfg->ssid);
    sys_cmd_run( cmd, NULL, NULL );

    switch(cfg->md) {
    case WAAM_OPEN:
        {
            snprintf(cmd,sizeof(cmd),"/sbin/iwpriv %s set_mib %s",ifname,"authtype=0");
            sys_cmd_run( cmd, NULL, NULL );
            snprintf(cmd,sizeof(cmd),"/sbin/iwpriv %s set_mib %s",ifname,"encmode=0");
            sys_cmd_run( cmd, NULL, NULL );
        }
        break;
    case WAAM_WEP:
        {
            snprintf(cmd,sizeof(cmd),"/sbin/iwpriv %s set_mib %s",ifname,"authtype=2");
            sys_cmd_run( cmd, NULL, NULL );
            snprintf(cmd,sizeof(cmd),"/sbin/iwpriv %s set_mib %s",ifname,"encmode=5");
            sys_cmd_run( cmd, NULL, NULL );
        }
        break;
    case WAAM_WPA_PSK:
        {
            snprintf(cmd,sizeof(cmd),"/sbin/iwpriv %s set_mib %s",ifname,"802_1x=0");
            sys_cmd_run( cmd, NULL, NULL );
            snprintf(cmd,sizeof(cmd),"/sbin/iwpriv %s set_mib %s",ifname,"authtype=2");
            sys_cmd_run( cmd, NULL, NULL );
            snprintf(cmd,sizeof(cmd),"/sbin/iwpriv %s set_mib %s",ifname,"psk_enable=3");
            sys_cmd_run( cmd, NULL, NULL );
            snprintf(cmd,sizeof(cmd),"/sbin/iwpriv %s set_mib %s",ifname,"encmod=2");
            sys_cmd_run( cmd, NULL, NULL );
            snprintf(cmd,sizeof(cmd),"/sbin/iwpriv %s set_mib %s",ifname,"wpa_cipher=10");
            sys_cmd_run( cmd, NULL, NULL );
            snprintf(cmd,sizeof(cmd),"/sbin/iwpriv %s set_mib %s",ifname,"wpa2_cipher=10");
            sys_cmd_run( cmd, NULL, NULL );
            snprintf(cmd,sizeof(cmd),"/sbin/iwpriv %s set_mib passphrase=%s",ifname,cfg->passwd);
            sys_cmd_run( cmd, NULL, NULL );
        }
        break;
    case WAAM_WPA2_PSK:
        {
            snprintf(cmd,sizeof(cmd),"/sbin/iwpriv %s set_mib %s",ifname,"authtype=2");
            sys_cmd_run( cmd, NULL, NULL );
            snprintf(cmd,sizeof(cmd),"/sbin/iwpriv %s set_mib %s",ifname,"encmod=2");
            sys_cmd_run( cmd, NULL, NULL );
            snprintf(cmd,sizeof(cmd),"/sbin/iwpriv %s set_mib %s",ifname,"802_1x=0");
            sys_cmd_run( cmd, NULL, NULL );
            snprintf(cmd,sizeof(cmd),"/sbin/iwpriv %s set_mib %s",ifname,"psk_enable=2");
            sys_cmd_run( cmd, NULL, NULL );
            snprintf(cmd,sizeof(cmd),"/sbin/iwpriv %s set_mib %s",ifname,"wpa_cipher=2");
            sys_cmd_run( cmd, NULL, NULL );
            snprintf(cmd,sizeof(cmd),"/sbin/iwpriv %s set_mib passphrase=%s",ifname,cfg->passwd);
            sys_cmd_run( cmd, NULL, NULL );
        }
        break;
    case WAAM_WPA_WPA2_PSK:
        {
            snprintf(cmd,sizeof(cmd),"/sbin/iwpriv %s set_mib %s",ifname,"authtype=2");
            sys_cmd_run( cmd, NULL, NULL );
            snprintf(cmd,sizeof(cmd),"/sbin/iwpriv %s set_mib %s",ifname,"encmod=2");
            sys_cmd_run( cmd, NULL, NULL );
            snprintf(cmd,sizeof(cmd),"/sbin/iwpriv %s set_mib %s",ifname,"802_1x=0");
            sys_cmd_run( cmd, NULL, NULL );
            snprintf(cmd,sizeof(cmd),"/sbin/iwpriv %s set_mib %s",ifname,"psk_enable=3");
            sys_cmd_run( cmd, NULL, NULL );
            snprintf(cmd,sizeof(cmd),"/sbin/iwpriv %s set_mib %s",ifname,"wpa_cipher=10");
            sys_cmd_run( cmd, NULL, NULL );
            snprintf(cmd,sizeof(cmd),"/sbin/iwpriv %s set_mib %s",ifname,"wpa2_cipher=10");
            sys_cmd_run( cmd, NULL, NULL );
            snprintf(cmd,sizeof(cmd),"/sbin/iwpriv %s set_mib passphrase=%s",ifname,cfg->passwd);
            sys_cmd_run( cmd, NULL, NULL );
        }
        break;
    case WAAM_WPA_WPA3_SAE:
        {
            snprintf(cmd,sizeof(cmd),"/sbin/iwpriv %s set_mib %s",ifname,"encmod=2");
            sys_cmd_run( cmd, NULL, NULL );
            snprintf(cmd,sizeof(cmd),"/sbin/iwpriv %s set_mib %s",ifname,"802_1x=0");
            sys_cmd_run( cmd, NULL, NULL );
            snprintf(cmd,sizeof(cmd),"/sbin/iwpriv %s set_mib %s",ifname,"psk_enable=3");
            sys_cmd_run( cmd, NULL, NULL );
            snprintf(cmd,sizeof(cmd),"/sbin/iwpriv %s set_mib %s",ifname,"wpa_cipher=8");
            sys_cmd_run( cmd, NULL, NULL );
            snprintf(cmd,sizeof(cmd),"/sbin/iwpriv %s set_mib %s",ifname,"wpa2_cipher=8");
            sys_cmd_run( cmd, NULL, NULL );
            snprintf(cmd,sizeof(cmd),"/sbin/iwpriv %s set_mib passphrase=%s",ifname,cfg->passwd);
            sys_cmd_run( cmd, NULL, NULL );
        }
        break;
    case WAAM_UNKNOWN:
    default:
        TKL_LOGE("unknow encryption type when start a soft ap");
        return -1;
    }

    /* wlan0 up */
    if(tkl_net_wifi_enable(ifname,1)) {
        TKL_LOGE("WiFi : start up %s failed",ifname);
        return -1;
    }

    snprintf(cmd,sizeof(cmd),"/sbin/ifconfig %s %s",ifname,cfg->ip.ip);
    sys_cmd_run( cmd, NULL, NULL );

    if(ip_prepare((char *)cfg->ip.ip,( char* )ipaddr_pre)) {
        return -1;
    }
    snprintf(udhcpd_ip_t.ip_start,sizeof(udhcpd_ip_t.ip_start), "%s%d",ipaddr_pre,100);
    snprintf(udhcpd_ip_t.ip_end,sizeof(udhcpd_ip_t.ip_end), "%s%d",ipaddr_pre,200);

    strcpy(udhcpd_ip_t.ifname,ifname);
    strcpy(udhcpd_ip_t.ip_route,cfg->ip.ip);
    strcpy(udhcpd_ip_t.ip_mask,cfg->ip.mask);
    strcpy(udhcpd_ip_t.ip_gw,cfg->ip.gw);

    iret = udhcpd_conf_prepare(udhcpd_ip_t);
    if(iret < 0) {
        TKL_LOGE("WiFi : start udhcpd failed");
        return -1;
    }

    snprintf(cmd,sizeof(cmd),"/bin/udhcpd %s ",WIFI_DHCPD_CONF);
    sys_cmd_run( cmd, NULL, NULL );
#endif
    return 0;
}
#endif

#ifdef NL80211
int tkl_net_ap_stop(void)
{

    int iret = 0;
    char *ifname = WLAN_DEV; /* default wlan0 */
    char *process1 = "hostapd";
    char *process2 = "udhcpd";
    char cmd[128] = {0x0};

    // system("killall -9 hostapd");
    process_kill(ifname, process1);
    // system("killall -9 udhcpd");
    snprintf(cmd, sizeof(cmd), "/usr/bin/killall %s", process2);
    sys_cmd_run(cmd, NULL, NULL);
    // iret = tkl_net_wifi_enable(ifname, 0);  /* wlan0 down*/  //ȥ���ر�����,����IOT����˳��release_ap ->tkl_wifi_station_connect->tkl_wifi_stop_ap���˴��ر����ڻᵼ���޷�����������
    // if(iret < 0){
    //     return -1;
    // }
    return 0;
}

#else
int tkl_net_ap_stop(void)
{
    // int iret = 0;
    // char *ifname = WLAN_DEV;    /* default wlan0 */
    char *process1 = "udhcpd";
    char cmd[128] = {0x0};

    snprintf(cmd, sizeof(cmd), "/usr/bin/killall %s", process1);
    sys_cmd_run(cmd, NULL, NULL);
    // iret = tkl_net_wifi_enable(ifname, 0);  /* wlan0 down*/
    // if(iret < 0){
    //     return -1;
    // }

    return 0;
}
#endif

int tkl_net_hostapd_conf_prepare(const WF_AP_CFG_IF_S *cfg)
{
    int fd = 0;
    char buf[1024] = {0};
    int iret = 0;
    char *ifname = WLAN_DEV;
    int maximum = 3;     /* max sta connect nums default:3 */
    int interval = 100;  /* beacon intervall default:100 */
    int hidden_ssid = 0; /* default not hidden ssid */

    if (cfg->ssid_hidden)
    {
        hidden_ssid = cfg->ssid_hidden;
    }

    if (cfg->max_conn != 0)
    {
        maximum = cfg->max_conn;
    }

    if (cfg->ms_interval != 0)
    {
        interval = cfg->ms_interval;
    }

    if (cfg->md == WAAM_OPEN)
    {
        sprintf(buf, "interface=%s\n"
                     "ctrl_interface_group=0\n"
                     "#driver=nl80211\n"
                     "ieee80211n=1\n"
                     "ssid=%s\n"
                     "hw_mode=g\n"
                     "channel=%d\n"
                     "max_num_sta=%d\n"
                     "macaddr_acl=0\n"
                     "#auth_algs=0\n"
                     "beacon_int=%d\n"
                     "ignore_broadcast_ssid=%d\n",
                ifname, cfg->ssid, cfg->chan, maximum, interval, hidden_ssid);
    }
    else if (cfg->md == WAAM_WEP)
    { /* ������ʹ�ã���ȫ��̫�� */
        sprintf(buf, "interface=%s\n"
                     "ctrl_interface_group=0\n"
                     "#driver=nl80211\n"
                     "ieee80211n=1\n"
                     "ssid=%s\nhw_mode=g\n"
                     "channel=%d\n"
                     "max_num_sta=%d\n"
                     "macaddr_acl=0\n"
                     "#auth_algs=1\n"
                     "ignore_broadcast_ssid=%d\n"
                     "wep_default_key=0\n"
                     "wep_key0=1234567890\n"
                     "wep_key1=\"vwxyz\"\n"
                     "wep_key2=0102030405060708090a0b0c0d\n"
                     "wep_key3=\".2.4.6.8.0.23\"\n"
                     "wep_key_len_broadcast=13\n"
                     "wep_key_len_unicast=13\n"
                     "wep_rekey_period=300",
                ifname, cfg->ssid, cfg->chan, maximum, hidden_ssid);
    }
    else if (cfg->md == WAAM_WPA_PSK)
    {
        sprintf(buf, "interface=%s\n"
                     "ctrl_interface_group=0\n"
                     "#driver=nl80211\n"
                     "ieee80211n=1\n"
                     "ssid=%s\n"
                     "hw_mode=g\n"
                     "channel=%d\n"
                     "max_num_sta=%d\n"
                     "macaddr_acl=0\n"
                     "auth_algs=1\n"
                     "ignore_broadcast_ssid=%d\n"
                     "wpa=1\n"
                     "wpa_passphrase=%s\n"
                     "wpa_key_mgmt=WPA-PSK\n"
                     "#wpa_pairwise=TKIP CCMP\n"
                     "rsn_pairwise=TKIP CCMP\n",
                ifname, cfg->ssid, cfg->chan, maximum, hidden_ssid, cfg->passwd);
    }
    else if (cfg->md == WAAM_WPA2_PSK || cfg->md == WAAM_WPA_WPA2_PSK)
    {
        sprintf(buf, "interface=%s\n"
                     "ctrl_interface_group=0\n"
                     "#driver=nl80211\n"
                     "ieee80211n=1\n"
                     "ssid=%s\n"
                     "hw_mode=g\n"
                     "channel=%d\n"
                     "max_num_sta=%d\n"
                     "macaddr_acl=0\n"
                     "auth_algs=1\n"
                     "ignore_broadcast_ssid=%d\n"
                     "wpa=3\n"
                     "wpa_passphrase=%s\n"
                     "wpa_key_mgmt=WPA-PSK\n"
                     "wpa_pairwise=TKIP CCMP\n"
                     "rsn_pairwise=TKIP CCMP\n",
                ifname, cfg->ssid, cfg->chan, maximum, hidden_ssid, cfg->passwd);
    }
    else if (cfg->md == WAAM_WPA_WPA3_SAE)
    { /* �ݲ�֧�� */
        TKL_LOGD("WIFI doesn't support WPA3");
    }
    else
    {
        TKL_LOGE("Wrong key_mgnt");
        return -1;
    }

    unlink(WIFI_HOSTAPD_CONF);
    fd = open(WIFI_HOSTAPD_CONF, O_RDWR | O_CREAT, 0x666);
    if (fd < 0)
    {
        TKL_LOGE("create configure error %d\n", errno);
        return -1;
    }
    iret = write(fd, buf, strlen(buf));
    if (iret < 0)
    {
        TKL_LOGE("write hostapd.conf failed");
        close(fd);
        return -1;
    }
    close(fd);

    return 0;
}

void tkl_net_8197_start_dhcp(char *ifname)
{
    char cmd[128] = {0x0};

    /* ȷ��udhcpc��kill���� */
    process_kill(ifname, UDHCPC);

    /* ��������udhcpc */
    snprintf(cmd, sizeof(cmd), "/bin/udhcpc -b -p /tmp/udhcpc-%s.pid -i %s -s %s", ifname, ifname, UDHCPC_SCRIPT_PATH);
    sys_cmd_run(cmd, NULL, NULL);
}
#ifndef NL80211
int tkl_net_set_sniffer_mode(int index)
{
    char *ifname = WLAN_DEV;
    int dl = 0;
    int status = 0;
    char errbuf[PCAP_ERRBUF_SIZE] = {0x00};

    if (index)
        ifname = WLAN_MONITOR;

    device = pcap_create(ifname, errbuf);
    if (!device)
    {
        TKL_LOGE("pcap_create failed\n");
        return -1;
    }

    if (pcap_set_rfmon(device, 1) == 0)
        TKL_LOGI("monitor mode enabled\n");

    pcap_set_snaplen(device, 2048);
    pcap_set_promisc(device, 1);

    status = pcap_activate(device);

    if (status < 0)
        TKL_LOGD("WLAN status = %d\n", status);

    dl = pcap_datalink(device);

    TKL_LOGD("The Data Link Type = %s\n", pcap_datalink_val_to_name(dl));

    return 0;
}
#endif

int udhcpd_conf_prepare(UDHCPD_IP_T udhcpd_ip_t)
{
    int fd = 0;
    char buf[256] = {0x0};
    int leases = 28800; /* default value*/
    int ret = 0;

    unlink("udhcpd.conf");
    fd = open(WIFI_DHCPD_CONF, O_RDWR | O_CREAT, 0x666);
    if (fd < 0)
    {
        TKL_LOGE("create configure error %d\n", errno);
        return -1;
    }

    snprintf(buf, sizeof(buf), "interface\t%s\nstart\t%s\nend\t%s\nopt subnet\t%s\nopt lease %d\nopt router\t%s\nopt dns %s\nopt domain Realtek\n",
             udhcpd_ip_t.ifname, udhcpd_ip_t.ip_start, udhcpd_ip_t.ip_end, udhcpd_ip_t.ip_mask, leases, udhcpd_ip_t.ip_route, udhcpd_ip_t.ip_route);

    ret = write(fd, buf, strlen(buf) + 1);
    if (ret < 0)
    {
        TKL_LOGE("write hostapd buf errno %d\n", errno);
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

int tkl_net_create_rawsocket(int protocol_to_sniff)
{
    int raw_sock;

    if ((raw_sock = socket(PF_PACKET, SOCK_RAW, htons(protocol_to_sniff))) == -1)
        perror("Error creating raw socket: ");

    return raw_sock;
}

int tkl_net_bind_rawsocket(char *device, int rawsock, int protocol)
{
    struct sockaddr_ll sll;
    struct ifreq ifr;
    int ret;

    bzero(&sll, sizeof(struct sockaddr_ll));
    bzero(&ifr, sizeof(struct ifreq));

    /* get interface index */
    strncpy((char *)ifr.ifr_name, device, IFNAMSIZ);
    if ((ret = ioctl(rawsock, SIOCGIFINDEX, &ifr)) < 0)
    {
        perror("Error get interface index: ");
        goto out;
    }

    /* bind to interface */
    sll.sll_family = AF_PACKET;
    sll.sll_ifindex = ifr.ifr_ifindex;
    sll.sll_protocol = htons(protocol);
    if ((ret = bind(rawsock, (struct sockaddr *)&sll, sizeof(sll))) == -1)
    {
        perror("Error binding raw socket to interface: ");
        goto out;
    }

out:
    return ret;
}

int tkl_net_send_rawpacket(int rawsock, unsigned char *pkt, size_t pkt_len)
{
    size_t pkt_sent = 0;

    if ((pkt_sent = write(rawsock, pkt, pkt_len)) != pkt_len)
    {
        perror("Eror sending raw socket data: ");
    }

    return pkt_sent;
}

int tkl_net_recv_rawpacket(int rawsock, unsigned char *pkt, int pkt_len)
{
    size_t recv = 0;

    recv = read(rawsock, pkt, pkt_len);

    return recv;
}
