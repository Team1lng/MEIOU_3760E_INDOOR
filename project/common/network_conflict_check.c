#include "network_conflict_check.h"

#include <arpa/inet.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

#include "Amy_lvgl.h"
#include "sat_user_common.h"
#include "sat_wifi.h"

#define NETWORK_CONFLICT_WIRED_IFACE "br0"
#define NETWORK_CONFLICT_WIFI_IFACE "wlan0"
#define NETWORK_CONFLICT_CHECK_INTERVAL_MS 30000

static bool network_conflict_ipv4_read(const char *iface, uint32_t *ip, uint32_t *mask)
{
    char ip_str[32] = {0};
    char mask_str[32] = {0};
    struct in_addr ip_addr;
    struct in_addr mask_addr;

    if (!sat_ip_mac_addres_get(iface, ip_str, NULL, mask_str))
        return false;
    if (inet_aton(ip_str, &ip_addr) == 0 || inet_aton(mask_str, &mask_addr) == 0)
        return false;
    if (ip_addr.s_addr == 0 || mask_addr.s_addr == 0)
        return false;

    *ip = ntohl(ip_addr.s_addr);
    *mask = ntohl(mask_addr.s_addr);
    return true;
}

static bool network_conflict_is_same_subnet(uint32_t wired_ip, uint32_t wired_mask, uint32_t wifi_ip, uint32_t wifi_mask)
{
    uint32_t common_mask = wired_mask & wifi_mask;

    if (common_mask == 0)
        return false;

    return (wired_ip & common_mask) == (wifi_ip & common_mask);
}

static void *network_conflict_check_thread(void *arg)
{
    (void)arg;
    pthread_detach(pthread_self());

    while (1)
    {
        bool wifi_connected = false;
        wifi_device_connection_status(wifi_context_get(), NULL, NULL, NULL, &wifi_connected, NULL);
        if (!wifi_connected)
        {
            usleep(NETWORK_CONFLICT_CHECK_INTERVAL_MS * 1000);
            continue;
        }

        uint32_t wired_ip = 0;
        uint32_t wired_mask = 0;
        uint32_t wifi_ip = 0;
        uint32_t wifi_mask = 0;
        bool conflicted = false;

        if (network_conflict_ipv4_read(NETWORK_CONFLICT_WIRED_IFACE, &wired_ip, &wired_mask) &&
            network_conflict_ipv4_read(NETWORK_CONFLICT_WIFI_IFACE, &wifi_ip, &wifi_mask))
        {
            conflicted = network_conflict_is_same_subnet(wired_ip, wired_mask, wifi_ip, wifi_mask);
        }

        if (conflicted)
            Amy_network_conflict_event_put();

        usleep(NETWORK_CONFLICT_CHECK_INTERVAL_MS * 1000);
    }

    return NULL;
}

void network_conflict_check_start(void)
{
    static bool s_once;
    pthread_t tid;

    if (s_once)
        return;
    s_once = true;

    if (pthread_create(&tid, sat_pthread_attr_get(), network_conflict_check_thread, NULL) != 0)
    {
        SAT_ERROR("network conflict check pthread_create failed");
        s_once = false;
    }
}
