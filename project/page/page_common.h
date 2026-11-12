#ifndef _PAGE_COMMON_H_
#define _PAGE_COMMON_H_
#include "lvgl/lvgl.h"
#include <stdbool.h>
#include <time.h>
#include "Amy_media_mgr.h"

/* 消息留言记录模式（复用 MEDIA_MODE_CUSTOM1） */
#define MEDIA_MODE_MESSAGE MEDIA_MODE_CUSTOM1

/* 媒体记录类型（event 页面入口决定打开哪种数据库） */
typedef enum
{
  MEDIA_RECORD_TYPE_CALL,      /* 通话记录 */
  MEDIA_RECORD_TYPE_MESSAGE,   /* 消息记录 */
  MEDIA_RECORD_TYPE_DETECTION, /* 检测记录 */
} media_record_type_t;

extern media_record_type_t g_media_record_type;

void page_common_door_status_init(void);

/* 门口机在线状态查询接口（禁止直接访问内部状态） */
bool page_common_door_online_get(int index);
const char *page_common_door_version_get(int index);

/* 共享顶部状态栏：创建图标 / 刷新在线状态+右对齐 */
void page_common_status_bar_create(lv_obj_t *parent);
void page_common_status_bar_update(void);

/* page_ota：门口机固件 OTA 下载完成后，设置进入高级页自动开始门口机升级 */
void page_advanced_set_auto_door_upgrade(bool enable);

/* 数据管理上下文：告知 data_list 页面当前显示的数据类型 */
typedef enum
{
  DATA_MANAGE_TYPE_NONE = 0,
  DATA_MANAGE_TYPE_CARD,
  DATA_MANAGE_TYPE_FINGER,
  DATA_MANAGE_TYPE_PASSWORD,
} data_manage_type_t;
extern data_manage_type_t g_data_manage_type;

/* 全局媒体文件浏览上下文（页面间传递当前选中的文件信息） */
typedef struct
{
  Amy_media_context ctx;
  media_file_info_t info;
  char path[128];
} page_media_browse_ctx_t;
extern page_media_browse_ctx_t g_media_browse;

/* 文件管理器照片浏览：当前照片索引（page_photo → page_photo_play 传递） */
extern int g_fm_photo_index;

/* 铃声设置上下文：当前选择的门口机（0 = Door 1, 1 = Door 2） */
extern int g_ring_door;

typedef enum
{ // 通用控件的专属ID
  LV_COMMON_MSGBOX_BG = 1000,
  LV_COMMON_MSGBOX_CONT,
  LV_COMMON_MSGBOX_TITLE,

  LV_COMMON_MSGBOX_CANCEL,
  LV_COMMON_MSGBOX_CONFIRM,
  LV_COMMON_MSGBOX_CONFIRM_ICON,
  LV_COMMON_MSGBOX_PROGRESS_BAR,
  LV_COMMON_MSGBOX_PROGRESS_LABEL,
  LV_COMMON_OPTIONBOX_CONT,
  LV_COMMON_OPTIONBOX_TITLE,
  LV_COMMON_OPTIONBOX_CANCEL,
  LV_COMMON_OPTIONBOX_CONFIRM,
  LV_COMMON_SPINNER_BG,
  LV_COMMON_SPINNER_SPINNER,
  LV_COMMON_SPINNER_TEXT,

  /* 待处理通话列表和状态提示 */
  lv_common_call_list_id,
  lv_common_call_state_label_id,

  STATUS_BAR_DOOR1_ICON_ID,
  STATUS_BAR_DOOR2_ICON_ID,
  STATUS_BAR_TUYA_CLOUD_ICON_ID,
  STATUS_BAR_WIFI_ICON_ID,
} page_common_id;

typedef enum
{
  /* home界面 - 呼叫 */
  HOME_CALL_CONT_ID,
  HOME_CALL_TEXT_ID,
  HOME_CALL_BTN_ID,
  /* home界面 - 事件 */
  HOME_EVENT_CONT_ID,
  HOME_EVENT_TEXT_ID,
  HOME_EVENT_BTN_ID,
  /* home界面 - 门口机 */
  HOME_GATE_CONT_ID,
  HOME_GATE_TEXT_ID,
  HOME_GATE_BTN_ID,
  /* home界面 - 媒体 */
  HOME_MEDIA_CONT_ID,
  HOME_MEDIA_TEXT_ID,
  HOME_MEDIA_BTN_ID,
  /* home界面 - 模式 */
  HOME_MODE_CONT_ID,
  HOME_MODE_TEXT_ID,
  HOME_MODE_BTN_ID,
  /* home界面 - 监视 */
  HOME_MONITOR_CONT_ID,
  HOME_MONITOR_TEXT_ID,
  HOME_MONITOR_BTN_ID,
  /* home界面 - 设置 */
  HOME_SETTING_CONT_ID,
  HOME_SETTING_TEXT_ID,
  HOME_SETTING_BTN_ID,
  /* home界面 - 待机 */
  HOME_STANDBY_CONT_ID,
  HOME_STANDBY_TEXT_ID,
  HOME_STANDBY_BTN_ID,
  /* home界面 - 返回 */
  HOME_BACK_BTN_ID,
} page_home_id;

typedef enum
{
  /* setting界面 - 门口机设置 */
  SETTING_DOOR_CONT_ID,
  SETTING_DOOR_TEXT_ID,
  SETTING_DOOR_BTN_ID,
  /* setting界面 - 网络设置 */
  SETTING_NETWORK_CONT_ID,
  SETTING_NETWORK_TEXT_ID,
  SETTING_NETWORK_BTN_ID,
  /* setting界面 - 摄像头设置 */
  SETTING_CAMERA_CONT_ID,
  SETTING_CAMERA_TEXT_ID,
  SETTING_CAMERA_BTN_ID,
  /* setting界面 - 场景设置 */
  SETTING_SCENE_CONT_ID,
  SETTING_SCENE_TEXT_ID,
  SETTING_SCENE_BTN_ID,
  /* setting界面 - 系统设置 */
  SETTING_SYSTEM_CONT_ID,
  SETTING_SYSTEM_TEXT_ID,
  SETTING_SYSTEM_BTN_ID,
  /* setting界面 - 系统信息 */
  SETTING_SYSTEM_INFO_CONT_ID,
  SETTING_SYSTEM_INFO_TEXT_ID,
  SETTING_SYSTEM_INFO_BTN_ID,
  /* setting界面 - 高级设置 */
  SETTING_ADVANCED_CONT_ID,
  SETTING_ADVANCED_TEXT_ID,
  SETTING_ADVANCED_BTN_ID,
  /* setting界面 - 返回 */
  SETTING_BACK_BTN_ID,
} page_setting_id;

