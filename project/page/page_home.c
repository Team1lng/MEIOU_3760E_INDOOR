#include "page_tabel.h"
#include "lv_common.h"
#include "page_common.h"
#include "user_data.h"
#include "tuya/tuya_dp_callback.h"
#include "tuya_common.h"
#include "Amy_tfcard.h"
#include "../monitor.h"
#include "../gpio.h"

/************************** 布局常量（2行 x 4列，对称居中） **************************/
#define BTN_W 180
#define BTN_H 190
#define START_X 87
#define START_Y 113
#define GAP_H 43
#define GAP_V 64
#define ROW1_Y START_Y
#define ROW2_Y (START_Y + BTN_H + GAP_V)
#define COL1_X START_X
#define COL2_X (START_X + BTN_W + GAP_H)
#define COL3_X (START_X + 2 * (BTN_W + GAP_H))
#define COL4_X (START_X + 3 * (BTN_W + GAP_H))

/************************** 页面上下文 **************************/
static struct
{
  bool gate_open_status;
} s_ctx;

/*顶部状态栏刷新定时器*/
static lv_timer_t *s_home_status_timer;

static void home_status_refresh_cb(lv_timer_t *timer)
{
  page_common_status_bar_update();
}

/************************** 函数声明 **************************/
static void home_call_obj_click(lv_event_t *ev);
static void home_monitor_obj_click(lv_event_t *ev);
static void home_event_obj_click(lv_event_t *ev);
static void home_media_obj_click(lv_event_t *ev);
static void home_setting_obj_click(lv_event_t *ev);
static void home_mode_obj_click(lv_event_t *ev);
static void home_mode_obj_display(void);
static void home_gate_obj_click(lv_event_t *ev);
static void home_gate_display(void);
static void home_standby_obj_click(lv_event_t *ev);
static void home_back_obj_click(lv_event_t *ev);

static void home_call_create(void);
static void home_monitor_create(void);
static void home_event_create(void);
static void home_media_create(void);
static void home_setting_create(void);
static void home_mode_create(void);
static void home_gate_create(void);
static void home_standby_create(void);
static void home_back_create(void);

/************************** 回调实现 **************************/

static void home_call_obj_click(lv_event_t *ev)
{
  Amy_lv_page_goto(lv_page(transfer), LV_SCR_LOAD_ANIM_NONE, );
}

static void home_monitor_obj_click(lv_event_t *ev)
{
  Amy_lv_page_goto(lv_page(monitor), LV_SCR_LOAD_ANIM_NONE, );
}

static void home_event_obj_click(lv_event_t *ev)
{
  Amy_lv_page_goto(lv_page(event), LV_SCR_LOAD_ANIM_NONE, );
}

static void home_media_obj_click(lv_event_t *ev)
{
  Amy_lv_page_goto(lv_page(media), LV_SCR_LOAD_ANIM_NONE, );
}

static void home_setting_obj_click(lv_event_t *ev)
{
  Amy_lv_page_goto(lv_page(setting), LV_SCR_LOAD_ANIM_NONE, );
}

static void home_mode_obj_click(lv_event_t *ev)
{
  scene_mode_type_def next;

  switch (user_data_get()->public_cfg.global_func.scene_mode)
  {
  case SCENE_MODE_HOME:
    next = SCENE_MODE_AWAY;
    break;
  case SCENE_MODE_AWAY:
    next = SCENE_MODE_SLEEP;
    break;
  case SCENE_MODE_SLEEP:
  default:
    next = SCENE_MODE_HOME;
    break;
  }

  scene_mode_switch_to(next);
  home_mode_obj_display();

  tuya_dp_at_home_response(next == SCENE_MODE_HOME);
  tuya_dp_away_mode_response(next == SCENE_MODE_AWAY);
  tuya_dp_sleep_mode_response(next == SCENE_MODE_SLEEP);

  if (next == SCENE_MODE_AWAY && !(Amy_tfcard_status() & SD_STATE_INSERT))
  {
    lv_common_msgbox_create(329, 172, 366, 256, 42,
                            lang_str_get(STR_PLEASE_INSTER_SD),
                            NULL, NULL);
  }
}

