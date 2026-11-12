#include <unistd.h>
#include "stdlib.h"
#include <stdio.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>

#include "ak_common_graphics.h"
#include "ak_common.h"
#include "ak_tde.h"
#include "ak_mem.h"

#include "Amy_sdk.h"
#include "Amy_lvgl.h"
#include "Amy_log.h"
#include "Amy_tfcard.h"
#include "lvgl.h"

#include "mempool/mempool.h"
#include "queues/queues.h"
#include "video_output.h"
#include "device/lv_disp_dev.h"
#include "device/lv_input_dev.h"
#include "device/lv_firmware_resource.h"
#include <linux/input.h>

typedef struct
{
    int fd;
    int root_x;
    int root_y;
    int button;
} lv_evdev_context;

void lv_disp_flush_callback(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p)
{
    Amy_gui_refresh(area, (unsigned char *)color_p);
    if (lv_disp_flush_is_last(disp_drv))
    {
        Amy_gui_display();
    }
    lv_disp_flush_ready(disp_drv);
}

void lv_indev_read_callback(lv_indev_drv_t *disp_drv, lv_indev_data_t *data)
{
    lv_evdev_context *evdev = (lv_evdev_context *)disp_drv->user_data;
    struct input_event in;
    while (read(evdev->fd, &in, sizeof(struct input_event)) > 0)
    {
        if (in.type == EV_REL)
        {
            if (in.code == REL_X)
                evdev->root_x += in.value;
            else if (in.code == REL_Y)
                evdev->root_y += in.value;
        }
        else if (in.type == EV_ABS)
        {
            if (in.code == ABS_X)
                evdev->root_x = in.value;
            else if (in.code == ABS_Y)
                evdev->root_y = in.value;
            else if (in.code == ABS_MT_POSITION_X)
                evdev->root_x = in.value;
            else if (in.code == ABS_MT_POSITION_Y)
                evdev->root_y = in.value;
            else if (in.code == 48)
            {
                if (in.value == 0)
                {
                    evdev->button = LV_INDEV_STATE_REL;
                }
                else
                {
                    evdev->button = LV_INDEV_STATE_PR;
                }
            }
        }
    }

    data->point.x = evdev->root_x;
    data->point.y = evdev->root_y;
    data->state = evdev->button;
    if (data->point.x < 0)
        data->point.x = 0;
    if (data->point.y < 0)
        data->point.y = 0;
    if (data->point.x >= lv_disp_get_hor_res(disp_drv->disp))
        data->point.x = lv_disp_get_hor_res(disp_drv->disp) - 1;
    if (data->point.y >= lv_disp_get_ver_res(disp_drv->disp))
        data->point.y = lv_disp_get_ver_res(disp_drv->disp) - 1;

    // AMY_LOG_INFO("touch:%d,%d state:%d\n", data->point.x, data->point.y, data->state);
}

typedef struct
{
    unsigned int id;
    void *argv;
} lv_ev_data;

typedef struct lv_event_context
{
    mp_pool *mp;
    queues_conetext *q;
} lv_event_context;
static lv_event_context lv_ctx;
static void (*tfcard_cleanup_callback)(void);

typedef struct
{
    const unsigned int id;
    void (*cb)(void *arg);
} lv_event_param;

static lv_event_param event_codes[AMY_EVENT_ID_TOTAL] = {
    {AMY_EVENT_ID_VIDEO_MODE, NULL},
    {AMY_EVENT_ID_SD_CHANGE, NULL},
    {AMY_EVENT_ID_SD_FORMAT_FINISH, NULL},
    {AMY_EVENT_ID_DOOR_CALL, NULL},
    {AMY_EVENT_ID_SNAPSHORT, NULL},
    {AMY_EVENT_ID_RECORD, NULL},
    {AMY_EVENT_ID_CALL, NULL},
    {AMY_EVENT_ID_TUYA, NULL},
    {AMY_EVENT_ID_IPCAMERA, NULL},
    {AMY_EVENT_ID_IPCAMERA_ONLINE, NULL},
    {AMY_EVENT_ID_DOOR_VERSION, NULL},
    {AMY_EVENT_ID_SYNC_TIME, NULL},
    {AMY_EVENT_ID_CARD, NULL},
    {AMY_EVENT_ID_PASSWORD, NULL},
    {AMY_EVENT_ID_FINGERPRINT, NULL},
    {AMY_EVENT_ID_VIDEO_PLAY_STATUS, NULL},
    {AMY_EVENT_ID_HUMAN_DETECT, NULL},
    {AMY_EVENT_ID_UPDATE_DOOR, NULL},
    {AMY_EVENT_ID_DEVICE_CONFLICT, NULL},
    {AMY_EVENT_ID_NETWORK_CONFLICT, NULL},
    {AMY_EVENT_ID_OTA, NULL},
    {AMY_EVENT_ID_MUSIC_FINISH, NULL},
    {AMY_EVENT_ID_KEY_MECHANICAL, NULL},
    {AMY_EVENT_ID_KEY_ADC, NULL},
};