typedef enum
{
  /* event界面 - 通话记录 */
  EVENT_RECORD_CALL_CONT_ID,
  EVENT_RECORD_CALL_TEXT_ID,
  EVENT_RECORD_CALL_BTN_ID,
  /* event界面 - 检测记录 */
  EVENT_DETECTION_RECORD_CONT_ID,
  EVENT_DETECTION_RECORD_TEXT_ID,
  EVENT_DETECTION_RECORD_BTN_ID,
  /* event界面 - 消息记录 */
  EVENT_MESSAGE_RECORD_CONT_ID,
  EVENT_MESSAGE_RECORD_TEXT_ID,
  EVENT_MESSAGE_RECORD_BTN_ID,
  /* event界面 - 返回 */
  EVENT_BACK_BTN_ID,
} page_event_id;

typedef enum
{
  /* media界面 - 照片 */
  MEDIA_PHOTO_CONT_ID,
  MEDIA_PHOTO_TEXT_ID,
  MEDIA_PHOTO_BTN_ID,
  /* media界面 - 音乐 */
  MEDIA_MUSIC_CONT_ID,
  MEDIA_MUSIC_TEXT_ID,
  MEDIA_MUSIC_BTN_ID,
  /* media界面 - 返回 */
  MEDIA_BACK_BTN_ID,
} page_media_id;

typedef enum
{
  /* monitor界面 - 门口机1 */
  MONITOR_DOOR1_CONT_ID,
  MONITOR_DOOR1_TEXT_ID,
  MONITOR_DOOR1_BTN_ID,
  /* monitor界面 - 门口机2 */
  MONITOR_DOOR2_CONT_ID,
  MONITOR_DOOR2_TEXT_ID,
  MONITOR_DOOR2_BTN_ID,
  /* monitor界面 - 摄像头1 */
  MONITOR_CAMERA1_CONT_ID,
  MONITOR_CAMERA1_TEXT_ID,
  MONITOR_CAMERA1_BTN_ID,
  /* monitor界面 - 摄像头2 */
  MONITOR_CAMERA2_CONT_ID,
  MONITOR_CAMERA2_TEXT_ID,
  MONITOR_CAMERA2_BTN_ID,
  /* monitor界面 - 返回 */
  MONITOR_BACK_BTN_ID,
} page_monitor_id;

typedef enum
{
  /* live界面 - 顶部容器 */
  monitor_top_cont_id,
  monitor_channel_id,
  monitor_date_id,
  monitor_timeout_id,
  monitor_sd_id,
  monitor_recording_time_icon_id,
  monitor_recording_time_label_id,
  monitor_tuya_state_label_id,
  /* live界面 - 底部操作按钮 */
  monitor_channel_switch_id,
  monitor_snapshot_id,
  monitor_video_record_id,
  monitor_gate_open_id,
  monitor_door_open_id,
  monitor_door2_open_id,
  monitor_talk_volume_id,
  monitor_adjust_id,
  monitor_talk_id,
  monitor_hangup_id,
  /* live界面 - 音量滑块 */
  monitor_volume_slider_id,
  /* live界面 - 调节设置面板 */
  monitor_adjust_setting_cont_id,
  monitor_adjust_door_sub_id,
  monitor_adjust_door_add_id,
  monitor_adjust_indoor_sub_id,
  monitor_adjust_indoor_add_id,
  monitor_adjust_brightness_sub_id,
  monitor_adjust_brightness_add_id,
} page_live_id;

typedef enum
{
  /* tuya界面 - 顶部容器 */
  tuya_top_cont_id,
  tuya_channel_id,
  tuya_date_id,
  /* tuya界面 - 中间提示 */
  tuya_center_label_id,
} page_tuya_id;

typedef enum
{
  /* admin界面 */
  ADMIN_TITLE_ICON_ID,
  ADMIN_TITLE_ID,
  ADMIN_ROW1_CONT_ID,
  ADMIN_ROW1_LABEL1_ID,
  ADMIN_ROW1_LABEL2_ID,
  ADMIN_ROW2_CONT_ID,
  ADMIN_ROW2_LABEL1_ID,
  ADMIN_ROW2_LABEL2_ID,
  ADMIN_ROW3_CONT_ID,
  ADMIN_ROW3_LABEL1_ID,
  ADMIN_ROW3_LABEL2_ID,
  ADMIN_ROW4_CONT_ID,
  ADMIN_ROW4_LABEL1_ID,
  ADMIN_ROW4_LABEL2_ID,
  ADMIN_ROW5_CONT_ID,
  ADMIN_ROW5_LABEL1_ID,
  ADMIN_ROW5_LABEL2_ID,
  ADMIN_ROW6_CONT_ID,
  ADMIN_ROW6_LABEL1_ID,
  ADMIN_ROW6_LABEL2_ID,
  ADMIN_BACK_BTN_ID,
} page_admin_id;

typedef enum
{
  /* advanced界面 */
  ADVANCED_TITLE_ICON_ID,
  ADVANCED_TITLE_ID,
  ADVANCED_ROW1_CONT_ID,
  ADVANCED_ROW1_LABEL1_ID,
  ADVANCED_ROW1_LABEL2_ID,
  ADVANCED_ROW2_CONT_ID,
  ADVANCED_ROW2_LABEL1_ID,
  ADVANCED_ROW2_LABEL2_ID,
  ADVANCED_ROW3_CONT_ID,
  ADVANCED_ROW3_LABEL1_ID,
  ADVANCED_ROW3_LABEL2_ID,
  ADVANCED_ROW4_CONT_ID,
  ADVANCED_ROW4_LABEL1_ID,
  ADVANCED_ROW4_LABEL2_ID,
  ADVANCED_ROW5_CONT_ID,
  ADVANCED_ROW5_LABEL1_ID,
  ADVANCED_ROW5_LABEL2_ID,
  ADVANCED_ROW6_CONT_ID,
  ADVANCED_ROW6_LABEL1_ID,
  ADVANCED_ROW6_LABEL2_ID,
  ADVANCED_ROW7_CONT_ID,
  ADVANCED_ROW7_LABEL1_ID,
  ADVANCED_ROW7_LABEL2_ID,
  ADVANCED_BACK_BTN_ID,
} page_advanced_id;

