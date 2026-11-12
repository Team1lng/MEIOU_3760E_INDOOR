
#include "../../../lvgl.h"

#if LV_USE_JPG
#include "lv_jpg.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
//#include "jpeglib.h"
#include <setjmp.h>
//#include "lv_common.h"

struct my_error_mgr {
	struct jpeg_error_mgr pub; /* "public" fields */
	jmp_buf setjmp_buffer;		 /* for return to caller */
};

typedef struct my_error_mgr* my_error_ptr;

static void my_error_exit(j_common_ptr cinfo) {
	/* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
	my_error_ptr myerr = (my_error_ptr)cinfo->err;

	/* Always display the message. */
	/* We could postpone this until after returning, if we chose. */
	(*cinfo->err->output_message)(cinfo);

	/* Return control to the setjmp point */
	longjmp(myerr->setjmp_buffer, 1);
}

/**********************
 *	STATIC PROTOTYPES
 **********************/
static lv_res_t decoder_info(struct _lv_img_decoder_t* decoder, const void* src, lv_img_header_t* header);
static lv_res_t decoder_open(lv_img_decoder_t* dec, lv_img_decoder_dsc_t* dsc);
static void decoder_close(lv_img_decoder_t* dec, lv_img_decoder_dsc_t* dsc);

// static void convert_color_depth(uint8_t *img, uint32_t px_cnt);

/*
 * @日期: 2022-08-09
 * @作者: leo.liu
 * @功能: 注册jpg解码库
 * @return:
 */
void lv_jpg_init(void) {
	lv_img_decoder_t* dec = lv_img_decoder_create();

	lv_img_decoder_set_info_cb(dec, decoder_info);
	lv_img_decoder_set_open_cb(dec, decoder_open);
	lv_img_decoder_set_close_cb(dec, decoder_close);
}

/**********************
 *	 STATIC FUNCTIONS
 **********************/
 /**
  * Get info about a PNG image
  * @param src can be file name or pointer to a C array
  * @param header store the info here
  * @return LV_RES_OK: no error; LV_RES_INV: can't get the info
  */
static lv_res_t decoder_info(struct _lv_img_decoder_t* decoder, const void* src, lv_img_header_t* header) {
	(void)decoder;																		/*Unused*/
	lv_img_src_t src_type = lv_img_src_get_type(src); /*Get the source type*/

	/*If it's a JPEG file...*/
	if (src_type == LV_IMG_SRC_FILE) {
		lv_img_src_info* info = (lv_img_src_info*)src;
		const char* fn = info->path;
		if ((strcmp(lv_fs_get_ext(fn), "jpg") == 0) || (strcmp(lv_fs_get_ext(fn), "JPG") == 0) ||
			(strcmp(lv_fs_get_ext(fn), "jpeg") == 0) || (strcmp(lv_fs_get_ext(fn), "JPEG") == 0)) {

			/* 	uint32_t *param = (uint32_t *)(fn + RESOURCE_FILE_PATH_MAX - sizeof(uint32_t) * 2 - 1); //(uint32_t *)&fn[strlen(fn) + 8];
			 */
			header->w = info->dst_width;
			header->h = info->dst_height;
			header->always_zero = 0;
			header->cf = LV_IMG_CF_TRUE_COLOR;
			/*The width and height are stored in Big endian format so convert them to little endian*/
			return LV_RES_OK;
		}
	}
	else if (src_type == LV_IMG_SRC_VARIABLE) {
	}

	return LV_RES_INV;
}


#ifdef PLATFORM_TYPE_X86_64
#include "ffmpeg-7.1/libswscale/swscale.h"
static char* lv_image_argb_scale(char* src, int src_w, int src_h, int dst_w, int dst_h) {
	struct SwsContext* scale_ctx = sws_getContext(
		src_w, src_h, AV_PIX_FMT_BGRA,
		dst_w, dst_h, AV_PIX_FMT_BGRA,
		SWS_BILINEAR, NULL, NULL, NULL);

	if (!scale_ctx) {
		LV_LOG_ERROR("scale_ctx get context failed \n");
		return NULL;
	}

	char* src_data[4];
	src_data[0] = src;
	int src_linesize[4] = { 0 };
	src_linesize[0] = src_w * 4;

	unsigned char* dst_data[4];
	char* scale_img_data = malloc(dst_w * dst_h * 4);
	dst_data[0] = (unsigned char*)scale_img_data;
	int dst_linesize[4] = { 0 };
	dst_linesize[0] = dst_w * 4;
	sws_scale(scale_ctx, (const uint8_t* const*)src_data, src_linesize, 0, src_h, dst_data, dst_linesize);
	sws_freeContext(scale_ctx);
	return scale_img_data;
}

