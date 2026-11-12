#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <linux/fb.h>

#include "tkl_display_fbdev.h"

#define FB_DEV_PATH "/dev/fb0"

/************************** anyka platform start ***********************************/

#include "ak_common.h"
#include "ak_mem.h"
#include "ak_common_graphics.h"
#include "ak_tde.h"
#include "ak_vo.h"
#include "ak_log.h"

#define TARGET_COLOR_FORMAT         GP_FORMAT_RGB565
#define LV_COLOR_DEPTH  16

#if (LV_COLOR_DEPTH == 32)
#define LAYER_SRC_FORMAT            GP_FORMAT_ARGB8888
#define IMAGE_BUFFER_SIZE           (320 * 480 * 4)
#elif (LV_COLOR_DEPTH == 24)
#define LAYER_SRC_FORMAT            GP_FORMAT_RGB888
#define IMAGE_BUFFER_SIZE           (320 * 480 * 3)
#elif (LV_COLOR_DEPTH == 16)
#define LAYER_SRC_FORMAT            GP_FORMAT_RGB565
#define IMAGE_BUFFER_SIZE           (320 * 480 * 2)
#endif

extern unsigned long g_ak_draw_buf_paddr;
static int console_fd = -1;
static struct fb_var_screeninfo vinfo;
static struct fb_fix_screeninfo finfo;
/************************** anyka platform end   ***********************************/

struct fb_buffer {
	uint32_t id;
	uint32_t pitch;
	uint32_t width;
	uint32_t height;
	uint32_t bpp;
	uint32_t size;
	void *data;
};

struct fbdev {
	int fd;
	uint32_t width, height;
	uint32_t mmWidth, mmHeight;
	uint32_t format;
	uint32_t dpi;
	TKL_DISP_POWER_MODE_E power;
	TKL_DISP_ROTATION_E rotation;
	TKL_DISP_EVENT_HANDLER_S *event_cb;
	struct fb_fix_screeninfo fi;
	struct fb_var_screeninfo vi;
	struct fb_buffer bufs[2];
	struct fb_buffer *cur_buf, *draw_buf;
	bool double_buffered;
};

static int fbdev_format_to_tkl(struct fb_var_screeninfo *vi)
{
	if ((vi->transp.length == 8) && (vi->red.length == 8) && (vi->green.length == 8) && (vi->blue.length == 8)) {
		if (vi->transp.offset == 24) {
			if (vi->red.offset == 0) {
				return TKL_DISP_PIXEL_FMT_ABGR;
			} else {
				return TKL_DISP_PIXEL_FMT_ARGB;
			}
		} else {
			if (vi->red.offset == 24)
				return TKL_DISP_PIXEL_FMT_RGBA;
		}
	} else if ((vi->red.length == 5) && (vi->green.length == 6) && (vi->blue.length == 5)) {
		return TKL_DISP_PIXEL_FMT_RGB565;
	}

	return 0;
}