typedef enum
{
  /* event_record界面 - 左侧导航栏 */
  EVENT_RECORD_SIDEBAR_CONT_ID,
  EVENT_RECORD_SIDEBAR_ICON_ID,
  EVENT_RECORD_SIDEBAR_TEXT_ID,
  /* event_record界面 - 右侧列表面板 */
  EVENT_RECORD_LIST_CONT_ID,
  EVENT_RECORD_LIST_TITLE_ID,
  EVENT_RECORD_LIST_PAGE_ID,
  /* event_record界面 - 动态记录条目（滚动列表，最多50项）
   * Container: BASE_ID + 0..49   Label1: LABEL1_BASE_ID + 0..49   Label2: LABEL2_BASE_ID + 0..49 */
  EVENT_RECORD_ITEM_BASE_ID,
  EVENT_RECORD_ITEM_LABEL1_BASE_ID = EVENT_RECORD_ITEM_BASE_ID + 50,
  EVENT_RECORD_ITEM_LABEL2_BASE_ID = EVENT_RECORD_ITEM_LABEL1_BASE_ID + 50,
  EVENT_RECORD_ITEM_IMG_BASE_ID = EVENT_RECORD_ITEM_LABEL2_BASE_ID + 50,
  /* event_record界面 - 记录条目 */
  EVENT_RECORD_ITEM1_CONT_ID,
  EVENT_RECORD_ITEM1_LABEL1_ID,
  EVENT_RECORD_ITEM1_LABEL2_ID,
  EVENT_RECORD_ITEM2_CONT_ID,
  EVENT_RECORD_ITEM2_LABEL1_ID,
  EVENT_RECORD_ITEM2_LABEL2_ID,
  EVENT_RECORD_ITEM3_CONT_ID,
  EVENT_RECORD_ITEM3_LABEL1_ID,
  EVENT_RECORD_ITEM3_LABEL2_ID,
  EVENT_RECORD_ITEM4_CONT_ID,
  EVENT_RECORD_ITEM4_LABEL1_ID,
  EVENT_RECORD_ITEM4_LABEL2_ID,
  EVENT_RECORD_ITEM5_CONT_ID,
  EVENT_RECORD_ITEM5_LABEL1_ID,
  EVENT_RECORD_ITEM5_LABEL2_ID,
  /* event_record界面 - 翻页 */
  EVENT_RECORD_PREV_BTN_ID,
  EVENT_RECORD_NEXT_BTN_ID,
  /* event_record界面 - 返回 */
  EVENT_RECORD_BACK_BTN_ID,
  /* event_record界面 - 列表容器 */
  EVENT_RECORD_LIST_CONTAINER_ID,
  /* event_record界面 - 操作弹窗 */
  EVENT_RECORD_POPUP_CONT_ID,
  EVENT_RECORD_POPUP_OPEN_BTN_ID,
  EVENT_RECORD_POPUP_DELETE_BTN_ID,
  EVENT_RECORD_POPUP_DELETE_ALL_BTN_ID,
  EVENT_RECORD_POPUP_CLOSE_BTN_ID,
} page_event_record_id;

typedef enum
{
  /* music界面 */
  MUSIC_TITLE_ICON_ID,
  MUSIC_TITLE_ID,
  MUSIC_ROW1_CONT_ID,
  MUSIC_ROW1_LABEL1_ID,
  MUSIC_ROW1_LABEL2_ID,
  MUSIC_ROW2_CONT_ID,
  MUSIC_ROW2_LABEL1_ID,
  MUSIC_ROW2_LABEL2_ID,
  MUSIC_ROW3_CONT_ID,
  MUSIC_ROW3_LABEL1_ID,
  MUSIC_ROW3_LABEL2_ID,
  MUSIC_ROW4_CONT_ID,
  MUSIC_ROW4_LABEL1_ID,
  MUSIC_ROW4_LABEL2_ID,
  MUSIC_NO_FILE_ID,
  MUSIC_BACK_BTN_ID,
  /* music界面 - 左侧导航栏 */
  MUSIC_SIDEBAR_CONT_ID,
  MUSIC_SIDEBAR_ICON_ID,
  MUSIC_SIDEBAR_TEXT_ID,
  /* music界面 - 右侧列表面板 */
  MUSIC_LIST_CONT_ID,
  MUSIC_LIST_TITLE_ID,
  MUSIC_LIST_PAGE_ID,
  MUSIC_LIST_DIVIDER_ID,
  /* music界面 - 动态列表条目（滚动列表）
   * Container: BASE_ID + 0..49   Label1: LABEL1_BASE_ID + 0..49   Label2: LABEL2_BASE_ID + 0..49 */
  MUSIC_ITEM_BASE_ID,
  MUSIC_ITEM_LABEL1_BASE_ID = MUSIC_ITEM_BASE_ID + 50,
  MUSIC_ITEM_LABEL2_BASE_ID = MUSIC_ITEM_LABEL1_BASE_ID + 50,
  /* music界面 - 循环播放按钮 */
  MUSIC_LOOP_BTN_ID,
  /* music界面 - 列表容器 */
  MUSIC_LIST_CONTAINER_ID,
  /* music界面 - 操作弹窗 */
  MUSIC_POPUP_CONT_ID,
} page_music_id;

typedef enum
{
  /* photo界面 - 左侧导航栏 */
  PHOTO_SIDEBAR_CONT_ID,
  PHOTO_SIDEBAR_ICON_ID,
  PHOTO_SIDEBAR_TEXT_ID,
  /* photo界面 - 右侧列表面板 */
  PHOTO_LIST_CONT_ID,
  PHOTO_LIST_TITLE_ID,
  PHOTO_LIST_PAGE_ID,
  /* photo界面 - 动态列表条目（滚动列表）
   * Container: BASE_ID + 0..49   Label1: LABEL1_BASE_ID + 0..49   Label2: LABEL2_BASE_ID + 0..49 */
  PHOTO_ITEM_BASE_ID,
  PHOTO_ITEM_LABEL1_BASE_ID = PHOTO_ITEM_BASE_ID + 50,
  PHOTO_ITEM_LABEL2_BASE_ID = PHOTO_ITEM_LABEL1_BASE_ID + 50,
  /* photo界面 - 列表容器 */
  PHOTO_LIST_CONTAINER_ID,
  /* photo界面 - 返回 */
  PHOTO_BACK_BTN_ID,
  /* photo界面 - 操作弹窗 */
  PHOTO_POPUP_CONT_ID,
} page_photo_id;

typedef enum
{
  /* music_play界面 */
  MUSIC_PLAY_TITLE_ID,
  MUSIC_PLAY_VINYL_CONT_ID,
  MUSIC_PLAY_ALBUM_ART_ID,
  MUSIC_PLAY_SONG_TITLE_ID,
  MUSIC_PLAY_ARTIST_ID,
  MUSIC_PLAY_ALBUM_ID,
  MUSIC_PLAY_TRACK_LABEL_ID,
  MUSIC_PLAY_PROGRESS_CONT_ID,
  MUSIC_PLAY_PROGRESS_SLIDER_ID,
  MUSIC_PLAY_CUR_TIME_ID,
  MUSIC_PLAY_TOTAL_TIME_ID,
  MUSIC_PLAY_PREV_BTN_ID,
  MUSIC_PLAY_PLAY_BTN_ID,
  MUSIC_PLAY_NEXT_BTN_ID,
  MUSIC_PLAY_VOL_CONT_ID,
  MUSIC_PLAY_VOL_SLIDER_ID,
  MUSIC_PLAY_VOL_MINUS_BTN_ID,
  MUSIC_PLAY_VOL_LABEL_ID,
  MUSIC_PLAY_VOL_PLUS_BTN_ID,
  MUSIC_PLAY_BACK_BTN_ID,
  /* music_play界面 - 循环播放按钮 */
  MUSIC_PLAY_LOOP_BTN_ID,
} page_music_play_id;