static void home_gate_release(lv_timer_t *t)
{
  s_ctx.gate_open_status = false;
  home_gate_display();
  lv_timer_del(t);
}

static void home_gate_display(void)
{

  lv_obj_t *cont = lv_obj_get_child_by_id(lv_scr_act_current(), HOME_GATE_CONT_ID);
  if (cont == NULL)
  {
    return;
  }
  lv_obj_t *obj = lv_obj_get_child_by_id(cont, HOME_GATE_BTN_ID);
  if (obj == NULL)
    return;
  if (s_ctx.gate_open_status != true)
  {
    lv_resource_init(icon, ICON_HOME_INDOOR_LOCK_PNG_OFFSET, ICON_HOME_INDOOR_LOCK_PNG_SIZE);
    lv_resource_init(icon_focus, ICON_HOME_INDOOR_LOCK_FOCUS_PNG_OFFSET, ICON_HOME_INDOOR_LOCK_FOCUS_PNG_SIZE);
    lv_obj_set_style_bg_img_src(obj, &icon, LV_PART_MAIN);
    lv_obj_set_style_bg_img_src(obj, &icon_focus, LV_PART_MAIN | LV_STATE_PRESSED);
  }
  else
  {
    lv_resource_init(icon, ICON_HOME_INDOOR_UNLOCK_PNG_OFFSET, ICON_HOME_INDOOR_UNLOCK_PNG_SIZE);
    lv_resource_init(icon_focus, ICON_HOME_INDOOR_UNLOCK_FOCUS_PNG_OFFSET, ICON_HOME_INDOOR_UNLOCK_FOCUS_PNG_SIZE);
    lv_obj_set_style_bg_img_src(obj, &icon, LV_PART_MAIN);
    lv_obj_set_style_bg_img_src(obj, &icon_focus, LV_PART_MAIN | LV_STATE_PRESSED);
  }
}

static void home_gate_obj_click(lv_event_t *ev)
{
  if (s_ctx.gate_open_status == true)
    return;
  sys_private_cfg_t *cfg = &user_data_get()->private_cfg;
  int lock_time = cfg->unlock_delay;
  tuya_ungate2_with_dp();
  s_ctx.gate_open_status = true;
  home_gate_display();
  lv_timer_create(home_gate_release, lock_time * 1000, NULL);
}

static void home_standby_obj_click(lv_event_t *ev)
{
  Amy_lv_page_goto(lv_page(standby), LV_SCR_LOAD_ANIM_NONE, );
}

static void home_back_obj_click(lv_event_t *ev)
{
  Amy_lv_page_goto(lv_page(standby), LV_SCR_LOAD_ANIM_NONE, );
}

/************************** 按钮创建 **************************/

static void home_call_create(void)
{
  lv_resource_init(icon, ICON_HOME_TRANSFER_PNG_OFFSET, ICON_HOME_TRANSFER_PNG_SIZE);
  lv_resource_init(icon_focus, ICON_HOME_TRANSFER_FOCUS_PNG_OFFSET, ICON_HOME_TRANSFER_FOCUS_PNG_SIZE);
  lv_obj_t *home_call = lv_common_img_text_btn_create(lv_scr_act_current(), HOME_CALL_CONT_ID, COL1_X, ROW1_Y, BTN_W, BTN_H,
                                                      home_call_obj_click, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
                                                      0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                      0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                      0, 130, BTN_W, 60, HOME_CALL_TEXT_ID,
                                                      lang_str_get(STR_INTERCOM), 0xffffff, 0xF91313, LV_TEXT_ALIGN_CENTER, lv_font_normal,
                                                      25, 0, 130, 130, HOME_CALL_BTN_ID,
                                                      &icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &icon_focus);
  lv_obj_set_ext_click_area(home_call, 21);
}

