#ifndef __TY_DSP_ADAPTER_H__
#define __TY_DSP_ADAPTER_H__


#ifdef __cplusplus
extern "C" {
#endif

#include "tuya_ipc_media.h"


#define E_DSP_HC_TARGET_MAX 3
#define E_DSP_MD_TARGET_MAX 3

typedef enum
{
    E_DSP_MEDIA_VIDEO_REQUEST_IDR = 0,                   /* 在最近的编码，请求IDR帧编码 */
    E_DSP_MEDIA_VIDEO_FLUSH_STREAM,                      /* 刷掉编码器里残留的旧码流，并以IDR帧开始编码 */
    E_DSP_MEDIA_VIDEO_IDR_TYPE_MAX,
}E_DSP_MEDIA_VIDEO_SET_IDR_TYPE;                         /* 视频流类型 */

typedef enum
{
    E_DSP_MEDIA_VIDEO_MAIN = 0,                          /* 主码流 */
    E_DSP_MEDIA_VIDEO_SUB,                               /* 子码流 */
    E_DSP_MEDIA_VIDEO_3RD,                               /* 第三路码流 */
    E_DSP_MEDIA_VIDEO_4TH,                               /* 第四路码流 */
    E_DSP_MEDIA_VIDEO_SNAP,                              /* 抓图 */
    E_DSP_MEDIA_VIDEO_MD,                                /* 动检 */
    E_DSP_MEDIA_VIDEO_HD,                                /* 人形 */
    E_DSP_MEDIA_VIDEO_QR,                                /* 二维码 */
    E_DSP_MEDIA_VIDEO_MAX,                              
}E_DSP_MEDIA_VIDEO_TYPE;                                 /* 视频流类型 */


typedef enum                                            
{
    E_DSP_MEDIA_AUDIO     = 0,                          
    E_DSP_MEDIA_AUDIO_2RD,                              
    E_DSP_MEDIA_AUDIO_3RD,                              
    E_DSP_MEDIA_AUDIO_4TH,                              
    E_DSP_MEDIA_AUDIO_MAX,                              
}E_DSP_MEDIA_AUDIO_TYPE;                                 /* 音频流 */


typedef enum
{
    E_DSP_SET_AUDIO_SPEAKER_START,                       /* spk开启 */
    E_DSP_SET_AUDIO_SPEAKER_STOP,                        /* spk关闭 */
    E_DSP_SET_AUDIO_SPEAKER_VOLUME,                      /* spk 音量*/
    E_DSP_SET_AUDIO_MAX,                                
}E_DSP_SET_AUDIO;                                        /* 音频设置 */


typedef enum
{
    E_DSP_SET_VIDEO_OSD,                                 /* osd设置 */
    E_DSP_SET_VIDEO_MAX,                                
}E_DSP_SET_VIDEO;                                        /* 视频设置 */

typedef enum
{
    E_DSP_AUDIO_SOURCE_TYPE_FILE      = 0,               /* 本地文件播放 */
    E_DSP_AUDIO_SOURCE_TYPE_P2P       = 1,               /* p2p流播放 */
    E_DSP_AUDIO_SOURCE_TYPE_WEBRTC    = 2,               /* webRTC流播放 */
    E_DSP_AUDIO_SOURCE_TYPE_STREAMER  = 3,               /* 推流网关，如Chromecast */
} E_DSP_AUDIO_SOURCE_TYPE;                               /* 播放的音频来源 */


typedef enum
{
    E_DSP_AUDIO_FILE_PCM,                                /* pcm文件 */
    E_DSP_AUDIO_FILE_WAV,                                /* wav文件 */
    E_DSP_AUDIO_FILE_TYPE_MAX,
} E_DSP_AUDIO_FILE_TYPE;                                 /* 本地播放的文件编码格式 */

typedef enum
{
    E_DSP_DETECT_TYPE_MD,                               /* 动检 */
    E_DSP_DETECT_TYPE_HD,                               /* 人行 */
    E_DSP_DETECT_TYPE_DB,                               /* 声音*/
    E_DSP_DETECT_TYPE_MAX,
} E_DSP_DETECT_TYPE;                                    /* 智能检测类型 */

typedef struct
{
    float x;
    float y;
    float width;
    float height;
} E_DSP_RECT;                                           //归一化数据

typedef struct
{
    E_DSP_RECT  drawRect;                               /* 坐标*/
    int         score;                                  /* 评分*/
} E_DSP_TARGET;                                         /* 目标 */


typedef struct
{
    int count;
    E_DSP_TARGET target[E_DSP_MD_TARGET_MAX];
} E_DSP_MD_DETECT_RESULT;                               /* 动检结果 */

typedef struct
{
    int count;
    E_DSP_TARGET target[E_DSP_HC_TARGET_MAX];
} E_DSP_HD_DETECT_RESULT;                               /* 人形结果 */

typedef struct
{
    int    pcm_db;                                      /* DB值*/
} E_DSP_DB_DETECT_RESULT;                               /* DB结果 */

typedef struct
{
    unsigned int channel_enable;                         /* 通道使能,1,使能,0,不使能 */
    E_DSP_MEDIA_AUDIO_TYPE media_type;                   /* 码流类型 */
    TUYA_AUDIO_SAMPLE_E sample;                          /* 采样率 */
    TUYA_AUDIO_DATABITS_E datebits;                      /* 位宽 */
    TUYA_AUDIO_CHANNEL_E channnel;                       /* 声道数 */ 
    TUYA_CODEC_ID codectype;                             /* 音频编码格式 */
    unsigned int fps;                                    /* 帧率，建议默认25fps */
    int mic_volume;                                      /* mic的音量 */
    int spk_volume;                                      /* spk的音量 */
    int spk_gpio;                                        /* spk的功放引脚号, <0, 无引脚 */
    int spk_gpio_polarity;                               /* 引脚极性,0高电平使能，1低电平使能 */
}S_DSP_AUDIO_CONFIG;                                     /* 音频属性配置 */


typedef struct
{
    unsigned int channel_enable;                         /* 通道使能,1,使能,0,不使能 */
    E_DSP_MEDIA_VIDEO_TYPE media_type;                   /* 码流类型 */
    unsigned int fps;                                    /* 帧率 */
    unsigned int gop;                                    /* i帧间隔 */
    TUYA_VIDEO_BITRATE_E bitrate;                        /* 码率 */ 
    unsigned int width;                                  /* 长度 */
    unsigned int height;                                 /* 宽度 */
    TUYA_CODEC_ID codectype;                             /* 编码格式 */
    unsigned int firstFrame;                             /* 低功耗取首图支持标志 */
    unsigned int minQp;                                  /* 最小编码质量，视频QP值越小质量越高，图片相反具体视平台而定*/
    unsigned int maxQp;                                  /* 最大编码质量*/
}S_DSP_VIDEO_CONFIG;                                     /* 视频属性配置 */

typedef struct
{
    unsigned int left;                                  /* osd 坐上角的位置 x坐标*/
    unsigned int top;                                   /* osd 坐上角的位置 y坐标*/
    unsigned int fontW;                                 /* osd 字体大小 字体的宽*/
    unsigned int fontH;                                 /* osd 字体大小 字体的高*/
}S_DSP_OSD_CONFIG;                                      /* OSD属性配置 */

typedef struct
{
    S_DSP_VIDEO_CONFIG video[E_DSP_MEDIA_VIDEO_MAX];     /* 视频属性配置 */
    S_DSP_AUDIO_CONFIG audio[E_DSP_MEDIA_AUDIO_MAX];     /* 音频属性配置 */
    S_DSP_OSD_CONFIG   osd[E_DSP_MEDIA_VIDEO_MAX];       /* osd属性配置 */
}S_DSP_MEDIA_INFO;


typedef struct
{
    MEDIA_FRAME_TYPE_E type;                             /* 帧类型 */
    char    *pbuf;                                       /* 帧数据 */
    unsigned int bufSize;                                /* pbuf的分配长度 */
    unsigned int usedSize;                               /* pbuf的使用长度 */
    unsigned int width;                                  /* 宽度 */
    unsigned int height;                                 /* 长度 */
    unsigned long long pts;                              /* pts，单位ums */
    unsigned long long timestamp;                        /* 时间戳，单位ums */
    TUYA_CODEC_ID codectype;                             /* 帧编码格式 */
    unsigned int seq;                                    /* 帧序号 */
}S_DSP_VIDEO_FRAME;                                      /* 视频帧属性 */


typedef struct
{
    MEDIA_FRAME_TYPE_E   type;                           /* 帧类型 */
    char                *pbuf;                           /* 帧数据 */
    unsigned int         bufSize;                        /* buf的分配长度 */
    unsigned int         usedSize;                       /* puf的使用长度 */
    unsigned long long   pts;                            /* pts，单位ums */ 
    unsigned long long   timestamp;                      /* 时间戳，单位ums */ 
    TUYA_CODEC_ID        codectype;                      /* 帧编码格式 */
    TUYA_AUDIO_SAMPLE_E  sample;                         /* 采样率 */
    unsigned int         seq;                            /* 帧序号 */
}S_DSP_AUDIO_FRAME;                                      /* 音频帧属性 */


typedef struct
{
    int aoVolume;                                          /* 音频输出音频 */
    int aiVolume;                                           /* 音频输入音频 */
}S_DSP_AUDIO_PARAM;                                        /* 设置或获取音频参数 */

typedef struct
{
    int enable;                                          /* 使能,1,使能,0,不使能 */
    int isDls;                                           /* 是否使用夏令时*/
}S_DSP_VIDEO_OSD;                                        /* osd属性 */


/***********************************************************
*  函数名: ty_dsp_init
*  功能：dsp 初始化函数
*  参数: Input，S_DSP_MEDIA_INFO *pinfo，音视频的初始化参数
*      
*  返回值: <0 失败，=0 成功
***********************************************************/
int ty_dsp_init(S_DSP_MEDIA_INFO *pinfo);


/***********************************************************
*  函数名: ty_dsp_video_get
*  功能：获取视频码流，包括主子码流，动检人形YUV数据等
*  参数: Input，int devId，多目预留
        Input，E_DSP_MEDIA_VIDEO_TYPE type，获取的码流类型
        Input，Output,pframe->pbuf和pframe->bufSize 入参，
              返回时，pbuf由底层cp数据，usedSize 为实际数据长度
*      
*  返回值: <0 失败，=0 成功
***********************************************************/
int ty_dsp_video_get(int devId, E_DSP_MEDIA_VIDEO_TYPE type, S_DSP_VIDEO_FRAME *pframe);


/***********************************************************
*  函数名: ty_dsp_video_get_first_snap
*  功能：获取首图接口
*  参数: Input，int devId，多目预留
        Input，Output,pframe->pbuf和pframe->bufSize 入参，
              返回时，pbuf由底层cp数据，usedSize 为实际数据长度
*  返回值: <0 失败，=0 成功
***********************************************************/
int ty_dsp_video_get_first_snap(int devId, S_DSP_VIDEO_FRAME *pframe);


/***********************************************************
*  函数名: ty_dsp_video_set
*  功能：video 属性设备，如osd配置等
*  参数: Input，int devId，多目预留
        Input，E_DSP_MEDIA_VIDEO_TYPE type，操作的码流类型
        Input，E_DSP_SET_VIDEO set，cmd
        Input，void *parg, 设置的参数
*      
*  返回值: <0 失败，=0 成功
***********************************************************/
int ty_dsp_video_set(int devId, E_DSP_MEDIA_VIDEO_TYPE type, E_DSP_SET_VIDEO set, void *parg);


/***********************************************************
*  函数名: ty_dsp_audio_get
*  功能：获取音频帧数据
*  参数:  Input，E_DSP_MEDIA_AUDIO_TYPE type，获取的码流类型
        Input，Output,  pframe->pbuf和pframe->bufSize 入参，
              返回时，pbuf由底层cp数据，usedSize 为实际数据长度
*      
*  返回值: <0 失败，=0 成功
***********************************************************/
int ty_dsp_audio_get(E_DSP_MEDIA_AUDIO_TYPE type, S_DSP_AUDIO_FRAME *pframe);


/***********************************************************
*  函数名: ty_dsp_audio_set
*  功能：设置音频属性
*  参数:  Input，E_DSP_MEDIA_AUDIO_TYPE type，操作的码流类型
        Input，E_DSP_SET_AUDIO set，cmd
        Input，void *parg, 设置的参数
*      
*  返回值: <0 失败，=0 成功
***********************************************************/
int ty_dsp_audio_set(E_DSP_MEDIA_AUDIO_TYPE type, E_DSP_SET_AUDIO set, void *parg);


/***********************************************************
*  函数名: ty_dsp_audio_put
*  功能：音频播放
*  参数:  Input，E_DSP_AUDIO_SOURCE_TYPE type，音频源
        Input，S_DSP_AUDIO_FRAME *pframe，音频帧数据
*      
*  返回值: <0 失败，=0 成功
***********************************************************/
int ty_dsp_audio_put(E_DSP_AUDIO_SOURCE_TYPE type, S_DSP_AUDIO_FRAME *pframe);


/***********************************************************
*  函数名: ty_dsp_audio_play_file
*  功能：播放文件接口
*  参数:  const char* fileName, 文件路径
       E_DSP_AUDIO_FILE_TYPE fileType, 文件格式类型
       int count, 播放次数，0，代表一直循环播放
       int timeOut， 播放阻塞时间单位，ms
       如：ty_dsp_audio_play_file("/tuya/app/skyeye/resource/sound/low_power.pcm",   E_DSP_AUDIO_FILE_PCM, 1, 3*1000);
*      
*  返回值: <0 失败，=0 成功
***********************************************************/
int ty_dsp_audio_play_file(const char* fileName, E_DSP_AUDIO_FILE_TYPE fileType, int count, int timeOut);


/***********************************************************
*  函数名: ty_dsp_audio_stop_play_file
*  功能：停止播放文件
*  参数:  无
*      
*  返回值: <0 失败，=0 成功
***********************************************************/
int ty_dsp_audio_stop_play_file(void);


/***********************************************************
*  函数名: ty_dsp_audio_get_play_file_status
*  功能：获取文件播放状态
*  参数:  无
*      
*  返回值: 0未播放文件，1正在播放文件
***********************************************************/
int ty_dsp_audio_get_play_file_status(void);


/***********************************************************
*  函数名: ty_dsp_uninit
*  功能：dsp去初始化
*  参数:  无
*      
*  返回值: <0 失败，=0 成功
***********************************************************/
int ty_dsp_uninit(void);

 /***********************************************************
 *  函数名: ty_dsp_start_smart_detect
 *  功能：dsp启动各种智能检测
 *  参数:  type 检测的类型
 *      
 *  返回值: <0 失败，=0 成功
 ***********************************************************/
 int ty_dsp_start_smart_detect(E_DSP_DETECT_TYPE type);

 /***********************************************************
 *  函数名: ty_dsp_start_smart_detect
 *  功能：dsp停止各种智能检测
 *  参数:  type 检测的类型
 *      
 *  返回值: <0 失败，=0 成功
 ***********************************************************/
 int ty_dsp_stop_smart_detect(E_DSP_DETECT_TYPE type);

/***********************************************************
*  函数名: ty_dsp_get_detect_result
*  功能：dsp启动各种智能检测结果返回
*  参数:  type  检测的类型
*       pResult 检测结果
*      
*  返回值: <0 失败，=0 成功
***********************************************************/
int ty_dsp_get_detect_result(E_DSP_DETECT_TYPE type, void *pResult);


int ty_dsp_audio_set_volume(int devid, int channel, unsigned int value);
#ifdef __cplusplus
} // extern "C"
#endif

#endif 
