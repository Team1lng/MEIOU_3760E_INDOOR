#include "page_tabel.h"
#include "lv_common.h"
#include "page_common.h"
#include "Amy_media_mgr.h"
#include "Amy_sdk.h"
#include "Amy_tfcard.h"
#include "video_decode.h"
#include "avilib/video_play.h"
#include "sat_user_common.h"
#include "ak_mem.h"
#include "ak_common_graphics.h"
#include "ak_tde.h"
#include "ak_common.h"
#include "user_standby.h"
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

/************************** 枚举 **************************/
typedef enum
{
    PAGE_PHOTO_PLAY,
    PAGE_VIDEO_PLAY
} page_play_type;

typedef enum
{
    PLAY_STATE_PLAYING,
    PLAY_STATE_PAUSED,
    PLAY_STATE_FINISH
} play_state_t;

/************************** 布局常量 **************************/
#define DISPLAY_W 1024
#define DISPLAY_H 600
#define BTN_W 36
#define BTN_H 64
#define PREV_BTN_X 16
#define PREV_BTN_Y 260
#define NEXT_BTN_X 972
#define NEXT_BTN_Y 260
#define CHANNEL_LABEL_X 38
#define CHANNEL_LABEL_Y 469
#define DATETIME_LABEL_X 38
#define DATETIME_LABEL_Y 517
#define BACK_BTN_X 929
#define BACK_BTN_Y 505
#define BACK_BTN_W 67
#define BACK_BTN_H 67
#define EXT_CLICK 34
#define FILE_PATH_BUF_SIZE 128
#define DECODE_TIMER_MS 10
#define ALPHA_OPAQUE 0xFF

#define PROGRESS_BAR_X 46
#define PROGRESS_BAR_Y 578
#define PROGRESS_BAR_W 932
#define PROGRESS_BAR_H 10
#define PROGRESS_BG_COLOR 0x2D2D2D
#define PROGRESS_BAR_COLOR 0x02A9FF
#define PROGRESS_TIMER_MS 500

#define CENTER_PLAY_W 168
#define CENTER_PLAY_H 168
#define CENTER_PLAY_X ((1024 - CENTER_PLAY_W) / 2)
#define CENTER_PLAY_Y ((600 - CENTER_PLAY_H) / 2)

/************************** 上下文 **************************/
typedef struct
{
    int page_type;
    play_state_t play_state;
    video_decode_context *mjpeg_dec_ctx;
    avi_player_context *video_ctx;
    lv_img_dsc_t *display_buffer;
    lv_timer_t *progress_timer;
    int total_duration;
    lv_obj_t *progress_bar;
    lv_obj_t *center_play_btn;
} page_record_play_ctx;

static page_record_play_ctx g_ctx = {0};

/************************** 函数声明 **************************/
static void record_play_prev_btn_click(lv_event_t *ev);
static void record_play_next_btn_click(lv_event_t *ev);
static void record_play_back_btn_click(lv_event_t *ev);
static void record_play_screen_click(lv_event_t *ev);
static void record_play_status_handle(void *attr);

static void record_play_prev_btn_create(void);
static void record_play_next_btn_create(void);
static void record_play_channel_label_create(void);
static void record_play_datetime_label_create(void);
static void record_play_back_btn_create(void);
static void record_play_progress_bar_create(void);
static void record_play_center_play_btn_create(void);

static void record_play_update_channel_label(void);
static void record_play_update_datetime_label(void);
static void record_play_progress_timer_cb(lv_timer_t *timer);
static void record_play_update_play_btn_visible(void);

static void record_play_decode_and_play(void);
static int record_play_decode_photo(void);
static int record_play_decode_video(void);
static void record_play_decode_frame_cb(lv_timer_t *timer);

static void record_play_deinit(void);

/************************** 回调 **************************/

static void record_play_prev_btn_click(lv_event_t *ev)
{
    if (!g_media_browse.ctx)
        return;

    const media_file_info_t *info = Amy_media_prev_media_info(g_media_browse.ctx);
    if (!info)
        return;

    memcpy(&g_media_browse.info, info, sizeof(media_file_info_t));
    Amy_media_info_conver_path(g_media_browse.ctx, info, g_media_browse.path, sizeof(g_media_browse.path));

    if (info->mask & MEDIA_ATTR_NEW)
        Amy_media_attr_new_clean(g_media_browse.ctx);

    Amy_lv_page_goto(lv_page(record_play), LV_SCR_LOAD_ANIM_NONE, );
}