OPERATE_RET fbdev_init(TKL_DISP_DEVICE_S *tdev, TKL_DISP_EVENT_HANDLER_S *event_cb)
{
	struct fbdev *dev;

	dev = malloc(sizeof(struct fbdev));
	if (!dev) {
		return OPRT_MALLOC_FAILED;
	}

	dev->fd = open(FB_DEV_PATH, O_RDWR|O_CLOEXEC);
	if (dev->fd < 0) {
		printf("Open fbdev %s failed\n", FB_DEV_PATH);
		goto err_fd;
	}

	if (ioctl(dev->fd, FBIOGET_FSCREENINFO, &dev->fi) < 0) {
		printf("Failed to get fb finfo\n");
		goto err_fd;;
	}

    // Print current kernel set buffer num
    if(dev->fi.reserved[0] == 0)
        printf("[AK-FB] Current fb is only 1, %d\n", dev->fi.reserved[0]);
    else
        printf("[AK-FB] Current fb is 2, %d\n", dev->fi.reserved[0]);

	if (ioctl(dev->fd, FBIOGET_VSCREENINFO, &dev->vi) < 0) {
		printf("Failed to get fb vinfo\n");
		goto err_fd;
	}
    
    vinfo = dev->vi;
    finfo = dev->fi;
    console_fd = dev->fd;

    vinfo.xres = vinfo.xres_virtual;
    vinfo.yres = vinfo.yres_virtual;
    ak_print_normal(MODULE_ID_APP, "vinfo.bits_per_pixel = %d; vinfo.xres = %d; vinfo.yres = %d\n", vinfo.bits_per_pixel, vinfo.xres, vinfo.yres);

    if (finfo.reserved[0] == 1)
    {
        ak_print_normal(MODULE_ID_APP, "vinfo.reserved[ 0 ] = %d\n", vinfo.reserved[0]);
        vinfo.reserved[0] = 0;
        if (ioctl(console_fd, FBIOPUT_VSCREENINFO, &vinfo) < 0)
        {
            ak_print_error_ex(MODULE_ID_APP, "FBIOPUT_VSCREENINFO failed!\n");
            return;
        }
    } 

	dev->width = dev->vi.xres;
	dev->height = dev->vi.yres;
    dev->dpi = 165;
    dev->format = fbdev_format_to_tkl(&dev->vi);
    dev->rotation = dev->vi.rotate;

	dev->bufs[0].id = 0;
	dev->bufs[0].width = dev->width;
	dev->bufs[0].height = dev->height;
	dev->bufs[0].pitch = dev->fi.line_length;
	dev->bufs[0].size = dev->height * dev->fi.line_length;
	dev->bufs[0].bpp = dev->vi.bits_per_pixel;
	//dev->bufs[0].data = bits;

	if (dev->vi.yres * dev->fi.line_length * 2 <= dev->fi.smem_len) {
		dev->double_buffered = true;
		dev->bufs[1].id = 1;
		dev->bufs[1].width = dev->width;
		dev->bufs[1].height = dev->height;
		dev->bufs[1].pitch = dev->fi.line_length;
		dev->bufs[1].size = dev->height * dev->fi.line_length;
		dev->bufs[1].bpp = dev->vi.bits_per_pixel;
		//dev->bufs[1].data = bits + dev->vi.yres * dev->fi.line_length;
	} else {
		dev->double_buffered = false;
	}
	dev->cur_buf = NULL;
	dev->draw_buf = &dev->bufs[0];
	dev->event_cb = event_cb;
    tdev->device_info = (PVOID_T)dev;

	return OPRT_OK;
err_fd:
	free(dev);
	return OPRT_COM_ERROR;
}

OPERATE_RET fbdev_deinit(TKL_DISP_DEVICE_S *tdev)
{
	struct fbdev *dev = (struct fbdev *)tdev->device_info;
	close(dev->fd);
	free(dev);
	tdev->device_info = NULL;

	return OPRT_OK;
}

OPERATE_RET fbdev_set_info(TKL_DISP_DEVICE_S *tdev, TKL_DISP_INFO_S *info)
{
	struct fbdev *dev = (struct fbdev *)tdev->device_info;
	struct fb_var_screeninfo var;

	if (ioctl(dev->fd, FBIOGET_VSCREENINFO, &var) < 0) {
		printf("Failed to get fb vinfo\n");
		return OPRT_NOT_SUPPORTED;
	}

    var.activate = FB_ACTIVATE_NOW | FB_ACTIVATE_FORCE;
    var.xres = var.xres_virtual;
    var.yres = var.yres_virtual;

	if (ioctl(dev->fd, FBIOPUT_VSCREENINFO, &var) < 0) {
		printf("Failed to set fb variable information\n");
		return OPRT_NOT_SUPPORTED;
	}

	if (ioctl(dev->fd, FBIOGET_FSCREENINFO, &dev->fi) < 0) {
		printf("Failed to get fb fixed information\n");
		return OPRT_NOT_SUPPORTED;
	}

	/* update new vscreeninfo */
	memcpy(&dev->vi, &var, sizeof(var));
	if (dev->vi.yres * dev->fi.line_length * 2 <= dev->fi.smem_len) {
		dev->double_buffered = true;
		dev->bufs[1].id = 1;
		dev->bufs[1].width = dev->width;
		dev->bufs[1].height = dev->height;
		dev->bufs[1].pitch = dev->fi.line_length;
		dev->bufs[1].size = dev->height * dev->fi.line_length;
		dev->bufs[1].bpp = dev->vi.bits_per_pixel;
		dev->bufs[1].data = dev->bufs[0].data + dev->vi.yres * dev->fi.line_length;
	} else {
		dev->double_buffered = false;
	}
	dev->width = info->width;
	dev->height = info->height;
	dev->format = info->format;

	return OPRT_OK;
}

