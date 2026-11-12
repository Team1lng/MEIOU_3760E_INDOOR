#ifndef _PTAL_MEDIO_H_
#define _PTAL_MEDIO_H_

#ifdef __cplusplus__
extern "C" {
#endif

#define MAX_AUDIO 1
#define MAX_VIDEO 2

typedef enum {
    E_AUDIO_MIC,    // mic  type
    E_AUDIO_SPEAKER,// speaker type
}AUDIO_DEV_TYPE_E;       

typedef enum
{
    E_MEDIA_AUDIO     = 0, //audio main channel
    E_MEDIA_AUDIO_2RD, // audio channel two
    E_MEDIA_AUDIO_3RD, // audio channel tree
    E_MEDIA_AUDIO_4TH, // audio channel foure
    E_MEDIA_AUDIO_MAX, // audio max
}AUDIO_TYPE_E;

typedef enum
{
    E_AUDIO_SAMPLE_8K     = 8000, // audio 8k sample rate
    E_AUDIO_SAMPLE_11K    = 11000,// audio 11k sample rate
    E_AUDIO_SAMPLE_12K    = 12000,// audio 12k sample rate
    E_AUDIO_SAMPLE_16K    = 16000,// audio 16k sample rate
    E_AUDIO_SAMPLE_22K    = 22000,// audio 22k sample rate
    E_AUDIO_SAMPLE_24K    = 24000,// audio 24k sample rate
    E_AUDIO_SAMPLE_32K    = 32000,// audio 32k sample rate
    E_AUDIO_SAMPLE_44K    = 44000,// audio 44k sample rate
    E_AUDIO_SAMPLE_48K    = 48000,// audio 48k sample rate
    E_AUDIO_SAMPLE_MAX    = 0xFFFFFFFF //audio sample max
}AUDIO_SAMPLE_E;

typedef enum
{
    E_AUDIO_DATABITS_8 = 8,   //audio 8bits len
    E_AUDIO_DATABITS_16 = 16, //audio 16bits len
    E_AUDIO_DATABITS_MAX = 0xFF//audio max
}AUDIO_DATABITS_E;

typedef enum
{ 
    E_AUDIO_CHANNEL_MONO,  //audio single channel
    E_AUDIO_CHANNEL_STERO, //audio stereo channel
}AUDIO_CHANNEL_E;

typedef enum
{
    E_CODEC_VIDEO_MPEG4 = 0,//mpeg4 format
    E_CODEC_VIDEO_H263,     //h263  format
    E_CODEC_VIDEO_H264,     //h264  format 
    E_CODEC_VIDEO_MJPEG,    //mjpeg format 
    E_CODEC_VIDEO_H265,     //h265  format 
    E_CODEC_VIDEO_YUV420,   //yuv420 format 
    E_CODEC_VIDEO_YUV422,   //yuv422 format
    E_CODEC_VIDEO_MAX = 99, // video max
    E_CODEC_AUDIO_ADPCM,    // adpcm format 
    E_CODEC_AUDIO_PCM,      // pcm format
    E_CODEC_AUDIO_AAC_RAW,  //aac raw format
    E_CODEC_AUDIO_AAC_ADTS, // aac adts format
    E_CODEC_AUDIO_AAC_LATM, // aac latm format
    E_CODEC_AUDIO_G711U,    // g711u format
    E_CODEC_AUDIO_G711A,    // g711a format
    E_CODEC_AUDIO_G726,     // g726 format
    E_CODEC_AUDIO_SPEEX,    // speex format
    E_CODEC_AUDIO_MP3,      // mp3 format
    E_CODEC_AUDIO_MAX = 199,// audio max
    E_CODEC_INVALID         // audio invalid
}MEDIA_CODEC_ID_E;

typedef struct audio_config_s
{
    unsigned int channel_enable; //channel enable flag
    AUDIO_TYPE_E media_type;     // audio type
    AUDIO_SAMPLE_E sample;       // audio sample rate
    AUDIO_DATABITS_E datebits;   // audio databits
    AUDIO_CHANNEL_E channel;     // audio channel type
    MEDIA_CODEC_ID_E codectype;  // audo codec type
    unsigned int fps;            // audio frame rate
    int mic_volume;              // audio mic volume
    int spk_volume;              // audio spk volume
}AUDIO_CONFIG_T;

typedef enum
{
    E_MEDIA_VIDEO_MAIN = 0, /*main stream*/
    E_MEDIA_VIDEO_SUB,      /*sub stream */
    E_MEDIA_VIDEO_3RD,      /*third stream */          
    E_MEDIA_VIDEO_4TH,      /*fourth steam */
    E_MEDIA_VIDEO_SNAP,     /*capture picture */
    E_MEDIA_VIDEO_MD,       /*move detect*/
    E_MEDIA_VIDEO_HD,       /*human detect */
    E_MEDIA_VIDEO_QR,       /*QR code */
    E_MEDIA_VIDEO_MAX,      /*video max*/
}VIDEO_TYPE_E;


typedef enum
{
    E_VIDEO_BITRATE_64K   = 64, //64k bitrate
    E_VIDEO_BITRATE_128K  = 128,//128k bitrate
    E_VIDEO_BITRATE_256K  = 256,//256k bitrate
    E_VIDEO_BITRATE_512K  = 512,//512k bitrate
    E_VIDEO_BITRATE_768K  = 768,//768k bitrate
    E_VIDEO_BITRATE_1M    = 1024,//1024k bitrate
    E_VIDEO_BITRATE_1_5M  = 1536,//1536k bitrate
    E_VIDEO_BITRATE_2M    = 2048 //2048k bitrate
} VIDEO_BITRATE_E;

typedef struct
{
    unsigned int channel_enable; //channel enabel
    VIDEO_TYPE_E media_type;     // video type
    unsigned int fps;            // frame rate
    unsigned int gop;            // I frame interval
    VIDEO_BITRATE_E bitrate;     // video bitrate
    unsigned int width;          // video width
    unsigned int height;         // video height
    MEDIA_CODEC_ID_E codectype;  // video codec type
    unsigned int firstFrame;     // video firstframe flag
    unsigned int minQp;          // video min Qp
    unsigned int maxQp;          // video max Qp
}VIDEO_CONFIG_T;


typedef struct media_config_s {
    AUDIO_CONFIG_T audio[MAX_AUDIO]; //audio config information
    VIDEO_CONFIG_T video[MAX_VIDEO]; //video config inforamtion
}MEDIA_CONFIG_T;

enum {
    E_AUDIO_FILE_PCM,
    E_AUDIO_FILE_WAV,
    E_AUDIO_FILE_MAX,
};

typedef enum {
    E_VIDEO_FRAME_PB,
    E_VIDEO_FRAME_I,
    E_VIDEO_FRAME_TS,
    E_VIDEO_FRAME_MAX,
    E_AUDIO_FRAME_MIC,
}VIDEO_FRAME_TYPE_E;

typedef enum {
   E_MODE_AUTO=0,
   E_MODE_DAY,
   E_MODE_NIGHT,
}ISP_MODE_E;


extern int ptal_load_day_isp_file(void);
extern int ptal_load_night_isp_file(void);
extern int ptal_media_init(MEDIA_CONFIG_T *config);
extern int ptal_mic_get_audio_rate(int *rate);

extern int ptal_audio_get_frame(char **buf,int *plen,uint64_t  *pts); 
extern int ptal_audio_put_frame(char *buf,int plen);


extern int ptal_speaker_set_volume(int devid, int channel, unsigned int value);
extern int ptal_speaker_get_volume(int devid, int channel, unsigned int *value);

extern int ptal_mic_set_volume(int devid, int channel, unsigned int value);
extern int ptal_mic_get_volume(int devid, int channel, unsigned int *value);

extern int ptal_speaker_play_file(const char* fileName, int fileType, int count, int timeOut);
extern int ptal_speaker_stop(void);
//extern int ptal_video_get_frame(int *type,char **buf,int *plen,uint64_t *pts);
extern int ptal_video_get_frame(int video_type,int*frame_type,char **buf,int *plen,uint64_t *pts);

// 0,none,1 mirror,2,flip,3,both
extern int ptal_video_set_mirror_flip(int mirror_flip);

//0,auto,1,day,2,night
extern int ptal_isp_set_color_to_gray(ISP_MODE_E mode);

//0,auto,1,day,2,night
extern int ptal_get_virtual_adc(ISP_MODE_E mode,uint32_t* val);

extern int ptal_video_code_type(int channel);

#ifdef __cplusplus__
}
#endif

#endif
