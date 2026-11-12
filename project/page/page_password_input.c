#include "page_tabel.h"
#include "lv_common.h"
#include "page_common.h"
#include "page_password_input.h"

/************************** 配置 **************************/
static struct
{
  bool title_visible;
  const char *title_text;
  const char *placeholder_text;
  bool password_mode;
  bool show_pwd_toggle;
  const char *default_text;
  lv_event_cb_t back_cb;
  lv_event_cb_t confirm_cb;
  bool pwd_visible;
  uint32_t max_length;
} g_cfg = {
    .title_visible = true,
    .title_text = "Password Input",
    .placeholder_text = "",
    .password_mode = true,
    .show_pwd_toggle = true,
    .default_text = "",
    .back_cb = NULL,
    .confirm_cb = NULL,
    .pwd_visible = false,
    .max_length = 0,
};

void page_password_input_set_title_visible(bool visible) { g_cfg.title_visible = visible; }
void page_password_input_set_title_text(const char *text) { g_cfg.title_text = text; }
void page_password_input_set_placeholder(const char *text) { g_cfg.placeholder_text = text; }
void page_password_input_set_password_mode(bool enabled) { g_cfg.password_mode = enabled; }
void page_password_input_set_pwd_toggle(bool show) { g_cfg.show_pwd_toggle = show; }
void page_password_input_set_back_cb(lv_event_cb_t cb) { g_cfg.back_cb = cb; }
void page_password_input_set_confirm_cb(lv_event_cb_t cb) { g_cfg.confirm_cb = cb; }
void page_password_input_set_max_length(uint32_t len) { g_cfg.max_length = len; }

void page_password_input_set_text(const char *text)
{
  g_cfg.default_text = text;
}

const char *page_password_input_get_text(void)
{
  lv_obj_t *ta = lv_obj_get_child_by_id(lv_scr_act_current(), PASSWORD_INPUT_TA_ID);
  if (ta == NULL)
    return "";
  return lv_textarea_get_text(ta);
}

/* 页面退出后复位配置，避免上次调用方残留的 default_text/password_mode 等
 * 泄漏到下一次进入（如管理员密码框显示上次的 WiFi 密码） */
void password_input_cfg_reset(void)
{
  g_cfg.title_visible = true;
  g_cfg.title_text = "Password Input";
  g_cfg.placeholder_text = "";
  g_cfg.password_mode = true;
  g_cfg.show_pwd_toggle = true;
  g_cfg.default_text = "";
  g_cfg.back_cb = NULL;
  g_cfg.confirm_cb = NULL;
  g_cfg.pwd_visible = false;
  g_cfg.max_length = 0;
}

/************************** 常量 **************************/
#define LV_KEYBOARD_CONFIRM_BTN 39

/************************** 函数声明 **************************/
static void password_input_back_obj_click(lv_event_t *ev);
static void password_input_pwd_mode_obj_click(lv_event_t *ev);
static void password_input_keyboard_confirm(lv_event_t *e);

static void password_input_title_create(void);
static void password_input_page_create(void);
static void password_input_back_create(void);

/************************** 回调实现 **************************/

static void password_input_back_obj_click(lv_event_t *ev)
{
  if (g_cfg.back_cb)
    g_cfg.back_cb(ev);
  else
    Amy_lv_page_goto(lv_page(setting), LV_SCR_LOAD_ANIM_NONE, );
}

static void password_input_pwd_mode_obj_click(lv_event_t *ev)
{
  lv_resource_init(pwd_icon_visible, ICON_KEYBOARD_VISIBLE_PNG_OFFSET, ICON_KEYBOARD_VISIBLE_PNG_SIZE);
  lv_resource_init(pwd_icon_invisible, ICON_KEYBOARD_INVISIBLE_PNG_OFFSET, ICON_KEYBOARD_INVISIBLE_PNG_SIZE);
  lv_obj_t *ta = lv_obj_get_child_by_id(lv_scr_act_current(), PASSWORD_INPUT_TA_ID);
  if (ta == NULL)
    return;

  g_cfg.pwd_visible = !g_cfg.pwd_visible;
  lv_textarea_set_password_mode(ta, !g_cfg.pwd_visible);

  lv_obj_t *btn = lv_event_get_target(ev);
  lv_obj_set_style_bg_img_src(btn, g_cfg.pwd_visible ? &pwd_icon_invisible : &pwd_icon_visible, LV_PART_MAIN);
}

static void password_input_keyboard_confirm(lv_event_t *e)
{
  if (e == NULL)
    return;

  lv_obj_t *keyboard = lv_event_get_current_target(e);
  if (keyboard == NULL)
    return;

  lv_keyboard_mode_t mode = lv_keyboard_get_mode(keyboard);
  int btn_id = lv_btnmatrix_get_selected_btn(keyboard);

  bool is_confirm_btn = ((mode == LV_KEYBOARD_MODE_TEXT_LOWER || mode == LV_KEYBOARD_MODE_TEXT_UPPER || LV_KEYBOARD_MODE_SPECIAL) &&
                         btn_id == LV_KEYBOARD_CONFIRM_BTN);
  if (!is_confirm_btn)
    return;

  if (g_cfg.confirm_cb)
    g_cfg.confirm_cb(e);
  else
    Amy_lv_page_goto(lv_page(admin), LV_SCR_LOAD_ANIM_NONE, );
}