typedef enum
{
  /* photo_play界面 */
  PHOTO_PLAY_PREV_BTN_ID,
  PHOTO_PLAY_PAUSE_BTN_ID,
  PHOTO_PLAY_NEXT_BTN_ID,
  PHOTO_PLAY_BACK_BTN_ID,
} page_photo_play_id;

typedef enum
{
  /* network界面 */
  NETWORK_TITLE_ICON_ID,
  NETWORK_TITLE_ID,
  NETWORK_TAB1_BTN_ID,
  NETWORK_TAB2_BTN_ID,
  NETWORK_TAB3_BTN_ID,
  NETWORK_TAB4_BTN_ID,
  NETWORK_ROW1_CONT_ID,
  NETWORK_ROW1_LABEL1_ID,
  NETWORK_ROW1_LABEL2_ID,
  NETWORK_ROW2_CONT_ID,
  NETWORK_ROW2_LABEL1_ID,
  NETWORK_ROW2_LABEL2_ID,
  NETWORK_ROW3_CONT_ID,
  NETWORK_ROW3_LABEL1_ID,
  NETWORK_ROW3_LABEL2_ID,
  NETWORK_ROW4_CONT_ID,
  NETWORK_ROW4_LABEL1_ID,
  NETWORK_ROW4_LABEL2_ID,
  NETWORK_ROW5_CONT_ID,
  NETWORK_ROW5_LABEL1_ID,
  NETWORK_ROW5_LABEL2_ID,
  NETWORK_ROW6_CONT_ID,
  NETWORK_ROW6_LABEL1_ID,
  NETWORK_ROW6_LABEL2_ID,
  NETWORK_PANEL_ITEM1_ID,
  NETWORK_PANEL_ITEM2_ID,
  NETWORK_PANEL_ITEM3_ID,
  NETWORK_PANEL_ITEM4_ID,
  NETWORK_PANEL_ITEM5_ID,
  NETWORK_PANEL_ITEM6_ID,
  NETWORK_PANEL_ITEM7_ID,
  NETWORK_PANEL_ITEM8_ID,
  NETWORK_PANEL_ITEM9_ID,
  NETWORK_BACK_BTN_ID,
} page_network_id;

typedef enum
{
  /* scene界面 */
  SCENE_TITLE_ICON_ID,
  SCENE_TITLE_ID,
  SCENE_ROW1_CONT_ID,
  SCENE_ROW1_LABEL1_ID,
  SCENE_ROW1_LABEL2_ID,
  SCENE_ROW2_CONT_ID,
  SCENE_ROW2_LABEL1_ID,
  SCENE_ROW2_LABEL2_ID,
  SCENE_ROW3_CONT_ID,
  SCENE_ROW3_LABEL1_ID,
  SCENE_ROW3_LABEL2_ID,
  SCENE_ROW4_CONT_ID,
  SCENE_ROW4_LABEL1_ID,
  SCENE_ROW4_LABEL2_ID,
  SCENE_BACK_BTN_ID,
} page_scene_id;

typedef enum
{
  /* system界面 */
  SYSTEM_TITLE_ICON_ID,
  SYSTEM_TITLE_ID,
  SYSTEM_ROW1_CONT_ID,
  SYSTEM_ROW1_LABEL1_ID,
  SYSTEM_ROW1_LABEL2_ID,
  SYSTEM_ROW2_CONT_ID,
  SYSTEM_ROW2_LABEL1_ID,
  SYSTEM_ROW2_LABEL2_ID,
  SYSTEM_ROW3_CONT_ID,
  SYSTEM_ROW3_LABEL1_ID,
  SYSTEM_ROW3_LABEL2_ID,
  SYSTEM_ROW4_CONT_ID,
  SYSTEM_ROW4_LABEL1_ID,
  SYSTEM_ROW4_LABEL2_ID,
  SYSTEM_ROW5_CONT_ID,
  SYSTEM_ROW5_LABEL1_ID,
  SYSTEM_ROW5_LABEL2_ID,
  SYSTEM_ROW6_CONT_ID,
  SYSTEM_ROW6_LABEL1_ID,
  SYSTEM_ROW6_LABEL2_ID,
  SYSTEM_ROW7_CONT_ID,
  SYSTEM_ROW7_LABEL1_ID,
  SYSTEM_ROW7_LABEL2_ID,
  SYSTEM_ROW8_CONT_ID,
  SYSTEM_ROW8_LABEL1_ID,
  SYSTEM_ROW8_LABEL2_ID,
  SYSTEM_ROW9_CONT_ID,
  SYSTEM_ROW9_LABEL1_ID,
  SYSTEM_ROW9_LABEL2_ID,
  SYSTEM_ROW10_CONT_ID,
  SYSTEM_ROW10_LABEL1_ID,
  SYSTEM_ROW10_LABEL2_ID,
  SYSTEM_ROW11_CONT_ID,
  SYSTEM_ROW11_LABEL1_ID,
  SYSTEM_ROW11_LABEL2_ID,
  SYSTEM_BACK_BTN_ID,
  SYSTEM_TIME_POPUP_OVERLAY_ID,
} page_system_id;

typedef enum
{
  /* system_info界面 */
  SYSTEM_INFO_TITLE_ICON_ID,
  SYSTEM_INFO_TITLE_ID,
  SYSTEM_INFO_ROW1_CONT_ID,
  SYSTEM_INFO_ROW1_LABEL1_ID,
  SYSTEM_INFO_ROW1_LABEL2_ID,
  SYSTEM_INFO_ROW2_CONT_ID,
  SYSTEM_INFO_ROW2_LABEL1_ID,
  SYSTEM_INFO_ROW2_LABEL2_ID,
  SYSTEM_INFO_ROW3_CONT_ID,
  SYSTEM_INFO_ROW3_LABEL1_ID,
  SYSTEM_INFO_ROW3_LABEL2_ID,
  SYSTEM_INFO_ROW4_CONT_ID,
  SYSTEM_INFO_ROW4_LABEL1_ID,
  SYSTEM_INFO_ROW4_LABEL2_ID,
  SYSTEM_INFO_ROW5_CONT_ID,
  SYSTEM_INFO_ROW5_LABEL1_ID,
  SYSTEM_INFO_ROW5_LABEL2_ID,
  SYSTEM_INFO_ROW6_CONT_ID,
  SYSTEM_INFO_ROW6_LABEL1_ID,
  SYSTEM_INFO_ROW6_LABEL2_ID,
  SYSTEM_INFO_ROW7_CONT_ID,
  SYSTEM_INFO_ROW7_LABEL1_ID,
  SYSTEM_INFO_ROW7_LABEL2_ID,
  SYSTEM_INFO_ROW8_CONT_ID,
  SYSTEM_INFO_ROW8_LABEL1_ID,
  SYSTEM_INFO_ROW8_LABEL2_ID,
  SYSTEM_INFO_BACK_BTN_ID,
} page_system_info_id;

