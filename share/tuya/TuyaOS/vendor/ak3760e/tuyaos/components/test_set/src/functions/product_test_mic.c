/* 
 *  @copyright Copyright (c) tuya.inc 2021
 */
#include <inttypes.h> 
#include <tinyalsa/asoundlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "product_test_media.h"
#include "product_test_mic.h"
#include "product_test_utils.h"
#include "product_test_config.h"
#include "tb_log.h"

#define FORMAT_PCM 1
#define MIC_MODE_KEY "mic_mode"


typedef struct wave_pcm_hdr_s {
    char            riff[4];                // = "RIFF"
    int             size_8;                 // = FileSize - 8
    char            wave[4];                // = "WAVE"
    char            fmt[4];                 // = "fmt "
    int             fmt_size;               // = "fmt_size": 16

    short int       format_tag;             // = PCM : 1
    short int       channels;               // = channels: 1
    int             samples_per_sec;        // = samples: 8000 | 16000
    int             avg_bytes_per_sec;      // = byte: samples_per_sec * channels * bits_per_sample / 8
    short int       block_align;            // = aliagn: Channels * bits_per_sample / 8
    short int       bits_per_sample;        // = sample_bi: 8 | 16

    char            data[4];                // = "data";
    int             data_size;              // = "data_size" : FileSize - 44 
} WAVE_PCM_HDR_T;

static WAVE_PCM_HDR_T sg_default_wav_hdr = 
{
    { 'R', 'I', 'F', 'F' },
    0,
    {'W', 'A', 'V', 'E'},
    {'f', 'm', 't', ' '},
    16,
    1,
    1,
    16000,
    32000,
    2,
    16,
    {'d', 'a', 't', 'a'},
    0  
};

static int sg_mic_record_exit = 0;
static int sg_capturing = 1;

static int __product_test_config_mic_get_mode(int *mode) 
{
    char *dev,*key;
    int enable;

    dev = CONFIG_DEVICE_PRODUCT_CFG;
    key = CONFIG_KEY_ENABLE;

    if(product_test_config_read_int(dev,key,&enable) || enable != 1) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return-1;
    }

    key = CONFIG_KEY_CFG_TYNY_ALSA;
    
    if(product_test_config_read_int(dev,key,mode)) {
        TB_LOGE("device:%s parm:%s fail",dev,key);
        return -1;
    }

    return 0;

}

static unsigned int __capture_sample_mic(int fd, unsigned int card, unsigned int device,
                            unsigned int channels, unsigned int rate,
                            enum pcm_format format, unsigned int period_size,
                            unsigned int period_count, unsigned int cap_time)
{
    struct pcm_config config;
    struct pcm *pcm;
    char *buffer;
    unsigned int size;
    unsigned int bytes_read = 0, frames;
    struct timespec end;
    struct timespec now;

    memset(&config, 0, sizeof(config));
    config.channels = channels;
    config.rate = rate;
    config.period_size = period_size;
    config.period_count = period_count;
    config.format = format;
    config.start_threshold = 0;
    config.stop_threshold = 0;
    config.silence_threshold = 0;

    pcm = pcm_open(card, device, PCM_IN, &config);
    if (!pcm || !pcm_is_ready(pcm)) {
        fprintf(stderr, "Unable to open PCM device (%s)\n",
                pcm_get_error(pcm));
        return 0;
    }

    size = pcm_frames_to_bytes(pcm, pcm_get_buffer_size(pcm));
    buffer = malloc(size);
    if (!buffer) {
        fprintf(stderr, "Unable to allocate %d bytes\n", size);
        free(buffer);
        pcm_close(pcm);
        return 0;
    }

    printf("Capturing sample: %u ch, %u hz, %u bit\n", channels, rate,
           pcm_format_to_bits(format));

    clock_gettime(CLOCK_MONOTONIC, &now);
    end.tv_sec = now.tv_sec + cap_time;
    end.tv_nsec = now.tv_nsec;

    while (sg_capturing && !pcm_read(pcm, buffer, size)) {
        if (send(fd,buffer,size,0) != size) {
            fprintf(stderr,"Error capturing sample\n");
            break;
        }
        bytes_read += size;
        if (cap_time) {
            clock_gettime(CLOCK_MONOTONIC, &now);
            if (now.tv_sec > end.tv_sec ||
                (now.tv_sec == end.tv_sec && now.tv_nsec >= end.tv_nsec))
                break;
        }
    }

    frames = pcm_bytes_to_frames(pcm, bytes_read);
    free(buffer);
    pcm_close(pcm);
    return frames;
}

