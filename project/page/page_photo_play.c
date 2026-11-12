#include "page_tabel.h"
#include "lv_common.h"
#include "page_common.h"
#include "Amy_tfcard.h"
#include "Amy_file_manager.h"
#include "Amy_sdk.h"
#include "user_data.h"
#include "user_standby.h"
#include "page_photo_play.h"
#include "Amy_log.h"
#include <jpeglib.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <setjmp.h>
#include "ak_mem.h"
#include "ak_common.h"
#include "ak_common_graphics.h"
#include "ak_tde.h"

/************************** 布局常量  **************************/
#define DISPLAY_W 1024
#define DISPLAY_H 600

/* 底部三按钮: PREV | PAUSE/PLAY | NEXT, 35x35, 间距 35px */
#define BOTTOM_BTN_W 35
#define BOTTOM_BTN_H 35
#define BOTTOM_BTN_GAP 35
#define BOTTOM_BTN_Y 521
#define PAUSE_BTN_X 494
#define PREV_BTN_X (PAUSE_BTN_X - BOTTOM_BTN_W - BOTTOM_BTN_GAP)
#define NEXT_BTN_X (PAUSE_BTN_X + BOTTOM_BTN_W + BOTTOM_BTN_GAP)

/* 返回按钮 */
#define BACK_BTN_X 929
#define BACK_BTN_Y 505
#define BACK_BTN_W 67
#define BACK_BTN_H 67
#define EXT_CLICK 34

/************************** libjpeg 错误处理 **************************/
struct my_error_mgr
{
    struct jpeg_error_mgr pub;
    jmp_buf setjmp_buffer;
};
typedef struct my_error_mgr *my_error_ptr;

static void my_error_exit(j_common_ptr cinfo)
{
    my_error_ptr myerr = (my_error_ptr)cinfo->err;
    (*cinfo->err->output_message)(cinfo);
    longjmp(myerr->setjmp_buffer, 1);
}

/************************** 上下文 **************************/
typedef struct
{
    lv_img_dsc_t img_dsc;
    lv_timer_t *slide_timer;
    lv_obj_t *pause_btn;
    bool is_playing; /* true=自动轮播中, false=暂停 */
    firamware_resource pause_res;
    firamware_resource play_res;
    firamware_resource pause_focus_res;
    firamware_resource play_focus_res;
} photo_play_ctx_t;

static photo_play_ctx_t g_ctx = {0};

/************************** 函数声明 **************************/
static void photo_play_prev_btn_click(lv_event_t *ev);
static void photo_play_pause_btn_click(lv_event_t *ev);
static void photo_play_next_btn_click(lv_event_t *ev);
static void photo_play_back_btn_click(lv_event_t *ev);

static void photo_play_prev_btn_create(void);
static void photo_play_pause_btn_create(void);
static void photo_play_next_btn_create(void);
static void photo_play_back_btn_create(void);

static void photo_play_slide_timer_cb(lv_timer_t *timer);
static void photo_play_display_current(void);
static void photo_play_deinit(void);
static void photo_play_update_pause_icon(void);

/************************** 软件 JPEG 解码 (libjpeg) **************************/

void *jpeg_decode_file(const char *file_path, int *out_w, int *out_h)
{
    FILE *jpeg_fd = fopen(file_path, "rb");
    if (!jpeg_fd)
        return NULL;
    struct jpeg_decompress_struct cinfo;
    struct my_error_mgr jerr;
    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;

    if (setjmp(jerr.setjmp_buffer))
    {
        jpeg_destroy_decompress(&cinfo);
        fclose(jpeg_fd);
        return NULL;
    }
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, jpeg_fd);
    (void)jpeg_read_header(&cinfo, TRUE);

    cinfo.out_color_space = JCS_RGB;
    (void)jpeg_start_decompress(&cinfo);
    int row_stride = cinfo.output_width * cinfo.output_components;
    int argb_stride = cinfo.output_width * 4;
    int img_h = cinfo.output_height;
    int img_w = cinfo.output_width;
    unsigned char *img_data = malloc(argb_stride * img_h);
    if (!img_data)
    {
        jpeg_destroy_decompress(&cinfo);
        fclose(jpeg_fd);
        return NULL;
    }

    JSAMPARRAY buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr)&cinfo, JPOOL_IMAGE, row_stride, 1);

    while (cinfo.output_scanline < cinfo.output_height)
    {
        (void)jpeg_read_scanlines(&cinfo, buffer, 1);

        /* RGB → ARGB8888 (little-endian: B,G,R,A) */
        unsigned char *dst = img_data + (cinfo.output_scanline - 1) * argb_stride;
        for (int x = 0; x < img_w; x++)
        {
            dst[x * 4 + 0] = buffer[0][x * 3 + 2]; /* B */
            dst[x * 4 + 1] = buffer[0][x * 3 + 1]; /* G */
            dst[x * 4 + 2] = buffer[0][x * 3 + 0]; /* R */
            dst[x * 4 + 3] = 0xFF;                 /* A */
        }
    }

    (void)jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    fclose(jpeg_fd);

    *out_w = img_w;
    *out_h = img_h;
    return img_data;
}

