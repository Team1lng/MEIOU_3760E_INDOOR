/**
 * @file page_ota.c
 * @brief OTA固件升级页面
 */
#include "page_tabel.h"
#include "page_common.h"
#include "lv_common.h"
#include "Amy_lvgl.h"
#include "Amy_sdk.h"
#include <sys/reboot.h>
#include "user_standby.h"

#define BAR_MAX_W 400

static void ota_bar_update(lv_obj_t *bg, int progress)
{
	/* bar_fill 挂在 bar_track 下，是 bg 的孙节点，lv_obj_get_child_by_id 不递归查找 */
	lv_obj_t *track = lv_obj_get_child_by_id(bg, ota_page_bar_track_id);
	if (!track)
		return;

	lv_obj_t *fill = lv_obj_get_child_by_id(track, ota_page_bar_fill_id);
	if (!fill)
		return;

	int w = (progress > 100) ? BAR_MAX_W : (progress * BAR_MAX_W / 100);
	if (w < 0)
		w = 0;
	lv_obj_set_style_width(fill, w, LV_PART_MAIN);
}

static void ota_ui_update(lv_obj_t *bg, int state, int progress, int dev_tp)
{
	lv_obj_t *status_label = lv_obj_get_child_by_id(bg, ota_page_status_label_id);
	lv_obj_t *progress_label = lv_obj_get_child_by_id(bg, ota_page_progress_label_id);
	if (!status_label || !progress_label)
		return;

	switch (state)
	{
	case OTA_STATE_DOWNLOADING:
		lv_label_set_text(status_label,
											dev_tp == OTA_DEV_TP_OUTDOOR ? "Downloading door station firmware..." : "Downloading indoor station firmware...");
		ota_bar_update(bg, progress);
		lv_label_set_text_fmt(progress_label, "%d%%", progress);
		break;
	case OTA_STATE_DOWNLOADED:
		lv_label_set_text(status_label,
											dev_tp == OTA_DEV_TP_OUTDOOR ? "Door station firmware download complete" : "Indoor station firmware download complete");
		ota_bar_update(bg, 100);
		lv_label_set_text(progress_label, "100%");
		break;
	case OTA_STATE_FAILED:
		lv_label_set_text(status_label,
											dev_tp == OTA_DEV_TP_OUTDOOR ? "Door station firmware download failed" : "Indoor station firmware download failed");
		lv_label_set_text(progress_label, "");
		break;
	default:
		break;
	}
}

static void ota_indoor_reboot_timer_cb(lv_timer_t *t)
{
	lv_timer_del(t);
	lv_common_msgbox_del();
	sync();
	reboot(RB_AUTOBOOT);
}

static void ota_door_goto_advanced_timer_cb(lv_timer_t *t)
{
	lv_timer_del(t);
	lv_common_msgbox_del();
	page_advanced_set_auto_door_upgrade(true);
	Amy_lv_page_goto(lv_page(advanced), LV_SCR_LOAD_ANIM_NONE, );
}

static void ota_event_cb(void *arg)
{
	ota_event_t *ev = (ota_event_t *)arg;
	lv_obj_t *bg = lv_obj_get_child_by_id(lv_scr_act_current(), ota_page_bg_id);
	if (!bg)
		return;

	if (ev->state == OTA_STATE_DOWNLOADED)
	{
		ota_ui_update(bg, ev->state, ev->progress, ev->dev_tp);

		if (ev->dev_tp == OTA_DEV_TP_OUTDOOR)
		{
			lv_common_msgbox_notify_create(251, 106, 460, 232, 42,
																		 "Door station firmware download complete. About to enter advanced mode for automatic upgrade.");
			lv_timer_create(ota_door_goto_advanced_timer_cb, 1500, NULL);
		}
		else
		{
			lv_common_msgbox_notify_create(251, 106, 460, 232, 42,
																		 "Firmware download complete. About to restart the device to upgrade.");
			lv_timer_create(ota_indoor_reboot_timer_cb, 1500, NULL);
		}
		return;
	}

	ota_ui_update(bg, ev->state, ev->progress, ev->dev_tp);
}