static void home_monitor_create(void)
{
  lv_resource_init(icon, ICON_HOME_MONITOR_PNG_OFFSET, ICON_HOME_MONITOR_PNG_SIZE);
  lv_resource_init(icon_focus, ICON_HOME_MONITOR_FOCUS_PNG_OFFSET, ICON_HOME_MONITOR_FOCUS_PNG_SIZE);
  lv_obj_t *home_monitor = lv_common_img_text_btn_create(lv_scr_act_current(), HOME_MONITOR_CONT_ID, COL2_X, ROW1_Y, BTN_W, BTN_H,
                                                         home_monitor_obj_click, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
                                                         0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                         0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                         0, 130, BTN_W, 60, HOME_MONITOR_TEXT_ID,
                                                         lang_str_get(STR_MONITOR), 0xffffff, 0xF91313, LV_TEXT_ALIGN_CENTER, lv_font_normal,
                                                         25, 0, 130, 130, HOME_MONITOR_BTN_ID,
                                                         &icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &icon_focus);
  lv_obj_set_ext_click_area(home_monitor, 21);
}

static void home_event_create(void)
{
  lv_resource_init(icon, ICON_HOME_EVENT_PNG_OFFSET, ICON_HOME_EVENT_PNG_SIZE);
  lv_resource_init(icon_focus, ICON_HOME_EVENT_FOCUS_PNG_OFFSET, ICON_HOME_EVENT_FOCUS_PNG_SIZE);
  lv_obj_t *home_event = lv_common_img_text_btn_create(lv_scr_act_current(), HOME_EVENT_CONT_ID, COL3_X, ROW1_Y, BTN_W, BTN_H,
                                                       home_event_obj_click, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
                                                       0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                       0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                       0, 130, BTN_W, 60, HOME_EVENT_TEXT_ID,
                                                       lang_str_get(STR_EVENT), 0xffffff, 0xF91313, LV_TEXT_ALIGN_CENTER, lv_font_normal,
                                                       25, 0, 130, 130, HOME_EVENT_BTN_ID,
                                                       &icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &icon_focus);
  lv_obj_set_ext_click_area(home_event, 21);
}

static void home_media_create(void)
{
  lv_resource_init(icon, ICON_HOME_MEDIA_PNG_OFFSET, ICON_HOME_MEDIA_PNG_SIZE);
  lv_resource_init(icon_focus, ICON_HOME_MEDIA_FOCUS_PNG_OFFSET, ICON_HOME_MEDIA_FOCUS_PNG_SIZE);
  lv_obj_t *home_media = lv_common_img_text_btn_create(lv_scr_act_current(), HOME_MEDIA_CONT_ID, COL4_X, ROW1_Y, BTN_W, BTN_H,
                                                       home_media_obj_click, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
                                                       0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                       0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                       0, 130, BTN_W, 60, HOME_MEDIA_TEXT_ID,
                                                       lang_str_get(STR_MEDIA), 0xffffff, 0xF91313, LV_TEXT_ALIGN_CENTER, lv_font_normal,
                                                       25, 0, 130, 130, HOME_MEDIA_BTN_ID,
                                                       &icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &icon_focus);
  lv_obj_set_ext_click_area(home_media, 21);
}

static void home_setting_create(void)
{
  lv_resource_init(icon, ICON_HOME_SETTING_PNG_OFFSET, ICON_HOME_SETTING_PNG_SIZE);
  lv_resource_init(icon_focus, ICON_HOME_SETTING_FOCUS_PNG_OFFSET, ICON_HOME_SETTING_FOCUS_PNG_SIZE);
  lv_obj_t *home_setting = lv_common_img_text_btn_create(lv_scr_act_current(), HOME_SETTING_CONT_ID, COL1_X, ROW2_Y, BTN_W, BTN_H,
                                                         home_setting_obj_click, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
                                                         0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                         0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                         0, 130, BTN_W, 60, HOME_SETTING_TEXT_ID,
                                                         lang_str_get(STR_SETTING), 0xffffff, 0xF91313, LV_TEXT_ALIGN_CENTER, lv_font_normal,
                                                         25, 0, 130, 130, HOME_SETTING_BTN_ID,
                                                         &icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &icon_focus);
  lv_obj_set_ext_click_area(home_setting, 21);
}