/************************** 双线性插值缩放 (ARGB8888) **************************/

void *scale_argb_bilinear(const uint8_t *src, int src_w, int src_h,
                          int dst_w, int dst_h)
{
    /* 1:1 直接拷贝，跳过无意义的缩放 */
    if (src_w == dst_w && src_h == dst_h)
    {
        uint8_t *copy = malloc(dst_w * dst_h * 4);
        if (copy)
            memcpy(copy, src, dst_w * dst_h * 4);
        return copy;
    }

    uint8_t *dst = malloc(dst_w * dst_h * 4);
    if (!dst)
        return NULL;

    /* 定点插值系数 (Q16.16 坐标 + Q8.8 权重)。ARMv5 无 FPU，float 会被软件模拟，极慢 */
    int scale_x = ((src_w - 1) << 16) / (dst_w - 1);
    int scale_y = ((src_h - 1) << 16) / (dst_h - 1);

    for (int dy = 0; dy < dst_h; dy++)
    {
        int sy_fp = dy * scale_y;
        int sy0 = sy_fp >> 16;
        int sy1 = (sy0 + 1 < src_h) ? sy0 + 1 : sy0;
        int fy = (sy_fp >> 8) & 0xFF;
        int wy0 = 256 - fy;

        for (int dx = 0; dx < dst_w; dx++)
        {
            int sx_fp = dx * scale_x;
            int sx0 = sx_fp >> 16;
            int sx1 = (sx0 + 1 < src_w) ? sx0 + 1 : sx0;
            int fx = (sx_fp >> 8) & 0xFF;
            int wx0 = 256 - fx;

            const uint8_t *p00 = src + (sy0 * src_w + sx0) * 4;
            const uint8_t *p10 = src + (sy0 * src_w + sx1) * 4;
            const uint8_t *p01 = src + (sy1 * src_w + sx0) * 4;
            const uint8_t *p11 = src + (sy1 * src_w + sx1) * 4;

            uint8_t *d = dst + (dy * dst_w + dx) * 4;
            for (int c = 0; c < 4; c++)
            {
                int t0 = (p00[c] * wx0 + p10[c] * fx) >> 8;
                int t1 = (p01[c] * wx0 + p11[c] * fx) >> 8;
                d[c] = (uint8_t)((t0 * wy0 + t1 * fy) >> 8);
            }
        }
    }
    return dst;
}

/************************** JPEG 降采样解码 + TDE 硬件缩放 (ARGB8888) **************************/

/*
 * 与 jpeg_decode_file + scale_argb_bilinear 等价，但把重活交给硬件：
 * libjpeg 在 DCT 阶段降采样（1/2、1/4、1/8），再由 TDE 缩放到目标尺寸。
 * 阻塞时间从数百毫秒降到几十毫秒，避免冻结 LVGL 任务导致丢点击。
 * dst 需调用方提供 dst_w*dst_h*4 字节。
 */
