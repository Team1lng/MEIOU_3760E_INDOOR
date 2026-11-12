#include "page_tabel.h"
#include "lv_common.h"
#include "page_common.h"
#include "Amy_tfcard.h"

/************************** 布局常量（1行 x 2列，居中对齐） **************************/
#define BTN_W 180
#define BTN_H 190
#define GAP_H 188
#define CENTER_2 ((1024 - 2 * BTN_W - GAP_H) / 2)
#define COL1_X CENTER_2
#define COL2_X (CENTER_2 + BTN_W + GAP_H)
#define ROW1_Y 113

/************************** 函数声明 **************************/
static void media_photo_obj_click(lv_event_t *ev);
static void media_music_obj_click(lv_event_t *ev);
static void media_back_obj_click(lv_event_t *ev);

static void media_photo_create(void);
static void media_music_create(void);
static void media_back_create(void);

/************************** 回调实现 **************************/

static void media_photo_obj_click(lv_event_t *ev)
{
    Amy_lv_page_goto(lv_page(photo), LV_SCR_LOAD_ANIM_NONE, );
}

static void media_music_obj_click(lv_event_t *ev)
{
    Amy_lv_page_goto(lv_page(music), LV_SCR_LOAD_ANIM_NONE, );
}

static void media_back_obj_click(lv_event_t *ev)
{
    Amy_lv_page_goto(lv_page(home), LV_SCR_LOAD_ANIM_NONE, );
}

static void media_sd_change_cb(void *arg)
{
    if (!(Amy_tfcard_status() & SD_STATE_INSERT))
        media_back_obj_click(NULL);
    page_common_tfcard_change_cb(arg);
}

/************************** 按钮创建 **************************/

static void media_photo_create(void)
{
    lv_resource_init(icon, ICON_MEDIA_PHOTO_PNG_OFFSET, ICON_MEDIA_PHOTO_PNG_SIZE);
    lv_resource_init(icon_focus, ICON_MEDIA_PHOTO_FOCUS_PNG_OFFSET, ICON_MEDIA_PHOTO_FOCUS_PNG_SIZE);
    lv_obj_t *media_photo = lv_common_img_text_btn_create(lv_scr_act_current(), MEDIA_PHOTO_CONT_ID, COL2_X, ROW1_Y, BTN_W, BTN_H,
                                                          media_photo_obj_click, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
                                                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                          0, 130, BTN_W, 60, MEDIA_PHOTO_TEXT_ID,
                                                          lang_str_get(STR_PHOTO), 0xffffff, 0xF91313, LV_TEXT_ALIGN_CENTER, lv_font_normal,
                                                          25, 0, 130, 130, MEDIA_PHOTO_BTN_ID,
                                                          &icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &icon_focus);
    lv_obj_set_ext_click_area(media_photo, 21);
}

static void media_music_create(void)
{
    lv_resource_init(icon, ICON_MEDIA_MUSIC_PNG_OFFSET, ICON_MEDIA_MUSIC_PNG_SIZE);
    lv_resource_init(icon_focus, ICON_MEDIA_MUSIC_FOCUS_PNG_OFFSET, ICON_MEDIA_MUSIC_FOCUS_PNG_SIZE);
    lv_obj_t *media_music = lv_common_img_text_btn_create(lv_scr_act_current(), MEDIA_MUSIC_CONT_ID, COL1_X, ROW1_Y, BTN_W, BTN_H,
                                                          media_music_obj_click, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
                                                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                          0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                          0, 130, BTN_W, 60, MEDIA_MUSIC_TEXT_ID,
                                                          lang_str_get(STR_MUSIC), 0xffffff, 0xF91313, LV_TEXT_ALIGN_CENTER, lv_font_normal,
                                                          25, 0, 130, 130, MEDIA_MUSIC_BTN_ID,
                                                          &icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &icon_focus);
    lv_obj_set_ext_click_area(media_music, 21);
}

static void media_back_create(void)
{
    lv_resource_init(icon, ICON_COMMON_BACK_PNG_OFFSET, ICON_COMMON_BACK_PNG_SIZE);
    lv_resource_init(icon_focus, ICON_COMMON_BACK_FOCUS_PNG_OFFSET, ICON_COMMON_BACK_FOCUS_PNG_SIZE);
    lv_obj_t *media_back = lv_common_img_btn_create(lv_scr_act_current(), MEDIA_BACK_BTN_ID, 929, 505, 67, 67,
                                                    media_back_obj_click, true, LV_OPA_TRANSP, 0x00, LV_OPA_TRANSP, 0x101010,
                                                    0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                    0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                                    &icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &icon_focus);
    lv_obj_set_ext_click_area(media_back, 21);
}

/************************** 页面生命周期 **************************/
static void Amy_lv_page_enter(media)
{
    /*背景*/
    lv_common_rect_create(lv_scr_act_current(), 0, 0, SCREEN_W, SCREEN_H, LV_COMMON_BG_COLOR);
    /*photo 照片*/
    media_photo_create();
    /*music 音乐*/
    media_music_create();
    /*back 返回*/
    media_back_create();
    Amy_event_cb_register(AMY_EVENT_ID_SD_CHANGE, media_sd_change_cb);
}

static void Amy_lv_page_quit(media)
{
    Amy_event_cb_register(AMY_EVENT_ID_SD_CHANGE, NULL);
}

Amy_lv_page_create(media);
