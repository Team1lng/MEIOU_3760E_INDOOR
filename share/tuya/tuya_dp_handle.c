#include "tuya_dp_define.h"
#include "tuya_dp_callback.h"
#include "tuya_ipc_api.h"
#include "Amy_lvgl.h"

typedef VOID (*TUYA_DP_HANDLER)(IN TY_OBJ_DP_S *p_obj_dp);
typedef struct
{
    BYTE_T dp_id;
    TUYA_DP_HANDLER handler;
} TUYA_DP_INFO_S;

static void hanlde_dp_lock_accessory(IN TY_OBJ_DP_S *p_obj_dp)
{
    tuya_dp_access_lock_support_response();
}

static void hanlde_dp_gate2_open(IN TY_OBJ_DP_S *p_obj_dp)
{
    tuya_cb_attr attr;
    attr.event = TUYA_EVENT_CMD_GATE2_OPEN;
    attr.reserved = p_obj_dp->value.dp_bool;
    Amy_tuya_event_put(&attr);
}

static void hanlde_dp_lock1_open(IN TY_OBJ_DP_S *p_obj_dp)
{
    tuya_cb_attr attr;
    attr.event = TUYA_EVENT_CMD_LOCK1_OPEN;
    attr.reserved = p_obj_dp->value.dp_bool;
    Amy_tuya_event_put(&attr);
}

static void hanlde_dp_lock2_open(IN TY_OBJ_DP_S *p_obj_dp)
{
    tuya_cb_attr attr;
    attr.event = TUYA_EVENT_CMD_LOCK2_OPEN; // 保留你原有的宏
    attr.reserved = p_obj_dp->value.dp_bool;
    Amy_tuya_event_put(&attr);
}

static void hanlde_dp_sleep(IN TY_OBJ_DP_S *p_obj_dp)
{
    tuya_cb_attr attr;
    attr.event = TUYA_EVENT_CMD_CH_SLEEP; // 保留你原有的宏
    Amy_tuya_event_put(&attr);
}

static void hanlde_dp_at_home(IN TY_OBJ_DP_S *p_obj_dp)
{
    tuya_cb_attr attr;
    attr.event = TUYA_EVENT_CMD_CH_AT_HOME; // 保留你原有的宏
    Amy_tuya_event_put(&attr);
}

static void hanlde_dp_away_home(IN TY_OBJ_DP_S *p_obj_dp)
{
    tuya_cb_attr attr;
    attr.event = TUYA_EVENT_CMD_AWAY_HOME;
    Amy_tuya_event_put(&attr);
}

static void hanlde_dp_switch_channel(IN TY_OBJ_DP_S *p_obj_dp)
{
    if (p_obj_dp == NULL)
    {
        return;
    }
    tuya_cb_attr attr;
    attr.event = TUYA_EVENT_CMD_SWITCH_CH;
    char *down_head_str = {"{\"cmd\":1,\"cc\":"};
    int channel_id = p_obj_dp->value.dp_str[strlen(down_head_str)] - 48;
    attr.reserved = channel_id;
    Amy_tuya_event_put(&attr);
}

static void hanlde_dp_sd_status_get(IN TY_OBJ_DP_S *p_obj_dp)
{
    tuya_cb_attr attr;
    attr.event = TUYA_EVENT_CMD_SD_STATUS;
    Amy_tuya_event_put(&attr);
}

static void hanlde_dp_sd_storage_get(IN TY_OBJ_DP_S *p_obj_dp)
{
    tuya_cb_attr attr;
    attr.event = TUYA_EVENT_CMD_SD_STORAGE;
    Amy_tuya_event_put(&attr);
}

static void hanlde_dp_sd_format(IN TY_OBJ_DP_S *p_obj_dp)
{
    tuya_cb_attr attr;
    attr.event = TUYA_EVENT_CMD_SD_FORMAT;
    Amy_tuya_event_put(&attr);
}

static void hanlde_dp_format_status_get(IN TY_OBJ_DP_S *p_obj_dp)
{
    tuya_cb_attr attr;
    attr.event = TUYA_EVENT_CMD_SD_FORMAT;
    Amy_tuya_event_put(&attr);
}

static TUYA_DP_INFO_S s_dp_table[] =
    {
        {TUYA_DP_ACCSESS_GATE2, hanlde_dp_gate2_open},
        {TUYA_DP_ACCSESS_LOCK, hanlde_dp_lock1_open},
        {TUYA_DP_ACCSESS_GATE1, hanlde_dp_lock2_open},
        {TUYA_DP_ACCSESS_LOCK_SUPPORT, hanlde_dp_lock_accessory},
        {TUYA_DP_SLEEP_MODE, hanlde_dp_sleep},
        {TUYA_DP_AT_MODE, hanlde_dp_at_home},
        {TUYA_DP_AWAY_MODE, hanlde_dp_away_home},
        {TUYA_DP_SWITCH_CHANNEL, hanlde_dp_switch_channel},
        {TUYA_DP_SD_STATUS_ONLY_GET, hanlde_dp_sd_status_get},
        {TUYA_DP_SD_STORAGE_ONLY_GET, hanlde_dp_sd_storage_get},
        {TUYA_DP_SD_FORMAT, hanlde_dp_sd_format},
        {TUYA_DP_SD_FORMAT_STATUS_ONLY_GET, hanlde_dp_format_status_get},
};

void tuya_handle_dp_objs(const TY_RECV_OBJ_DP_S *dp_rev)
{
    TY_OBJ_DP_S *dp_data = (TY_OBJ_DP_S *)(dp_rev->dps);
    UINT_T cnt = dp_rev->dps_cnt;
    INT_T table_idx = 0;
    INT_T table_count = (sizeof(s_dp_table) / sizeof(s_dp_table[0]));
    INT_T index = 0;
    TY_OBJ_DP_S *p_dp_obj = dp_data + index;
    printf("event id is %d\r\n", p_dp_obj->dpid);
    for (index = 0; index < cnt; index++)
    {
        TY_OBJ_DP_S *p_dp_obj = dp_data + index;

        for (table_idx = 0; table_idx < table_count; table_idx++)
        {
            if (s_dp_table[table_idx].dp_id == p_dp_obj->dpid)
            {
                s_dp_table[table_idx].handler(p_dp_obj);
                break;
            }
        }
    }
}
