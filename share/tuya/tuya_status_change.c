#include "tuya_sdk.h"
#include "tuya_ipc_event.h"
#include "tuya_ipc_sdk_init.h"
#include "tuya_ipc_upgrade.h"
#include "tuya_cloud_com_defs.h"
#include "tuya_ipc_media_adapter.h"
#include "tuya_ring_buffer.h"
#include "Amy_log.h"
#include "Amy_sdk.h"

#include <string.h>

/** 各状态组最新快照（由 ipc_status_change_cb 全量更新） */
static TUYA_IPC_STATUS_E s_ipc_status[TUYA_IPC_STATUS_GROUP_MAX];
static int s_link_state = TUYA_MQTT_LINK_UNKNOWN;
static int s_prev_link_for_online_event = TUYA_MQTT_LINK_UNKNOWN;

typedef void (*ipc_change_status_func)(void *arg);
typedef struct
{
    TUYA_IPC_STATUS_E stat;
    ipc_change_status_func cb;
} ipc_status_change_cb_t;

static void ipc_status_register_cb(void *arg)
{
    AMY_LOG_INFO("<<<--------\n");
}

static void ipc_status_register_fail_cb(void *arg)
{
    AMY_LOG_INFO("<<<--------\n");
}

static void ipc_status_actived_cb(void *arg)
{
    AMY_LOG_INFO("<<<--------\n");
}

static void ipc_status_reset_cb(void *arg)
{
    AMY_LOG_INFO("<<<--------\n");
}

static void ipc_status_netcfg_start_cb(void *arg)
{
    AMY_LOG_INFO("<<<--------\n");
}

static void ipc_status_netcfg_stop_cb(void *arg)
{
    AMY_LOG_INFO("<<<--------\n");
}

static void ipc_status_wifi_sta_unconn_cb(void *arg)
{
    AMY_LOG_INFO("<<<--------\n");
}

static void ipc_status_wifi_sta_cb(void *arg)
{
    AMY_LOG_INFO("<<<--------\n");
}

static void ipc_status_wire_unconn_cb(void *arg)
{
    AMY_LOG_INFO("<<<--------\n");
}

static void ipc_status_wire_conn_cb(void *arg)
{
    AMY_LOG_INFO("<<<--------\n");
}

static void ipc_status_online_cb(void *arg)
{
    AMY_LOG_INFO("tuya ipc cloud online\n");
}

static void ipc_status_offline_cb(void *arg)
{
    AMY_LOG_INFO("tuya ipc cloud offline\n");
}

static void ipc_status_fw_upgrade_start_cb(void *arg)
{
    AMY_LOG_INFO("<<<--------\n");
}

static int tuya_link_state_compute(void)
{
    TUYA_IPC_STATUS_E cloud = s_ipc_status[TUYA_IPC_STATUS_GROUP_CLOUD];
    if (cloud == TUYA_IPC_STATUS_ONLINE)
    {
        return TUYA_MQTT_LINK_ONLINE;
    }
    if (cloud == TUYA_IPC_STATUS_OFFLINE)
    {
        TUYA_IPC_STATUS_E wifi = s_ipc_status[TUYA_IPC_STATUS_GROUP_WIFI];
        TUYA_IPC_STATUS_E wire = s_ipc_status[TUYA_IPC_STATUS_GROUP_WIRE];
        int link_up = (wifi == TUYA_IPC_STATUS_WIFI_STA_CONN) || (wire == TUYA_IPC_STATUS_WIRE_CONN);
        return link_up ? TUYA_MQTT_LINK_OFFLINE_NET_OK : TUYA_MQTT_LINK_OFFLINE_NET_FAIL;
    }
    return TUYA_MQTT_LINK_UNKNOWN;
}

static ipc_status_change_cb_t stat_change_cb_maps[] = {
    {TUYA_IPC_STATUS_REGISTER, ipc_status_register_cb},
    {TUYA_IPC_STATUS_REGISTER_FAILED, ipc_status_register_fail_cb},
    {TUYA_IPC_STATUS_ACTIVED, ipc_status_actived_cb},
    {TUYA_IPC_STATUS_RESET, ipc_status_reset_cb},
    {TUYA_IPC_STATUS_NETCFG_START, ipc_status_netcfg_start_cb},
    {TUYA_IPC_STATUS_NETCFG_STOP, ipc_status_netcfg_stop_cb},
    {TUYA_IPC_STATUS_WIFI_STA_UNCONN, ipc_status_wifi_sta_unconn_cb},
    {TUYA_IPC_STATUS_WIFI_STA_CONN, ipc_status_wifi_sta_cb},
    {TUYA_IPC_STATUS_WIRE_UNCONN, ipc_status_wire_unconn_cb},
    {TUYA_IPC_STATUS_WIRE_CONN, ipc_status_wire_conn_cb},
    {TUYA_IPC_STATUS_ONLINE, ipc_status_online_cb},
    {TUYA_IPC_STATUS_OFFLINE, ipc_status_offline_cb},
    {TUYA_IPC_STATUS_UPGRADE_START, ipc_status_fw_upgrade_start_cb},
};

void ipc_status_change_cb(TUYA_IPC_STATUS_GROUP_E changed_group, const TUYA_IPC_STATUS_E status[TUYA_IPC_STATUS_GROUP_MAX])
{
    (void)changed_group;
    memcpy(s_ipc_status, status, sizeof(s_ipc_status));

    TUYA_IPC_STATUS_E cur_status = status[changed_group];
    AMY_LOG_INFO("status change: group[%d] status[%d]\n", (int)changed_group, (int)cur_status);
    for (unsigned i = 0; i < sizeof(stat_change_cb_maps) / sizeof(stat_change_cb_maps[0]); i++)
    {
        if (stat_change_cb_maps[i].stat == cur_status)
        {
            if (stat_change_cb_maps[i].cb)
            {
                stat_change_cb_maps[i].cb(NULL);
            }
            else
            {
                AMY_LOG_ERROR("status ignore .cb is null.stat:%d\n", (int)changed_group);
            }
        }
    }

    int new_link = tuya_link_state_compute();
    if (new_link != s_link_state)
    {
        AMY_LOG_INFO("tuya link state: %d -> %d\n", s_link_state, new_link);
        s_link_state = new_link;
    }

    if (s_link_state == TUYA_MQTT_LINK_ONLINE && s_prev_link_for_online_event != TUYA_MQTT_LINK_ONLINE)
    {
        tuya_cb_attr attr;
        memset(&attr, 0, sizeof(attr));
        attr.event = TUYA_EVENT_CMD_ONLINE;
        Amy_tuya_event_put(&attr);
    }
    s_prev_link_for_online_event = s_link_state;
}

int tuya_mqtt_link_state_get(void)
{
    return s_link_state;
}