static void lv_event_timer(lv_timer_t *t)
{
    lv_event_context *ev = (lv_event_context *)t->user_data;
    lv_ev_data *data;
    while (Amy_queues_get(ev->q, (void **)&data, NULL, NULL) == 0)
    {
        unsigned int sd_sequence = 0;
        if (data->id == AMY_EVENT_ID_SD_CHANGE)
        {
            if (data->argv)
            {
                sd_sequence = *(unsigned int *)data->argv;
                if (!Amy_tfcard_event_is_current(sd_sequence))
                {
                    Amy_mempool_free(ev->mp, data);
                    continue;
                }
            }
            if (!(Amy_tfcard_status() & SD_STATE_INSERT) && tfcard_cleanup_callback)
                tfcard_cleanup_callback();
        }
        for (int i = 0; i < sizeof(event_codes) / sizeof(lv_event_param); i++)
        {
            if (event_codes[i].id == data->id)
            {
                if (event_codes[i].cb)
                {
                    event_codes[i].cb(data->argv);
                }
                break;
            }
        }
        if (sd_sequence)
            Amy_tfcard_event_processed(sd_sequence);
        if (data->id == AMY_EVENT_ID_SD_FORMAT_FINISH)
            Amy_tfcard_format_event_processed();
        Amy_mempool_free(ev->mp, data);
    }
}

void Amy_lvgl_init(const char *mtd_name, unsigned long ttf_base, unsigned long ttf_size)
{
    lv_init(mtd_name, ttf_base, ttf_size);
    lv_disp_device_init(1024, 600, lv_disp_flush_callback, NULL);
    lv_evdev_context *evdev = (lv_evdev_context *)Amy_malloc(sizeof(lv_evdev_context));
    memset(evdev, 0, sizeof(lv_evdev_context));
    evdev->fd = open("/dev/input/event0", O_RDWR | O_NOCTTY | O_NDELAY);
    if (evdev->fd >= 0)
    {
        fcntl(evdev->fd, F_SETFL, O_ASYNC | O_NONBLOCK);
        lv_input_device_init(lv_indev_read_callback, evdev);
    }
    firamware_resource_set(mtd_name);
    lv_disp_set_bg_color(lv_disp_get_default(), lv_color_hex(0xFF0000));
    lv_disp_set_bg_opa(lv_disp_get_default(), LV_OPA_COVER);
}
int Amy_lvgl_run(Amy_lv_page_info *first_page)
{
    int size = sizeof(lv_ev_data) + 2048;
    lv_ctx.mp = Amy_mempool_init(size * 8, size * 8);
    if (!lv_ctx.mp)
    {
        AMY_LOG_ERROR("lv context alloc mempool failed\n");
        return -1;
    }
    lv_ctx.q = Amy_queues_open(8, 0, 0, 0, 0);
    if (!lv_ctx.q)
    {
        AMY_LOG_ERROR("queues open failed\n");
        return -1;
    }

    lv_timer_create(lv_event_timer, 30, &lv_ctx);

    uint64_t p_titck;
    uint64_t titck;

    _lv_page_goto(first_page, LV_SCR_LOAD_ANIM_NONE);
    Amy_timestamp(p_titck);
    while (1)
    {
        lv_timer_handler();
        Amy_timestamp(titck);
        if (titck > p_titck)
        {
            lv_tick_inc(titck - p_titck);
        }
        Amy_timestamp(p_titck);
        Amy_msleep(10);
    }
    return 0;
}

