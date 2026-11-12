#include "page_tabel.h"
#include "lv_common.h"
#include "page_common.h"
#include "page_call.h"
#include "user_data.h"
#include "device_types.h"

/************************** 设备ID范围 **************************/
#define DEV_ID_MIN 1
#define DEV_ID_MAX 6

/************************** 布局常量 **************************/
#define BTN_W 78
#define BTN_H 78
#define BTN_Y 261
#define BTN_GAP 40

/************************** 图标资源（索引 0-5 = ID1-ID6，索引 6 = ALL） **************************/
static const int dev_icons_offset[] = {
    ICON_TRANSFER_EXTENSION1_PNG_OFFSET,
    ICON_TRANSFER_EXTENSION2_PNG_OFFSET,
    ICON_TRANSFER_EXTENSION3_PNG_OFFSET,
    ICON_TRANSFER_EXTENSION4_PNG_OFFSET,
    ICON_TRANSFER_EXTENSION5_PNG_OFFSET,
    ICON_TRANSFER_EXTENSION6_PNG_OFFSET,
    ICON_TRANSFER_EXTENSION_ALL_PNG_OFFSET,
};
static const int dev_icons_size[] = {
    ICON_TRANSFER_EXTENSION1_PNG_SIZE,
    ICON_TRANSFER_EXTENSION2_PNG_SIZE,
    ICON_TRANSFER_EXTENSION3_PNG_SIZE,
    ICON_TRANSFER_EXTENSION4_PNG_SIZE,
    ICON_TRANSFER_EXTENSION5_PNG_SIZE,
    ICON_TRANSFER_EXTENSION6_PNG_SIZE,
    ICON_TRANSFER_EXTENSION_ALL_PNG_SIZE,
};
static const int dev_icons_focus_offset[] = {
    ICON_TRANSFER_EXTENSION1_FOCUS_PNG_OFFSET,
    ICON_TRANSFER_EXTENSION2_FOCUS_PNG_OFFSET,
    ICON_TRANSFER_EXTENSION3_FOCUS_PNG_OFFSET,
    ICON_TRANSFER_EXTENSION4_FOCUS_PNG_OFFSET,
    ICON_TRANSFER_EXTENSION5_FOCUS_PNG_OFFSET,
    ICON_TRANSFER_EXTENSION6_FOCUS_PNG_OFFSET,
    ICON_TRANSFER_EXTENSION_ALL_FOCUS_PNG_OFFSET,
};
static const int dev_icons_focus_size[] = {
    ICON_TRANSFER_EXTENSION1_FOCUS_PNG_SIZE,
    ICON_TRANSFER_EXTENSION2_FOCUS_PNG_SIZE,
    ICON_TRANSFER_EXTENSION3_FOCUS_PNG_SIZE,
    ICON_TRANSFER_EXTENSION4_FOCUS_PNG_SIZE,
    ICON_TRANSFER_EXTENSION5_FOCUS_PNG_SIZE,
    ICON_TRANSFER_EXTENSION6_FOCUS_PNG_SIZE,
    ICON_TRANSFER_EXTENSION_ALL_FOCUS_PNG_SIZE,
};
static const int dev_btn_ids[] = {
    TRANSFER_DEV1_BTN_ID,
    TRANSFER_DEV2_BTN_ID,
    TRANSFER_DEV3_BTN_ID,
    TRANSFER_DEV4_BTN_ID,
    TRANSFER_DEV5_BTN_ID,
    TRANSFER_DEV6_BTN_ID,
    TRANSFER_ALL_BTN_ID,
};

/************************** 页面上下文 **************************/
static struct
{
    int visible_count;
    int visible_icon_idx[7]; /* icon array index for each visible slot */
    int visible_dev_type[7]; /* device_type (1-6) or 7=ALL for each visible slot */
} s_ctx;

/************************** 函数声明 **************************/
static void transfer_dev_obj_click(lv_event_t *ev);
static void transfer_back_obj_click(lv_event_t *ev);

static void transfer_dev_buttons_create(void);
static void transfer_back_create(void);

/************************** 回调实现 **************************/

