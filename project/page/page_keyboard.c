#include "page_tabel.h"
#include "lv_common.h"
#include "page_common.h"


/************************** 函数声明 **************************/
static void keyboard_back_obj_click(lv_event_t *ev);

static void keyboard_back_create(void);

/************************** 回调实现 **************************/

static void keyboard_back_obj_click(lv_event_t *ev)
{
  Amy_lv_page_goto(lv_page(home), LV_SCR_LOAD_ANIM_NONE, );
}

/************************** 控件创建 **************************/

static void keyboard_back_create(void)
{
  lv_resource_init(icon, ICON_COMMON_BACK_PNG_OFFSET, ICON_COMMON_BACK_PNG_SIZE);
  lv_resource_init(icon_focus, ICON_COMMON_BACK_FOCUS_PNG_OFFSET, ICON_COMMON_BACK_FOCUS_PNG_SIZE);
  lv_obj_t *back = lv_common_img_btn_create(lv_scr_act_current(), KEYBOARD_BACK_BTN_ID, 25, 25, 60, 60,
                                            keyboard_back_obj_click, true, LV_OPA_70, 0x393939, LV_OPA_70, 0x4D7AFF,
                                            45, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                            45, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                            &icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &icon_focus);
  lv_obj_set_ext_click_area(back, 21);
}

/************************** 页面生命周期 **************************/
static void Amy_lv_page_enter(keyboard)
{
  lv_common_rect_create(lv_scr_act_current(), 0, 0, SCREEN_W, SCREEN_H, LV_COMMON_BG_COLOR);
  keyboard_back_create();
  lv_common_keyboard_create(lv_scr_act_current(), -1, 0, 200, SCREEN_W, 400,
                            NULL, LV_OPA_COVER, 0x002538, LV_OPA_COVER, 0xFC0000,
                            20, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                            20, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                            0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0x002538,
                            0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0xFC0000,
                            0xffffff, 0xffffff, LV_TEXT_ALIGN_CENTER, lv_font_normal,
                            7, 7, 7, 5, 7, 7,
                            LV_OPA_COVER, 0x00131D, LV_OPA_TRANSP, 0);
  Amy_event_cb_register(AMY_EVENT_ID_SD_CHANGE, page_common_tfcard_change_cb);
}

static void Amy_lv_page_quit(keyboard)
{
  Amy_event_cb_register(AMY_EVENT_ID_SD_CHANGE, NULL);
}

Amy_lv_page_create(keyboard);