#elif PLATFORM_TYPE_SSD20X
static char* lv_image_argb_scale(char* src, int src_w, int src_h, int dst_w, int dst_h) {
	// struct SwsContext* scale_ctx = sws_getContext(
	// 	src_w, src_h, AV_PIX_FMT_BGRA,
	// 	dst_w, dst_h, AV_PIX_FMT_BGRA,
	// 	SWS_BILINEAR, NULL, NULL, NULL);

	// if (!scale_ctx) {
	// 	LV_LOG_ERROR("scale_ctx get context failed \n");
	// 	return NULL;
	// }

	// char* src_data[4];
	// src_data[0] = src;
	// int src_linesize[4] = { 0 };
	// src_linesize[0] = src_w * 4;

	// unsigned char* dst_data[4];
	// char* scale_img_data = malloc(dst_w * dst_h * 4);
	// dst_data[0] = (unsigned char*)scale_img_data;
	// int dst_linesize[4] = { 0 };
	// dst_linesize[0] = dst_w * 4;
	// sws_scale(scale_ctx, (const uint8_t* const*)src_data, src_linesize, 0, src_h, dst_data, dst_linesize);
	// sws_freeContext(scale_ctx);
	// return scale_img_data;
	return NULL;
}

#endif




/*
 * @日期: 2022-08-13
 * @作者: leo.liu
 * @注释: 当前jpg的解码个数
 */
static int jpeg_decode_total = 0;

/**
 * Open a PNG image and return the decided image
 * @param src can be file name or pointer to a C array
 * @param style style of the image object (unused now but certain formats might use it)
 * @return pointer to the decoded image or `LV_IMG_DECODER_OPEN_FAIL` if failed
 */