OPERATE_RET fbdev_get_info(TKL_DISP_DEVICE_S *tdev, TKL_DISP_INFO_S *info)
{
	struct fbdev *dev = (struct fbdev *)tdev->device_info;
	unsigned int htotal, vtotal;

	if (!info)
		return OPRT_INVALID_PARM;

	if (dev->rotation == TKL_DISP_ROTATION_90 || dev->rotation == TKL_DISP_ROTATION_270) {
		info->width = dev->height;
		info->height = dev->width;
	} else {
		info->width = dev->width;
		info->height = dev->height;
	}

	htotal = dev->vi.xres + dev->vi.left_margin + dev->vi.right_margin + dev->vi.hsync_len;
	vtotal = dev->vi.yres + dev->vi.upper_margin + dev->vi.lower_margin + dev->vi.vsync_len;

	info->format = dev->format;
	info->bpp = dev->vi.bits_per_pixel;
	info->fps = dev->vi.pixclock / (htotal * vtotal);
	info->dpi = dev->dpi;

	return OPRT_OK;
}

static uint32_t *pixel_at(struct fb_buffer *fb, TKL_DISP_ROTATION_E rotation, int x, int y)
{
	int row_pixels = fb->pitch / (fb->bpp / 8);

	switch (rotation) {
	case TKL_DISP_ROTATION_0:
		return (uint32_t *)(fb->data) + y * row_pixels + x;
	case TKL_DISP_ROTATION_90:
		return (uint32_t *)(fb->data) + x * row_pixels + (fb->width - y - 1);
	case TKL_DISP_ROTATION_180:
		return (uint32_t *)(fb->data) + (fb->height - 1 - y) * row_pixels + (fb->width - 1 - x);
	case TKL_DISP_ROTATION_270:
		return (uint32_t *)(fb->data) + (fb->height - 1 - x) * row_pixels + y;
	default:
		printf("invalid rotation %d", rotation);
	}
	return NULL;
}

static void incr_x(TKL_DISP_ROTATION_E rotation, uint32_t **p, int row_pixels)
{
	switch (rotation) {
	case TKL_DISP_ROTATION_90:
		*p = *p + row_pixels;
		break;
	case TKL_DISP_ROTATION_180:
		*p = *p - 1;
		break;
	case TKL_DISP_ROTATION_270:
		*p = *p - row_pixels;
		break;
	default:
		*p = *p + 1;
		break;
	}
}

static void incr_y(TKL_DISP_ROTATION_E rotation, uint32_t **p, int row_pixels)
{
	switch (rotation) {
	case TKL_DISP_ROTATION_90:
		*p = *p - 1;
		break;
	case TKL_DISP_ROTATION_180:
		*p = *p - row_pixels;
		break;
	case TKL_DISP_ROTATION_270:
		*p = *p + 1;
		break;
	default:
		*p = *p + row_pixels;
		break;
	}
}

OPERATE_RET fbdev_blit(TKL_DISP_DEVICE_S *tdev, TKL_DISP_FRAMEBUFFER_S *fb, TKL_DISP_RECT_S *rect)
{
	return OPRT_OK;
    
	struct fbdev *dev = (struct fbdev *)tdev->device_info;
	struct fb_buffer *fbuf = dev->draw_buf;
	int pixel_bytes = fbuf->bpp / 8;
	int src_row_bytes = fb->rect.width * pixel_bytes;
	int dst_row_bytes = fbuf->pitch;
	int rotation = dev->rotation;

	/* Partial update */
	if ((rect->width != dev->width || rect->height != dev->height) && dev->cur_buf)
		memcpy(fbuf->data, dev->cur_buf->data, fbuf->size);

	if (rotation) {
		int src_row_pixels = fb->rect.width;
		int row_pixels = fbuf->pitch / pixel_bytes;
		const uint32_t *src_py = (uint32_t *)fb->buffer + fb->rect.y * src_row_bytes / 4 + fb->rect.x;
		uint32_t *dst_py = pixel_at(fbuf, rotation, rect->x, rect->y);
        int x, y;
		for (y = 0; y < rect->height; y += 1) {
			const uint32_t *src_px = src_py;
			uint32_t *dst_px = dst_py;
			for (x = 0; x < rect->width; x += 1) {
				*dst_px = *src_px++;
				incr_x(rotation, &dst_px, row_pixels);
			}
			src_py += src_row_pixels;
			incr_y(rotation, &dst_py, row_pixels);
		}
	} else {
		uint8_t *src_p = (uint8_t *)fb->buffer + (fb->rect.y * src_row_bytes) + (fb->rect.x * pixel_bytes);
		uint8_t *dst_p = (uint8_t *)fbuf->data + (rect->y * dst_row_bytes) + (rect->x * pixel_bytes);
        int i = 0;
		for (i = 0; i < rect->height; ++i) {
			memcpy(dst_p, src_p, src_row_bytes);
			src_p += src_row_bytes;
			dst_p += dst_row_bytes;
		}
	}

	return OPRT_OK;
}