int Amy_event_cb_register(int id, void (*event_cb)(void *arg))
{
    for (int i = 0; i < sizeof(event_codes) / sizeof(lv_event_param); i++)
    {
        if (event_codes[i].id == id)
        {
            event_codes[i].cb = event_cb;
            break;
        }
    }
    return 0;
}

// 通用事件入队函数（封装重复逻辑）
static int Amy_event_put_base(int event_id, void *param, uint32_t param_size)
{
    /* 事件总线（内存池/队列）在 Amy_lvgl_run 中才初始化，
     * 此处兜底防止启动阶段（如按键检测线程）总线未就绪时空指针崩溃 */
    if (!lv_ctx.mp || !lv_ctx.q)
        return -1;
    uint32_t total_size = sizeof(lv_ev_data) + (param_size > 0 ? param_size : 0);
    lv_ev_data *data = Amy_mempool_alloc(lv_ctx.mp, total_size);
    if (!data)
    {
        AMY_LOG_ERROR("event(%d) mempool alloc failed\n", event_id);
        return -1;
    }
    data->id = event_id;
    if (param && param_size > 0)
    {
        data->argv = (void *)((char *)data + sizeof(lv_ev_data));
        memcpy(data->argv, param, param_size);
    }
    else
    {
        data->argv = NULL;
    }
    if (Amy_queues_put(lv_ctx.q, data))
    {
        Amy_mempool_free(lv_ctx.mp, data);
        return -1;
    }
    return 0;
}

int Amy_snapshort_event_put(int start_or_finish)
{
    return Amy_event_put_base(AMY_EVENT_ID_SNAPSHORT, &start_or_finish, sizeof(int));
}

int Amy_record_event_put(int start_or_finish)
{
    return Amy_event_put_base(AMY_EVENT_ID_RECORD, &start_or_finish, sizeof(int));
}

int Amy_video_mode_event_put(video_mode_cb_attr *attr)
{
    return Amy_event_put_base(AMY_EVENT_ID_VIDEO_MODE, attr, sizeof(video_mode_cb_attr));
}

int Amy_card_event_put(comm_card_op_res_t *attr)
{
    return Amy_event_put_base(AMY_EVENT_ID_CARD, attr, sizeof(comm_card_op_res_t));
}

int Amy_password_event_put(comm_password_op_res_t *attr)
{
    return Amy_event_put_base(AMY_EVENT_ID_PASSWORD, attr, sizeof(comm_password_op_res_t));
}

int Amy_fingerprint_event_put(comm_finger_op_res_t *attr)
{
    return Amy_event_put_base(AMY_EVENT_ID_FINGERPRINT, attr, sizeof(comm_finger_op_res_t));
}

int Amy_sync_time_event_put(struct tm *attr)
{
    return Amy_event_put_base(AMY_EVENT_ID_SYNC_TIME, attr, sizeof(struct tm));
}

int Amy_door_version_event_put(door_version_cb_attr *attr)
{
    return Amy_event_put_base(AMY_EVENT_ID_DOOR_VERSION, attr, sizeof(door_version_cb_attr));
}

int Amy_ipcamera_online_event_put(ipcamera_online_check_attr *attr)
{
    return Amy_event_put_base(AMY_EVENT_ID_IPCAMERA_ONLINE, attr, sizeof(door_version_cb_attr));
}

int Amy_ipcamera_event_put(ipcamera_cb_attr *attr)
{
    return Amy_event_put_base(AMY_EVENT_ID_IPCAMERA, attr, sizeof(ipcamera_cb_attr));
}

int sat_call_event_put(call_cb_attr *attr)
{
    return Amy_event_put_base(AMY_EVENT_ID_CALL, attr, sizeof(call_cb_attr));
}

int Amy_tuya_event_put(tuya_cb_attr *attr)
{
    return Amy_event_put_base(AMY_EVENT_ID_TUYA, attr, sizeof(tuya_cb_attr));
}

int Amy_tfcard_event_put(void)
{
    return Amy_event_put_base(AMY_EVENT_ID_SD_CHANGE, NULL, 0);
}

