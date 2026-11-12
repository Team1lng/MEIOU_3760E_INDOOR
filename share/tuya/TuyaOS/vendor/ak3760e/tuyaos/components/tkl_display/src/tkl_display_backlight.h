#ifndef __TKL_DISPLAY_BACKLIGHT_H__
#define __TKL_DISPLAY_BACKLIGHT_H__

#include "tkl_display.h"

#ifdef __cplusplus
    extern "C" {
#endif

int tkl_set_brightness_common(int brightness);
int tkl_get_brightness_common(void);

#ifdef __cplusplus
}
#endif

#endif