OPERATE_RET fbdev_fill(TKL_DISP_DEVICE_S *tdev, TKL_DISP_RECT_S *rect, TKL_DISP_COLOR_U color)
{
	struct fbdev *dev = (struct fbdev *)tdev->device_info;
	struct fb_buffer *fbuf = dev->draw_buf;
	int row_pixels = fbuf->pitch / (fbuf->bpp / 8);

	uint32_t *p = pixel_at(fbuf, dev->rotation, rect->x, rect->y);
    int x, y;
	for (y = rect->y; y < rect->y + rect->height; ++y) {
		uint32_t *px = p;
		for (x = rect->x; x < rect->x + rect->width; ++x) {
			*px = color.full;
			incr_x(dev->rotation, &px, row_pixels);
		}
		incr_y(dev->rotation, &p, row_pixels);
	}

	return OPRT_OK;
}

static OPERATE_RET fbdev_pan_display(struct fbdev *dev)
{
	dev->vi.yoffset = dev->vi.yres * dev->draw_buf->id;
#if 0
	if (ioctl(dev->fd, FBIOPAN_DISPLAY, &dev->vi) < 0) {
		printf("FBIOPAN_DISPLAY failed\n");
		return OPRT_COM_ERROR;
	}
#endif
	return OPRT_OK;
}

static inline void set_tde_layer(struct ak_tde_layer *p_tde_layer, unsigned short lw, unsigned short lh,
                                 unsigned short x, unsigned short y, unsigned short w, unsigned short h,
                                 unsigned int p, enum ak_gp_format f);

/**********************************************************************
 * 函数名称： set_tde_layer
 * 功能描述： 设置图层描述结构
 * 输入参数：
 * 输出参数：
 * 返 回 值： void
 ***********************************************************************/
static inline void set_tde_layer(struct ak_tde_layer *p_tde_layer, unsigned short lw, unsigned short lh,
                                 unsigned short x, unsigned short y, unsigned short w, unsigned short h,
                                 unsigned int p, enum ak_gp_format f)
{
    if (p_tde_layer != NULL)
    {
        p_tde_layer->width = lw;
        p_tde_layer->height = lh;
        p_tde_layer->pos_left = x;
        p_tde_layer->pos_top = y;
        p_tde_layer->pos_width = w;
        p_tde_layer->pos_height = h;
        p_tde_layer->phyaddr = p;
        p_tde_layer->format_param = f;
    }

    return;
}

static void wait_lcd_available(void)
{
    int m;
    char c_get_time = AK_FALSE;
    struct ak_timeval ak_timeval_begin, ak_timeval_end;
    int lcdc_status;
    unsigned long lcdc_is_busy;

    /* 轮询等待lcdc控制器可用, 这里设置查询次数为 5 */
    for (m = 0; m < 5; m++)
    {
        if (c_get_time == AK_TRUE)
        {
            ak_get_ostime(&ak_timeval_end);
            /* 在循环时间超过 20 ms 则直接退出循环 */
            if (ak_diff_ms_time(&ak_timeval_end, &ak_timeval_begin) > 20) break;
        }

        /* 0x4680,should match the cmd defined in ak_fb.c */
        if (ioctl(console_fd, 0x4680, &lcdc_is_busy) < 0) lcdc_status = AK_FAILED;
        else lcdc_status = AK_SUCCESS;

        if ((lcdc_status == AK_SUCCESS) && (lcdc_is_busy == AK_FALSE)) break;

        if (c_get_time == AK_FALSE) {
            ak_get_ostime(&ak_timeval_begin);
            c_get_time = AK_TRUE;
        }
        ak_sleep_ms(1);
    }
}