static void Amy_lv_page_enter(ota)
{
	standby_timer_close();

	lv_obj_t *bg = lv_obj_create(lv_scr_act());
	lv_common_style_set_common(bg, ota_page_bg_id, 0, 0, 1024, 600, LV_ALIGN_DEFAULT, LV_PART_MAIN);
	lv_obj_clear_flag(bg, LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLLABLE);
	lv_common_style_set_boader(bg, 0, LV_OPA_TRANSP, 0, LV_BORDER_SIDE_NONE, 0, LV_PART_MAIN);
	lv_common_style_set_event(bg, NULL, false, LV_OPA_COVER, 0x1A1A2E, LV_PART_MAIN,
														LV_OPA_COVER, 0x1A1A2E, LV_STATE_PRESSED);

	/* 标题 */
	lv_common_text_create(bg, -1, 0, 60, 1024, 40,
												NULL, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0,
												0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
												0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
												"Firmware Upgrade", 0xFFFFFF, 0xFFFFFF,
												LV_TEXT_ALIGN_CENTER, lv_font_large);

	/* 进度条轨道 */
	lv_obj_t *bar_track = lv_obj_create(bg);
	lv_common_style_set_common(bar_track, ota_page_bar_track_id, (1024 - BAR_MAX_W) / 2, 270,
														 BAR_MAX_W, 20, LV_ALIGN_DEFAULT, LV_PART_MAIN);
	lv_obj_clear_flag(bar_track, LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLLABLE);
	lv_common_style_set_boader(bar_track, 10, LV_OPA_TRANSP, 0, LV_BORDER_SIDE_NONE, 0, LV_PART_MAIN);
	lv_common_style_set_event(bar_track, NULL, false, LV_OPA_COVER, 0x333355, LV_PART_MAIN,
														LV_OPA_COVER, 0x333355, LV_STATE_PRESSED);

	/* 进度条填充 */
	lv_obj_t *bar_fill = lv_obj_create(bar_track);
	lv_common_style_set_common(bar_fill, ota_page_bar_fill_id, 0, 0, 0, 20,
														 LV_ALIGN_DEFAULT, LV_PART_MAIN);
	lv_obj_clear_flag(bar_fill, LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLLABLE);
	lv_common_style_set_boader(bar_fill, 10, LV_OPA_TRANSP, 0, LV_BORDER_SIDE_NONE, 0, LV_PART_MAIN);
	lv_common_style_set_event(bar_fill, NULL, false, LV_OPA_COVER, 0x00A8FF, LV_PART_MAIN,
														LV_OPA_COVER, 0x00A8FF, LV_STATE_PRESSED);

	/* 进度百分比 */
	lv_common_text_create(bg, ota_page_progress_label_id, 0, 300, 1024, 30,
												NULL, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0,
												0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
												0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
												"0%", 0x00A8FF, 0x00A8FF,
												LV_TEXT_ALIGN_CENTER, lv_font_normal);

	/* 状态文本 */
	lv_common_text_create(bg, ota_page_status_label_id, 0, 340, 1024, 30,
												NULL, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0,
												0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
												0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
												"Downloading firmware", 0xCCCCCC, 0xCCCCCC,
												LV_TEXT_ALIGN_CENTER, lv_font_normal);

	/* 警告文本 */
	lv_common_text_create(bg, ota_page_warning_label_id, 0, 390, 1024, 30,
												NULL, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0,
												0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
												0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
												"Please don't do anything", 0xFF6600, 0xFF6600,
												LV_TEXT_ALIGN_CENTER, lv_font_small);

	Amy_event_cb_register(AMY_EVENT_ID_OTA, ota_event_cb);
}

static void Amy_lv_page_quit(ota)
{
	standby_timer_restart(true, true);
	Amy_event_cb_register(AMY_EVENT_ID_OTA, page_default_ota_event_cb);
}

Amy_lv_page_create(ota);
