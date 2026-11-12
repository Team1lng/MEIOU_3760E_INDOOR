#ifndef __TKL_DISPLAY_FBDEV_H__
#define __TKL_DISPLAY_FBDEV_H__

#include "tkl_display.h"

#ifdef __cplusplus
    extern "C" {
#endif

OPERATE_RET fbdev_init(TKL_DISP_DEVICE_S *tdev, TKL_DISP_EVENT_HANDLER_S *event_cb);
OPERATE_RET fbdev_deinit(TKL_DISP_DEVICE_S *tdev);
OPERATE_RET fbdev_set_info(TKL_DISP_DEVICE_S *tdev, TKL_DISP_INFO_S *info);
OPERATE_RET fbdev_get_info(TKL_DISP_DEVICE_S *tdev, TKL_DISP_INFO_S *info);
OPERATE_RET fbdev_blit(TKL_DISP_DEVICE_S *tdev, TKL_DISP_FRAMEBUFFER_S *fb, TKL_DISP_RECT_S *rect);
OPERATE_RET fbdev_fill(TKL_DISP_DEVICE_S *tdev, TKL_DISP_RECT_S *rect, TKL_DISP_COLOR_U color);
OPERATE_RET fbdev_flush(TKL_DISP_DEVICE_S *tdev);
OPERATE_RET fbdev_wait_vsync(TKL_DISP_DEVICE_S *tdev);
TKL_DISP_FRAMEBUFFER_S *fbdev_alloc_fb(TKL_DISP_DEVICE_S *tdev);
void fbdev_free_fb(TKL_DISP_DEVICE_S *tdev, TKL_DISP_FRAMEBUFFER_S *buf);
OPERATE_RET fbdev_set_power_mode(TKL_DISP_DEVICE_S *tdev, TKL_DISP_POWER_MODE_E mode);
OPERATE_RET fbdev_get_power_mode(TKL_DISP_DEVICE_S *tdev, TKL_DISP_POWER_MODE_E *mode);

#ifdef __cplusplus
}
#endif

#endif