static void record_play_next_btn_click(lv_event_t *ev)
{
    if (!g_media_browse.ctx)
        return;

    const media_file_info_t *info = Amy_media_next_media_info(g_media_browse.ctx);
    if (!info)
        return;

    memcpy(&g_media_browse.info, info, sizeof(media_file_info_t));
    Amy_media_info_conver_path(g_media_browse.ctx, info, g_media_browse.path, sizeof(g_media_browse.path));

    if (info->mask & MEDIA_ATTR_NEW)
        Amy_media_attr_new_clean(g_media_browse.ctx);

    Amy_lv_page_goto(lv_page(record_play), LV_SCR_LOAD_ANIM_NONE, );
}

static void record_play_back_btn_click(lv_event_t *ev)
{
    Amy_lv_page_goto(lv_page(event_record), LV_SCR_LOAD_ANIM_NONE, );
}

static void record_play_sd_change_cb(void *arg)
{
    if (!(Amy_tfcard_status() & SD_STATE_INSERT))
        record_play_back_btn_click(NULL);
    page_common_tfcard_change_cb(arg);
}

static void record_play_screen_click(lv_event_t *ev)
{
    if (g_ctx.page_type != PAGE_VIDEO_PLAY || !g_ctx.video_ctx)
        return;
    avi_player_toggle_play(g_ctx.video_ctx);
}

static void record_play_status_handle(void *attr)
{
    if (!attr)
        return;

    int play_event = *(int *)attr;
    switch (play_event)
    {
    case AMY_PLAY_STATE_EVENT_ID_PLAY:
        standby_timer_close();
        g_ctx.play_state = PLAY_STATE_PLAYING;
        break;
    case AMY_PLAY_STATE_EVENT_ID_PAUSE:
        standby_timer_restart(true, true);
        g_ctx.play_state = PLAY_STATE_PAUSED;
        break;
    case AMY_PLAY_STATE_EVENT_ID_FINISH:
        standby_timer_restart(true, true);
        g_ctx.play_state = PLAY_STATE_FINISH;
        break;
    default:
        break;
    }
    record_play_update_play_btn_visible();
}

/************************** UI 创建 **************************/

static void record_play_prev_btn_create(void)
{
    lv_resource_init(icon, ICON_RECORD_PLAY_PREV_PNG_OFFSET, ICON_RECORD_PLAY_PREV_PNG_SIZE);
    lv_obj_t *btn = lv_common_img_btn_create(lv_scr_act_current(), RECORD_PLAY_PREV_BTN_ID,
                                             PREV_BTN_X, PREV_BTN_Y, BTN_W, BTN_H,
                                             record_play_prev_btn_click, true,
                                             LV_OPA_TRANSP, 0x000000, LV_OPA_TRANSP, 0x000000,
                                             0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                             0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                             &icon, LV_OPA_COVER, 0x02A9FF, LV_ALIGN_CENTER, NULL);
    lv_obj_set_ext_click_area(btn, EXT_CLICK);
    lv_obj_set_rendering(btn);
}

static void record_play_next_btn_create(void)
{
    lv_resource_init(icon, ICON_RECORD_PLAY_NEXT_PNG_OFFSET, ICON_RECORD_PLAY_NEXT_PNG_SIZE);
    lv_obj_t *btn = lv_common_img_btn_create(lv_scr_act_current(), RECORD_PLAY_NEXT_BTN_ID,
                                             NEXT_BTN_X, NEXT_BTN_Y, BTN_W, BTN_H,
                                             record_play_next_btn_click, true,
                                             LV_OPA_TRANSP, 0x000000, LV_OPA_TRANSP, 0x000000,
                                             0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                             0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                             &icon, LV_OPA_COVER, 0x02A9FF, LV_ALIGN_CENTER, NULL);
    lv_obj_set_ext_click_area(btn, EXT_CLICK);
    lv_obj_set_rendering(btn);
}