typedef enum
{
  /* door 门口机设置界面 */
  DOOR_DOOR1_BTN_ID,
  DOOR_DOOR1_LABEL_ID,
  DOOR_DOOR2_BTN_ID,
  DOOR_DOOR2_LABEL_ID,
  DOOR_PANEL_TITLE_ID,
  DOOR_ROW1_CONT_ID,
  DOOR_ROW1_LABEL1_ID,
  DOOR_ROW1_LABEL2_ID,
  DOOR_ROW2_CONT_ID,
  DOOR_ROW2_LABEL1_ID,
  DOOR_ROW2_LABEL2_ID,
  DOOR_ROW3_CONT_ID,
  DOOR_ROW3_LABEL1_ID,
  DOOR_ROW3_LABEL2_ID,
  DOOR_ROW4_CONT_ID,
  DOOR_ROW4_LABEL1_ID,
  DOOR_ROW4_LABEL2_ID,
  DOOR_ROW5_CONT_ID,
  DOOR_ROW5_LABEL1_ID,
  DOOR_ROW5_LABEL2_ID,
  DOOR_ROW6_CONT_ID,
  DOOR_ROW6_LABEL1_ID,
  DOOR_ROW6_LABEL2_ID,
  DOOR_ROW7_CONT_ID,
  DOOR_ROW7_LABEL1_ID,
  DOOR_ROW7_LABEL2_ID,
  DOOR_ROW8_CONT_ID,
  DOOR_ROW8_LABEL1_ID,
  DOOR_ROW8_LABEL2_ID,
  DOOR_ROW9_CONT_ID,
  DOOR_ROW9_LABEL1_ID,
  DOOR_ROW9_LABEL2_ID,
  DOOR_ROW10_CONT_ID,
  DOOR_ROW10_LABEL1_ID,
  DOOR_ROW10_LABEL2_ID,
  DOOR_BACK_BTN_ID,
} page_door_id;

/* ---------- 以下为新增页面枚举 ---------- */

typedef enum
{
  /* call界面 - 被叫名称 */
  CALL_CALLEE_LABEL_ID,
  /* call界面 - 拨号 */
  CALL_DIAL_BTN_ID,
  /* call界面 - 挂断 */
  CALL_HANGUP_BTN_ID,
  /* call界面 - 来电挂断 */
  CALL_IN_HANGUP_BTN_ID,
  /* call界面 - 接听 */
  CALL_ANSWER_BTN_ID,
  /* call界面 - 通话挂断 */
  CALL_TALK_HANGUP_BTN_ID,
  /* call界面 - 本机ID标签 */
  CALL_LOCAL_ID_LABEL_ID,
  /* call界面 - 对方ID标签 */
  CALL_REMOTE_ID_LABEL_ID,
  /* call界面 - 返回 */
  CALL_BACK_BTN_ID,
  /* call界面(talking) - 日期 */
  CALL_DATE_ID,
  /* call界面(talking) - 超时 */
  CALL_TIMEOUT_ID,
  /* call界面(talking) - 通话状态图标 */
  CALL_STATUS_IMG_ID,
  /* call界面(talking) - 通话状态标签 */
  CALL_STATUS_LABEL_ID,
  /* call界面(talking) - 通话音量按钮 */
  CALL_TALK_VOLUME_ID,
  /* call界面(talking) - 音量滑块 */
  CALL_VOLUME_SLIDER_ID,
} page_call_id;

typedef enum
{
  /* cctv界面 - 侧栏 */
  CCTV_TITLE_ICON_ID,
  CCTV_TITLE_ID,
  /* cctv界面 Level1 - 手动添加行 */
  CCTV_ADD_CONT_ID,
  CCTV_ADD_LABEL1_ID,
  CCTV_ADD_LABEL2_ID,
  /* cctv界面 Level1 - CCTV1行 */
  CCTV_ROW1_CONT_ID,
  CCTV_ROW1_LABEL1_ID,
  CCTV_ROW1_LABEL2_ID,
  /* cctv界面 Level1 - CCTV2行 */
  CCTV_ROW2_CONT_ID,
  CCTV_ROW2_LABEL1_ID,
  CCTV_ROW2_LABEL2_ID,
  /* cctv界面 Level2 - Camera Mode行 */
  CCTV_MODE_CONT_ID,
  CCTV_MODE_LABEL1_ID,
  CCTV_MODE_LABEL2_ID,
  /* cctv界面 Level2 - Camera IP行 */
  CCTV_IP_CONT_ID,
  CCTV_IP_LABEL1_ID,
  CCTV_IP_LABEL2_ID,
  /* cctv界面 Level2 - Account行 */
  CCTV_ACCOUNT_CONT_ID,
  CCTV_ACCOUNT_LABEL1_ID,
  CCTV_ACCOUNT_LABEL2_ID,
  /* cctv界面 Level2 - Password行 */
  CCTV_PWD_CONT_ID,
  CCTV_PWD_LABEL1_ID,
  CCTV_PWD_LABEL2_ID,
  /* cctv界面 - 摄像头1/2 切换按钮 */
  CCTV_CAM1_BTN_ID,
  CCTV_CAM2_BTN_ID,
  /* cctv界面 - 返回 */
  CCTV_BACK_BTN_ID,
  /* cctv界面 - 扫描列表 */
  CCTV_REGISTER_LIST_CONT_ID,
  CCTV_SCAN_LIST_CONT_ID,
  CCTV_SCAN_ADD_MANUAL_ID,
  /* cctv界面 - 手动添加视图 */
  CCTV_MANUAL_TITLE_ID,
  CCTV_MANUAL_CONFIRM_BTN_ID,
  CCTV_MANUAL_BACK_BTN_ID,
  /* cctv界面 - 键盘视图 */
  CCTV_KB_CONT_ID,
  CCTV_KB_TA_ID,
  CCTV_KB_KB_ID,
  CCTV_KB_BACK_BTN_ID,
  /* cctv界面 - 预览视图 */
  CCTV_PREVIEW_HEADER_ID,
  CCTV_PREVIEW_BACK_BTN_ID,
  CCTV_PREVIEW_BOTTOM_BAR_ID,
  CCTV_PREVIEW_REGISTER_BTN_ID,
  /* cctv界面 - 注册弹窗 */
  CCTV_REG_POPUP_CONT_ID,
  CCTV_REG_POPUP_CAM1_BTN_ID,
  CCTV_REG_POPUP_CAM2_BTN_ID,
  CCTV_REG_POPUP_CLOSE_BTN_ID,
  /* cctv界面 - spinner遮罩 */
  CCTV_SPINNER_OVERLAY_ID,
  /* cctv界面 - 视图容器 */
  CCTV_VIEW_CONTAINER_ID,
} page_cctv_id;