/************************** 控件创建 **************************/

static void password_input_title_create(void)
{
  if (!g_cfg.title_visible)
    return;

  lv_common_text_create(lv_scr_act_current(), PASSWORD_INPUT_TITLE_ID,
                        312, 30, 400, 40,
                        NULL, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0,
                        0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                        0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                        g_cfg.title_text, 0xffffff, 0x02A9FF, LV_TEXT_ALIGN_CENTER, lv_font_normal);
}

static void password_input_page_create(void)
{

  lv_obj_t *txt = lv_common_textarea_create(lv_scr_act_current(), PASSWORD_INPUT_TA_ID, 40, 95, 942, 55,
                                            NULL, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0,
                                            LV_OPA_COVER, 0x4372EB, LV_OPA_COVER, 0x4372EB,
                                            0, 1, LV_BORDER_SIDE_BOTTOM, LV_OPA_COVER, 0X878787,
                                            0, 1, LV_BORDER_SIDE_BOTTOM, LV_OPA_COVER, 0X878787,
                                            g_cfg.default_text, 0xffffff, 0xffffff, LV_TEXT_ALIGN_LEFT, lv_font_normal, 32,
                                            0, 200);
  lv_textarea_set_one_line(txt, true);
  lv_textarea_set_password_mode(txt, g_cfg.password_mode);
  lv_textarea_set_placeholder_text(txt, g_cfg.placeholder_text);
  lv_textarea_set_max_length(txt, g_cfg.max_length);
  lv_obj_set_style_bg_opa(txt, LV_OPA_TRANSP, LV_PART_CURSOR);

  if (g_cfg.show_pwd_toggle)
  {
    lv_resource_init(pwd_icon_visible, ICON_KEYBOARD_VISIBLE_PNG_OFFSET, ICON_KEYBOARD_VISIBLE_PNG_SIZE);
    lv_resource_init(pwd_icon_invisible, ICON_KEYBOARD_INVISIBLE_PNG_OFFSET, ICON_KEYBOARD_INVISIBLE_PNG_SIZE);
    lv_obj_t *pwd_mode = lv_common_img_btn_create(lv_scr_act_current(), -1, 932, 72, 64, 64,
                                                  password_input_pwd_mode_obj_click, true,
                                                  LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0,
                                                  0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                  0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                  &pwd_icon_visible, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, NULL);
    lv_obj_set_ext_click_area(pwd_mode, 15);
    lv_obj_set_style_bg_img_src(pwd_mode, g_cfg.pwd_visible ? &pwd_icon_invisible : &pwd_icon_visible, LV_PART_MAIN);
  }

  lv_obj_t *kb = lv_common_keyboard_create(lv_scr_act_current(), PASSWORD_INPUT_KB_ID, 0, 195, SCREEN_W, 405,
                                           password_input_keyboard_confirm, LV_OPA_COVER, 0x002538, LV_OPA_COVER, 0xFC0000,
                                           0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                           0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                           0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                           0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                           0xffffff, 0xffffff, LV_TEXT_ALIGN_CENTER, lv_font_normal,
                                           7, 7, 7, 5, 7, 7,
                                           LV_OPA_COVER, 0x00131D, LV_OPA_COVER, 0x00131D);

  lv_obj_t *ta = lv_obj_get_child_by_id(lv_scr_act_current(), PASSWORD_INPUT_TA_ID);
  lv_keyboard_set_textarea(kb, ta);
  lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_TEXT_LOWER);
}

static void password_input_back_create(void)
{
  lv_resource_init(icon, ICON_KEYBOARD_BACK_PNG_OFFSET, ICON_KEYBOARD_BACK_PNG_SIZE);
  lv_obj_t *back = lv_common_img_btn_create(lv_scr_act_current(), PASSWORD_INPUT_BACK_BTN_ID, 25, 25, 48, 48,
                                            password_input_back_obj_click, true, LV_OPA_70, 0x393939, LV_OPA_70, 0x4D7AFF,
                                            45, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                            45, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                            &icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, NULL);
  lv_obj_set_ext_click_area(back, 21);
}

/************************** 页面生命周期 **************************/
static void Amy_lv_page_enter(password_input)
{
  lv_common_rect_create(lv_scr_act_current(), 0, 0, SCREEN_W, SCREEN_H, LV_COMMON_BG_COLOR);
  g_cfg.pwd_visible = !g_cfg.password_mode;
  password_input_title_create();
  password_input_back_create();
  password_input_page_create();
  Amy_event_cb_register(AMY_EVENT_ID_SD_CHANGE, page_common_tfcard_change_cb);
}

static void Amy_lv_page_quit(password_input)
{
  // password_input_cfg_reset();
  Amy_event_cb_register(AMY_EVENT_ID_SD_CHANGE, NULL);
}

Amy_lv_page_create(password_input);
