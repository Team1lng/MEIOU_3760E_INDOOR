#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <libgen.h>
#include <signal.h>
#include <sys/prctl.h>

#include "utilities/uni_log.h"
#include "tuya_iot_config.h"
#include "tuya_ipc_api.h"
#include "ty_sdk_common.h"

STATIC INT_T s_mqtt_status = 0;
typedef VOID (*ON_IPC_STATUS_CHANGE)(VOID *arg);
typedef struct {
    TUYA_IPC_STATUS_E stat;
    ON_IPC_STATUS_CHANGE cb;
}IPC_STATUS_CHANGE_CB_MAP_T;


STATIC VOID __on_register_fail(VOID *arg)
{
    
    PR_DEBUG("%s, get registe fail status.", __func__);

    return;
}

STATIC VOID __on_netcfg_start(VOID *arg){

    PR_DEBUG("%s, netcfg start. You can start your own netcfg task now.", __func__);

    return;
}

STATIC VOID __on_netcfg_stop(VOID *arg){
    
    PR_DEBUG("%s, netcfg stop. You can stop your own netcfg task now.", __func__);

    return;
}

STATIC VOID __on_status_offline(VOID *arg){

    PR_INFO("offline: network status MQTT disconnected"); //according to requirements of shadow device, do not modify!!!
    
    return;
}

STATIC VOID __on_status_fw_upgrade_start(VOID *arg){

    PR_INFO("fw upgrade: recv start cmd"); 
    return;
}

STATIC VOID __on_status_online(VOID *arg){

    PR_INFO("online: network status MQTT connected"); //according to requirements of shadow device, do not modify!!!
    s_mqtt_status = 1;

    return;
}


static IPC_STATUS_CHANGE_CB_MAP_T status_change_cb_map[] = {
    {TUYA_IPC_STATUS_REGISTER,          NULL},
    {TUYA_IPC_STATUS_REGISTER_FAILED,   __on_register_fail},
    {TUYA_IPC_STATUS_ACTIVED,           NULL},
    {TUYA_IPC_STATUS_RESET,             NULL},
    {TUYA_IPC_STATUS_NETCFG_START,      __on_netcfg_start},
    {TUYA_IPC_STATUS_NETCFG_STOP,       __on_netcfg_stop},
    {TUYA_IPC_STATUS_WIFI_STA_UNCONN,   NULL},
    {TUYA_IPC_STATUS_WIFI_STA_CONN,     NULL},
    {TUYA_IPC_STATUS_WIRE_UNCONN,       NULL},
    {TUYA_IPC_STATUS_WIRE_CONN,         NULL}, 
    {TUYA_IPC_STATUS_ONLINE,            __on_status_online},
    {TUYA_IPC_STATUS_OFFLINE,           __on_status_offline},    
    {TUYA_IPC_STATUS_UPGRADE_START,     __on_status_fw_upgrade_start}, 
};

VOID TUYA_IPC_Status_Changed_cb(IN TUYA_IPC_STATUS_GROUP_E changed_group, IN CONST TUYA_IPC_STATUS_E status[TUYA_IPC_STATUS_GROUP_MAX])
{
    TUYA_IPC_STATUS_E cur_status = status[changed_group];
    INT_T i;
    PR_DEBUG("status chaged: group[%d] status[%d]", changed_group, cur_status);
    
    for(i = 0; i < SIZEOF(status_change_cb_map)/SIZEOF(IPC_STATUS_CHANGE_CB_MAP_T); i++) {
        if(status_change_cb_map[i].stat == cur_status) {
            if(status_change_cb_map[i].cb) {
                status_change_cb_map[i].cb(NULL);
            } else {
                PR_DEBUG("status ignore. cb is null.");
            }
        }
    }    
}

INT_T TUYA_IPC_sd_status_upload(INT_T status)
{
	IPC_APP_report_sd_status_changed(status);
	return 0;
}

VOID TUYA_IPC_qrcode_shorturl_cb(CHAR_T* shorturl)
{
    if(shorturl)
        PR_INFO("shorturl: %s}", shorturl); //according to requirements of shadow device, do not modify!!!

    return;
}


/* 
Callback when the user clicks on the APP to remove the device
*/
VOID TUYA_IPC_Reset_System_CB(GW_RESET_TYPE_E type)
{
    PR_INFO("reset ipc success. please restart the ipc %d\n", type);

    //TODO
    /* Developers need to restart IPC operations */
}

VOID TUYA_IPC_Restart_Process_CB(VOID)
{
    PR_INFO("sdk internal restart request. please restart the ipc\n");
    //TODO
    /* Developers need to implement restart operations. Restart the process or restart the device. */
}

INT_T TUYA_IPC_Get_MqttStatus()
{
    return s_mqtt_status;
}