int jpeg_decode_scaled_tde(const char *file_path, uint8_t *dst, int dst_w, int dst_h)
{
    if (!dst || dst_w <= 0 || dst_h <= 0)
        return -1;

    FILE *jpeg_fd = fopen(file_path, "rb");
    if (!jpeg_fd)
        return -1;

    struct jpeg_decompress_struct cinfo;
    struct my_error_mgr jerr;
    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;
    if (setjmp(jerr.setjmp_buffer))
    {
        jpeg_destroy_decompress(&cinfo);
        fclose(jpeg_fd);
        return -1;
    }
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, jpeg_fd);
    (void)jpeg_read_header(&cinfo, TRUE);

    cinfo.out_color_space = JCS_RGB;

    /* 取解码后仍不小于目标尺寸的最大降采样比（libjpeg 只支持 1/1、1/2、1/4、1/8），
     * 保证 TDE 只做缩小、不放大 */
    cinfo.scale_num = 1;
    cinfo.scale_denom = 1;
    while (cinfo.scale_denom < 8 &&
           cinfo.image_width / (cinfo.scale_denom * 2) >= (unsigned int)dst_w &&
           cinfo.image_height / (cinfo.scale_denom * 2) >= (unsigned int)dst_h)
    {
        cinfo.scale_denom *= 2;
    }

    (void)jpeg_start_decompress(&cinfo);

    int dec_w = cinfo.output_width;
    int dec_h = cinfo.output_height;
    int row_stride = dec_w * cinfo.output_components;

    uint8_t *raw = (uint8_t *)ak_mem_dma_alloc(MODULE_ID_GUI, dec_h * row_stride);
    if (!raw)
    {
        AMY_LOG_ERROR("photo decode dma alloc failed: %d bytes\n", dec_h * row_stride);
        jpeg_destroy_decompress(&cinfo);
        fclose(jpeg_fd);
        return -1;
    }

    JSAMPARRAY buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr)&cinfo, JPOOL_IMAGE, row_stride, 1);
    while (cinfo.output_scanline < cinfo.output_height)
    {
        (void)jpeg_read_scanlines(&cinfo, buffer, 1);
        memcpy(raw + (cinfo.output_scanline - 1) * row_stride, buffer[0], row_stride);
    }

    uint8_t *scaled = (uint8_t *)ak_mem_dma_alloc(MODULE_ID_GUI, dst_w * dst_h * 3);
    if (!scaled)
    {
        AMY_LOG_ERROR("photo scale dma alloc failed: %d bytes\n", dst_w * dst_h * 3);
        ak_mem_dma_free(raw);
        jpeg_destroy_decompress(&cinfo);
        fclose(jpeg_fd);
        return -1;
    }

    struct ak_tde_layer src_layer, dst_layer;
    tde_layer_init(src_layer, dec_w, dec_h, 0, 0, dec_w, dec_h, GP_FORMAT_RGB888, raw);
    tde_layer_init(dst_layer, dst_w, dst_h, 0, 0, dst_w, dst_h, GP_FORMAT_BGR888, scaled);
    int ret = ak_tde_opt_scale(&src_layer, &dst_layer);
    if (ret != 0)
    {
        AMY_LOG_ERROR("tde scale failed(%d): %s\n", ret, file_path);
    }
    else
    {
        /* TDE 只能输出 3 字节，这里扩成 LVGL 需要的 4 字节 (B,G,R,A) */
        for (int i = 0; i < dst_w * dst_h; i++)
        {
            dst[i * 4 + 0] = scaled[i * 3 + 0];
            dst[i * 4 + 1] = scaled[i * 3 + 1];
            dst[i * 4 + 2] = scaled[i * 3 + 2];
            dst[i * 4 + 3] = 0xFF;
        }
    }
    ak_mem_dma_free(scaled);
    ak_mem_dma_free(raw);
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    fclose(jpeg_fd);
    return ret == 0 ? 0 : -1;
}

/************************** 暂停按钮图标切换  **************************/

static void photo_play_update_pause_icon(void)
{
    if (!g_ctx.pause_btn)
        return;
    void *src = g_ctx.is_playing
                    ? (void *)&g_ctx.pause_res /* 播放中 → 显示暂停图标 */
                    : (void *)&g_ctx.play_res; /* 暂停中 → 显示播放图标 */
    void *focus = g_ctx.is_playing
                      ? (void *)&g_ctx.pause_focus_res /* 播放中 → 按下显示暂停_focus */
                      : (void *)&g_ctx.play_focus_res; /* 暂停中 → 按下显示播放_focus */
    lv_obj_set_style_bg_img_src(g_ctx.pause_btn, src, LV_PART_MAIN);
    lv_obj_set_style_bg_img_src(g_ctx.pause_btn, focus, LV_STATE_PRESSED);
}

/************************** 回调 **************************/

static void photo_play_prev_btn_click(lv_event_t *ev)
{
    /* 仅在暂停时允许手动切换 */
    if (g_ctx.is_playing)
        return;

    int total = Amy_fm_get_count(FM_DIR_PHOTO);
    if (total <= 0)
        return;

    if (g_fm_photo_index > 0)
        g_fm_photo_index--;
    else
        g_fm_photo_index = total - 1;

    photo_play_display_current();
}

static void photo_play_pause_btn_click(lv_event_t *ev)
{
    g_ctx.is_playing = !g_ctx.is_playing;
    if (g_ctx.is_playing)
    {
        standby_timer_close();
        if (g_ctx.slide_timer)
            lv_timer_reset(g_ctx.slide_timer);
    }
    else
    {
        standby_timer_restart(true, true);
    }
    photo_play_update_pause_icon();
}