static void record_play_update_channel_label(void)
{
    lv_obj_t *label = lv_obj_get_child_by_id(lv_scr_act_current(), RECORD_PLAY_CHANNEL_LABEL_ID);
    if (!label)
        return;

    int ch = Amy_media_mask_to_channel(g_media_browse.info.mask);
    const char *name = NULL;
    switch (ch)
    {
    case 0:
        name = lang_str_get(STR_DOOR1);
        break;
    case 1:
        name = lang_str_get(STR_DOOR2);
        break;
    case 2:
        name = lang_str_get(STR_CAMERA1);
        break;
    case 3:
        name = lang_str_get(STR_CAMERA2);
        break;
    default:
        name = "---";
        break;
    }
    lv_label_set_text(label, name);
}

static void record_play_channel_label_create(void)
{
    lv_obj_t *label = lv_common_text_create(lv_scr_act_current(), RECORD_PLAY_CHANNEL_LABEL_ID,
                                            CHANNEL_LABEL_X, CHANNEL_LABEL_Y, 160, 38,
                                            NULL, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0,
                                            0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                            0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                            "CAM 1", 0xffffff, 0xffffff, LV_TEXT_ALIGN_LEFT, lv_font_normal);
    record_play_update_channel_label();
    lv_obj_set_rendering(label);
}

static void record_play_update_datetime_label(void)
{
    lv_obj_t *label = lv_obj_get_child_by_id(lv_scr_act_current(), RECORD_PLAY_DATETIME_LABEL_ID);
    if (!label)
        return;

    unsigned long long ts = 0;
    sscanf(g_media_browse.info.timestamp, "%llu", &ts);
    struct tm local_tm;
    Amy_time2localtime(ts, local_tm);
    lv_label_set_text_fmt(label, "%04d-%02d-%02d  %02d:%02d:%02d",
                          local_tm.tm_year, local_tm.tm_mon, local_tm.tm_mday,
                          local_tm.tm_hour, local_tm.tm_min, local_tm.tm_sec);
}

static void record_play_datetime_label_create(void)
{
    lv_obj_t *label = lv_common_text_create(lv_scr_act_current(), RECORD_PLAY_DATETIME_LABEL_ID,
                                            DATETIME_LABEL_X, DATETIME_LABEL_Y, 280, 33,
                                            NULL, LV_OPA_TRANSP, 0, LV_OPA_TRANSP, 0,
                                            0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                            0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                            "2024-01-01  00:00:00", 0xffffff, 0xffffff, LV_TEXT_ALIGN_LEFT, lv_font_normal);
    record_play_update_datetime_label();
    lv_obj_set_rendering(label);
}

static void record_play_back_btn_create(void)
{
    lv_resource_init(icon, ICON_COMMON_BACK_PNG_OFFSET, ICON_COMMON_BACK_PNG_SIZE);
    lv_resource_init(icon_focus, ICON_COMMON_BACK_FOCUS_PNG_OFFSET, ICON_COMMON_BACK_FOCUS_PNG_SIZE);
    lv_obj_t *btn = lv_common_img_btn_create(lv_scr_act_current(), RECORD_PLAY_BACK_BTN_ID,
                                             BACK_BTN_X, BACK_BTN_Y, BACK_BTN_W, BACK_BTN_H,
                                             record_play_back_btn_click, true,
                                             LV_OPA_TRANSP, 0x101010, LV_OPA_TRANSP, 0x101010,
                                             0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                             0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                             &icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &icon_focus);
    lv_obj_set_ext_click_area(btn, EXT_CLICK);
    lv_obj_set_rendering(btn);
}

