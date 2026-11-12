#include "Amy_log.h"
#include <stdio.h>
#include <setjmp.h>
#include "jpeglib.h"
struct my_error_mgr {
    struct jpeg_error_mgr pub;	/* "public" fields */

    jmp_buf setjmp_buffer;	/* for return to caller */
};

static void my_error_exit(j_common_ptr cinfo)
{
    /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
    struct my_error_mgr* myerr = (struct my_error_mgr*)cinfo->err;

    /* Always display the message. */
    /* We could postpone this until after returning, if we chose. */
    (*cinfo->err->output_message) (cinfo);

    /* Return control to the setjmp point */
    longjmp(myerr->setjmp_buffer, 1);
}

int jpeg_decodec(void* in_data, int in_size, void* out_data, int w, int h) {
    struct jpeg_decompress_struct cinfo;
    struct my_error_mgr jerr;
    JSAMPARRAY buffer;
    int row_stride;

    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;

    if (setjmp(jerr.setjmp_buffer)) {
        jpeg_destroy_decompress(&cinfo);
        AMY_LOG_ERROR("setjmp buffer\n");
        return -1;
    }

    jpeg_create_decompress(&cinfo);
    jpeg_mem_src(&cinfo, in_data, in_size);

    jpeg_read_header(&cinfo, TRUE);
    if ((cinfo.image_width != w) || (cinfo.image_height != h)) {
        jpeg_destroy_decompress(&cinfo);
        AMY_LOG_ERROR("output pixel error(%dx%d) != (%dx%d)\n", cinfo.image_width, cinfo.image_height, w, h);
        return -1;
    }


    jpeg_start_decompress(&cinfo);

    row_stride = cinfo.output_width * cinfo.output_components;
    buffer = (*cinfo.mem->alloc_sarray)	((j_common_ptr)&cinfo, JPOOL_IMAGE, row_stride, 1);

    char* dst = out_data;
    while (cinfo.output_scanline < cinfo.output_height) {
        (void)jpeg_read_scanlines(&cinfo, buffer, 1);
        memcpy(dst, buffer[0], row_stride);
        dst += row_stride;
    }

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    return 0;
}