static void photo_play_next_btn_click(lv_event_t *ev)
{
    /* 仅在暂停时允许手动切换 */
    if (g_ctx.is_playing)
        return;

    int total = Amy_fm_get_count(FM_DIR_PHOTO);
    if (total <= 0)
        return;

    if (g_fm_photo_index + 1 < total)
        g_fm_photo_index++;
    else
        g_fm_photo_index = 0;

    photo_play_display_current();
}

static void photo_play_back_btn_click(lv_event_t *ev)
{
    Amy_lv_page_goto(lv_page(photo), LV_SCR_LOAD_ANIM_NONE, );
}

static void photo_play_sd_change_cb(void *arg)
{
    if (!(Amy_tfcard_status() & SD_STATE_INSERT))
        Amy_lv_page_goto(lv_page(home), LV_SCR_LOAD_ANIM_NONE, );
    page_common_tfcard_change_cb(arg);
}

/************************** 自动轮播定时器 **************************/

static void photo_play_slide_timer_cb(lv_timer_t *timer)
{
    if (!g_ctx.is_playing)
        return;

    int total = Amy_fm_get_count(FM_DIR_PHOTO);
    if (total <= 0)
        return;

    if (g_fm_photo_index + 1 < total)
        g_fm_photo_index++;
    else
        g_fm_photo_index = 0;

    photo_play_display_current();
}

/************************** UI 创建 **************************/

static void photo_play_prev_btn_create(void)
{
    lv_resource_init(icon, ICON_MEDIA_PREV_PNG_OFFSET, ICON_MEDIA_PREV_PNG_SIZE);
    lv_resource_init(icon_focus, ICON_MEDIA_PREV_FOCUS_PNG_OFFSET, ICON_MEDIA_PREV_FOCUS_PNG_SIZE);
    lv_common_img_btn_create(lv_scr_act_current(), PHOTO_PLAY_PREV_BTN_ID,
                             PREV_BTN_X, BOTTOM_BTN_Y, BOTTOM_BTN_W, BOTTOM_BTN_H,
                             photo_play_prev_btn_click, true,
                             LV_OPA_TRANSP, 0x000000, LV_OPA_TRANSP, 0x000000,
                             0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                             0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                             &icon, LV_OPA_COVER, 0x02A9FF, LV_ALIGN_CENTER, &icon_focus);
}

static void photo_play_pause_btn_create(void)
{
    /* 记录 play/pause 图标资源用于切换 */
    lv_resource_init(play_icon, ICON_MEDIA_PLAY_PNG_OFFSET, ICON_MEDIA_PLAY_PNG_SIZE);
    lv_resource_init(play_focus, ICON_MEDIA_PLAY_FOCUS_PNG_OFFSET, ICON_MEDIA_PLAY_FOCUS_PNG_SIZE);
    lv_resource_init(pause_icon, ICON_MEDIA_PAUSE_PNG_OFFSET, ICON_MEDIA_PAUSE_PNG_SIZE);
    lv_resource_init(pause_focus, ICON_MEDIA_PAUSE_FOCUS_PNG_OFFSET, ICON_MEDIA_PAUSE_FOCUS_PNG_SIZE);
    memcpy(&g_ctx.play_res, &play_icon, sizeof(firamware_resource));
    memcpy(&g_ctx.pause_res, &pause_icon, sizeof(firamware_resource));
    memcpy(&g_ctx.play_focus_res, &play_focus, sizeof(firamware_resource));
    memcpy(&g_ctx.pause_focus_res, &pause_focus, sizeof(firamware_resource));

    g_ctx.pause_btn = lv_common_img_btn_create(lv_scr_act_current(), PHOTO_PLAY_PAUSE_BTN_ID,
                                               PAUSE_BTN_X, BOTTOM_BTN_Y, BOTTOM_BTN_W, BOTTOM_BTN_H,
                                               photo_play_pause_btn_click, true,
                                               LV_OPA_TRANSP, 0x000000, LV_OPA_TRANSP, 0x000000,
                                               0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                               0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                               &g_ctx.pause_res, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &pause_focus);
    photo_play_update_pause_icon();
}

static void photo_play_next_btn_create(void)
{
    lv_resource_init(icon, ICON_MEDIA_NEXT_PNG_OFFSET, ICON_MEDIA_NEXT_PNG_SIZE);
    lv_resource_init(icon_focus, ICON_MEDIA_NEXT_FOCUS_PNG_OFFSET, ICON_MEDIA_NEXT_FOCUS_PNG_SIZE);
    lv_common_img_btn_create(lv_scr_act_current(), PHOTO_PLAY_NEXT_BTN_ID,
                             NEXT_BTN_X, BOTTOM_BTN_Y, BOTTOM_BTN_W, BOTTOM_BTN_H,
                             photo_play_next_btn_click, true,
                             LV_OPA_TRANSP, 0x000000, LV_OPA_TRANSP, 0x000000,
                             0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                             0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                             &icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &icon_focus);
}

