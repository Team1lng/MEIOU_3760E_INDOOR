/**
 * @file lv_mp4.h
 *
 */

#ifndef LV_MP4_H
#define LV_MP4_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "../../../lv_conf_internal.h"
#if LV_USE_MP4

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Register the PNG decoder functions in LVGL
 */
void lv_mp4_init(void);

/**********************
 *      MACROS
 **********************/

#endif /*LV_USE_PNG*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*LV_PNG_H*/
