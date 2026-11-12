#include "standby_photo_frame.h"
#include "page_photo_play.h"
#include "Amy_file_manager.h"
#include "Amy_tfcard.h"
#include "lv_common.h"
#include <stdlib.h>
#include <stdio.h>

/************************** 布局常量 **************************/
#define DISPLAY_W 1024
#define DISPLAY_H 600

/************************** 状态 **************************/
static struct
{
    lv_img_dsc_t img_dsc;
    lv_timer_t *slide_timer;
    int photo_index;
    bool running;
} g_frame = {0};

/************************** 函数声明 **************************/
static void photo_frame_display_current(void);
static void photo_frame_slide_timer_cb(lv_timer_t *timer);
static void photo_frame_deinit(void);

/************************** 显示当前照片 **************************/

static void photo_frame_display_current(void)
{
    int total = Amy_fm_get_count(FM_DIR_PHOTO);
    if (total <= 0)
        return;

    fm_file_info_t info;
    if (Amy_fm_get_file(FM_DIR_PHOTO, g_frame.photo_index, &info) != 0)
        return;

    char file_path[512];
    snprintf(file_path, sizeof(file_path), TFCARD_MOUNT_PATH "/picture/%s", info.name);

    uint8_t *argb = (uint8_t *)malloc(DISPLAY_W * DISPLAY_H * 4);
    if (!argb)
        return;
    /* 解码+缩放走 libjpeg 降采样 + TDE，阻塞在几十毫秒量级，不再冻结 LVGL 任务 */
    if (jpeg_decode_scaled_tde(file_path, argb, DISPLAY_W, DISPLAY_H) != 0)
    {
        free(argb);
        return;
    }

    if (g_frame.img_dsc.data)
        free((void *)g_frame.img_dsc.data);

    g_frame.img_dsc.header.w = DISPLAY_W;
    g_frame.img_dsc.header.h = DISPLAY_H;
    g_frame.img_dsc.header.always_zero = 0;
    g_frame.img_dsc.header.cf = LV_IMG_CF_TRUE_COLOR;
    g_frame.img_dsc.data_size = DISPLAY_W * DISPLAY_H * 4;
    g_frame.img_dsc.data = (const uint8_t *)argb;

    lv_obj_set_style_bg_img_src(lv_scr_act_current(), &g_frame.img_dsc, LV_PART_MAIN);
}

/************************** 自动轮播定时器 **************************/

static void photo_frame_slide_timer_cb(lv_timer_t *timer)
{
    int total = Amy_fm_get_count(FM_DIR_PHOTO);
    if (total <= 0)
        return;

    if (g_frame.photo_index + 1 < total)
        g_frame.photo_index++;
    else
        g_frame.photo_index = 0;

    photo_frame_display_current();
}

/************************** 资源释放 **************************/

static void photo_frame_deinit(void)
{
    if (g_frame.slide_timer)
    {
        lv_timer_del(g_frame.slide_timer);
        g_frame.slide_timer = NULL;
    }
    if (g_frame.img_dsc.data)
    {
        free((void *)g_frame.img_dsc.data);
        g_frame.img_dsc.data = NULL;
    }
    g_frame.photo_index = 0;
    g_frame.running = false;
}

/************************** 公开 API **************************/

int standby_photo_frame_start(int interval_sec)
{
    if (g_frame.running)
        return 0;

    Amy_fm_refresh(FM_DIR_PHOTO);

    int total = Amy_fm_get_count(FM_DIR_PHOTO);
    if (total <= 0)
        return -1;

    g_frame.photo_index = 0;
    photo_frame_display_current();

    g_frame.slide_timer = lv_timer_create(photo_frame_slide_timer_cb, interval_sec * 1000, NULL);
    g_frame.running = true;
    return 0;
}

void standby_photo_frame_stop(void)
{
    if (!g_frame.running)
        return;

    photo_frame_deinit();
    lv_obj_set_style_bg_img_src(lv_scr_act_current(), NULL, LV_PART_MAIN);
}

bool standby_photo_frame_is_running(void)
{
    return g_frame.running;
}
