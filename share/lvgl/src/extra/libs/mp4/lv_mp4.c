
#include "../../../lvgl.h"

#if LV_USE_MP4
#include "lv_mp4.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "lv_common.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavcodec/bsf.h"

typedef struct {
    int decoder_count;
    AVCodecContext* h265_codec_ctx;
    //AVCodecParserContext* h265_parser_ctx;
    AVCodecContext* h264_codec_ctx;
    // AVCodecParserContext* h264_parser_ctx;

    AVPacket* pkt;
    AVFrame* frame;
}lv_mp4_private_data_t;


/**********************
 *	STATIC PROTOTYPES
 **********************/
static lv_res_t decoder_info(struct _lv_img_decoder_t* decoder, const void* src, lv_img_header_t* header);
static lv_res_t decoder_open(lv_img_decoder_t* dec, lv_img_decoder_dsc_t* dsc);
static void decoder_close(lv_img_decoder_t* dec, lv_img_decoder_dsc_t* dsc);
/*
 * @日期: 2022-08-09
 * @作者: leo.liu
 * @功能: 注册jpg解码库
 * @return:
 */
void lv_mp4_init(void) {
    lv_img_decoder_t* dec = lv_img_decoder_create();

    lv_img_decoder_set_info_cb(dec, decoder_info);
    lv_img_decoder_set_open_cb(dec, decoder_open);
    lv_img_decoder_set_close_cb(dec, decoder_close);

    lv_mp4_private_data_t* mp4 = lv_mem_alloc(sizeof(lv_mp4_private_data_t));
    memset(mp4, 0, sizeof(lv_mp4_private_data_t));
    dec->user_data = mp4;
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
        if ((strcmp(lv_fs_get_ext(fn), "mp4") == 0) || (strcmp(lv_fs_get_ext(fn), "MP4") == 0)) {
            lv_fs_file_t f;
            lv_fs_res_t res = lv_fs_open(&f, fn, LV_FS_MODE_RD);
            if (res != LV_FS_RES_OK) {
                LV_LOG_ERROR("lv fs open failed %s\n", fn);
                return LV_RES_INV;
            }
            lv_fs_close(&f);
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
static char* lv_image_frame_scale(AVFrame* frame, int dst_w, int dst_h) {
    struct SwsContext* scale_ctx = sws_getContext(
        frame->width, frame->height, frame->format,
        dst_w, dst_h, AV_PIX_FMT_BGRA,
        SWS_BILINEAR, NULL, NULL, NULL);

    if (!scale_ctx) {
        LV_LOG_ERROR("scale_ctx get context failed \n");
        return NULL;
    }

    unsigned char* dst_data[4];
    char* scale_img_data = malloc(dst_w * dst_h * 4);
    dst_data[0] = (unsigned char*)scale_img_data;
    int dst_linesize[4] = { 0 };
    dst_linesize[0] = dst_w * 4;
    sws_scale(scale_ctx, (const uint8_t* const*)frame->data, frame->linesize, 0, frame->height, dst_data, dst_linesize);
    sws_freeContext(scale_ctx);
    return scale_img_data;
}
#elif PLATFORM_TYPE_SSD20X
static char* lv_image_frame_scale(AVFrame* frame, int dst_w, int dst_h) {
    // struct SwsContext* scale_ctx = sws_getContext(
    //     frame->width, frame->height, frame->format,
    //     dst_w, dst_h, AV_PIX_FMT_BGRA,
    //     SWS_BILINEAR, NULL, NULL, NULL);

    // if (!scale_ctx) {
    //     LV_LOG_ERROR("scale_ctx get context failed \n");
    //     return NULL;
    // }

    // unsigned char* dst_data[4];
    // char* scale_img_data = malloc(dst_w * dst_h * 4);
    // dst_data[0] = (unsigned char*)scale_img_data;
    // int dst_linesize[4] = { 0 };
    // dst_linesize[0] = dst_w * 4;
    // sws_scale(scale_ctx, (const uint8_t* const*)frame->data, frame->linesize, 0, frame->height, dst_data, dst_linesize);
    // sws_freeContext(scale_ctx);
    // return scale_img_data;
    return NULL;
}
#endif

static int avformat_data_free(lv_mp4_private_data_t* mp4) {

    if (mp4->h264_codec_ctx) {
        avcodec_free_context(&mp4->h264_codec_ctx);
        mp4->h264_codec_ctx = NULL;
    }


    if (mp4->h265_codec_ctx) {
        avcodec_free_context(&mp4->h265_codec_ctx);
        mp4->h265_codec_ctx = NULL;
    }

    if (mp4->pkt) {
        av_packet_free(&mp4->pkt);
        mp4->pkt = NULL;
    }

    if (mp4->frame) {
        av_frame_free(&mp4->frame);
        mp4->frame = NULL;
    }

    return 0;
}

static int lv_avcodec_by_id_open(AVCodecContext** pcodec_ctx, enum AVCodecID id) {

    const AVCodec* codec = avcodec_find_decoder(id);
    if (!codec) {
        LV_LOG_ERROR("Codec not found\n");
        return -1;
    }

    AVCodecContext* codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        LV_LOG_ERROR("Could not allocate video codec context\n");
        return -1;
    }

    if (avcodec_open2(codec_ctx, codec, NULL) < 0) {
        LV_LOG_ERROR("Could not open codec\n");
        avcodec_free_context(&codec_ctx);
        return -1;
    }
    *pcodec_ctx = codec_ctx;
    return 0;
}

static int  lv_avcodec_decodec(AVCodecContext* codec_ctx, AVPacket* pkt, AVFrame* frame) {

    int reslut = avcodec_send_packet(codec_ctx, pkt);
    if (reslut < 0) {
        LV_LOG_ERROR("Error sending a packet for decoding\n");
        return -1;
    }

    while (reslut >= 0) {
        av_frame_unref(frame);
        reslut = avcodec_receive_frame(codec_ctx, frame);
        if (reslut == AVERROR(EAGAIN) || reslut == AVERROR_EOF) {
            break;
        }
        else if (reslut < 0) {
            LV_LOG_ERROR(" Error during decoding\n");
            break;
        }
        return 0;
    }
    return -1;
}

/**
 * Open a PNG image and return the decided image
 * @param src can be file name or pointer to a C array
 * @param style style of the image object (unused now but certain formats might use it)
 * @return pointer to the decoded image or `LV_IMG_DECODER_OPEN_FAIL` if failed
 */
static lv_res_t decoder_open(lv_img_decoder_t* decoder, lv_img_decoder_dsc_t* dsc) {
    (void)decoder; /*Unused*/
    int reslut = 0;
    char* img_data = NULL;
    // unsigned long long start = user_timestamp_get();
    /*If it's a PNG file...*/
    if (dsc->src_type == LV_IMG_SRC_FILE) {
        const char* fn = dsc->src + 2;
        if ((strcmp(lv_fs_get_ext(fn), "mp4") == 0) || (strcmp(lv_fs_get_ext(fn), "MP4") == 0)) {
            uint32_t obj_width = dsc->header.w;
            uint32_t obj_hight = dsc->header.h;
            lv_mp4_private_data_t* mp4 = decoder->user_data;
            if (!mp4) {
                LV_LOG_ERROR("mp4 handle get failed ");
                return LV_RES_INV;
            }

            AVFormatContext* fmt_ctx = NULL;
            AVCodecContext* codec_ctx = NULL;
            const AVBitStreamFilter* bsf_filter = NULL;
            AVBSFContext* bsf_ctx = NULL;

            if (avformat_open_input(&fmt_ctx, fn, NULL, NULL) < 0) {
                LV_LOG_ERROR("Could not open source file:%s\n", fn);
                return LV_RES_INV;
            }

            if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
                LV_LOG_ERROR("Conld not find stream information \n");
                return LV_RES_INV;
            }

            int video_stream_index = -1;
            for (int i = 0; i < fmt_ctx->nb_streams; i++) {
                if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                    video_stream_index = i;
                    break;
                }
            }
            if (video_stream_index == -1) {
                avformat_close_input(&fmt_ctx);
                return LV_RES_INV;
            }

            // av_dump_format(fmt_ctx, 0, fn, 0);

            enum AVCodecID codec_id = fmt_ctx->streams[video_stream_index]->codecpar->codec_id;
            if (codec_id == AV_CODEC_ID_H264) {
                if (mp4->h264_codec_ctx == NULL) {
                    if (lv_avcodec_by_id_open(&mp4->h264_codec_ctx, AV_CODEC_ID_H264) != 0) {
                        avformat_close_input(&fmt_ctx);
                        return LV_RES_INV;
                    }
                }
                codec_ctx = mp4->h264_codec_ctx;
                bsf_filter = av_bsf_get_by_name("h264_mp4toannexb");

            }
            else if (codec_id == AV_CODEC_ID_HEVC) {
                if (mp4->h265_codec_ctx == NULL) {
                    if (lv_avcodec_by_id_open(&mp4->h265_codec_ctx, AV_CODEC_ID_HEVC) != 0) {
                        avformat_close_input(&fmt_ctx);
                        return LV_RES_INV;
                    }
                }
                codec_ctx = mp4->h265_codec_ctx;
                bsf_filter = av_bsf_get_by_name("hevc_mp4toannexb");
            }
            else {
                LV_LOG_ERROR("unknow video codec id:%d\n", codec_id);
                avformat_close_input(&fmt_ctx);
                return LV_RES_INV;
            }

            if (mp4->pkt == NULL) {
                mp4->pkt = av_packet_alloc();
            }
            if (mp4->frame == NULL) {
                mp4->frame = av_frame_alloc();
            }

            av_bsf_alloc(bsf_filter, &bsf_ctx);
            avcodec_parameters_copy(bsf_ctx->par_in, fmt_ctx->streams[video_stream_index]->codecpar);
            av_bsf_init(bsf_ctx);


            while (av_read_frame(fmt_ctx, mp4->pkt) >= 0) {
                if (mp4->pkt->stream_index == video_stream_index) {
                    reslut = av_bsf_send_packet(bsf_ctx, mp4->pkt);
                    if (reslut < 0) {
                        LV_LOG_USER("av bsf send packet failed\n");
                        av_packet_unref(mp4->pkt);
                        continue;
                    }

                    av_packet_unref(mp4->pkt);
                    while (av_bsf_receive_packet(bsf_ctx, mp4->pkt) >= 0) {
                        if (lv_avcodec_decodec(codec_ctx, mp4->pkt, mp4->frame) == 0) {
                            img_data = lv_image_frame_scale(mp4->frame, obj_width, obj_hight);
                        }
                        av_packet_unref(mp4->pkt);
                        if (img_data) {
                            break;
                        }
                    }
                }
                if (img_data) {
                    break;
                }
            }

            if (img_data) {
                mp4->decoder_count++;
                dsc->img_data = (void*)img_data;
                LV_LOG_USER("mp4 decode total:%d", mp4->decoder_count);
            }

            if (bsf_ctx) {
                av_bsf_free(&bsf_ctx);
            }

            if (fmt_ctx) {
                avformat_close_input(&fmt_ctx);
            }
            return img_data ? LV_RES_OK : LV_RES_INV; /*The image is fully decoded. Return with its pointer*/
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
        free((void*)dsc->img_data);
        dsc->img_data = NULL;
    }

    lv_mp4_private_data_t* mp4 = (lv_mp4_private_data_t*)decoder->user_data;
    if (mp4) {
        mp4->decoder_count--;
        if (mp4->decoder_count == 0) {
            avformat_data_free(mp4);
        }
        LV_LOG_USER("mp4 decode total:%d ", mp4->decoder_count);
    }
    return;
}

#endif