OPERATE_RET fbdev_flush(TKL_DISP_DEVICE_S *tdev)
{
    struct ak_tde_layer tde_layer_dst; // 图层描述结构2,绑定fb的物理地址

    if(vinfo.reserved[0] == 0) {
        vinfo.reserved[0] = 1;
        set_tde_layer(&tde_layer_dst, vinfo.xres, vinfo.yres, 0, 0, vinfo.xres, vinfo.yres, finfo.smem_start + finfo.smem_len / 2, TARGET_COLOR_FORMAT);
    }else {
        vinfo.reserved[0] = 0;
        set_tde_layer(&tde_layer_dst, vinfo.xres, vinfo.yres, 0, 0, vinfo.xres, vinfo.yres, finfo.smem_start, TARGET_COLOR_FORMAT);
    }

    wait_lcd_available();

    struct ak_tde_layer tde_layer_src; // 图层描述结构1,绑定lvgl绘图物理地址
    set_tde_layer(&tde_layer_src, vinfo.xres, vinfo.yres, 0, 0, vinfo.xres, vinfo.yres, g_ak_draw_buf_paddr, LAYER_SRC_FORMAT);
    ak_tde_opt_blit(&tde_layer_src, &tde_layer_dst);

    /* May be some direct update command is required */
    if (finfo.reserved[0] == AK_TRUE) {
        if (ioctl(console_fd, FBIOPUT_VSCREENINFO, &vinfo) < 0) ak_print_error_ex(MODULE_ID_APP, "FBIOPUT_VSCREENINFO failed!\n");
    }

    return OPRT_OK;
}

OPERATE_RET fbdev_wait_vsync(TKL_DISP_DEVICE_S *tdev)
{
	struct fbdev *dev = (struct fbdev *)tdev->device_info;

	if (ioctl(dev->fd, FBIO_WAITFORVSYNC, 0) < 0) {
		printf("FBIO_WAITFORVSYNC failed\n");
		return OPRT_COM_ERROR;
	}

	if (dev->event_cb && dev->event_cb->vsync_cb) {
		struct timeval tv;
		gettimeofday(&tv, NULL);
		dev->event_cb->vsync_cb(tdev->device_port, tv.tv_sec * 1000000000 + tv.tv_usec);
	}

	return OPRT_OK;
}

TKL_DISP_FRAMEBUFFER_S *fbdev_alloc_fb(TKL_DISP_DEVICE_S *tdev)
{
	return NULL;
}

void fbdev_free_fb(TKL_DISP_DEVICE_S *tdev, TKL_DISP_FRAMEBUFFER_S *buf)
{
}

OPERATE_RET fbdev_set_power_mode(TKL_DISP_DEVICE_S *tdev, TKL_DISP_POWER_MODE_E mode)
{
	struct fbdev *dev = (struct fbdev *)tdev->device_info;
	int fb_blank = 0;

	if (!dev) {
		return OPRT_NOT_FOUND;
	}

	if (mode == TKL_DISP_POWER_OFF) {
		fb_blank = FB_BLANK_POWERDOWN;
	} else {
		fb_blank = FB_BLANK_UNBLANK;
	}

	int err = ioctl(dev->fd, FBIOBLANK, fb_blank ? FB_BLANK_POWERDOWN : FB_BLANK_UNBLANK);
	if (err < 0) {
		printf("Set power mode %d failed\n", mode);
		return OPRT_COM_ERROR;
	} else {
		dev->power = mode;
	}

	return OPRT_OK;
}

OPERATE_RET fbdev_get_power_mode(TKL_DISP_DEVICE_S *tdev, TKL_DISP_POWER_MODE_E *mode)
{
	struct fbdev *dev = (struct fbdev *)tdev->device_info;

	*mode = dev->power;

	return OPRT_OK;
}