static int __product_test_tinycap_mic(int fd, unsigned int card, unsigned int device,
           unsigned int channels, unsigned int rate, unsigned int bits,
           unsigned int period_size, unsigned int period_count, unsigned int cap_time)
{
    unsigned int frames;
    enum pcm_format format;

    switch (bits) {
    case 32:
        format = PCM_FORMAT_S32_LE;
        break;
    case 24:
        format = PCM_FORMAT_S24_LE;
        break;
    case 16:
        format = PCM_FORMAT_S16_LE;
        break;
    default:
        fprintf(stderr, "%d bits is not supported.\n", bits);
        return 1;
    }
    frames = __capture_sample_mic(fd, card, device, channels,
                            rate, format,
                            period_size, period_count, cap_time);
    printf("Captured %d frames\n", frames);
    return 0;
}

int product_test_mic_tiny_capature_and_upload(int fd)
{
    return __product_test_tinycap_mic(fd,0, 0,6, 16000, 16, 1024, 4, 5);
}

int product_test_mic_record_start(char *file_path,int seconds) 
{
    FILE *fp = NULL;
    int rate;
    unsigned long start_time;
    int total = 0;
    WAVE_PCM_HDR_T wav_hdr = sg_default_wav_hdr;

    TB_LOGD("open %s",file_path);

    product_test_media_init();

    fp = fopen(file_path,"wb");

    if(fp == NULL) {

        TB_LOGE("open %s fail",file_path);  
        return -1;  
    }

    if(product_test_mic_get_sample_rate(&rate)) {
        TB_LOGE("get audio rate fail");
        return -1;
    }

    TB_LOGD("mic rate:%d",rate);

    wav_hdr.bits_per_sample   = 16;
    wav_hdr.samples_per_sec   = rate;
    wav_hdr.block_align       = 16 / 8;
    wav_hdr.avg_bytes_per_sec = wav_hdr.samples_per_sec * wav_hdr.bits_per_sample / 8 ;
    fwrite(&wav_hdr, sizeof(wav_hdr) ,1, fp);

    start_time = product_test_get_curtime();

    while(LESS_TARGET(product_test_get_curtime(),start_time,seconds)) {
        char *buf;
        int size;
        uint64_t pts;
        char * pPcmData = NULL;
        int    pcmLen = 0;

        if(product_test_audio_get_frame(&buf,&size,&pts) == 0) {
            pPcmData = malloc(size*3);
            if (NULL == pPcmData){
                TB_LOGE("%s %d malloc failed\n",__func__,__LINE__);
                return -1;
            }

            product_test_g711_to_pcm((unsigned short *)buf,(unsigned int)size, (unsigned char *)pPcmData, (unsigned int *)&pcmLen);
            fwrite(pPcmData,pcmLen,1, fp);
            total += pcmLen;
            free(pPcmData);
        }

        usleep(5*1000); 
    }

    TB_LOGD("mic record:%d bytes",total);

    wav_hdr.data_size = total;
    wav_hdr.size_8 += wav_hdr.data_size + (sizeof(wav_hdr) - 8);

    fseek(fp, 4, 0);
    fwrite(&wav_hdr.size_8, sizeof(wav_hdr.size_8), 1, fp);
    fseek(fp, 40, 0);
    fwrite(&wav_hdr.data_size, sizeof(wav_hdr.data_size), 1, fp);
    fclose(fp);

    return 0;
}