typedef enum
{
  /* camera_display界面 - 顶部栏 */
  CAMERA_DISPLAY_HEADER_ID,
  CAMERA_DISPLAY_CLOSE_BTN_ID,
  /* camera_display界面 - 底部按钮 */
  CAMERA_DISPLAY_BOTTOM_BTN_ID,
  /* camera_display界面 - 注册弹窗 */
  CAMERA_DISPLAY_REG_POPUP_CONT_ID,
  CAMERA_DISPLAY_REG_POPUP_CAM1_BTN_ID,
  CAMERA_DISPLAY_REG_POPUP_CAM2_BTN_ID,
  CAMERA_DISPLAY_REG_POPUP_CLOSE_BTN_ID,
  /* camera_display界面 - 加载 spinner 遮罩 */
  CAMERA_DISPLAY_SPINNER_OVERLAY_ID,
} page_camera_display_id;

typedef enum
{
  /* interphone界面 - 标题 */
  INTERPHONE_TITLE_LABEL_ID,
  /* interphone界面 - 返回按钮 */
  INTERPHONE_BACK_BTN_ID,
  /* interphone界面 - 设备1 */
  INTERPHONE_DEV1_CONT_ID,
  INTERPHONE_DEV1_LABEL_ID,
  /* interphone界面 - 设备2 */
  INTERPHONE_DEV2_CONT_ID,
  INTERPHONE_DEV2_LABEL_ID,
  /* interphone界面 - 设备3 */
  INTERPHONE_DEV3_CONT_ID,
  INTERPHONE_DEV3_LABEL_ID,
  /* interphone界面 - 呼叫/挂断按钮 */
  INTERPHONE_CALL_BTN_ID,
} page_interphone_id;

typedef enum
{
  /* keyboard界面 - 返回按钮 */
  KEYBOARD_BACK_BTN_ID,
} page_keyboard_id;

typedef enum
{
  /* password_input界面 - 标题 */
  PASSWORD_INPUT_TITLE_ID,
  /* password_input界面 - 返回 */
  PASSWORD_INPUT_BACK_BTN_ID,
  /* password_input界面 - 密码可见切换 */
  PASSWORD_INPUT_PWD_MODE_ID,
  /* password_input界面 - 分隔线 */
  PASSWORD_INPUT_SEP_ID,
  /* password_input界面 - 键盘 */
  PASSWORD_INPUT_KB_ID,
  /* password_input界面 - 文本区域 */
  PASSWORD_INPUT_TA_ID,
} page_password_input_id;

typedef enum
{
  /* playback界面 - 标题 */
  PLAYBACK_TITLE_ID,
  /* playback界面 - 返回 */
  PLAYBACK_CANCEL_BTN_ID,
  /* playback界面 - 锁定按钮 */
  PLAYBACK_LOCK_BTN_ID,
  /* playback界面 - 删除全部按钮 */
  PLAYBACK_DELETE_BTN_ID,
  /* playback界面 - 左翻页 */
  PLAYBACK_LEFT_BTN_ID,
  /* playback界面 - 右翻页 */
  PLAYBACK_RIGHT_BTN_ID,
  /* playback界面 - 页码标签 */
  PLAYBACK_PAGE_LABEL_ID,
  /* playback界面 - 底部动画容器 */
  PLAYBACK_DOT_CONT_ID,
  PLAYBACK_DOT1_ID,
  PLAYBACK_DOT2_ID,
  PLAYBACK_DOT3_ID,
  /* playback界面 - 缩略图容器 0-5 */
  PLAYBACK_THUMB_CONT0_ID,
  PLAYBACK_THUMB_CONT1_ID,
  PLAYBACK_THUMB_CONT2_ID,
  PLAYBACK_THUMB_CONT3_ID,
  PLAYBACK_THUMB_CONT4_ID,
  PLAYBACK_THUMB_CONT5_ID,
} page_playback_id;

typedef enum
{
  /* video界面 - 信息标签容器 */
  VIDEO_INFO_CONT_ID,
  VIDEO_CHANNEL_LABEL_ID,
  VIDEO_TIME_LABEL_ID,
  /* video界面 - 锁定图标 */
  VIDEO_LOCK_IMG_ID,
  /* video界面 - 播放按钮容器 */
  VIDEO_PLAY_CONT_ID,
  VIDEO_PLAY_ICON_ID,
  /* video界面 - 进度条 */
  VIDEO_PROGRESS_BAR_ID,
  /* video界面 - 左切换 */
  VIDEO_LEFT_BTN_ID,
  /* video界面 - 右切换 */
  VIDEO_RIGHT_BTN_ID,
  /* video界面 - 返回 */
  VIDEO_BACK_BTN_ID,
} page_video_id;

typedef enum
{
  /* transfer界面 - 分机1 */
  TRANSFER_DEV1_BTN_ID,
  /* transfer界面 - 分机2 */
  TRANSFER_DEV2_BTN_ID,
  /* transfer界面 - 分机3 */
  TRANSFER_DEV3_BTN_ID,
  /* transfer界面 - 分机4 */
  TRANSFER_DEV4_BTN_ID,
  /* transfer界面 - 分机5 */
  TRANSFER_DEV5_BTN_ID,
  /* transfer界面 - 分机6 */
  TRANSFER_DEV6_BTN_ID,
  /* transfer界面 - 全部 */
  TRANSFER_ALL_BTN_ID,
  /* transfer界面 - 返回 */
  TRANSFER_BACK_BTN_ID,
} page_transfer_id;

typedef enum
{
  /* device_busy界面 - 消息文本 */
  DEVICE_BUSY_MSG_ID,
} page_device_busy_id;

typedef enum
{
  /* add_wifi界面 - 标题 */
  ADD_WIFI_TITLE_ID,
  /* add_wifi界面 - 名称容器 */
  ADD_WIFI_NAME_CONT_ID,
  ADD_WIFI_NAME_LABEL_ID,
  ADD_WIFI_NAME_VAL_LABEL_ID,
  /* add_wifi界面 - 密码容器 */
  ADD_WIFI_PWD_CONT_ID,
  ADD_WIFI_PWD_LABEL_ID,
  ADD_WIFI_PWD_VAL_LABEL_ID,
  /* add_wifi界面 - 确认/取消按钮 */
  ADD_WIFI_CONFIRM_BTN_ID,
  ADD_WIFI_CANCEL_BTN_ID,
  /* add_wifi界面 - 返回 */
  ADD_WIFI_BACK_BTN_ID,
  /* add_wifi界面 - 分隔线 */
  ADD_WIFI_SEP1_ID,
  ADD_WIFI_SEP2_ID,
  ADD_WIFI_SEP3_ID,
  ADD_WIFI_SEP4_ID,
  /* add_wifi界面 - 键盘 */
  ADD_WIFI_NAME_KB_ID,
  /* add_wifi界面 - 连接中遮罩 */
  ADD_WIFI_SPINNER_OVERLAY_ID,
} page_add_wifi_id;

