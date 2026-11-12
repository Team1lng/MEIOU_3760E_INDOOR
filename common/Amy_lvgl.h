#ifndef _AMY_LVGL_H_
#define _AMY_LVGL_H_
#include "lvgl.h"
#include "device/lv_disp_dev.h"
#include "call_common.h"
#include "sat_ipcamera.h"
#include "card_common_net_proto.h"
#include "password_common_net_proto.h"
#include "fingerprint_common_net_proto.h"
#include <time.h>
typedef enum
{
    AMY_EVENT_ID_VIDEO_MODE,
    AMY_EVENT_ID_SD_CHANGE,
    AMY_EVENT_ID_SD_FORMAT_FINISH,
    AMY_EVENT_ID_DOOR_CALL,
    AMY_EVENT_ID_SNAPSHORT,
    AMY_EVENT_ID_RECORD,
    AMY_EVENT_ID_TUYA,
    AMY_EVENT_ID_CALL,
    AMY_EVENT_ID_IPCAMERA,
    AMY_EVENT_ID_IPCAMERA_ONLINE,
    AMY_EVENT_ID_DOOR_VERSION,
    AMY_EVENT_ID_SYNC_TIME,
    AMY_EVENT_ID_CARD,
    AMY_EVENT_ID_PASSWORD,
    AMY_EVENT_ID_FINGERPRINT,
    AMY_EVENT_ID_VIDEO_PLAY_STATUS,
    AMY_EVENT_ID_HUMAN_DETECT,
    AMY_EVENT_ID_UPDATE_DOOR,
    AMY_EVENT_ID_DEVICE_CONFLICT,
    AMY_EVENT_ID_NETWORK_CONFLICT,
    AMY_EVENT_ID_OTA,
    AMY_EVENT_ID_MUSIC_FINISH,
    AMY_EVENT_ID_KEY_MECHANICAL,
    AMY_EVENT_ID_KEY_ADC,
    AMY_EVENT_ID_TOTAL
} Amy_lv_event_type;

typedef enum
{
    TUYA_EVENT_CMD_VIDEO_START,
    TUYA_EVENT_CMD_VIDEO_STOP,
    TUYA_EVENT_CMD_AUDIO_START,
    TUYA_EVENT_CMD_AUDIO_STOP,
    TUYA_EVENT_CMD_ONLINE,

    /**DP点处理* */
    TUYA_EVENT_CMD_SWITCH_CH,
    TUYA_EVENT_CMD_LOCK1_OPEN,
    TUYA_EVENT_CMD_LOCK2_OPEN,
    TUYA_EVENT_CMD_GATE2_OPEN, // DP 148 室内机锁
    TUYA_EVENT_CMD_CH_AT_HOME,
    TUYA_EVENT_CMD_AWAY_HOME,
    TUYA_EVENT_CMD_CH_SLEEP,
    TUYA_EVENT_CMD_SD_STATUS,
    TUYA_EVENT_CMD_SD_STORAGE,
    TUYA_EVENT_CMD_SD_FORMAT,
    TUYA_EVENT_CMD_LIGHT,
    TUYA_EVENT_CMD_ATMOSPHERE,
    TUYA_EVENT_CMD_TOTAL,
} tuya_event_type;
typedef enum
{
    AMY_CALL_EVENT_ID_CALL_INCOMING,
    AMY_CALL_EVENT_ID_CALL_OUTGOING_PROCESS,
    AMY_CALL_EVENT_ID_CALL_OUTGOING_RINGING,
    AMY_CALL_EVENT_ID_CALL_CONNECTED,
    AMY_CALL_EVENT_ID_CALL_END,
} UI_call_event_type;

typedef enum
{
    AMY_PLAY_STATE_EVENT_ID_PLAY,
    AMY_PLAY_STATE_EVENT_ID_PAUSE,
    AMY_PLAY_STATE_EVENT_ID_FINISH,
} PLAY_STATE;
typedef struct
{
    lv_obj_t *srceen;
    void (*enter)(void);
    void (*quit)(void);
} Amy_lv_page_info;

#define lv_resource_init(img, offset, size) \
    firamware_resource img = {{'S', '@'}, offset, size};