static void transfer_dev_obj_click(lv_event_t *ev)
{
    lv_obj_t *target = lv_event_get_current_target(ev);
    int id = lv_obj_get_id(target);
    int dev = 0;

    if (id == TRANSFER_DEV1_BTN_ID)
        dev = DEVICE_TYPE_INDOOR_ID1;
    else if (id == TRANSFER_DEV2_BTN_ID)
        dev = DEVICE_TYPE_INDOOR_ID2;
    else if (id == TRANSFER_DEV3_BTN_ID)
        dev = DEVICE_TYPE_INDOOR_ID3;
    else if (id == TRANSFER_DEV4_BTN_ID)
        dev = DEVICE_TYPE_INDOOR_ID4;
    else if (id == TRANSFER_DEV5_BTN_ID)
        dev = DEVICE_TYPE_INDOOR_ID5;
    else if (id == TRANSFER_DEV6_BTN_ID)
        dev = DEVICE_TYPE_INDOOR_ID6;
    else if (id == TRANSFER_ALL_BTN_ID)
        dev = DEVICE_TYPE_INDOOR_FAMILY;

    call_set_selected_dev(dev);
    Amy_lv_page_goto(lv_page(call), LV_SCR_LOAD_ANIM_NONE, );
}

static void transfer_back_obj_click(lv_event_t *ev)
{
    Amy_lv_page_goto(lv_page(home), LV_SCR_LOAD_ANIM_NONE, );
}

/************************** 控件创建 **************************/

static void transfer_dev_buttons_create(void)
{
    int total_w = BTN_W * s_ctx.visible_count + BTN_GAP * (s_ctx.visible_count - 1);
    int x_start = (SCREEN_W - total_w) / 2;

    for (int i = 0; i < s_ctx.visible_count; i++)
    {
        int icon_idx = s_ctx.visible_icon_idx[i];
        lv_resource_init(icon, dev_icons_offset[icon_idx], dev_icons_size[icon_idx]);
        lv_resource_init(icon_focus, dev_icons_focus_offset[icon_idx], dev_icons_focus_size[icon_idx]);
        int x = x_start + i * (BTN_W + BTN_GAP);
        lv_obj_t *btn = lv_common_img_btn_create(lv_scr_act_current(), dev_btn_ids[icon_idx], x, BTN_Y, BTN_W, BTN_H,
                                                 transfer_dev_obj_click, true, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0x101010,
                                                 0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                 0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                 &icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &icon_focus);
        lv_obj_set_ext_click_area(btn, 21);
    }
}

static void transfer_back_create(void)
{
    lv_resource_init(icon, ICON_COMMON_BACK_PNG_OFFSET, ICON_COMMON_BACK_PNG_SIZE);
    lv_resource_init(icon_focus, ICON_COMMON_BACK_FOCUS_PNG_OFFSET, ICON_COMMON_BACK_FOCUS_PNG_SIZE);
    lv_obj_t *back = lv_common_img_btn_create(lv_scr_act_current(), TRANSFER_BACK_BTN_ID, 929, 505, 67, 67,
                                              transfer_back_obj_click, true, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0x101010,
                                              0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                              0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                              &icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &icon_focus);
    lv_obj_set_ext_click_area(back, 21);
}

/************************** 构建可见设备列表（排除自身） **************************/
static void transfer_build_visible_list(void)
{
    int self_type = user_data_get()->private_cfg.device.attr.device_type;

    s_ctx.visible_count = 0;

    for (int dt = DEV_ID_MIN; dt <= DEV_ID_MAX; dt++)
    {
        if (dt == self_type)
            continue;
        s_ctx.visible_icon_idx[s_ctx.visible_count] = dt - DEV_ID_MIN;
        s_ctx.visible_dev_type[s_ctx.visible_count] = dt;
        s_ctx.visible_count++;
    }

    /* ALL 按钮 */
    s_ctx.visible_icon_idx[s_ctx.visible_count] = DEV_ID_MAX; /* index 6 */
    s_ctx.visible_dev_type[s_ctx.visible_count] = 7;
    s_ctx.visible_count++;
}

/************************** 页面生命周期 **************************/
static void Amy_lv_page_enter(transfer)
{
    lv_common_rect_create(lv_scr_act_current(), 0, 0, SCREEN_W, SCREEN_H, LV_COMMON_BG_COLOR);
    transfer_build_visible_list();
    transfer_dev_buttons_create();
    transfer_back_create();
    Amy_event_cb_register(AMY_EVENT_ID_SD_CHANGE, page_common_tfcard_change_cb);
}

static void Amy_lv_page_quit(transfer)
{
    Amy_event_cb_register(AMY_EVENT_ID_SD_CHANGE, NULL);
}

Amy_lv_page_create(transfer);
