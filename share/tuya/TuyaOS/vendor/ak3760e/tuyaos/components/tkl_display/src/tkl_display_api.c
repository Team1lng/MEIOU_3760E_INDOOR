#ifdef HAVE_IOTOS_CONFIG
#include "tuya_iot_config.h"
#endif
#include "tkl_display.h"
#ifdef TUYA_TKL_DISPLAY_DRM
#include "tkl_display_drm.h"
#endif
#ifdef TUYA_TKL_DISPLAY_FBDEV
#include "tkl_display_fbdev.h"
#endif
#include "tkl_display_backlight.h"

/**
 * @brief Init and config display device
 * 
 * @param display_device display device
 * @param cfg display configuration
 * @return OPERATE_RET 0 on success. A negative error code on error.
 */
OPERATE_RET tkl_disp_init(TKL_DISP_DEVICE_S *display_device, TKL_DISP_EVENT_HANDLER_S *event_handler)
{
#if defined(TUYA_TKL_DISPLAY_DRM)
	return drm_init(display_device, event_handler);
#elif defined(TUYA_TKL_DISPLAY_FBDEV)
	return fbdev_init(display_device, event_handler);
#else
	return OPRT_NOT_SUPPORTED;
#endif
}

/**
 * @brief Release display device
 * 
 * @param display_device display device
 * @return OPERATE_RET 0 on success. A negative error code on error.
 */
OPERATE_RET tkl_disp_deinit(TKL_DISP_DEVICE_S *display_device)
{
#if defined(TUYA_TKL_DISPLAY_DRM)
	return drm_deinit(display_device);
#elif defined(TUYA_TKL_DISPLAY_FBDEV)
	return fbdev_deinit(display_device);
#else
	return OPRT_NOT_SUPPORTED;
#endif
}

/**
 * @brief Set display info
 * 
 * @param display_device display device
 * @param info display device info
 * @return OPERATE_RET 
 */
OPERATE_RET tkl_disp_set_info(TKL_DISP_DEVICE_S *display_device, TKL_DISP_INFO_S *info)
{
#if defined(TUYA_TKL_DISPLAY_DRM)
	return drm_set_info(display_device, info);
#elif defined(TUYA_TKL_DISPLAY_FBDEV)
	return fbdev_set_info(display_device, info);
#else
	return OPRT_NOT_SUPPORTED;
#endif
}

/**
 * @brief Get display info
 * 
 * @param display_device display device
 * @param info display device info
 * @return OPERATE_RET 0 on success. A negative error code on error.
 */
OPERATE_RET tkl_disp_get_info(TKL_DISP_DEVICE_S *display_device, TKL_DISP_INFO_S *info)
{
#if defined(TUYA_TKL_DISPLAY_DRM)
	return drm_get_info(display_device, info);
#elif defined(TUYA_TKL_DISPLAY_FBDEV)
	return fbdev_get_info(display_device, info);
#else
	return OPRT_NOT_SUPPORTED;
#endif
}

/**
 * @brief 
 * 
 * @param display_device display device
 * @param buf framebuffer
 * @param rect destination area
 * @return OPERATE_RET 0 on success. A negative error code on error.
 */
OPERATE_RET tkl_disp_blit(TKL_DISP_DEVICE_S *display_device, TKL_DISP_FRAMEBUFFER_S *buf, TKL_DISP_RECT_S *rect)
{
#if defined(TUYA_TKL_DISPLAY_DRM)
	return drm_blit(display_device, buf, rect);
#elif defined(TUYA_TKL_DISPLAY_FBDEV)
	return fbdev_blit(display_device, buf, rect);
#else
	return OPRT_NOT_SUPPORTED;
#endif
}

/**
 * @brief Fill destination area with color
 * 
 * @param display_device display device
 * @param rect destination area to fill
 * @param color color to fill
 * @return OPERATE_RET 
 */
OPERATE_RET tkl_disp_fill(TKL_DISP_DEVICE_S *display_device, TKL_DISP_RECT_S *rect, TKL_DISP_COLOR_U color)
{
#if defined(TUYA_TKL_DISPLAY_DRM)
	return drm_fill(display_device, rect, color);
#elif defined(TUYA_TKL_DISPLAY_FBDEV)
	return fbdev_fill(display_device, rect, color);
#else
	return OPRT_NOT_SUPPORTED;
#endif
}

/**
 * @brief Flush buffers to display device
 * 
 * @param display_device display device
 * @return OPERATE_RET 0 on success. A negative error code on error.
 */
OPERATE_RET tkl_disp_flush(TKL_DISP_DEVICE_S *display_device)
{
#if defined(TUYA_TKL_DISPLAY_DRM)
	return drm_flush(display_device);
#elif defined(TUYA_TKL_DISPLAY_FBDEV)
	return fbdev_flush(display_device);
#else
	return OPRT_NOT_SUPPORTED;
#endif
}

