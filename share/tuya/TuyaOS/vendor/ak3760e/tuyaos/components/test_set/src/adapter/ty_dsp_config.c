#ifdef CONFIG_MEDIA_TKL_SUPPORT

#include "ty_dsp_adapter.h"


S_DSP_MEDIA_INFO gMediaInfo = 
{
    /* 主码流 */
    .video[E_DSP_MEDIA_VIDEO_MAIN].channel_enable = 1,
    .video[E_DSP_MEDIA_VIDEO_MAIN].media_type = E_DSP_MEDIA_VIDEO_MAIN,
    .video[E_DSP_MEDIA_VIDEO_MAIN].fps = 20,
    .video[E_DSP_MEDIA_VIDEO_MAIN].gop = 40,
#if 0
    .video[E_DSP_MEDIA_VIDEO_MAIN].bitrate = 1536,
    .video[E_DSP_MEDIA_VIDEO_MAIN].width = 1920,
    .video[E_DSP_MEDIA_VIDEO_MAIN].height = 1080,
    .video[E_DSP_MEDIA_VIDEO_MAIN].codectype = 4,
    .video[E_DSP_MEDIA_VIDEO_MAIN].minQp = 26,
    .video[E_DSP_MEDIA_VIDEO_MAIN].maxQp = 48,
#endif

    .video[E_DSP_MEDIA_VIDEO_MAIN].bitrate = 1536,
    .video[E_DSP_MEDIA_VIDEO_MAIN].width = 640,
    .video[E_DSP_MEDIA_VIDEO_MAIN].height = 360,
    .video[E_DSP_MEDIA_VIDEO_MAIN].codectype = 4,
    .video[E_DSP_MEDIA_VIDEO_MAIN].minQp = 26,
    .video[E_DSP_MEDIA_VIDEO_MAIN].maxQp = 48,
    
    /*主码流osd*/
    .osd[E_DSP_MEDIA_VIDEO_MAIN].left = 96,
    .osd[E_DSP_MEDIA_VIDEO_MAIN].top = 54,
    .osd[E_DSP_MEDIA_VIDEO_MAIN].fontW = 32,
    .osd[E_DSP_MEDIA_VIDEO_MAIN].fontH = 66,

    /* 子码流 */
    .video[E_DSP_MEDIA_VIDEO_SUB].channel_enable = 1,
    .video[E_DSP_MEDIA_VIDEO_SUB].media_type = E_DSP_MEDIA_VIDEO_SUB,
    .video[E_DSP_MEDIA_VIDEO_SUB].fps = 20,
    .video[E_DSP_MEDIA_VIDEO_SUB].gop = 40,
    .video[E_DSP_MEDIA_VIDEO_SUB].bitrate = 512,
    .video[E_DSP_MEDIA_VIDEO_SUB].width = 640,
    .video[E_DSP_MEDIA_VIDEO_SUB].height = 360,
    .video[E_DSP_MEDIA_VIDEO_SUB].codectype = 2,
    .video[E_DSP_MEDIA_VIDEO_SUB].minQp = 26,
    .video[E_DSP_MEDIA_VIDEO_SUB].maxQp = 48,
    
    /*子码流osd*/
    .osd[E_DSP_MEDIA_VIDEO_SUB].left = 32,
    .osd[E_DSP_MEDIA_VIDEO_SUB].top = 18,
    .osd[E_DSP_MEDIA_VIDEO_SUB].fontW = 12,
    .osd[E_DSP_MEDIA_VIDEO_SUB].fontH = 26,

    /* 第三码流 */
    .video[E_DSP_MEDIA_VIDEO_3RD].channel_enable = 0,
    .video[E_DSP_MEDIA_VIDEO_3RD].media_type = E_DSP_MEDIA_VIDEO_3RD,
    .video[E_DSP_MEDIA_VIDEO_3RD].fps = 0,
    .video[E_DSP_MEDIA_VIDEO_3RD].gop = 0,
    .video[E_DSP_MEDIA_VIDEO_3RD].bitrate = 0,
    .video[E_DSP_MEDIA_VIDEO_3RD].width = 0,
    .video[E_DSP_MEDIA_VIDEO_3RD].height = 0,
    .video[E_DSP_MEDIA_VIDEO_3RD].codectype = 0,
    .video[E_DSP_MEDIA_VIDEO_3RD].minQp = 0,
    .video[E_DSP_MEDIA_VIDEO_3RD].maxQp = 0,
    
    /*第三码流osd*/
    .osd[E_DSP_MEDIA_VIDEO_3RD].left = 0,
    .osd[E_DSP_MEDIA_VIDEO_3RD].top = 0,
    .osd[E_DSP_MEDIA_VIDEO_3RD].fontW = 0,
    .osd[E_DSP_MEDIA_VIDEO_3RD].fontH = 0,
    
    /* 第四码流 */
    .video[E_DSP_MEDIA_VIDEO_4TH].channel_enable = 0,
    .video[E_DSP_MEDIA_VIDEO_4TH].media_type = E_DSP_MEDIA_VIDEO_4TH,
    .video[E_DSP_MEDIA_VIDEO_4TH].fps = 0,
    .video[E_DSP_MEDIA_VIDEO_4TH].gop = 0,
    .video[E_DSP_MEDIA_VIDEO_4TH].bitrate = 0,
    .video[E_DSP_MEDIA_VIDEO_4TH].width = 0,
    .video[E_DSP_MEDIA_VIDEO_4TH].height = 0,
    .video[E_DSP_MEDIA_VIDEO_4TH].codectype = 0,
    .video[E_DSP_MEDIA_VIDEO_4TH].minQp = 0,
    .video[E_DSP_MEDIA_VIDEO_4TH].maxQp = 0,
    
    /*第四码流osd*/
    .osd[E_DSP_MEDIA_VIDEO_4TH].left = 0,
    .osd[E_DSP_MEDIA_VIDEO_4TH].top = 0,
    .osd[E_DSP_MEDIA_VIDEO_4TH].fontW = 0,
    .osd[E_DSP_MEDIA_VIDEO_4TH].fontH = 0,

    /* 抓图 */
    .video[E_DSP_MEDIA_VIDEO_SNAP].channel_enable = 1,
    .video[E_DSP_MEDIA_VIDEO_SNAP].media_type = E_DSP_MEDIA_VIDEO_SNAP,
    .video[E_DSP_MEDIA_VIDEO_SNAP].fps = 25,
    .video[E_DSP_MEDIA_VIDEO_SNAP].gop = 50,
    .video[E_DSP_MEDIA_VIDEO_SNAP].bitrate = 512,
    .video[E_DSP_MEDIA_VIDEO_SNAP].width = 640,
    .video[E_DSP_MEDIA_VIDEO_SNAP].height = 360,
    .video[E_DSP_MEDIA_VIDEO_SNAP].codectype = 3,
    .video[E_DSP_MEDIA_VIDEO_SNAP].firstFrame = 0,
    .video[E_DSP_MEDIA_VIDEO_SNAP].minQp = 15,
    .video[E_DSP_MEDIA_VIDEO_SNAP].maxQp = 15,
    
    /*JPEG码流osd*/
    .osd[E_DSP_MEDIA_VIDEO_SNAP].left = 32,
    .osd[E_DSP_MEDIA_VIDEO_SNAP].top = 18,
    .osd[E_DSP_MEDIA_VIDEO_SNAP].fontW = 12,
    .osd[E_DSP_MEDIA_VIDEO_SNAP].fontH = 16,

    /* 动检 */
    .video[E_DSP_MEDIA_VIDEO_MD].channel_enable = 1,
    .video[E_DSP_MEDIA_VIDEO_MD].media_type = E_DSP_MEDIA_VIDEO_MD,
    .video[E_DSP_MEDIA_VIDEO_MD].fps = 25,
    .video[E_DSP_MEDIA_VIDEO_MD].gop = 50,
    .video[E_DSP_MEDIA_VIDEO_MD].bitrate = 512,
    .video[E_DSP_MEDIA_VIDEO_MD].width = 160,
    .video[E_DSP_MEDIA_VIDEO_MD].height = 90,
    .video[E_DSP_MEDIA_VIDEO_MD].codectype = 5,

    
    /* 人形 */
    .video[E_DSP_MEDIA_VIDEO_HD].channel_enable = 0,
    .video[E_DSP_MEDIA_VIDEO_HD].media_type = E_DSP_MEDIA_VIDEO_HD,
    .video[E_DSP_MEDIA_VIDEO_HD].fps = 0,
    .video[E_DSP_MEDIA_VIDEO_HD].gop = 0,
    .video[E_DSP_MEDIA_VIDEO_HD].bitrate = 0,
    .video[E_DSP_MEDIA_VIDEO_HD].width = 0,
    .video[E_DSP_MEDIA_VIDEO_HD].height = 0,
    .video[E_DSP_MEDIA_VIDEO_HD].codectype = 0,

    /* 二维码 */
    .video[E_DSP_MEDIA_VIDEO_QR].channel_enable = 1,
    .video[E_DSP_MEDIA_VIDEO_QR].media_type = E_DSP_MEDIA_VIDEO_QR,
    .video[E_DSP_MEDIA_VIDEO_QR].fps =25,
    .video[E_DSP_MEDIA_VIDEO_QR].gop = 50,
    .video[E_DSP_MEDIA_VIDEO_QR].bitrate = 512,
    .video[E_DSP_MEDIA_VIDEO_QR].width = 640,
    .video[E_DSP_MEDIA_VIDEO_QR].height = 360,
    .video[E_DSP_MEDIA_VIDEO_QR].codectype = 5,

    /* 主音频 */
    .audio[E_DSP_MEDIA_AUDIO].channel_enable = 1,
    .audio[E_DSP_MEDIA_AUDIO].media_type = E_DSP_MEDIA_AUDIO,
    .audio[E_DSP_MEDIA_AUDIO].sample = 8000,
    .audio[E_DSP_MEDIA_AUDIO].datebits = 16,
    .audio[E_DSP_MEDIA_AUDIO].channnel = 0,
    .audio[E_DSP_MEDIA_AUDIO].codectype = 105,
    .audio[E_DSP_MEDIA_AUDIO].fps = 25,
    .audio[E_DSP_MEDIA_AUDIO].mic_volume = 15,
    .audio[E_DSP_MEDIA_AUDIO].spk_volume = 5,
    .audio[E_DSP_MEDIA_AUDIO].spk_gpio = 14,
    .audio[E_DSP_MEDIA_AUDIO].spk_gpio_polarity = 0,

    /* 第二路音频 */
    .audio[E_DSP_MEDIA_AUDIO_2RD].channel_enable = 0,
    .audio[E_DSP_MEDIA_AUDIO_2RD].media_type = E_DSP_MEDIA_AUDIO_2RD,
    .audio[E_DSP_MEDIA_AUDIO_2RD].sample = 0,
    .audio[E_DSP_MEDIA_AUDIO_2RD].datebits = 0,
    .audio[E_DSP_MEDIA_AUDIO_2RD].channnel = 0,
    .audio[E_DSP_MEDIA_AUDIO_2RD].codectype = 0,
    .audio[E_DSP_MEDIA_AUDIO_2RD].fps = 0,
    .audio[E_DSP_MEDIA_AUDIO_2RD].mic_volume = 0,
    .audio[E_DSP_MEDIA_AUDIO_2RD].spk_volume = 0,
    .audio[E_DSP_MEDIA_AUDIO_2RD].spk_gpio = 0,
    .audio[E_DSP_MEDIA_AUDIO_2RD].spk_gpio_polarity = 0,

    /* 第三路音频 */
    .audio[E_DSP_MEDIA_AUDIO_3RD].channel_enable = 0,
    .audio[E_DSP_MEDIA_AUDIO_3RD].media_type = E_DSP_MEDIA_AUDIO_3RD,
    .audio[E_DSP_MEDIA_AUDIO_3RD].sample = 0,
    .audio[E_DSP_MEDIA_AUDIO_3RD].datebits = 0,
    .audio[E_DSP_MEDIA_AUDIO_3RD].channnel = 0,
    .audio[E_DSP_MEDIA_AUDIO_3RD].codectype = 0,
    .audio[E_DSP_MEDIA_AUDIO_3RD].fps = 0,
    .audio[E_DSP_MEDIA_AUDIO_3RD].mic_volume = 0,
    .audio[E_DSP_MEDIA_AUDIO_3RD].spk_volume = 0,
    .audio[E_DSP_MEDIA_AUDIO_3RD].spk_gpio = 0,
    .audio[E_DSP_MEDIA_AUDIO_3RD].spk_gpio_polarity = 0,
    
    /* 第四路音频 */
    .audio[E_DSP_MEDIA_AUDIO_4TH].channel_enable = 0,
    .audio[E_DSP_MEDIA_AUDIO_4TH].media_type = E_DSP_MEDIA_AUDIO_4TH,
    .audio[E_DSP_MEDIA_AUDIO_4TH].sample = 0,
    .audio[E_DSP_MEDIA_AUDIO_4TH].datebits = 0,
    .audio[E_DSP_MEDIA_AUDIO_4TH].channnel = 0,
    .audio[E_DSP_MEDIA_AUDIO_4TH].codectype = 0,
    .audio[E_DSP_MEDIA_AUDIO_4TH].fps = 0,
    .audio[E_DSP_MEDIA_AUDIO_4TH].mic_volume = 0,
    .audio[E_DSP_MEDIA_AUDIO_4TH].spk_volume = 0,
    .audio[E_DSP_MEDIA_AUDIO_4TH].spk_gpio = 0,
    .audio[E_DSP_MEDIA_AUDIO_4TH].spk_gpio_polarity = 0,
};

#endif
