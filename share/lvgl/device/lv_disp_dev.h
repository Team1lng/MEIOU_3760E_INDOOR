#ifndef LV_DISP_DEV_H
#define LV_DISP_DEV_H
#include "lvgl.h"
#ifdef __cplusplus
extern "C"
{
#endif
    typedef struct {
        int spx, spy, spw, sph, sw, sh;
        int sfmt;
        unsigned long sphy;
        int dpx, dpy, dpw, dph;
    }video_mode_attr;

    typedef  int (*video_mode_attr_func)(video_mode_attr*, void* user_data);

    typedef struct {
        video_mode_attr_func func;
        void* user_data;
    }video_mode_cb_attr;

    void lv_disp_device_init(int w, int h, void (*callback)(lv_disp_drv_t* disp_drv, const lv_area_t* area, lv_color_t* color_p), void* user_data);

    void lv_video_mode_enable(char enable);

    void Amy_gui_refresh(const lv_area_t*, unsigned char* data);

    void Amy_gui_display(void);

    int  Amy_rendering_area(lv_area_t* area);


    int Amy_video_mode_register_device(video_mode_attr_func func, void* user_data);

    /* 解码线程解出一帧新视频后调用，唤醒视频刷新线程立即渲染，避免轮询采样漏帧 */
    void Amy_video_frame_notify(void);
#ifdef __cplusplus
} /*extern "C"*/
#endif
#endif // LV_DISP_DEV_H
