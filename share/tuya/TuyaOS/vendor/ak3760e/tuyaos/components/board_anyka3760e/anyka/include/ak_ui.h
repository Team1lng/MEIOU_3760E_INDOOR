#ifndef _AK_APP_UI_H
#define _AK_APP_UI_H

#include <stdint.h>
#include "ak_vo.h"

typedef struct rect_info
{
    int width;
    int height;
    int x;
    int y;
}AK_RECT;

/* get the ui lib version */
const char* ak_app_ui_get_version(void);

/*
 * ak_app_screen_init  --  init the screen
 * @width[IN]   :   screen width
 * @height[IN]  :   screen height
 * @degree[IN]  :   degree for rotation
 * return AK_SUCCESS if successful, -1 if failed
 */
int ak_app_screen_init(int width, int height, AK_GP_ROTATE degree);

/*
 * ak_app_screen_deinit  --  deinit the screen device
 * return AK_SUCCESS if successful, -1 if failed
 */
int ak_app_screen_deinit(void);

/*
 * ak_app_win_create  --  create window on screen
 * @win_id[OUT] :   window id;
 * @rect[IN]    :   window size and position on screen;
 * @format[IN]  :   window pixel format
 * return AK_SUCCESS if successful, -1 if failed
 */
int ak_app_win_create(int *win_id, AK_RECT rect, AK_GP_FORMAT format);

/*
 * ak_app_win_destroy  --  destroy the window 
 * @win_id[IN] :   window id;
 * return AK_SUCCESS if successful, -1 if failed
 */
int ak_app_win_destroy(int win_id);

/*
 * ak_app_attach_video_to_win  --  attach the vdec to dst window 
 * @win_id[IN] :   window id;
 * @vdec_id[IN] :   vdec id;
 * return AK_SUCCESS if successful, -1 if failed
 */
int ak_app_attach_video_to_win(int win_id, int vdec_id);

/*
 * ak_app_detach_video_to_win  --  attach the vdec to dst window 
 * @win_id[IN] :   window id;
 * @vdec_id[IN] :   vdec id;
 * return AK_SUCCESS if successful, -1 if failed
 */
int ak_app_detach_video_to_win(int win_id, int vdec_id);

/*
 * ak_app_show_win  --  display the window 
 * @win_id[IN] :   window id;
 * return AK_SUCCESS if successful, -1 if failed
 */
int ak_app_show_win(int win_id);

/*
 * ak_app_hide_win  --  hide the window 
 * @win_id[IN] :   window id;
 * return AK_SUCCESS if successful, -1 if failed
 */
int ak_app_hide_win(int win_id);

/*
 * ak_app_ui_init  --  init the ui fb device
 * @fb_dev[IN]   :   dev path
 * return AK_SUCCESS if successful, -1 if failed
 * Note : the function should be called after you set the fb param through calling ioctl func.
 */
int ak_app_ui_init(char *fb_dev);

/*
 * ak_app_ui_deinit  --  deinit the ui fb device
 * return AK_SUCCESS if successful, -1 if failed
 */
int ak_app_ui_deinit(void);

/*
 * ak_app_ui_set_colorkey  --  set the colorkey for UI display
 * @colorkey[IN] :   the color to be ignored
 * return AK_SUCCESS if successful, -1 if failed
 */
int ak_app_ui_set_colorkey(unsigned int colorkey);

/*
 * ak_app_ui_update  --  update the ui to display
 * @win_id[IN] :   window to display
 * return AK_SUCCESS if successful, -1 if failed
 * Note :the layer you use should be the BG or GUI  layer.
 */
int ak_app_ui_update(int win_id);
//(enum ak_vo_layer layer_no, AK_GP_FORMAT format, struct ak_vo_layer_out *info);

#endif
