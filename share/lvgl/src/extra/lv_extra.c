/**
 * @file lv_extra.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "../lvgl.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/
void *lv_font_small = NULL;
void *lv_font_normal = NULL;
void *lv_font_large = NULL;
void *lv_font_large_s = NULL;
void *lv_font_plus = NULL;

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/
#if LV_USE_FREETYPE
static void *lv_font_base_create(const char *mtd_name, unsigned long ttf_base, unsigned long ttf_size, int weight)
{
    lv_ft_info_t ft_info;
    memset(&ft_info, 0, sizeof(lv_ft_info_t));
    ft_info.weight = weight;
    ft_info.name = mtd_name;
    ft_info.style = FT_FONT_STYLE_NORMAL;
    ft_info.mem = (void *)ttf_base;
    ft_info.mem_size = ttf_size;
    if (!lv_ft_font_init(&ft_info))
    {
        LV_LOG_ERROR("\n---------->>create failed.:%p size:%d\n", ft_info.mem, ft_info.mem_size);
    }
    return ft_info.font;
}
#endif
/**********************
 *   GLOBAL FUNCTIONS
 **********************/
void lv_extra_init(const char *mtd_name, unsigned long ttf_base, unsigned long ttf_size)
{
#if LV_USE_FLEX
    lv_flex_init();
#endif

#if LV_USE_GRID
    lv_grid_init();
#endif

#if LV_USE_MSG
    lv_msg_init();
#endif

#if LV_USE_FS_FATFS != '\0'
    lv_fs_fatfs_init();
#endif

#if LV_USE_FS_LITTLEFS != '\0'
    lv_fs_littlefs_init();
#endif

#if LV_USE_FS_STDIO != '\0'
    lv_fs_stdio_init();
#endif

#if LV_USE_FS_POSIX != '\0'
    lv_fs_posix_init();
#endif

#if LV_USE_FS_WIN32 != '\0'
    lv_fs_win32_init();
#endif

#if LV_USE_FFMPEG
    lv_ffmpeg_init();
#endif

#if LV_USE_PNG
    lv_png_init();
#endif

#if LV_USE_ARGB32
    lv_argb32_data_init();
#endif

#if LV_USE_JPG
    lv_jpg_init();
#endif
#if LV_USE_MP4
    lv_mp4_init();
#endif

#if LV_USE_SJPG
    lv_split_jpeg_init();
#endif

#if LV_USE_BMP
    lv_bmp_init();
#endif

#if LV_USE_FREETYPE
    /*Init freetype library*/
#if LV_FREETYPE_CACHE_SIZE >= 0
    lv_freetype_init(LV_FREETYPE_CACHE_FT_FACES, LV_FREETYPE_CACHE_FT_SIZES, LV_FREETYPE_CACHE_SIZE);
#else
    lv_freetype_init(0, 0, 0);
#endif
    lv_font_small = lv_font_base_create(mtd_name, ttf_base, ttf_size, 18);
    lv_font_normal = lv_font_base_create(mtd_name, ttf_base, ttf_size, 20);
    lv_font_large = lv_font_base_create(mtd_name, ttf_base, ttf_size, 30);
    lv_font_large_s = lv_font_base_create(mtd_name, ttf_base, ttf_size, 52);
    lv_font_plus = lv_font_base_create(mtd_name, ttf_base, ttf_size, 110);
#endif
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