/**
 * @brief Wait for vsync signal
 * 
 * @param display_device display device
 * @return OPERATE_RET 0 on success. A negative error code on error.
 */
OPERATE_RET tkl_disp_wait_vsync(TKL_DISP_DEVICE_S *display_device)
{
#if defined(TUYA_TKL_DISPLAY_DRM)
	return drm_wait_vsync(display_device);
#elif defined(TUYA_TKL_DISPLAY_FBDEV)
	return fbdev_wait_vsync(display_device);
#else
	return OPRT_NOT_SUPPORTED;
#endif
}

/**
 * @brief Set display brightness(Backlight or HSB)
 * 
 * @param display_device display device
 * @param brightness brightness
 * @return OPERATE_RET 0 on success. A negative error code on error.
 */
OPERATE_RET tkl_disp_set_brightness(TKL_DISP_DEVICE_S *display_device, INT_T brightness)
{
	return tkl_set_brightness_common(brightness);
}

/**
 * @brief Get display brightness(Backlight or HSB)
 * 
 * @param display_device display device
 * @param brightness brightness
 * @return OPERATE_RET 0 on success. A negative error code on error.
 */
OPERATE_RET tkl_disp_get_brightness(TKL_DISP_DEVICE_S *display_device, INT_T *brightness)
{
	int value = tkl_get_brightness_common();

	*brightness = value;

	return OPRT_OK;
}

/**
 * @brief Sets the display screen's power state
 * 
 * @param display_device display device
 * @param power_mode power state
 * @return OPERATE_RET 0 on success. A negative error code on error.
 */
OPERATE_RET tkl_disp_set_power_mode(TKL_DISP_DEVICE_S *display_device, TKL_DISP_POWER_MODE_E power_mode)
{
#if defined(TUYA_TKL_DISPLAY_DRM)
	return drm_set_power_mode(display_device, power_mode);
#elif defined(TUYA_TKL_DISPLAY_FBDEV)
	return fbdev_set_power_mode(display_device, power_mode);
#else
	return OPRT_NOT_SUPPORTED;
#endif
}

/**
 * @brief Gets the display screen's power state
 * 
 * @param display_device display device
 * @param power_mode power state
 * @return OPERATE_RET 0 on success. A negative error code on error.
 */
OPERATE_RET tkl_disp_get_power_mode(TKL_DISP_DEVICE_S *display_device, TKL_DISP_POWER_MODE_E *power_mode)
{
#if defined(TUYA_TKL_DISPLAY_DRM)
	return drm_get_power_mode(display_device, power_mode);
#elif defined(TUYA_TKL_DISPLAY_FBDEV)
	return fbdev_get_power_mode(display_device, power_mode);
#else
	return OPRT_NOT_SUPPORTED;
#endif
}

/**
 * @brief Alloc mapped framebuffer or layer
 * 
 * @param display_device display device
 * @return VOID_T* Pointer to mapped memory
 */
TKL_DISP_FRAMEBUFFER_S *tkl_disp_alloc_framebuffer(TKL_DISP_DEVICE_S *display_device)
{
#if defined(TUYA_TKL_DISPLAY_DRM)
	return drm_alloc_fb(display_device);
#elif defined(TUYA_TKL_DISPLAY_FBDEV)
	return fbdev_alloc_fb(display_device);
#else
	return NULL;
#endif
}

/**
 * @brief Free mapped framebuffer or layer
 * 
 * @param display_device display device
 * @param buf Pointer to mapped memory
 * @return VOID_T 
 */
VOID_T tkl_disp_free_framebuffer(TKL_DISP_DEVICE_S *display_device, TKL_DISP_FRAMEBUFFER_S *buf)
{
#if defined(TUYA_TKL_DISPLAY_DRM)
	drm_free_fb(display_device, buf);
#elif defined(TUYA_TKL_DISPLAY_FBDEV)
	fbdev_free_fb(display_device, buf);
#endif
}

/**
 * @brief Get capabilities supported by display(For external display device. e.g. HDMI/VGA)
 * 
 * @param display_device display device
 * @param cfg configurations
 * @return OPERATE_RET 0 on success. A negative error code on error.
 */
OPERATE_RET tkl_disp_get_capabilities(TKL_DISP_DEVICE_S *display_device, TKL_DISP_INFO_S **cfg)
{
	return OPRT_NOT_SUPPORTED;
}

/**
 * @brief Free capabilities get by tkl_disp_get_capabilities()
 * 
 * @param display_device display device
 * @param cfg configurations
 * @return OPERATE_RET 0 on success. A negative error code on error.
 */
OPERATE_RET tkl_disp_free_capabilities(TKL_DISP_DEVICE_S *display_device, TKL_DISP_INFO_S *cfg)
{
	return OPRT_NOT_SUPPORTED;
}
