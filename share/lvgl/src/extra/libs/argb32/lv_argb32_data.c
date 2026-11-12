/**
 * @file lv_bmp.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "../../../lvgl.h"
#if LV_USE_ARGB32

#include <string.h>
#include "device/lv_firmware_resource.h"
/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static lv_res_t decoder_info(lv_img_decoder_t *decoder, const void *src, lv_img_header_t *header);
static lv_res_t decoder_open(lv_img_decoder_t *dec, lv_img_decoder_dsc_t *dsc);

static void decoder_close(lv_img_decoder_t *dec, lv_img_decoder_dsc_t *dsc);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
void lv_argb32_data_init(void)
{
    lv_img_decoder_t *dec = lv_img_decoder_create();
    lv_img_decoder_set_info_cb(dec, decoder_info);
    lv_img_decoder_set_open_cb(dec, decoder_open);
    lv_img_decoder_set_close_cb(dec, decoder_close);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/**
 * Get info about a PNG image
 * @param src can be file name or pointer to a C array
 * @param header store the info here
 * @return LV_RES_OK: no error; LV_RES_INV: can't get the info
 */
static lv_res_t decoder_info(lv_img_decoder_t *decoder, const void *src, lv_img_header_t *header)
{
    LV_UNUSED(decoder);

    lv_img_src_t src_type = lv_img_src_get_type(src); /*Get the source type*/

    /*If it's a BMP file...*/
    if (src_type == LV_IMG_SRC_FILE)
    {
        firamware_resource *resource = (firamware_resource *)src;
        char *data = firmware_read_resouce_bin(resource);
        if (data && (strncmp(data, "S@AC", 4) == 0))
        { /*Check the extension*/
            /*Save the data in the header*/
            char *ptr = data + 4;
            header->w = ptr[0] | (ptr[1] << 8) | (ptr[2] << 16) | (ptr[3] << 24);
            ptr = data + 4 + sizeof(unsigned int);
            header->h = ptr[0] | (ptr[1] << 8) | (ptr[2] << 16) | (ptr[3] << 24);
            header->always_zero = 0;
#if LV_COLOR_DEPTH == 32
            header->cf = LV_IMG_CF_TRUE_COLOR_ALPHA;
#else
            header->cf = LV_IMG_CF_TRUE_COLOR;
#endif
            // AMY_LOG_INFO("%#X,argb32:%dx%d \n", data, header->w, header->h);
            return LV_RES_OK;
        }
    }
    return LV_RES_INV; /*If didn't succeeded earlier then it's an error*/
}

/**
 * Open a PNG image and return the decided image
 * @param src can be file name or pointer to a C array
 * @param style style of the image object (unused now but certain formats might use it)
 * @return pointer to the decoded image or `LV_IMG_DECODER_OPEN_FAIL` if failed
 */
static lv_res_t decoder_open(lv_img_decoder_t *decoder, lv_img_decoder_dsc_t *dsc)
{
    LV_UNUSED(decoder);
    LV_LOG_INFO("dsc->src_type%d \n", dsc->src_type);
    /*If it's a PNG file...*/
    if (dsc->src_type == LV_IMG_SRC_FILE)
    {
        firamware_resource *resource = (firamware_resource *)dsc->src;
        char *data = firmware_read_resouce_bin(resource);
        if ((!data) || (strncmp(data, "S@AC", 4)))
        {
            return LV_RES_INV; /*Check the extension*/
        }
        dsc->img_data = ((unsigned char *)data + 4 + sizeof(unsigned int) + sizeof(unsigned int));
        //  AMY_LOG_INFO("decodec img_data:%#x ,data=%#x\n", dsc->img_data, data);
        return LV_RES_OK;
    }
    return LV_RES_INV; /*If not returned earlier then it failed*/
}

/**
 * Free the allocated resources
 */
static void decoder_close(lv_img_decoder_t *decoder, lv_img_decoder_dsc_t *dsc)
{
    if ((dsc) && (dsc->img_data))
    {
        // AMY_LOG_INFO("free argb32:%dx%d \n", dsc->header.w, dsc->header.h);
        uint8_t *data = (uint8_t *)(dsc->img_data - 4 - sizeof(unsigned int) - sizeof(unsigned int));
        firmware_resource_release((void *)data);
        dsc->img_data = NULL;
    }
    LV_UNUSED(decoder);
}

#endif /*LV_USE_BMP*/
