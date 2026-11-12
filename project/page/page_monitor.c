#include "page_tabel.h"
#include "lv_common.h"
#include "page_common.h"
#include "../monitor.h"
#include "page_live.h"
#include "user_data.h"
#include "call_api.h"

/************************** 布局常量（2行 x 2列，居中对齐） **************************/
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
static void monitor_door1_obj_click(lv_event_t *ev);
static void monitor_door2_obj_click(lv_event_t *ev);
static void monitor_camera1_obj_click(lv_event_t *ev);
static void monitor_camera2_obj_click(lv_event_t *ev);
static void monitor_back_obj_click(lv_event_t *ev);

static void monitor_door1_create(void);
static void monitor_door2_create(void);
static void monitor_camera1_create(void);
static void monitor_camera2_create(void);
static void monitor_back_create(void);

/************************** 回调实现 **************************/

static void monitor_door1_obj_click(lv_event_t *ev)
{
  if (!user_data_get()->public_cfg.door_device[MON_CHANNEL_DOOR1].device_valid)
    return;

  device_info_attr attr;
  attr.building = user_data_get()->private_cfg.device.attr.building;
  attr.room = 0X00;
  attr.device_type = DEVICE_TYPE_DOOR_ID1;
  sat_call_dial(&attr);
  monitor_channel_set(MON_CHANNEL_DOOR1);
  monitor_status_set(MONITOR_OUTGOING_CONNECTING);
  monitor_device_info_set(&attr);
  Amy_lv_page_goto(lv_page(live), LV_SCR_LOAD_ANIM_NONE, );
}

static void monitor_door2_obj_click(lv_event_t *ev)
{
  if (!user_data_get()->public_cfg.door_device[MON_CHANNEL_DOOR2].device_valid)
    return;

  device_info_attr attr;
  attr.building = user_data_get()->private_cfg.device.attr.building;
  attr.room = 0X00;
  attr.device_type = DEVICE_TYPE_DOOR_ID2;
  sat_call_dial(&attr);
  monitor_channel_set(MON_CHANNEL_DOOR2);
  monitor_status_set(MONITOR_OUTGOING_CONNECTING);
  monitor_device_info_set(&attr);
  Amy_lv_page_goto(lv_page(live), LV_SCR_LOAD_ANIM_NONE, );
}

static void monitor_camera1_obj_click(lv_event_t *ev)
{
  if (monitor_channel_is_valid(MON_CHANNEL_CCTV1) != 1)
    return;

  monitor_channel_set(MON_CHANNEL_CCTV1);
  monitor_status_set(MONITOR_OUTGOING_CONNECTING);
  Amy_lv_page_goto(lv_page(live), LV_SCR_LOAD_ANIM_NONE, );
}

static void monitor_camera2_obj_click(lv_event_t *ev)
{
  if (monitor_channel_is_valid(MON_CHANNEL_CCTV2) != 1)
    return;

  monitor_channel_set(MON_CHANNEL_CCTV2);
  monitor_status_set(MONITOR_OUTGOING_CONNECTING);
  Amy_lv_page_goto(lv_page(live), LV_SCR_LOAD_ANIM_NONE, );
}

static void monitor_back_obj_click(lv_event_t *ev)
{
  Amy_lv_page_goto(lv_page(home), LV_SCR_LOAD_ANIM_NONE, );
}

/************************** 按钮创建 **************************/

static void monitor_door1_create(void)
{
  lv_resource_init(icon, ICON_MONITOR_DOOR1_PNG_OFFSET, ICON_MONITOR_DOOR1_PNG_SIZE);
  lv_resource_init(icon_focus, ICON_MONITOR_DOOR1_FOCUS_PNG_OFFSET, ICON_MONITOR_DOOR1_FOCUS_PNG_SIZE);
  lv_obj_t *monitor_door1 = lv_common_img_text_btn_create(lv_scr_act_current(), MONITOR_DOOR1_CONT_ID, COL1_X, ROW1_Y, BTN_W, BTN_H,
                                                          monitor_door1_obj_click, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
                                                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                          0, 130, BTN_W, 60, MONITOR_DOOR1_TEXT_ID,
                                                          lang_str_get(STR_DOOR1), 0xffffff, 0xF91313, LV_TEXT_ALIGN_CENTER, lv_font_normal,
                                                          25, 0, 130, 130, MONITOR_DOOR1_BTN_ID,
                                                          &icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &icon_focus);
  lv_obj_set_ext_click_area(monitor_door1, 21);
}

