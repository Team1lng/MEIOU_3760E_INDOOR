/**
 * @file page_tuya_uuid_register.c
 * @brief 涂鸦UUID序列号注册页面（双次确认 + 数字键盘）
 */
#include "page_tabel.h"
#include "lv_common.h"
#include "page_common.h"
#include "tuya_common.h"
#include "tuya/tuya_uuid_and_key.h"
#include "user_data.h"
#include <stdlib.h>
#include <string.h>

/* =========================== 常量定义 =========================== */
#define TUYA_UUID_REGISTER_INPUT_MAX_LEN 5

/* 页面局部控件ID（避免依赖历史logo页面ID） */
typedef enum
{
  tuya_uuid_register_title_id = 50,
  tuya_uuid_register_tips_id,
  tuya_uuid_register_textarea_id,
  tuya_uuid_register_keyboard_id,
} tuya_uuid_register_obj_id_t;

typedef enum
{
  TUYA_UUID_REGISTER_STEP_FIRST_INPUT = 0,
  TUYA_UUID_REGISTER_STEP_CONFIRM_INPUT = 1,
} tuya_uuid_register_step_t;

typedef struct
{
  tuya_uuid_register_step_t step;
  char first_input[TUYA_UUID_REGISTER_INPUT_MAX_LEN + 1];
} tuya_uuid_register_ctx_t;

/* =========================== 全局状态 =========================== */
static tuya_uuid_register_ctx_t g_tuya_uuid_register_ctx = {0};

/* =========================== 静态函数声明 =========================== */
static void tuya_uuid_register_ui_create_all(void);
static void tuya_uuid_register_ui_refresh_tips(void);
static void tuya_uuid_register_ui_clear_input(void);
static void tuya_uuid_register_ui_msgbox_ok(lv_event_t *e);
static void tuya_uuid_register_ui_msgbox_ok_and_back_standby(lv_event_t *e);
static void tuya_uuid_register_keyboard_event(lv_event_t *e);
static void tuya_uuid_register_submit(void);
static bool tuya_uuid_register_apply_serial(const char *serial_text);
static void tuya_uuid_register_sync_user_data(void);

/* =========================== 静态函数实现 =========================== */
static lv_obj_t *tuya_uuid_register_textarea_get(void)
{
  return lv_obj_get_child_by_id(lv_scr_act_current(), tuya_uuid_register_textarea_id);
}

static void tuya_uuid_register_ui_msgbox_ok(lv_event_t *e)
{
  lv_common_msgbox_del();
}

static void tuya_uuid_register_ui_msgbox_ok_and_back_standby(lv_event_t *e)
{
  lv_common_msgbox_del();
  Amy_lv_page_goto(lv_page(standby), LV_SCR_LOAD_ANIM_NONE, );
}

static void tuya_uuid_register_ui_refresh_tips(void)
{
  lv_obj_t *tips = lv_obj_get_child_by_id(lv_scr_act_current(), tuya_uuid_register_tips_id);
  if (tips == NULL)
  {
    return;
  }

  if (g_tuya_uuid_register_ctx.step == TUYA_UUID_REGISTER_STEP_FIRST_INPUT)
  {
    lv_label_set_text(tips, "Please enter the serial number");
  }
  else
  {
    lv_label_set_text(tips, "Please re-enter the serial number");
  }
}

static void tuya_uuid_register_ui_clear_input(void)
{
  lv_obj_t *ta = tuya_uuid_register_textarea_get();
  if (ta == NULL)
  {
    return;
  }
  lv_textarea_set_text(ta, "");
  lv_textarea_set_cursor_pos(ta, 0);
}

static bool tuya_uuid_register_apply_serial(const char *serial_text)
{
  char *endptr = NULL;
  long serial_index = strtol(serial_text, &endptr, 10);
  if ((endptr == NULL) || (*endptr != '\0') || (serial_index <= 0))
  {
    return false;
  }

  if (!tuya_key_and_key_xls_register((int)serial_index))
  {
    return false;
  }

  tuya_uuid_register_sync_user_data();
  return tuya_sdk_start_from_user_data();
}

