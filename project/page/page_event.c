#include "page_tabel.h"
#include "lv_common.h"
#include "page_common.h"

page_media_browse_ctx_t g_media_browse = {0};
media_record_type_t g_media_record_type;


/************************** 布局常量（Row1:2列居中, Row2:1列左对齐） **************************/
#define BTN_W 180
#define BTN_H 190
#define GAP_H 188
#define GAP_V 64
#define CENTER_2 ((1024 - 2 * BTN_W - GAP_H) / 2)
#define COL1_X CENTER_2
#define COL2_X (CENTER_2 + BTN_W + GAP_H)
#define ROW1_Y 113
#define ROW2_Y (ROW1_Y + BTN_H + GAP_V)

/************************** 函数声明 **************************/
static void event_record_call_btn_click(lv_event_t *ev);
static void event_detection_record_obj_click(lv_event_t *ev);
static void event_message_record_obj_click(lv_event_t *ev);
static void event_back_obj_click(lv_event_t *ev);

static void event_record_call_btn_create(void);
static void event_detection_record_create(void);
static void event_message_record_create(void);
static void event_back_create(void);

/************************** 回调实现 **************************/

static void event_record_call_btn_click(lv_event_t *ev)
{
    g_media_record_type = MEDIA_RECORD_TYPE_CALL;
    Amy_lv_page_goto(lv_page(event_record), LV_SCR_LOAD_ANIM_NONE, );
}

static void event_detection_record_obj_click(lv_event_t *ev)
{
    g_media_record_type = MEDIA_RECORD_TYPE_MESSAGE;
    Amy_lv_page_goto(lv_page(event_record), LV_SCR_LOAD_ANIM_NONE, );
}

static void event_message_record_obj_click(lv_event_t *ev)
{
    g_media_record_type = MEDIA_RECORD_TYPE_DETECTION;
    Amy_lv_page_goto(lv_page(event_record), LV_SCR_LOAD_ANIM_NONE, );
}

static void event_back_obj_click(lv_event_t *ev)
{
    Amy_lv_page_goto(lv_page(home), LV_SCR_LOAD_ANIM_NONE, );
}

/************************** 按钮创建 **************************/

static void event_record_call_btn_create(void)
{
    lv_resource_init(icon, ICON_RECORD_CALL_RECORD_PNG_OFFSET, ICON_RECORD_CALL_RECORD_PNG_SIZE);
    lv_resource_init(icon_focus, ICON_RECORD_CALL_RECORD_FOCUS_PNG_OFFSET, ICON_RECORD_CALL_RECORD_FOCUS_PNG_SIZE);
    lv_obj_t *event_record_call_btn = lv_common_img_text_btn_create(lv_scr_act_current(), EVENT_RECORD_CALL_CONT_ID, COL1_X, ROW1_Y, BTN_W, BTN_H,
                                                                event_record_call_btn_click, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
                                                                0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                                0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                                0, 130, BTN_W, 60, EVENT_RECORD_CALL_TEXT_ID,
                                                                lang_str_get(STR_CALL_RECORD), 0xffffff, 0xF91313, LV_TEXT_ALIGN_CENTER, lv_font_normal,
                                                                25, 0, 130, 130, EVENT_RECORD_CALL_BTN_ID,
                                                                &icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &icon_focus);
    lv_obj_set_ext_click_area(event_record_call_btn, 21);
}

static void event_detection_record_create(void)
{
    lv_resource_init(icon, ICON_RECORD_MESSAGE_RECORD_PNG_OFFSET, ICON_RECORD_MESSAGE_RECORD_PNG_SIZE);
    lv_resource_init(icon_focus, ICON_RECORD_MESSAGE_RECORD_FOCUS_PNG_OFFSET, ICON_RECORD_MESSAGE_RECORD_FOCUS_PNG_SIZE);
    lv_obj_t *event_detection_record = lv_common_img_text_btn_create(lv_scr_act_current(), EVENT_MESSAGE_RECORD_CONT_ID, COL2_X, ROW1_Y, BTN_W, BTN_H,
                                                                     event_detection_record_obj_click, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
                                                                     0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                                     0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                                     0, 130, BTN_W, 60, EVENT_MESSAGE_RECORD_TEXT_ID,
                                                                     lang_str_get(STR_MESSAGE_RECORD), 0xffffff, 0xF91313, LV_TEXT_ALIGN_CENTER, lv_font_normal,
                                                                     25, 0, 130, 130, EVENT_MESSAGE_RECORD_BTN_ID,
                                                                     &icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &icon_focus);
    lv_obj_set_ext_click_area(event_detection_record, 21);
}

static void event_message_record_create(void)
{
    lv_resource_init(icon, ICON_RECORD_DETECTION_RECORD_PNG_OFFSET, ICON_RECORD_DETECTION_RECORD_PNG_SIZE);
    lv_resource_init(icon_focus, ICON_RECORD_DETECTION_RECORD_FOCUS_PNG_OFFSET, ICON_RECORD_DETECTION_RECORD_FOCUS_PNG_SIZE);
    lv_obj_t *event_message_record = lv_common_img_text_btn_create(lv_scr_act_current(), EVENT_DETECTION_RECORD_CONT_ID, COL1_X, ROW2_Y, BTN_W, BTN_H,
                                                                   event_message_record_obj_click, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
                                                                   0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                                   0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                                   0, 130, BTN_W, 60, EVENT_DETECTION_RECORD_TEXT_ID,
                                                                   lang_str_get(STR_MOTION_RECORD), 0xffffff, 0xF91313, LV_TEXT_ALIGN_CENTER, lv_font_normal,
                                                                   25, 0, 130, 130, EVENT_DETECTION_RECORD_BTN_ID,
                                                                   &icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &icon_focus);
    lv_obj_set_ext_click_area(event_message_record, 21);
}

static void event_back_create(void)
{
    lv_resource_init(icon, ICON_COMMON_BACK_PNG_OFFSET, ICON_COMMON_BACK_PNG_SIZE);
  lv_resource_init(icon_focus, ICON_COMMON_BACK_FOCUS_PNG_OFFSET, ICON_COMMON_BACK_FOCUS_PNG_SIZE);
    lv_obj_t *event_back = lv_common_img_btn_create(lv_scr_act_current(), EVENT_BACK_BTN_ID, 929, 505, 67, 67,
                                                    event_back_obj_click, true, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
                                                    0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                    0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                    &icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &icon_focus);
    lv_obj_set_ext_click_area(event_back, 21);
}

/************************** 页面生命周期 **************************/
static void Amy_lv_page_enter(event)
{
    /*背景*/
    lv_common_rect_create(lv_scr_act_current(), 0, 0, SCREEN_W, SCREEN_H, LV_COMMON_BG_COLOR);
    /*call record 通话记录*/
    event_record_call_btn_create();
    /*detection record 检测记录*/
    event_detection_record_create();
    /*message record 消息记录*/
    event_message_record_create();
    /*back 返回*/
    event_back_create();
  Amy_event_cb_register(AMY_EVENT_ID_SD_CHANGE, page_common_tfcard_change_cb);
}

static void Amy_lv_page_quit(event)
{
  Amy_event_cb_register(AMY_EVENT_ID_SD_CHANGE, NULL);
}

Amy_lv_page_create(event);