static void monitor_door2_create(void)
{
  lv_resource_init(icon, ICON_MONITOR_DOOR2_PNG_OFFSET, ICON_MONITOR_DOOR2_PNG_SIZE);
  lv_resource_init(icon_focus, ICON_MONITOR_DOOR2_FOCUS_PNG_OFFSET, ICON_MONITOR_DOOR2_FOCUS_PNG_SIZE);
  lv_obj_t *monitor_door2 = lv_common_img_text_btn_create(lv_scr_act_current(), MONITOR_DOOR2_CONT_ID, COL2_X, ROW1_Y, BTN_W, BTN_H,
                                                          monitor_door2_obj_click, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
                                                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                          0, 130, BTN_W, 60, MONITOR_DOOR2_TEXT_ID,
                                                          lang_str_get(STR_DOOR2), 0xffffff, 0xF91313, LV_TEXT_ALIGN_CENTER, lv_font_normal,
                                                          25, 0, 130, 130, MONITOR_DOOR2_BTN_ID,
                                                          &icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &icon_focus);
  lv_obj_set_ext_click_area(monitor_door2, 21);
}

static void monitor_camera1_create(void)
{
  lv_resource_init(icon, ICON_MONITOR_CAMERA1_PNG_OFFSET, ICON_MONITOR_CAMERA1_PNG_SIZE);
  lv_resource_init(icon_focus, ICON_MONITOR_CAMERA1_FOCUS_PNG_OFFSET, ICON_MONITOR_CAMERA1_FOCUS_PNG_SIZE);
  lv_obj_t *monitor_camera1 = lv_common_img_text_btn_create(lv_scr_act_current(), MONITOR_CAMERA1_CONT_ID, COL1_X, ROW2_Y, BTN_W, BTN_H,
                                                            monitor_camera1_obj_click, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
                                                            0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                            0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                            0, 130, BTN_W, 60, MONITOR_CAMERA1_TEXT_ID,
                                                            lang_str_get(STR_CAMERA1), 0xffffff, 0xF91313, LV_TEXT_ALIGN_CENTER, lv_font_normal,
                                                            25, 0, 130, 130, MONITOR_CAMERA1_BTN_ID,
                                                            &icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &icon_focus);
  lv_obj_set_ext_click_area(monitor_camera1, 21);
}

static void monitor_camera2_create(void)
{
  lv_resource_init(icon, ICON_MONITOR_CAMERA2_PNG_OFFSET, ICON_MONITOR_CAMERA2_PNG_SIZE);
  lv_resource_init(icon_focus, ICON_MONITOR_CAMERA2_FOCUS_PNG_OFFSET, ICON_MONITOR_CAMERA2_FOCUS_PNG_SIZE);
  lv_obj_t *monitor_camera2 = lv_common_img_text_btn_create(lv_scr_act_current(), MONITOR_CAMERA2_CONT_ID, COL2_X, ROW2_Y, BTN_W, BTN_H,
                                                            monitor_camera2_obj_click, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
                                                            0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                            0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                            0, 130, BTN_W, 60, MONITOR_CAMERA2_TEXT_ID,
                                                            lang_str_get(STR_CAMERA2), 0xffffff, 0xF91313, LV_TEXT_ALIGN_CENTER, lv_font_normal,
                                                            25, 0, 130, 130, MONITOR_CAMERA2_BTN_ID,
                                                            &icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &icon_focus);
  lv_obj_set_ext_click_area(monitor_camera2, 21);
}

static void monitor_back_create(void)
{
  lv_resource_init(icon, ICON_COMMON_BACK_PNG_OFFSET, ICON_COMMON_BACK_PNG_SIZE);
  lv_resource_init(icon_focus, ICON_COMMON_BACK_FOCUS_PNG_OFFSET, ICON_COMMON_BACK_FOCUS_PNG_SIZE);
  lv_obj_t *monitor_back = lv_common_img_btn_create(lv_scr_act_current(), MONITOR_BACK_BTN_ID, 929, 505, 67, 67,
                                                    monitor_back_obj_click, true, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
                                                    0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                    0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                    &icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &icon_focus);
  lv_obj_set_ext_click_area(monitor_back, 21);
}

/************************** 页面生命周期 **************************/
static void Amy_lv_page_enter(monitor)
{
  /*背景*/
  lv_common_rect_create(lv_scr_act_current(), 0, 0, SCREEN_W, SCREEN_H, LV_COMMON_BG_COLOR);
  /*door1 门口机1*/
  monitor_door1_create();
  /*door2 门口机2*/
  monitor_door2_create();
  /*camera1 摄像头1*/
  monitor_camera1_create();
  /*camera2 摄像头2*/
  monitor_camera2_create();
  /*back 返回*/
  monitor_back_create();
  Amy_event_cb_register(AMY_EVENT_ID_SD_CHANGE, page_common_tfcard_change_cb);
}

static void Amy_lv_page_quit(monitor)
{
  Amy_event_cb_register(AMY_EVENT_ID_SD_CHANGE, NULL);
}

Amy_lv_page_create(monitor);