static void home_mode_obj_display(void)
{
  lv_obj_t *cont = lv_obj_get_child_by_id(lv_scr_act_current(), HOME_MODE_CONT_ID);
  if (cont == NULL)
  {
    return;
  }

  lv_obj_t *obj_text = lv_obj_get_child_by_id(cont, HOME_MODE_TEXT_ID);
  lv_obj_t *obj_icon = lv_obj_get_child_by_id(cont, HOME_MODE_BTN_ID);
  if (obj_text == NULL || obj_icon == NULL)
  {
    return;
  }

  unsigned long offset = 0;
  unsigned long size = 0;
  unsigned long focus_offset = 0;
  unsigned long focus_size = 0;
  const char *string = NULL;
  switch (user_data_get()->public_cfg.global_func.scene_mode)
  {
  case SCENE_MODE_HOME:
    offset = ICON_HOME_MODE_AT_HOME_PNG_OFFSET;
    size = ICON_HOME_MODE_AT_HOME_PNG_SIZE;
    focus_offset = ICON_HOME_MODE_AT_HOME_FOCUS_PNG_OFFSET;
    focus_size = ICON_HOME_MODE_AT_HOME_FOCUS_PNG_SIZE;
    string = lang_str_get(STR_AT_HOME);
    break;
  case SCENE_MODE_AWAY:
    offset = ICON_HOME_MODE_NOT_AT_HOME_PNG_OFFSET;
    size = ICON_HOME_MODE_NOT_AT_HOME_PNG_SIZE;
    focus_offset = ICON_HOME_MODE_NOT_AT_HOME_FOCUS_PNG_OFFSET;
    focus_size = ICON_HOME_MODE_NOT_AT_HOME_FOCUS_PNG_SIZE;
    string = lang_str_get(STR_NOT_AT_HOME);
    break;
  case SCENE_MODE_SLEEP:
    offset = ICON_HOME_MODE_DORMANT_PNG_OFFSET;
    size = ICON_HOME_MODE_DORMANT_PNG_SIZE;
    focus_offset = ICON_HOME_MODE_DORMANT_FOCUS_PNG_OFFSET;
    focus_size = ICON_HOME_MODE_DORMANT_FOCUS_PNG_SIZE;
    string = lang_str_get(STR_DORMANT);
    break;
  default:
    return;
  }

  lv_label_set_text(obj_text, string);
  lv_resource_init(icon, offset, size);
  lv_obj_set_style_bg_img_src(obj_icon, &icon, LV_PART_MAIN);
  lv_resource_init(icon_focus, focus_offset, focus_size);
  lv_obj_set_style_bg_img_src(obj_icon, &icon_focus, LV_STATE_PRESSED);
}

static void home_mode_create(void)
{
  lv_resource_init(icon, ICON_HOME_MODE_AT_HOME_PNG_OFFSET, ICON_HOME_MODE_AT_HOME_PNG_SIZE);
  lv_resource_init(icon_focus, ICON_HOME_MODE_AT_HOME_FOCUS_PNG_OFFSET, ICON_HOME_MODE_AT_HOME_FOCUS_PNG_SIZE);
  lv_obj_t *home_mode = lv_common_img_text_btn_create(lv_scr_act_current(), HOME_MODE_CONT_ID, COL2_X, ROW2_Y, BTN_W, BTN_H,
                                                      home_mode_obj_click, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
                                                      0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                      0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                      0, 130, BTN_W, 60, HOME_MODE_TEXT_ID,
                                                      lang_str_get(STR_AT_HOME), 0xffffff, 0xF91313, LV_TEXT_ALIGN_CENTER, lv_font_normal,
                                                      25, 0, 130, 130, HOME_MODE_BTN_ID,
                                                      &icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &icon_focus);
  lv_obj_set_ext_click_area(home_mode, 21);
  home_mode_obj_display();
}

