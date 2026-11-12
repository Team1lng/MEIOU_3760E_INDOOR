/**
 * @file lv_extra.h
 *
 */

#ifndef LV_EXTRA_H
#define LV_EXTRA_H

#ifdef __cplusplus
extern "C" {
#endif

    /*********************
     *      INCLUDES
     *********************/

#include "layouts/lv_layouts.h"
#include "libs/lv_libs.h"
#include "others/lv_others.h"
#include "themes/lv_themes.h"
#include "widgets/lv_widgets.h"

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
         * Initialize the extra components
         */
    void lv_extra_init(const char* mtd_name,unsigned  long offset,unsigned long size);

    /**********************
     *      MACROS
     **********************/
#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_EXTRA_H*/