int Amy_tfcard_state_event_put(unsigned int sequence)
{
    return Amy_event_put_base(AMY_EVENT_ID_SD_CHANGE, &sequence, sizeof(sequence));
}

void Amy_tfcard_cleanup_cb_register(void (*callback)(void))
{
    tfcard_cleanup_callback = callback;
}

int Amy_tfcard_format_finish_event_put(void)
{
    return Amy_tfcard_format_result_event_put(0);
}

int Amy_tfcard_format_result_event_put(int result)
{
    return Amy_event_put_base(AMY_EVENT_ID_SD_FORMAT_FINISH, &result, sizeof(result));
}

int Amy_play_event_put(int play_state)
{
    return Amy_event_put_base(AMY_EVENT_ID_VIDEO_PLAY_STATUS, &play_state, sizeof(int));
}

int Amy_human_detect_event_put(int channel)
{
    return Amy_event_put_base(AMY_EVENT_ID_HUMAN_DETECT, &channel, sizeof(int));
}

int Amy_update_door_event_put(int state, int progress)
{
    door_upgrade_event_t ev;
    ev.state = state;
    ev.progress = progress;
    return Amy_event_put_base(AMY_EVENT_ID_UPDATE_DOOR, &ev, sizeof(door_upgrade_event_t));
}

int Amy_device_conflict_event_put(void)
{
    return Amy_event_put_base(AMY_EVENT_ID_DEVICE_CONFLICT, NULL, 0);
}

int Amy_network_conflict_event_put(void)
{
    return Amy_event_put_base(AMY_EVENT_ID_NETWORK_CONFLICT, NULL, 0);
}

int Amy_ota_event_put(int state, int progress, int dev_tp)
{
    ota_event_t ev;
    ev.state = state;
    ev.progress = progress;
    ev.dev_tp = dev_tp;
    return Amy_event_put_base(AMY_EVENT_ID_OTA, &ev, sizeof(ota_event_t));
}

int Amy_music_finish_event_put(void)
{
    return Amy_event_put_base(AMY_EVENT_ID_MUSIC_FINISH, NULL, 0);
}

int Amy_mechanical_key_event_put(unsigned long arg1, unsigned long arg2)
{
    amy_key_event_t ev = {arg1, arg2};
    return Amy_event_put_base(AMY_EVENT_ID_KEY_MECHANICAL, &ev, sizeof(amy_key_event_t));
}

int Amy_adc_key_event_put(unsigned long arg1, unsigned long arg2)
{
    amy_key_event_t ev = {arg1, arg2};
    return Amy_event_put_base(AMY_EVENT_ID_KEY_ADC, &ev, sizeof(amy_key_event_t));
}

/************************************************************************************************************* */
static Amy_lv_page_info *lv_page_current = NULL;
static Amy_lv_page_info *lv_page_prev = NULL;
static Amy_lv_page_info *lv_page_next = NULL;
bool _lv_page_goto(Amy_lv_page_info *page, lv_scr_load_anim_t anim)
{
    lv_timer_recycle();
    lv_anim_del_all();
    lv_page_prev = lv_page_current;
    lv_page_next = page;
    if ((lv_page_prev != NULL) && (lv_page_prev->quit != NULL))
    {
        lv_page_prev->quit();
    }
#if 0
    if (page->enter)
    {
        lv_obj_t* new_scr = lv_obj_create(NULL);
        lv_obj_clear_flag(new_scr, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
        lv_page_current = page;
        lv_page_current->srceen = new_scr;
        lv_page_current->enter();
        if (anim != LV_SCR_LOAD_ANIM_NONE)
        {
            lv_scr_load_anim(new_scr, anim, 500, 0, true);
        }
    }
#else
    lv_obj_clean(lv_scr_act());
    lv_obj_clear_flag(lv_scr_act(), LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_page_current = page;
    lv_page_current->srceen = lv_scr_act();
    lv_page_current->enter();
#endif
    return true;
}

lv_obj_t *lv_scr_act_current(void)
{
    if (lv_page_current == NULL)
    {
        return NULL;
    }
    return lv_page_current->srceen;
}

const Amy_lv_page_info *lv_current_page_get(void)
{
    return lv_page_current;
}
const Amy_lv_page_info *lv_next_page_get(void)
{
    return lv_page_next;
}