static void tuya_uuid_register_sync_user_data(void)
{
  user_data_info *data = user_data_get();
  if (data == NULL)
  {
    return;
  }

  unsigned char uuid[64] = {0};
  unsigned char key[64] = {0};
  if (tuya_uuid_and_key_read(uuid, key))
  {
    strncpy(data->private_cfg.tuya_info.uid, (const char *)uuid, sizeof(data->private_cfg.tuya_info.uid) - 1);
    data->private_cfg.tuya_info.uid[sizeof(data->private_cfg.tuya_info.uid) - 1] = '\0';

    strncpy(data->private_cfg.tuya_info.key, (const char *)key, sizeof(data->private_cfg.tuya_info.key) - 1);
    data->private_cfg.tuya_info.key[sizeof(data->private_cfg.tuya_info.key) - 1] = '\0';
  }

  char serial[32] = {0};
  if (tuya_serial_number_get(serial, sizeof(serial)))
  {
    strncpy(data->private_cfg.tuya_info.qr_info, serial, sizeof(data->private_cfg.tuya_info.qr_info) - 1);
    data->private_cfg.tuya_info.qr_info[sizeof(data->private_cfg.tuya_info.qr_info) - 1] = '\0';
  }

  user_data_save();
}

static void tuya_uuid_register_submit(void)
{
  lv_obj_t *ta = tuya_uuid_register_textarea_get();
  if (ta == NULL)
  {
    return;
  }

  const char *current_input = lv_textarea_get_text(ta);
  if (current_input == NULL || current_input[0] == '\0')
  {
    lv_common_msgbox_create(-1, -1, 460, 232, 42, "The serial number is empty",
                            tuya_uuid_register_ui_msgbox_ok, NULL);
    return;
  }

  if (g_tuya_uuid_register_ctx.step == TUYA_UUID_REGISTER_STEP_FIRST_INPUT)
  {
    strncpy(g_tuya_uuid_register_ctx.first_input, current_input, sizeof(g_tuya_uuid_register_ctx.first_input) - 1);
    g_tuya_uuid_register_ctx.first_input[sizeof(g_tuya_uuid_register_ctx.first_input) - 1] = '\0';
    g_tuya_uuid_register_ctx.step = TUYA_UUID_REGISTER_STEP_CONFIRM_INPUT;
    tuya_uuid_register_ui_clear_input();
    tuya_uuid_register_ui_refresh_tips();
    return;
  }

  if (strcmp(g_tuya_uuid_register_ctx.first_input, current_input) != 0)
  {
    g_tuya_uuid_register_ctx.step = TUYA_UUID_REGISTER_STEP_FIRST_INPUT;
    memset(g_tuya_uuid_register_ctx.first_input, 0, sizeof(g_tuya_uuid_register_ctx.first_input));
    tuya_uuid_register_ui_clear_input();
    tuya_uuid_register_ui_refresh_tips();
    lv_common_msgbox_create(-1, -1, 460, 232, 42, "The serial number is incorrect",
                            tuya_uuid_register_ui_msgbox_ok, NULL);
    return;
  }

  if (!tuya_uuid_register_apply_serial(current_input))
  {
    g_tuya_uuid_register_ctx.step = TUYA_UUID_REGISTER_STEP_FIRST_INPUT;
    memset(g_tuya_uuid_register_ctx.first_input, 0, sizeof(g_tuya_uuid_register_ctx.first_input));
    tuya_uuid_register_ui_clear_input();
    tuya_uuid_register_ui_refresh_tips();
    lv_common_msgbox_create(-1, -1, 460, 232, 42, "Registration failed",
                            tuya_uuid_register_ui_msgbox_ok, NULL);
    return;
  }

  lv_common_msgbox_create(-1, -1, 460, 232, 42, "Registration successful",
                          tuya_uuid_register_ui_msgbox_ok_and_back_standby, NULL);
}

