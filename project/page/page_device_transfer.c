#include "page_tabel.h"
#include "lv_common.h"
#include "page_common.h"

/************************** 布局常量 **************************/
#define BTN_W 77
#define BTN_H 77
#define BTN_Y 261
#define BTN_GAP 40
#define BTN_MAX 7

/************************** 函数声明 **************************/
static void device_transfer_dev_obj_click(lv_event_t *ev);
static void device_transfer_back_obj_click(lv_event_t *ev);

static void device_transfer_dev_buttons_create(void);
static void device_transfer_back_create(void);

/************************** 回调实现 **************************/

static void device_transfer_dev_obj_click(lv_event_t *ev)
{
  /* TODO: set target device based on ev, then navigate to call page */
  Amy_lv_page_goto(lv_page(call), LV_SCR_LOAD_ANIM_NONE, );
}

static void device_transfer_back_obj_click(lv_event_t *ev)
{
  Amy_lv_page_goto(lv_page(home), LV_SCR_LOAD_ANIM_NONE, );
}

/************************** 控件创建 **************************/

static void device_transfer_create_one_btn(int idx, int x, int icon_offset, int icon_size, int icon_focus_offset, int icon_focus_size)
{
  lv_resource_init(icon, icon_offset, icon_size);
  lv_resource_init(icon_focus, icon_focus_offset, icon_focus_size);
  int id = DEVICE_TRANSFER_DEV_CONT_ID_BASE + idx;
  lv_obj_t *btn = lv_common_img_btn_create(lv_scr_act_current(), id, x, BTN_Y, BTN_W, BTN_H,
                                           device_transfer_dev_obj_click, true, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0x101010,
                                           0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                           0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                           &icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &icon_focus);
  lv_obj_set_ext_click_area(btn, 21);
}

static void device_transfer_dev_buttons_create(void)
{
  /* TODO: query online extension device count, then create buttons dynamically */
  /* For now, create all 7 buttons (6 extensions + ALL) centered horizontally */
  int count = 7;
  int total_w = BTN_W * count + BTN_GAP * (count - 1);
  int x_offset = (SCREEN_W - total_w) / 2;

  int unfocus_icons[] = {
      ICON_HOME_TRANSFER_PNG_OFFSET, ICON_HOME_MONITOR_PNG_OFFSET, ICON_HOME_EVENT_PNG_OFFSET,
      ICON_HOME_MEDIA_PNG_OFFSET, ICON_HOME_SETTING_PNG_OFFSET, ICON_HOME_MODE_AT_HOME_PNG_OFFSET,
      ICON_HOME_LOCK_PNG_OFFSET};
  int unfocus_sizes[] = {
      ICON_HOME_TRANSFER_PNG_SIZE, ICON_HOME_MONITOR_PNG_SIZE, ICON_HOME_EVENT_PNG_SIZE,
      ICON_HOME_MEDIA_PNG_SIZE, ICON_HOME_SETTING_PNG_SIZE, ICON_HOME_MODE_AT_HOME_PNG_SIZE,
      ICON_HOME_LOCK_PNG_SIZE};
  int focus_icons[] = {
      ICON_HOME_TRANSFER_FOCUS_PNG_OFFSET, ICON_HOME_MONITOR_FOCUS_PNG_OFFSET, ICON_HOME_EVENT_FOCUS_PNG_OFFSET,
      ICON_HOME_MEDIA_FOCUS_PNG_OFFSET, ICON_HOME_SETTING_FOCUS_PNG_OFFSET, ICON_HOME_MODE_AT_HOME_FOCUS_PNG_OFFSET,
      ICON_HOME_LOCK_FOCUS_PNG_OFFSET};
  int focus_sizes[] = {
      ICON_HOME_TRANSFER_FOCUS_PNG_SIZE, ICON_HOME_MONITOR_FOCUS_PNG_SIZE, ICON_HOME_EVENT_FOCUS_PNG_SIZE,
      ICON_HOME_MEDIA_FOCUS_PNG_SIZE, ICON_HOME_SETTING_FOCUS_PNG_SIZE, ICON_HOME_MODE_AT_HOME_FOCUS_PNG_SIZE,
      ICON_HOME_LOCK_FOCUS_PNG_SIZE};

  for (int i = 0; i < count; i++)
  {
    int x = x_offset + i * (BTN_W + BTN_GAP);
    device_transfer_create_one_btn(i, x, unfocus_icons[i], unfocus_sizes[i], focus_icons[i], focus_sizes[i]);
  }
}

static void device_transfer_back_create(void)
{
  lv_resource_init(icon, ICON_COMMON_BACK_PNG_OFFSET, ICON_COMMON_BACK_PNG_SIZE);
  lv_obj_t *back = lv_common_img_btn_create(lv_scr_act_current(), DEVICE_TRANSFER_BACK_BTN_ID, 929, 505, 67, 67,
                                            device_transfer_back_obj_click, true, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0x101010,
                                            0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                            0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                            &icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, NULL);
  lv_obj_set_ext_click_area(back, 21);
}

/************************** 页面生命周期 **************************/
static void Amy_lv_page_enter(device_transfer)
{
  lv_common_rect_create(lv_scr_act_current(), 0, 0, SCREEN_W, SCREEN_H, LV_COMMON_BG_COLOR);
  device_transfer_dev_buttons_create();
  device_transfer_back_create();
  Amy_event_cb_register(AMY_EVENT_ID_SD_CHANGE, page_common_tfcard_change_cb);
}

static void Amy_lv_page_quit(device_transfer)
{
  Amy_event_cb_register(AMY_EVENT_ID_SD_CHANGE, NULL);
}

Amy_lv_page_create(device_transfer);