static void record_play_progress_bar_create(void)
{
    lv_obj_t *bar = lv_bar_create(lv_scr_act_current());
    lv_obj_set_size(bar, PROGRESS_BAR_W, PROGRESS_BAR_H);
    lv_obj_set_pos(bar, PROGRESS_BAR_X, PROGRESS_BAR_Y);
    lv_bar_set_range(bar, 0, 1000);
    lv_bar_set_value(bar, 0, LV_ANIM_OFF);

    lv_obj_set_style_bg_color(bar, lv_color_hex(PROGRESS_BG_COLOR), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_opa(bar, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_radius(bar, 0, LV_PART_MAIN);

    lv_obj_set_style_bg_color(bar, lv_color_hex(PROGRESS_BAR_COLOR), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_radius(bar, 0, LV_PART_INDICATOR);

    lv_obj_set_child_id(bar, RECORD_PLAY_PROGRESS_BAR_ID);
    g_ctx.progress_bar = bar;

    lv_obj_set_rendering(bar);
}

static void record_play_update_play_btn_visible(void)
{
    if (!g_ctx.center_play_btn)
        return;

    if (g_ctx.play_state == PLAY_STATE_PAUSED || g_ctx.play_state == PLAY_STATE_FINISH)
        lv_obj_clear_flag(g_ctx.center_play_btn, LV_OBJ_FLAG_HIDDEN);
    else
        lv_obj_add_flag(g_ctx.center_play_btn, LV_OBJ_FLAG_HIDDEN);
}

static void record_play_center_play_btn_create(void)
{
    lv_resource_init(icon, ICON_RECORD_PLAY_PLAY_PNG_OFFSET, ICON_RECORD_PLAY_PLAY_PNG_SIZE);
    lv_obj_t *btn = lv_common_img_btn_create(lv_scr_act_current(), RECORD_PLAY_CENTER_PLAY_BTN_ID,
                                             CENTER_PLAY_X, CENTER_PLAY_Y, CENTER_PLAY_W, CENTER_PLAY_H,
                                             NULL, false,
                                             LV_OPA_TRANSP, 0x000000, LV_OPA_TRANSP, 0x000000,
                                             10, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                             10, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                             &icon, LV_OPA_COVER, 0x02A9FF, LV_ALIGN_CENTER, NULL);
    g_ctx.center_play_btn = btn;
    lv_obj_set_rendering(btn);
}

/************************** 进度条定时器 **************************/

static void record_play_progress_timer_cb(lv_timer_t *timer)
{
    if (!g_ctx.video_ctx || g_ctx.play_state != PLAY_STATE_PLAYING)
        return;

    int remain = 0;
    if (video_play_param_get(g_ctx.video_ctx, &remain) != 0)
        return;

    if (g_ctx.total_duration <= 0)
        return;

    int elapsed = g_ctx.total_duration - remain;
    if (elapsed < 0)
        elapsed = 0;
    if (elapsed > g_ctx.total_duration)
        elapsed = g_ctx.total_duration;

    int val = (int)((long long)elapsed * 1000 / g_ctx.total_duration);
    lv_bar_set_value(g_ctx.progress_bar, val, LV_ANIM_OFF);
}

/************************** 解码 **************************/

static void record_play_decode_frame_cb(lv_timer_t *timer)
{
    if (!g_ctx.mjpeg_dec_ctx || !g_ctx.display_buffer)
        return;

    struct ak_vdec_frame frame = {0};
    if (video_decodec_get_frame(g_ctx.mjpeg_dec_ctx, &frame, NULL) != 0)
        return;

    char *rgb_buf = (char *)ak_mem_dma_alloc(MODULE_ID_VDEC, DISPLAY_W * DISPLAY_H * 3);
    if (!rgb_buf)
        return;

    struct ak_tde_layer src_layer, dst_layer;
    tde_layer_init(src_layer, frame.frame_obj.data.pitch_width, frame.frame_obj.data.pitch_height,
                   0, 0, frame.width, frame.height, GP_FORMAT_YUV420SP, frame.frame_obj.data.data);
    tde_layer_init(dst_layer, DISPLAY_W, DISPLAY_H,
                   0, 0, DISPLAY_W, DISPLAY_H, GP_FORMAT_RGB888, rgb_buf);
    ak_tde_opt_format(&src_layer, &dst_layer);

    uint8_t *argb_dst = (uint8_t *)g_ctx.display_buffer->data;
    const uint8_t *rgb_src = (uint8_t *)rgb_buf;
    for (uint32_t i = 0; i < DISPLAY_W * DISPLAY_H; i++)
    {
        argb_dst[i * 4 + 0] = rgb_src[i * 3 + 0];
        argb_dst[i * 4 + 1] = rgb_src[i * 3 + 1];
        argb_dst[i * 4 + 2] = rgb_src[i * 3 + 2];
        argb_dst[i * 4 + 3] = ALPHA_OPAQUE;
    }
    ak_mem_dma_free(rgb_buf);
    lv_timer_del(timer);
    lv_obj_set_style_bg_img_src(lv_scr_act_current(), g_ctx.display_buffer, LV_PART_MAIN);
}

static int record_play_decode_photo(void)
{
    if (!g_ctx.mjpeg_dec_ctx)
    {
        g_ctx.mjpeg_dec_ctx = video_decodec_open(DISPLAY_W, DISPLAY_H, MJPEG_ENC_TYPE);
        if (!g_ctx.mjpeg_dec_ctx)
            return -1;
    }

    if (!g_ctx.display_buffer)
    {
        g_ctx.display_buffer = lv_mem_alloc(sizeof(lv_img_dsc_t));
        if (!g_ctx.display_buffer)
        {
            video_decodec_close(g_ctx.mjpeg_dec_ctx);
            g_ctx.mjpeg_dec_ctx = NULL;
            return -1;
        }
        lv_memset_00(g_ctx.display_buffer, sizeof(lv_img_dsc_t));

        uint32_t data_size = DISPLAY_W * DISPLAY_H * 4;
        g_ctx.display_buffer->data = malloc(data_size);
        if (!g_ctx.display_buffer->data)
        {
            lv_mem_free(g_ctx.display_buffer);
            g_ctx.display_buffer = NULL;
            video_decodec_close(g_ctx.mjpeg_dec_ctx);
            g_ctx.mjpeg_dec_ctx = NULL;
            return -1;
        }
        g_ctx.display_buffer->header.w = DISPLAY_W;
        g_ctx.display_buffer->header.h = DISPLAY_H;
        g_ctx.display_buffer->header.cf = LV_IMG_CF_TRUE_COLOR;
        g_ctx.display_buffer->data_size = data_size;
    }

    char file_path[FILE_PATH_BUF_SIZE] = {0};
    if (Amy_media_info_conver_path(g_media_browse.ctx, &g_media_browse.info,
                                   file_path, sizeof(file_path) - 1) != 0)
        return -1;

    if (g_media_browse.info.mask & MEDIA_ATTR_NEW)
        Amy_media_attr_new_clean(g_media_browse.ctx);

    unsigned long frame_size = sat_file_size_get(file_path);
    if (frame_size == 0)
        return -1;

    stream_data *stream = video_decodec_buf_get(g_ctx.mjpeg_dec_ctx, frame_size);
    if (!stream)
    {
        video_decodec_close(g_ctx.mjpeg_dec_ctx);
        g_ctx.mjpeg_dec_ctx = NULL;
        return -1;
    }

    int fd = open(file_path, O_RDONLY);
    if (fd < 0)
    {
        video_decodec_buf_put(g_ctx.mjpeg_dec_ctx, stream);
        video_decodec_close(g_ctx.mjpeg_dec_ctx);
        g_ctx.mjpeg_dec_ctx = NULL;
        return -1;
    }

    int ret = read(fd, stream->data, stream->size);
    close(fd);

    if (ret != (int)stream->size)
    {
        video_decodec_buf_put(g_ctx.mjpeg_dec_ctx, stream);
        video_decodec_close(g_ctx.mjpeg_dec_ctx);
        g_ctx.mjpeg_dec_ctx = NULL;
        return -1;
    }

    if (video_decodec_send_stream(g_ctx.mjpeg_dec_ctx, stream))
    {
        video_decodec_close(g_ctx.mjpeg_dec_ctx);
        g_ctx.mjpeg_dec_ctx = NULL;
        return -1;
    }

    lv_timer_create_recycle(record_play_decode_frame_cb, DECODE_TIMER_MS, NULL);
    return 0;
}

static int record_play_decode_video(void)
{
    char file_path[FILE_PATH_BUF_SIZE] = {0};
    if (Amy_media_info_conver_path(g_media_browse.ctx, &g_media_browse.info,
                                   file_path, sizeof(file_path) - 1) != 0)
        return -1;

    if (g_media_browse.info.mask & MEDIA_ATTR_NEW)
        Amy_media_attr_new_clean(g_media_browse.ctx);

    g_ctx.video_ctx = avi_player_open(file_path);
    if (!g_ctx.video_ctx)
        return -1;

    /* 获取视频总时长（初始时 remaining == total） */
    int remain = 0;
    if (video_play_param_get(g_ctx.video_ctx, &remain) == 0)
        g_ctx.total_duration = remain;

    /* 启动进度条更新定时器 */
    g_ctx.progress_timer = lv_timer_create(record_play_progress_timer_cb, PROGRESS_TIMER_MS, NULL);

    /* 自动开始播放（avi_player_open 后状态为 INIT_LOADED → PAUSE） */
    avi_player_toggle_play(g_ctx.video_ctx);

    return 0;
}

static void record_play_decode_and_play(void)
{
    if (g_ctx.page_type == PAGE_PHOTO_PLAY)
        record_play_decode_photo();
    else
        record_play_decode_video();
}

/************************** 资源释放 **************************/

static void record_play_deinit(void)
{
    if (g_ctx.progress_timer)
    {
        lv_timer_del(g_ctx.progress_timer);
        g_ctx.progress_timer = NULL;
    }
    if (g_ctx.mjpeg_dec_ctx)
    {
        video_decodec_close(g_ctx.mjpeg_dec_ctx);
        g_ctx.mjpeg_dec_ctx = NULL;
    }
    if (g_ctx.video_ctx)
    {
        avi_player_close(g_ctx.video_ctx);
        g_ctx.video_ctx = NULL;
    }
    if (g_ctx.display_buffer)
    {
        if (g_ctx.display_buffer->data)
            free((void *)g_ctx.display_buffer->data);
        lv_mem_free(g_ctx.display_buffer);
        g_ctx.display_buffer = NULL;
    }
    g_ctx.progress_bar = NULL;
    g_ctx.center_play_btn = NULL;
}

/************************** 页面生命周期 **************************/

static void Amy_lv_page_enter(record_play)
{
    g_ctx.mjpeg_dec_ctx = NULL;
    g_ctx.video_ctx = NULL;
    g_ctx.display_buffer = NULL;
    g_ctx.progress_timer = NULL;
    g_ctx.total_duration = 0;
    g_ctx.progress_bar = NULL;
    g_ctx.center_play_btn = NULL;
    g_ctx.play_state = PLAY_STATE_PAUSED;

    if (g_media_browse.info.mask & MEDIA_TYPE_VIDEO)
        g_ctx.page_type = PAGE_VIDEO_PLAY;
    else
        g_ctx.page_type = PAGE_PHOTO_PLAY;

    /* 屏幕点击 → 暂停/播放 */
    lv_obj_add_flag(lv_scr_act_current(), LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(lv_scr_act_current(), record_play_screen_click, LV_EVENT_CLICKED, NULL);

    record_play_prev_btn_create();
    record_play_next_btn_create();
    record_play_channel_label_create();
    record_play_datetime_label_create();
    if (g_ctx.page_type == PAGE_VIDEO_PLAY)
    {
        record_play_progress_bar_create();
        record_play_center_play_btn_create();
        record_play_update_play_btn_visible();
    }
    record_play_back_btn_create();
    Amy_event_cb_register(AMY_EVENT_ID_VIDEO_PLAY_STATUS, record_play_status_handle);

    record_play_decode_and_play();
    Amy_event_cb_register(AMY_EVENT_ID_SD_CHANGE, record_play_sd_change_cb);
}

static void Amy_lv_page_quit(record_play)
{
    lv_obj_clear_flag(lv_scr_act_current(), LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_event_cb(lv_scr_act_current(), record_play_screen_click);
    record_play_deinit();
    lv_obj_set_style_bg_img_src(lv_scr_act_current(), NULL, LV_PART_MAIN);
    Amy_event_cb_register(AMY_EVENT_ID_VIDEO_PLAY_STATUS, NULL);

    if (lv_next_page_get() != lv_page(record_play) && g_media_browse.ctx)
    {
        Amy_media_close(g_media_browse.ctx);
        g_media_browse.ctx = NULL;
    }
    Amy_event_cb_register(AMY_EVENT_ID_SD_CHANGE, NULL);
}

Amy_lv_page_create(record_play);