typedef enum
{
  /* connect_wifi界面 - 标题 */
  CONNECT_WIFI_TITLE_ID,
  /* connect_wifi界面 - 密码输入框容器 */
  CONNECT_WIFI_PWD_CONT_ID,
  /* connect_wifi界面 - 密码可见切换 */
  CONNECT_WIFI_PWD_MODE_ID,
  /* connect_wifi界面 - 分隔线 */
  CONNECT_WIFI_SEP_ID,
  /* connect_wifi界面 - 键盘 */
  CONNECT_WIFI_KB_ID,
  /* connect_wifi界面 - 文本区域 */
  CONNECT_WIFI_TA_ID,
  /* connect_wifi界面 - 返回 */
  CONNECT_WIFI_BACK_BTN_ID,
} page_connect_wifi_id;

typedef enum
{
  /* wifi界面 - 标题 */
  WIFI_TITLE_ID,
  /* wifi界面 - 滚动列表容器 */
  WIFI_LIST_CONT_ID,
  /* wifi界面 - 返回 */
  WIFI_BACK_BTN_ID,
} page_wifi_id;

typedef enum
{
  /* device_transfer界面 - 分机按钮(动态) */
  DEVICE_TRANSFER_DEV_CONT_ID_BASE,
  /* device_transfer界面 - 返回 */
  DEVICE_TRANSFER_BACK_BTN_ID,
} page_device_transfer_id;

typedef enum
{
  /* file_list界面 - 标题 */
  FILE_LIST_TITLE_ID,
  /* file_list界面 - 文件列表容器 */
  FILE_LIST_CONT_ID,
  /* file_list界面 - 返回 */
  FILE_LIST_BACK_BTN_ID,
  /* file_list界面 - 打开/删除弹窗 */
  FILE_LIST_POPUP_CONT_ID,
  FILE_LIST_POPUP_OPEN_BTN_ID,
  FILE_LIST_POPUP_DELETE_BTN_ID,
  FILE_LIST_POPUP_DELETE_ALL_BTN_ID,
  FILE_LIST_POPUP_CLOSE_BTN_ID,
  /* file_list界面 - 翻页 */
  FILE_LIST_PREV_BTN_ID,
  FILE_LIST_NEXT_BTN_ID,
  FILE_LIST_PAGE_LABEL_ID,
  /* file_list界面 - 动态列表项（10个可见项） */
  FILE_LIST_ITEM_BASE_ID,
  FILE_LIST_ITEM_MAX = FILE_LIST_ITEM_BASE_ID + 9,
} page_file_list_id;

typedef enum
{
  /* data_list界面 - 标题 */
  DATA_LIST_TITLE_ID,
  /* data_list界面 - Tab按钮 */
  DATA_LIST_TAB1_BTN_ID,
  DATA_LIST_TAB2_BTN_ID,
  /* data_list界面 - 列表容器 */
  DATA_LIST_CONT_ID,
  /* data_list界面 - 返回 */
  DATA_LIST_BACK_BTN_ID,
} page_data_list_id;

typedef enum
{
  /* card_manager界面 - 左侧门口机选择 */
  CARD_MGR_DOOR1_BTN_ID,
  CARD_MGR_DOOR1_LABEL_ID,
  CARD_MGR_DOOR2_BTN_ID,
  CARD_MGR_DOOR2_LABEL_ID,
  /* card_manager界面 - 右侧面板 */
  CARD_MGR_PANEL_TITLE_ID,
  /* card_manager界面 - 列表表头 */
  CARD_MGR_LIST_HEADER_ID,
  CARD_MGR_LIST_HEADER_NO_ID,
  CARD_MGR_LIST_HEADER_CARD_ID,
  CARD_MGR_LIST_HEADER_LOCK_ID,
  CARD_MGR_LIST_HEADER_GATE1_ID,
  /* card_manager界面 - 列表容器 */
  CARD_MGR_LIST_CONT_ID,
  /* card_manager界面 - 分页 */
  CARD_MGR_PAGE_CONT_ID,
  CARD_MGR_PAGE_TEXT_ID,
  CARD_MGR_PAGE_PREV_ICON_ID,
  CARD_MGR_PAGE_NEXT_ICON_ID,
  /* card_manager界面 - 添加/返回按钮 */
  CARD_MGR_ADD_BTN_ID,
  CARD_MGR_BACK_BTN_ID,
  /* card_manager界面 - 操作弹窗 */
  CARD_MGR_POPUP_CONT_ID,
  CARD_MGR_POPUP_DELETE_BTN_ID,
  CARD_MGR_POPUP_DELETE_ALL_BTN_ID,
  CARD_MGR_POPUP_CLOSE_BTN_ID,
  /* card_manager界面 - 添加卡overlay */
  CARD_MGR_ADD_OVERLAY_ID,
  CARD_MGR_ADD_SUCCESS_LABEL_ID,
} page_card_manager_id;

typedef enum
{
  /* fingerprint_manager界面 - 左侧门口机选择 */
  FINGER_MGR_DOOR1_BTN_ID,
  FINGER_MGR_DOOR1_LABEL_ID,
  FINGER_MGR_DOOR2_BTN_ID,
  FINGER_MGR_DOOR2_LABEL_ID,
  /* fingerprint_manager界面 - 右侧面板 */
  FINGER_MGR_PANEL_TITLE_ID,
  FINGER_MGR_LIST_CONT_ID,
  /* fingerprint_manager界面 - 添加/返回按钮 */
  FINGER_MGR_ADD_BTN_ID,
  FINGER_MGR_BACK_BTN_ID,
  /* fingerprint_manager界面 - 操作弹窗 */
  FINGER_MGR_POPUP_CONT_ID,
  FINGER_MGR_POPUP_DELETE_BTN_ID,
  FINGER_MGR_POPUP_DELETE_ALL_BTN_ID,
  FINGER_MGR_POPUP_CLOSE_BTN_ID,
  /* fingerprint_manager界面 - 添加指纹overlay */
  FINGER_MGR_ADD_OVERLAY_ID,
  FINGER_MGR_ADD_SUCCESS_LABEL_ID,
} page_fingerprint_manager_id;

typedef enum
{
  /* password_manager界面 - 左侧门口机选择 */
  PASSW_MGR_DOOR1_BTN_ID,
  PASSW_MGR_DOOR1_LABEL_ID,
  PASSW_MGR_DOOR2_BTN_ID,
  PASSW_MGR_DOOR2_LABEL_ID,
  /* password_manager界面 - 右侧面板 */
  PASSW_MGR_PANEL_TITLE_ID,
  PASSW_MGR_LIST_CONT_ID,
  /* password_manager界面 - 添加/返回按钮 */
  PASSW_MGR_ADD_BTN_ID,
  PASSW_MGR_BACK_BTN_ID,
  /* password_manager界面 - 操作弹窗 */
  PASSW_MGR_POPUP_CONT_ID,
  PASSW_MGR_POPUP_DELETE_BTN_ID,
  PASSW_MGR_POPUP_DELETE_ALL_BTN_ID,
  PASSW_MGR_POPUP_CLOSE_BTN_ID,
  /* password_manager界面 - 添加密码overlay */
  PASSW_MGR_ADD_OVERLAY_ID,
  PASSW_MGR_ADD_PWD_LABEL_ID,
  PASSW_MGR_ADD_SHIELD_ID,
} page_password_manager_id;

