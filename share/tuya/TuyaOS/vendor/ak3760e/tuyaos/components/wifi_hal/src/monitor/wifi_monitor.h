#ifndef __WIFI_MONITOR_H
#define __WIFI_MONITOR_H
 
#ifdef __cplusplus
extern "C" {
#endif

void* wifi_monitor_thread(void *arg);

int start_wifi_monitor();

#ifdef __cplusplus
}
#endif

#endif
