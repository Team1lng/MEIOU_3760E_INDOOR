#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <linux/route.h>
#include <sys/un.h>
#include <time.h>
 
#include "wpa_command.h"
#include "wifi_monitor.h"

static pthread_t monitor_thId;
int buflen = 2047;
char monitor_buf[2048] = {0};
int reason_for_error = 0;

int wifi_get_monitor_event()
{
    int ret = 0;
    int hardsharkfaild = 0;
    int nofoundtime = 0;
    int network_num = 0;
    int size = 0;

    ret = wifi_get_listnetwork(&network_num);

    ret = wifi_connect_moni_socket(WIFI_WPA_CTRL_PATH);
    if (ret < 0) {
        printf("monitor socket create error %d\n", ret);
        return -1;
    }

    while (1) {

        buflen = 2047;
        memset(monitor_buf, 0, sizeof(monitor_buf));

        /* 接收wpa_supplicant的事件 */
        ret = wifi_ctrl_recv(monitor_buf, &buflen);
        if (ret == 0) {
            if (strstr(monitor_buf, "CTRL-EVENT-")) {
                if (strstr(monitor_buf, "CTRL-EVENT-BSS-") == NULL) {

                    printf("[wifi] ret %d buf: %s len  %d size\n", ret, monitor_buf, buflen, size);

                    if (strstr(monitor_buf, "CTRL-EVENT-NETWORK-NOT-FOUND")) {

                    } else if (strstr(monitor_buf, "CTRL-EVENT-SSID-TEMP-DISABLED")) {

                        if (strstr(monitor_buf, "reason=WRONG_KEY")/* || strstr(monitor_buf, "reason=CONN_FAILED")*/) {

                            if (reason_for_error == 1) {
                                reason_for_error = 0;
                            } else {
                            }
                        }

                    } else if (strstr(monitor_buf, "CTRL-EVENT-DISCONNECTED")) {

                        if (strstr(monitor_buf, "reason=6") || strstr(monitor_buf, "reason=7") ||
                              strstr(monitor_buf, "reason=8") || strstr(monitor_buf, "reason=9"))
                                reason_for_error = 1;

                        /* 路由器断开提示 */
                        if (strstr(monitor_buf, "reason=19")) {
                        }

                    } else if (strstr(monitor_buf, "CTRL-EVENT-CONNECTED")) {

                    } else {

                    }
                }
            }
        } else if (ret == 1) {

        } else {
            printf("recv error: %d\n", errno);
            wifi_monitor_release();
            ret = wifi_connect_moni_socket(WIFI_WPA_CTRL_PATH);
            if (ret < 0) {
                printf("monitor socket create error %d\n", errno);
                return -1;
            }
        }
    }

    return 0;
}

void* wifi_monitor_thread(void *arg)
{
    while (1) {
        if ((access(WIFI_WPA_CTRL_PATH, F_OK)) != -1) {
            break;
        } else {
            sleep(1);
            continue;
        }
    }

    printf(" xxxxxxxxxx wpa_supplicant start ok\n");

    wifi_get_monitor_event();

    return NULL;
}

int start_wifi_monitor()
{
    pthread_create(&monitor_thId, NULL, wifi_monitor_thread, NULL);

    return 0;
}