static lv_res_t decoder_open(lv_img_decoder_t* decoder, lv_img_decoder_dsc_t* dsc) {
	(void)decoder; /*Unused*/

	// unsigned long long start = user_timestamp_get();
	/*If it's a PNG file...*/
	if (dsc->src_type == LV_IMG_SRC_FILE) {
		const char* fn = dsc->src;

		if ((strcmp(lv_fs_get_ext(fn), "jpg") == 0) || (strcmp(lv_fs_get_ext(fn), "JPG") == 0) ||
			(strcmp(lv_fs_get_ext(fn), "jpeg") == 0) || (strcmp(lv_fs_get_ext(fn), "JPEG") == 0)) {

			uint32_t obj_width = dsc->header.w;
			uint32_t obj_hight = dsc->header.h;

			lv_fs_file_t f;
			lv_fs_res_t res = lv_fs_open(&f, fn, LV_FS_MODE_RD);
			if (res != LV_FS_RES_OK) {
				LV_LOG_ERROR("lv fs open failed %s\n", fn);
				return LV_RES_INV;
			}
			struct jpeg_decompress_struct cinfo;
			struct my_error_mgr jerr;
			cinfo.err = jpeg_std_error(&jerr.pub);
			jerr.pub.error_exit = my_error_exit;
			if (setjmp(jerr.setjmp_buffer)) {
				/* If we get here, the JPEG code has signaled an error.
				 * We need to clean up the JPEG object, close the input file, and return.
				 */
				jpeg_destroy_decompress(&cinfo);
				lv_fs_close(&f);
				LV_LOG_ERROR("setjmp(jerr.setjmp_buffer) failed %s\n",fn);
				return LV_RES_INV;
			}

			jpeg_create_decompress(&cinfo);

			/* Step 2: specify data source (eg, a file) */
			jpeg_stdio_src(&cinfo, f.file_d);

			/* Step 3: read file parameters with jpeg_read_header() */
			(void)jpeg_read_header(&cinfo, TRUE);

			cinfo.out_color_space = JCS_EXT_BGRX;

			if ((obj_width * obj_hight) < (cinfo.image_width * cinfo.image_height)) {
				if (((abs(obj_width - cinfo.image_width)) < (abs(obj_hight - cinfo.image_height))) &&
					(obj_hight < cinfo.image_height)) {
					cinfo.scale_num = obj_hight;
					cinfo.scale_denom = cinfo.image_height;
				}
				else {
					cinfo.scale_num = obj_hight;
					cinfo.scale_denom = cinfo.image_width;
				}
			}

			(void)jpeg_start_decompress(&cinfo);

			unsigned char* buffer[1] = { NULL };     /* Output row buffer */
			unsigned short* buffer12[1] = { NULL }; /* 12-bit output row buffer */
			int row_stride = cinfo.output_width * cinfo.output_components;
			char* img_data = malloc(row_stride * cinfo.output_height); // lv_mem_alloc(
			if (!img_data) {
				jpeg_destroy_decompress(&cinfo);
				lv_fs_close(&f);
				LV_LOG_ERROR("lv_mem_alloc failed %s size:%d kb\n", fn, row_stride * cinfo.output_height / 1024);
				return LV_RES_INV;
			}

			int little_endian = 1;
			int col;
			if (cinfo.data_precision == 12) {
				while (cinfo.output_scanline < cinfo.output_height) {
					/* jpeg12_read_scanlines expects an array of pointers to scanlines.
					 * Here the array is only one element long, but you could ask for
					 * more than one scanline at a time if that's more convenient.
					 */

					buffer12[0] = (unsigned short*)(&img_data[cinfo.output_scanline * row_stride]);
					(void)jpeg12_read_scanlines(&cinfo, (J12SAMPARRAY)buffer12, 1);
					if (*(char*)&little_endian == 1) {
						/* Swap MSB and LSB in each sample */
						for (col = 0; col < row_stride; col++)
							buffer12[0][col] = ((buffer12[0][col] & 0xFF) << 8) | ((buffer12[0][col] >> 8) & 0xFF);
					}

					//fwrite(buffer12[0], 1, row_stride * sizeof(J12SAMPLE), outfile);
				}
			}
			else {
				while (cinfo.output_scanline < cinfo.output_height) {
					/* jpeg_read_scanlines expects an array of pointers to scanlines.
					 * Here the array is only one element long, but you could ask for
					 * more than one scanline at a time if that's more convenient.
					 */

					buffer[0] = (unsigned char*)(&img_data[cinfo.output_scanline * row_stride]);
					(void)jpeg_read_scanlines(&cinfo, (JSAMPARRAY)buffer, 1);
					//fwrite(buffer[0], 1, row_stride, outfile)
				}
			}
			/*
			 * @日期: 2022-08-09
			 * @作者: leo.liu
			 * @注释: 关闭解码器
			 */
			 /* Step 7: Finish decompression */
			(void)jpeg_finish_decompress(&cinfo);
			/* This is an important step since it will release a good deal of memory. */
			jpeg_destroy_decompress(&cinfo);
			lv_fs_close(&f);

			if ((obj_width != cinfo.output_width) || (obj_hight != cinfo.output_height)) {

				LV_LOG_USER("jpeg scale %dx%d->%dx%d ", cinfo.output_width, cinfo.output_height, obj_width, obj_hight);
				char* scale_img_data = lv_image_argb_scale(img_data, cinfo.output_width, cinfo.output_height, obj_width, obj_hight);
				free(img_data);
				if (!scale_img_data) {
					return LV_RES_INV;
				}
				img_data = scale_img_data;
			}

			dsc->img_data = (void*)img_data;
			jpeg_decode_total++;
			LV_LOG_USER("jpeg decode total:%d:%s ", jpeg_decode_total, fn);
			return LV_RES_OK; /*The image is fully decoded. Return with its pointer*/
		}
	}

	return LV_RES_INV; /*If not returned earlier then it failed*/
}

/**
 * Free the allocated resources
 */
static void decoder_close(lv_img_decoder_t* decoder, lv_img_decoder_dsc_t* dsc) {
	LV_UNUSED(decoder); /*Unused*/

	if (dsc->img_data) {
		//ak_mem_dma_free((void*)dsc->img_data);
		//lv_mem_free((void*)dsc->img_data);
		free((void*)dsc->img_data);
		dsc->img_data = NULL;
		jpeg_decode_total--;
		LV_LOG_USER("jpeg decode total:%d ", jpeg_decode_total);
	}

	return;
}

#endif