int product_test_mic_record_start_no_timeout(char *file_path) 
{
    FILE *fp = NULL;
    int rate;
    int total = 0;
    WAVE_PCM_HDR_T wav_hdr = sg_default_wav_hdr;

    TB_LOGD("open %s",file_path);

    product_test_media_init();

    fp = fopen(file_path,"wb");

    if(fp == NULL) {

        TB_LOGE("open %s fail",file_path);  
        return -1;
    }

    if(product_test_mic_get_sample_rate(&rate)) {
        TB_LOGE("get audio rate fail");
        return -1;
    }

    TB_LOGD("mic rate:%d",rate);

    wav_hdr.bits_per_sample   = 16;
    wav_hdr.samples_per_sec   = rate;
    wav_hdr.block_align       = 16 / 8;
    wav_hdr.avg_bytes_per_sec = wav_hdr.samples_per_sec * wav_hdr.bits_per_sample / 8 ;
    fwrite(&wav_hdr, sizeof(wav_hdr) ,1, fp);

    sg_mic_record_exit = 0;

    while(!sg_mic_record_exit) {
        char *buf;
        int size;
        uint64_t pts;
        char * pPcmData = NULL;
        int    pcmLen = 0;

        if(product_test_audio_get_frame(&buf,&size,&pts) == 0) {
            pPcmData = malloc(size*3);
            if (NULL == pPcmData) {
                TB_LOGE("%s %d malloc failed\n",__func__,__LINE__);
                return -1;
            }

            product_test_g711_to_pcm((unsigned short *)buf,(unsigned int)size, (unsigned char *)pPcmData, (unsigned int *)&pcmLen);
            fwrite(pPcmData,pcmLen,1, fp);
            total += pcmLen;
            free(pPcmData);
        }

        usleep(5*1000); 
    }

    TB_LOGD("mic exit record:%d bytes",total);

    wav_hdr.data_size = total;
    wav_hdr.size_8 += wav_hdr.data_size + (sizeof(wav_hdr) - 8);

    fseek(fp, 4, 0);
    fwrite(&wav_hdr.size_8, sizeof(wav_hdr.size_8), 1, fp);
    fseek(fp, 40, 0);
    fwrite(&wav_hdr.data_size, sizeof(wav_hdr.data_size), 1, fp);
    fclose(fp);

    return 0;
}

int product_test_mic_record_stop(void) 
{
    sg_mic_record_exit  = 1;
    return 0;
}

int product_test_mic_get_volume(int dev_id,int channel,uint32_t *value)
{

    return 0;
}

int product_test_mic_set_volume(int dev_id,int channel,uint32_t value)
{

    return 0;
}

int product_test_mic_set_mode(MIC_INFACE_TYPE_E mode) 
{
    return product_test_utils_write_int(MIC_MODE_KEY,mode,0);
}

int product_test_mic_get_mode(MIC_INFACE_TYPE_E *mode) 
{
    int iret;
    int t_mode;

    iret =  product_test_utils_read_int(MIC_MODE_KEY,&t_mode);

    if(iret) {
        iret = __product_test_config_mic_get_mode(&t_mode);
    }

    *mode = (MIC_INFACE_TYPE_E)(t_mode & 0xff);

    return iret;
}


int product_test_mic_get_frame(char *buf,int plen)
{
    char *data;
    int size;
    int len;

    if(buf == NULL || plen==0 ) {
        TB_LOGE("invalid parm");
        return -1;
    }

    if(product_test_audio_get_frame(&data,&size,NULL) == 0) {
       if(plen < size*3) {
           TB_LOGE("buffer size(%d) < len(%d)",plen,len*3);
           return -1; 
       }

       product_test_g711_to_pcm((unsigned short *)data,(unsigned int)size, (unsigned char *)buf, (unsigned int *)&len);
    
    }

    return len;
}