static void photo_play_back_btn_create(void)
{
    lv_resource_init(icon, ICON_COMMON_BACK_PNG_OFFSET, ICON_COMMON_BACK_PNG_SIZE);
    lv_resource_init(icon_focus, ICON_COMMON_BACK_FOCUS_PNG_OFFSET, ICON_COMMON_BACK_FOCUS_PNG_SIZE);
    lv_obj_t *btn = lv_common_img_btn_create(lv_scr_act_current(), PHOTO_PLAY_BACK_BTN_ID,
                                             BACK_BTN_X, BACK_BTN_Y, BACK_BTN_W, BACK_BTN_H,
                                             photo_play_back_btn_click, true,
                                             LV_OPA_TRANSP, 0x101010, LV_OPA_TRANSP, 0x101010,
                                             0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                             0, 0, LV_BORDER_SIDE_NONE, LV_OPA_TRANSP, 0,
                                             &icon, LV_OPA_TRANSP, 0x02A9FF, LV_ALIGN_CENTER, &icon_focus);
    lv_obj_set_ext_click_area(btn, EXT_CLICK);
}

/************************** 显示当前照片 **************************/

static void photo_play_display_current(void)
{
    fm_file_info_t info;
    if (Amy_fm_get_file(FM_DIR_PHOTO, g_fm_photo_index, &info) != 0)
        return;

    char file_path[512];
    snprintf(file_path, sizeof(file_path), TFCARD_MOUNT_PATH "/picture/%s", info.name);

    int img_w, img_h;
    void *img_data = jpeg_decode_file(file_path, &img_w, &img_h);
    if (!img_data)
        return;

    /* 缩放到全屏 */
    void *scaled = scale_argb_bilinear(img_data, img_w, img_h, DISPLAY_W, DISPLAY_H);
    free(img_data);
    if (!scaled)
        return;

    if (g_ctx.img_dsc.data)
        free((void *)g_ctx.img_dsc.data);

    g_ctx.img_dsc.header.w = DISPLAY_W;
    g_ctx.img_dsc.header.h = DISPLAY_H;
    g_ctx.img_dsc.header.always_zero = 0;
    g_ctx.img_dsc.header.cf = LV_IMG_CF_TRUE_COLOR;
    g_ctx.img_dsc.data_size = DISPLAY_W * DISPLAY_H * 4;
    g_ctx.img_dsc.data = (const uint8_t *)scaled;

    lv_obj_set_style_bg_img_src(lv_scr_act_current(), &g_ctx.img_dsc, LV_PART_MAIN);
}

/************************** 资源释放 **************************/

static void photo_play_deinit(void)
{
    if (g_ctx.slide_timer)
    {
        lv_timer_del(g_ctx.slide_timer);
        g_ctx.slide_timer = NULL;
    }
    if (g_ctx.img_dsc.data)
    {
        free((void *)g_ctx.img_dsc.data);
        g_ctx.img_dsc.data = NULL;
    }
    g_ctx.pause_btn = NULL;
}

/************************** 页面生命周期 **************************/

static void Amy_lv_page_enter(photo_play)
{
    memset(&g_ctx, 0, sizeof(g_ctx));
    g_ctx.is_playing = false; /* 默认单张预览 */

    photo_play_prev_btn_create();
    photo_play_pause_btn_create();
    photo_play_next_btn_create();
    photo_play_back_btn_create();

    /* 显示当前照片 */
    photo_play_display_current();

    /* 自动轮播间隔: 使用场景模式设置的时间 (默认 5 秒) */
    int slide_interval = user_data_get()->private_cfg.scene.digital_photo_sw_time * 1000;
    g_ctx.slide_timer = lv_timer_create(photo_play_slide_timer_cb, slide_interval, NULL);
    Amy_event_cb_register(AMY_EVENT_ID_SD_CHANGE, photo_play_sd_change_cb);
}

static void Amy_lv_page_quit(photo_play)
{
    photo_play_deinit();
    lv_obj_set_style_bg_img_src(lv_scr_act_current(), NULL, LV_PART_MAIN);
    standby_timer_restart(true, true);
    Amy_event_cb_register(AMY_EVENT_ID_SD_CHANGE, NULL);
}

Amy_lv_page_create(photo_play);
