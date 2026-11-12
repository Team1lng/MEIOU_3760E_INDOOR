#ifndef __TKL_DISPLAY_DRM_H__
#define __TKL_DISPLAY_DRM_H__

#include "tkl_display.h"

#ifdef __cplusplus
    extern "C" {
#endif

OPERATE_RET drm_init(TKL_DISP_DEVICE_S *tdev, TKL_DISP_EVENT_HANDLER_S *event_cb);
OPERATE_RET drm_deinit(TKL_DISP_DEVICE_S *tdev);
OPERATE_RET drm_set_info(TKL_DISP_DEVICE_S *tdev, TKL_DISP_INFO_S *info);
OPERATE_RET drm_get_info(TKL_DISP_DEVICE_S *tdev, TKL_DISP_INFO_S *info);
OPERATE_RET drm_blit(TKL_DISP_DEVICE_S *tdev, TKL_DISP_FRAMEBUFFER_S *fb, TKL_DISP_RECT_S *rect);
OPERATE_RET drm_fill(TKL_DISP_DEVICE_S *tdev, TKL_DISP_RECT_S *rect, TKL_DISP_COLOR_U color);
OPERATE_RET drm_flush(TKL_DISP_DEVICE_S *tdev);
OPERATE_RET drm_wait_vsync(TKL_DISP_DEVICE_S *tdev);
TKL_DISP_FRAMEBUFFER_S *drm_alloc_fb(TKL_DISP_DEVICE_S *tdev);
void drm_free_fb(TKL_DISP_DEVICE_S *tdev, TKL_DISP_FRAMEBUFFER_S *buf);
OPERATE_RET drm_set_power_mode(TKL_DISP_DEVICE_S *tdev, TKL_DISP_POWER_MODE_E mode);
OPERATE_RET drm_get_power_mode(TKL_DISP_DEVICE_S *tdev, TKL_DISP_POWER_MODE_E *mode);

#ifdef __cplusplus
}
#endif

#endif