#define lv_resource_alloc(_img_ptr, _offset, _size)                            \
    firamware_resource *_img_ptr = NULL;                                       \
    do                                                                         \
    {                                                                          \
        firamware_resource _temp_img = {{'S', '@'}, _offset, _size};           \
        (_img_ptr) = (firamware_resource *)malloc(sizeof(firamware_resource)); \
        if ((_img_ptr) != NULL)                                                \
        {                                                                      \
            memcpy((_img_ptr), &_temp_img, sizeof(firamware_resource));        \
        }                                                                      \
    } while (0)

void Amy_lvgl_init(const char *mtd_name, unsigned long ttf_base, unsigned long ttf_size);

int Amy_lvgl_run(Amy_lv_page_info *first_page);

#define extern_page(x) extern Amy_lv_page_info lv_page_##x;
#define Amy_lv_page_create(x) Amy_lv_page_info lv_page_##x = {  \
                                  .enter = lv_page_##x##_enter, \
                                  .quit = lv_page_##x##_quit};

#define Amy_lv_page_enter(x) lv_page_##x##_enter(void)

#define Amy_lv_page_quit(x) lv_page_##x##_quit(void)

#define lv_page(x) (&lv_page_##x)

bool _lv_page_goto(Amy_lv_page_info *page, lv_scr_load_anim_t anim);

lv_obj_t *lv_scr_act_current(void);

const Amy_lv_page_info *lv_current_page_get(void);
const Amy_lv_page_info *lv_next_page_get(void);
#define Amy_lv_page_goto(x, a, r) \
    _lv_page_goto(x, a);          \
    return r;

int Amy_video_mode_event_put(video_mode_cb_attr *attr);
int Amy_event_cb_register(int id, void (*event_cb)(void *arg));
int Amy_tfcard_event_put(void);
int Amy_tfcard_format_finish_event_put(void);
int Amy_doorcall_event_put(unsigned int door);
int Amy_snapshort_event_put(int start_or_finish);
int Amy_record_event_put(int start_or_finish);
typedef struct
{
    int event;
    int reserved;
} tuya_cb_attr;
int Amy_tuya_event_put(tuya_cb_attr *attr);
#define SAT_DEVICE_INFO_DEFAULT {                                        \
    .device_type = user_data_get()->private_cfg.device.attr.device_type, \
    .building = user_data_get()->private_cfg.device.attr.building,       \
    .room = user_data_get()->private_cfg.device.attr.room}

int sat_call_event_put(call_cb_attr *attr);

int Amy_ipcamera_event_put(ipcamera_cb_attr *attr);

int Amy_door_version_event_put(door_version_cb_attr *attr);

int Amy_ipcamera_online_event_put(ipcamera_online_check_attr *attr);

int Amy_play_event_put(int play_state);

int Amy_human_detect_event_put(int channel);

int Amy_sync_time_event_put(struct tm *attr);

int Amy_card_event_put(comm_card_op_res_t *attr);

int Amy_password_event_put(comm_password_op_res_t *attr);

int Amy_fingerprint_event_put(comm_finger_op_res_t *attr);

int Amy_update_door_event_put(int state, int progress);

int Amy_device_conflict_event_put(void);

int Amy_network_conflict_event_put(void);

int Amy_stream_timeout_event_put(int channel);

#define OTA_STATE_DOWNLOAD_START 0
#define OTA_STATE_DOWNLOADING 1
#define OTA_STATE_DOWNLOADED 2
#define OTA_STATE_FAILED -1
#define OTA_DEV_TP_OUTDOOR 0
#define OTA_DEV_TP_INDOOR 1
typedef struct
{
    int state;
    int progress;
    int dev_tp;
} ota_event_t;

typedef struct
{
    int state;    /* user_upgrade_state 枚举值 */
    int progress; /* 0-100，仅 DOOR_UPGRADE_PROGRESS 有效 */
} door_upgrade_event_t;

int Amy_ota_event_put(int state, int progress, int dev_tp);

int Amy_music_finish_event_put(void);

typedef struct
{
    unsigned long arg1;
    unsigned long arg2;
} amy_key_event_t;

int Amy_mechanical_key_event_put(unsigned long arg1, unsigned long arg2);

int Amy_adc_key_event_put(unsigned long arg1, unsigned long arg2);

#endif