typedef enum
{
  /* ota界面 */
  ota_page_bg_id,
  ota_page_bar_track_id,
  ota_page_bar_fill_id,
  ota_page_progress_label_id,
  ota_page_status_label_id,
  ota_page_warning_label_id,
} page_ota_id;

typedef enum
{
  /* standby界面 - 全屏背景容器（所有待机控件挂在此容器下，预览时整体隐藏） */
  STANDBY_BG_CONT_ID,
  /* standby界面 - 底部面板 */
  STANDBY_PANEL_CONT_ID,
  /* standby界面 - 通知图标 */
  STANDBY_EVENT_RECORD_ICON_ID,
  STANDBY_DETECTION_ICON_ID,
  STANDBY_MESSAGE_ICON_ID,
  STANDBY_MONITOR_ICON_ID,
  /* standby界面 - 通知标签 */
  STANDBY_EVENT_RECORD_LABEL_ID,
  STANDBY_DETECTION_LABEL_ID,
  STANDBY_MESSAGE_LABEL_ID,
  STANDBY_MONITOR_LABEL_ID,
  /* standby界面 - 角标 */
  STANDBY_BADGE1_ID,
  STANDBY_BADGE2_ID,
  STANDBY_BADGE3_ID,
  STANDBY_BADGE4_ID,
  /* standby界面 - 时钟 */
  STANDBY_CLOCK_LABEL_ID,
  /* standby界面 - 日期 */
  STANDBY_DATE_LABEL_ID,
  /* standby界面 - 天气 */
  STANDBY_WEATHER_TEMP_CUR_ID,
  STANDBY_WEATHER_TEMP_RANGE_ID,
  STANDBY_WEATHER_CONDITION_LABEL_ID,
  STANDBY_WEATHER_CONDITION_IMG_ID,
  STANDBY_WEATHER_SEPARATOR_ID,
  /* standby界面 - AEC 模式切换测试按钮（左上角） */
  STANDBY_AEC_TEST_BTN_ID,
  /* motion预览 - 与待机页同一屏幕，共用此枚举避免控件ID冲突 */
  MOTION_HEADER_CONT_ID,     /* 预览顶部容器 */
  MOTION_CHANNEL_ID,         /* 通道显示 */
  MOTION_TITLE_LABEL_ID,     /* 标题 "Motion Detection" */
  MOTION_RECORDING_LABEL_ID, /* 录制时长和模式（如 "REC 00"） */
  MOTION_BACK_BTN_ID,        /* 右下角返回按钮 */
} page_standby_id;

typedef enum
{
  /* record_play界面 - 上一张/段 */
  RECORD_PLAY_PREV_BTN_ID,
  /* record_play界面 - 下一张/段 */
  RECORD_PLAY_NEXT_BTN_ID,
  /* record_play界面 - 通道标签 */
  RECORD_PLAY_CHANNEL_LABEL_ID,
  /* record_play界面 - 日期时间标签 */
  RECORD_PLAY_DATETIME_LABEL_ID,
  /* record_play界面 - 返回 */
  RECORD_PLAY_BACK_BTN_ID,
  /* record_play界面 - 进度条 */
  RECORD_PLAY_PROGRESS_BAR_ID,
  /* record_play界面 - 暂停时中间播放按钮 */
  RECORD_PLAY_CENTER_PLAY_BTN_ID,
} page_record_play_id;

typedef enum
{
  /* ring界面 - 左侧 Ring1/2/3 选择器 */
  RING_SEL_RING1_BTN_ID,
  RING_SEL_RING1_LABEL_ID,
  RING_SEL_RING2_BTN_ID,
  RING_SEL_RING2_LABEL_ID,
  RING_SEL_RING3_BTN_ID,
  RING_SEL_RING3_LABEL_ID,
  /* ring界面 - 右侧面板标题 */
  RING_PANEL_TITLE_ID,
  /* ring界面 - 5行设置项（Schedule / Ring Time / Ring Mode / Ring Select / Ring Volume） */
  RING_ROW1_CONT_ID,
  RING_ROW1_LABEL1_ID,
  RING_ROW1_LABEL2_ID,
  RING_ROW2_CONT_ID,
  RING_ROW2_LABEL1_ID,
  RING_ROW2_LABEL2_ID,
  RING_ROW3_CONT_ID,
  RING_ROW3_LABEL1_ID,
  RING_ROW3_LABEL2_ID,
  RING_ROW4_CONT_ID,
  RING_ROW4_LABEL1_ID,
  RING_ROW4_LABEL2_ID,
  RING_ROW5_CONT_ID,
  RING_ROW5_LABEL1_ID,
  RING_ROW5_LABEL2_ID,
  /* ring界面 - 返回按钮 */
  RING_BACK_BTN_ID,
  /* ring界面 - Schedule弹窗 */
  RING_POPUP_OVERLAY_ID = 900,
  RING_POPUP_PANEL_ID,
  RING_POPUP_OK_BTN_ID,
  RING_POPUP_CANCEL_BTN_ID,
} page_ring_id;

struct rtsp_client_context;

void cam_display_ctx_fill(const char *ip, const char *rtsp_url, const char *account,
                          const char *password, int model, int dev_id, bool from_manual);
void cam_display_ctx_set_rtsp_ctx(struct rtsp_client_context *ctx);
void cctv_set_enter_from_cam_display(bool val);

/** 设备冲突事件默认 UI 回调（非设备号设置页提示语） */
void page_default_device_conflict_event_cb(void *arg);
/** 网卡冲突事件默认 UI 回调 */
void page_default_network_conflict_event_cb(void *arg);

void page_default_ota_event_cb(void *arg);

/**
 * @brief SD卡插拔通用回调，显示插入/拔出消息框
 */
void page_common_tfcard_change_cb(void *arg);
/** UI线程执行的全局SD资源释放，完成后才允许卸载旧挂载。 */
void page_common_tfcard_cleanup(void);

/** SD卡插拔时回退铃声模式：无自定义铃声则回退为标准铃声（跨页面全局生效） */
void ring_mode_on_sd_changed(void);

/**
 * @brief SD卡格式化完成通用回调，显示格式化结果消息框
 */
void page_common_tfcard_format_finish_cb(void *arg);

/**
 * @brief 预加载所有动态 TTF 字体，提前解析并缓存，避免首次使用卡顿
 */
void font_preload_all(void);
/**
 * @brief 待机监测任务初始化
 */
void page_common_standby_check_init(void);
/**
 * @brief 按键点击默认回调
 */
void page_default_obj_press_event_cb(lv_event_t *event);
#endif