static void tuya_uuid_register_keyboard_event(lv_event_t *e)
{
  if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED)
  {
    return;
  }

  lv_obj_t *kb = lv_event_get_target(e);
  lv_obj_t *ta = tuya_uuid_register_textarea_get();
  if (kb == NULL || ta == NULL)
  {
    return;
  }

  uint32_t btn_id = lv_btnmatrix_get_selected_btn(kb);
  const char *text = lv_btnmatrix_get_btn_text(kb, btn_id);
  if (text == NULL)
  {
    return;
  }

  if (strcmp(text, "OK") == 0)
  {
    tuya_uuid_register_submit();
    return;
  }

  if (strcmp(text, " ") == 0)
  {
    lv_textarea_del_char(ta);
    return;
  }

  if ((text[0] < '0') || (text[0] > '9') || (text[1] != '\0'))
  {
    return;
  }

  const char *current = lv_textarea_get_text(ta);
  if (current != NULL && strlen(current) >= TUYA_UUID_REGISTER_INPUT_MAX_LEN)
  {
    return;
  }
  lv_textarea_add_text(ta, text);
}

static void tuya_uuid_register_ui_create_all(void)
{
  lv_common_text_create(lv_scr_act_current(), tuya_uuid_register_title_id, 0, 27, 1024, 60,
                        NULL, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0,
                        0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                        0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                        "UUID Register", 0xFFFFFF, 0xFFFFFF, LV_TEXT_ALIGN_CENTER, lv_font_large);

  lv_common_text_create(lv_scr_act_current(), tuya_uuid_register_tips_id, 0, 120, 1024, 60,
                        NULL, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0,
                        0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                        0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                        "Please enter the serial number", 0xFFFFFF, 0xFFFFFF, LV_TEXT_ALIGN_CENTER, lv_font_normal);

  lv_obj_t *ta = lv_common_textarea_create(lv_scr_act_current(), tuya_uuid_register_textarea_id, 60, 220, 904, 54,
                                           NULL, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0x101010,
                                           LV_OPA_COVER, 0xFFFFFF, LV_OPA_COVER, 0xFFFFFF,
                                           0, 1, LV_BORDER_SIDE_BOTTOM, LV_OPA_COVER, 0x323237,
                                           0, 1, LV_BORDER_SIDE_BOTTOM, LV_OPA_COVER, 0x323237,
                                           "", 0xFFFFFF, 0xFFFFFF, LV_TEXT_ALIGN_CENTER, lv_font_normal,
                                           TUYA_UUID_REGISTER_INPUT_MAX_LEN, 0, 500);
  lv_obj_add_state(ta, LV_STATE_FOCUSED);
  lv_obj_set_style_anim_time(ta, 500, LV_STATE_FOCUSED | LV_PART_CURSOR);
  lv_textarea_set_one_line(ta, true);

  lv_obj_t *kb = lv_common_number_input_keyboard_create(lv_scr_act_current(), tuya_uuid_register_keyboard_id, 397, 279, 230, 371,
                                                        tuya_uuid_register_keyboard_event, LV_OPA_COVER, 0x808080, LV_OPA_COVER, 0x00a8ff,
                                                        36, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                        36, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                        0xFFFFFF, 0xFFFFFF, LV_TEXT_ALIGN_CENTER, lv_font_large,
                                                        13, 22);
  static const char *btnm_map[] = {
      "1", "2", "3", "\n",
      "4", "5", "6", "\n",
      "7", "8", "9", "\n",
      "OK", "0", " ", "\n", ""};
  lv_btnmatrix_set_map(kb, btnm_map);
}

/* =========================== 页面生命周期 =========================== */
static void Amy_lv_page_enter(tuya_uuid_register)
{
  memset(&g_tuya_uuid_register_ctx, 0, sizeof(g_tuya_uuid_register_ctx));
  g_tuya_uuid_register_ctx.step = TUYA_UUID_REGISTER_STEP_FIRST_INPUT;
  tuya_uuid_register_ui_create_all();
}

static void Amy_lv_page_quit(tuya_uuid_register)
{
  lv_common_msgbox_del();
  memset(&g_tuya_uuid_register_ctx, 0, sizeof(g_tuya_uuid_register_ctx));
}

/* =========================== 页面注册 =========================== */
Amy_lv_page_create(tuya_uuid_register);