static void home_gate_create(void)
{
  lv_resource_init(icon, ICON_HOME_INDOOR_LOCK_PNG_OFFSET, ICON_HOME_INDOOR_LOCK_PNG_SIZE);
  lv_resource_init(icon_focus, ICON_HOME_INDOOR_LOCK_FOCUS_PNG_OFFSET, ICON_HOME_INDOOR_LOCK_FOCUS_PNG_SIZE);
  lv_obj_t *home_gate = lv_common_img_text_btn_create(lv_scr_act_current(), HOME_GATE_CONT_ID, COL3_X, ROW2_Y, BTN_W, BTN_H,
                                                      home_gate_obj_click, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
                                                      0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                      0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                      0, 130, BTN_W, 60, HOME_GATE_TEXT_ID,
                                                      lang_str_get(STR_GATE2), 0xffffff, 0xF91313, LV_TEXT_ALIGN_CENTER, lv_font_normal,
                                                      25, 0, 130, 130, HOME_GATE_BTN_ID,
                                                      &icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &icon_focus);
  lv_obj_set_ext_click_area(home_gate, 21);
  home_gate_display();
}

static void home_standby_create(void)
{
  lv_resource_init(icon, ICON_HOME_STANDBY_PNG_OFFSET, ICON_HOME_STANDBY_PNG_SIZE);
  lv_resource_init(icon_focus, ICON_HOME_STANDBY_FOCUS_PNG_OFFSET, ICON_HOME_STANDBY_FOCUS_PNG_SIZE);
  lv_obj_t *home_standby = lv_common_img_text_btn_create(lv_scr_act_current(), HOME_STANDBY_CONT_ID, COL4_X, ROW2_Y, BTN_W, BTN_H,
                                                         home_standby_obj_click, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
                                                         0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                         0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                         0, 130, BTN_W, 60, HOME_STANDBY_TEXT_ID,
                                                         lang_str_get(STR_STANDBY), 0xffffff, 0xF91313, LV_TEXT_ALIGN_CENTER, lv_font_normal,
                                                         25, 0, 130, 130, HOME_STANDBY_BTN_ID,
                                                         &icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &icon_focus);
  lv_obj_set_ext_click_area(home_standby, 21);
}

static void home_back_create(void)
{
  lv_resource_init(icon, ICON_COMMON_BACK_PNG_OFFSET, ICON_COMMON_BACK_PNG_SIZE);
  lv_resource_init(icon_focus, ICON_COMMON_BACK_FOCUS_PNG_OFFSET, ICON_COMMON_BACK_FOCUS_PNG_SIZE);
  lv_obj_t *home_back = lv_common_img_btn_create(lv_scr_act_current(), HOME_BACK_BTN_ID, 929, 505, 67, 67,
                                                 home_back_obj_click, true, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
                                                 0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                 0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                 &icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &icon_focus);
  lv_obj_set_ext_click_area(home_back, 21);
}

/************************** 页面生命周期 **************************/
static void Amy_lv_page_enter(home)
{
  /*背景*/
  lv_common_rect_create(lv_scr_act_current(), 0, 0, SCREEN_W, SCREEN_H, LV_COMMON_BG_COLOR);
  /*顶部状态栏：门口机/WiFi/涂鸦云在线状态*/
  page_common_status_bar_create(lv_scr_act_current());
  s_home_status_timer = lv_timer_create(home_status_refresh_cb, 1000, NULL);
  /*call 呼叫*/
  home_call_create();
  /*monitor 监视*/
  home_monitor_create();
  /*event 事件*/
  home_event_create();
  /*media 媒体*/
  home_media_create();
  /*setting 设置*/
  home_setting_create();
  /*mode 模式*/
  home_mode_create();
  /*gate 室内机*/
  home_gate_create();
  /*standby 待机*/
  home_standby_create();
  /*back 返回*/
  home_back_create();
  Amy_event_cb_register(AMY_EVENT_ID_SD_CHANGE, page_common_tfcard_change_cb);
  /* 设备冲突提示仅在主界面弹出，避免在熄屏待机等其他页面出现不可见弹窗 */
  Amy_event_cb_register(AMY_EVENT_ID_DEVICE_CONFLICT, page_default_device_conflict_event_cb);
}

static void Amy_lv_page_quit(home)
{
  if (s_home_status_timer != NULL)
  {
    lv_timer_del(s_home_status_timer);
    s_home_status_timer = NULL;
  }
  Amy_event_cb_register(AMY_EVENT_ID_SD_CHANGE, NULL);
  Amy_event_cb_register(AMY_EVENT_ID_DEVICE_CONFLICT, NULL);
}

Amy_lv_page_create(home);
