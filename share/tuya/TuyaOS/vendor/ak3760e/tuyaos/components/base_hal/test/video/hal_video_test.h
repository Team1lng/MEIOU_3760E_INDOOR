/*
 * Copyright (C) 2021 Tuya Inc.
 */

#ifndef __HAL_VIDEO_TEST_H
#define __HAL_VIDEO_TEST_H

/* audio / video */

typedef enum {
	E_CHANNEL_VIDEO_MAIN = 0,
	E_CHANNEL_VIDEO_SUB,
	E_CHANNEL_VIDEO_3RD,
	E_CHANNEL_VIDEO_4TH,
	E_CHANNEL_VIDEO_MAX = 8, /* max video streams */
	E_CHANNEL_AUDIO,
	E_CHANNEL_AUDIO_2RD,
	E_CHANNEL_AUDIO_3RD,
	E_CHANNEL_AUDIO_4TH,
	E_CHANNEL_MAX = 16
} CHANNEL_E;

typedef enum {
	E_VIDEO_PB_FRAME = 0,
	E_VIDEO_I_FRAME,
	E_VIDEO_TS_FRAME,
	E_AUDIO_FRAME,
	E_CMD_FRAME,
	E_MEDIA_FRAME_TYPE_MAX
} MEDIA_FRAME_TYPE_E;

//enum for codec, sync with tuya cloud and APP, should NOT be changed
typedef enum {
	TUYA_CODEC_VIDEO_MPEG4 = 0,
	TUYA_CODEC_VIDEO_H263,
	TUYA_CODEC_VIDEO_H264,
	TUYA_CODEC_VIDEO_MJPEG,
	TUYA_CODEC_VIDEO_H265,
	TUYA_CODEC_VIDEO_YUV420,
	TUYA_CODEC_VIDEO_YUV422,
	TUYA_CODEC_VIDEO_MAX = 99,

	TUYA_CODEC_AUDIO_ADPCM,
	TUYA_CODEC_AUDIO_PCM,
	TUYA_CODEC_AUDIO_AAC_RAW,
	TUYA_CODEC_AUDIO_AAC_ADTS,
	TUYA_CODEC_AUDIO_AAC_LATM,
	TUYA_CODEC_AUDIO_G711U,
	TUYA_CODEC_AUDIO_G711A,
	TUYA_CODEC_AUDIO_G726,
	TUYA_CODEC_AUDIO_SPEEX,
	TUYA_CODEC_AUDIO_MP3,
	TUYA_CODEC_AUDIO_MAX = 199,
	TUYA_CODEC_INVALID
} TUYA_CODEC_ID;


/* audio */

typedef enum {
	E_DSP_MEDIA_AUDIO      = 0,
	E_DSP_MEDIA_AUDIO_2RD,
	E_DSP_MEDIA_AUDIO_3RD,
	E_DSP_MEDIA_AUDIO_4TH,
	E_DSP_MEDIA_AUDIO_MAX,
} E_DSP_MEDIA_AUDIO_TYPE;

typedef enum {
	TUYA_AUDIO_SAMPLE_8K     = 8000,
	TUYA_AUDIO_SAMPLE_11K    = 11000,
	TUYA_AUDIO_SAMPLE_12K    = 12000,
	TUYA_AUDIO_SAMPLE_16K    = 16000,
	TUYA_AUDIO_SAMPLE_22K    = 22000,
	TUYA_AUDIO_SAMPLE_24K    = 24000,
	TUYA_AUDIO_SAMPLE_32K    = 32000,
	TUYA_AUDIO_SAMPLE_44K    = 44000,
	TUYA_AUDIO_SAMPLE_48K    = 48000,
	TUYA_AUDIO_SAMPLE_MAX    = 0xFFFFFFFF
} TUYA_AUDIO_SAMPLE_E;

typedef struct {
	MEDIA_FRAME_TYPE_E   type;      /* 帧类型 */
	char                *pbuf;      /* 帧数据 */
	unsigned int         bufSize;   /* buf的分配长度 */
	unsigned int         usedSize;  /* puf的使用长度 */
	unsigned long long   pts;       /* pts，单位us */
	unsigned long long   timestamp; /* 时间戳，单位us */
	TUYA_CODEC_ID        codectype; /* 帧编码格式 */
	TUYA_AUDIO_SAMPLE_E  sample;    /* 采样率 */
	unsigned int         seq;       /* 帧序号 */
} S_DSP_AUDIO_FRAME;


/* video */

#define VIDEO_H264_IP_QP_DELTA	0
#define VIDEO_H264_MAX_IP_PROP	100
#define VIDEO_H264_MAX_QP	48
#define VIDEO_H264_MIN_QP	26
#define VIDEO_H264_MAX_IQP	48
#define VIDEO_H264_MIN_IQP	28

#define VIDEO_H265_IP_QP_DELTA	0
#define VIDEO_H265_MAX_IP_PROP	100
#define VIDEO_H265_MAX_QP	48
#define VIDEO_H265_MIN_QP	26
#define VIDEO_H265_MAX_IQP	48
#define VIDEO_H265_MIN_IQP	28

typedef struct {
	//MEDIA_FRAME_TYPE_E type;      /* 帧类型 */
	char *pbuf;                     /* 帧数据 */
	unsigned int bufSize;           /* pbuf的分配长度 */
	unsigned int usedSize;          /* pbuf的使用长度 */
	unsigned int width;             /* 宽度 */
	unsigned int height;            /* 长度 */
	unsigned long long pts;         /* pts，单位us */
	unsigned long long timestamp;   /* 时间戳，单位us */
	//TUYA_CODEC_ID codectype;      /* 帧编码格式 */
	unsigned int seq;               /* 帧序号 */
} S_DSP_VIDEO_FRAME;

typedef enum {
	E_DSP_MEDIA_VIDEO_MAIN = 0,     /* 主码流 */
	E_DSP_MEDIA_VIDEO_SUB,          /* 子码流 */
	E_DSP_MEDIA_VIDEO_3RD,          /* 第三路码流 */
	E_DSP_MEDIA_VIDEO_4TH,          /* 第四路码流 */
	E_DSP_MEDIA_VIDEO_SNAP,         /* 抓图 */
	E_DSP_MEDIA_VIDEO_MD,           /* 动检 */
	E_DSP_MEDIA_VIDEO_HD,           /* 人形 */
	E_DSP_MEDIA_VIDEO_QR,           /* 二维码 */
	E_DSP_MEDIA_VIDEO_MAX,
} E_DSP_MEDIA_VIDEO_TYPE;

#endif
