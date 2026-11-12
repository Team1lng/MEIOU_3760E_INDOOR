#ifndef _TY_DSP_ADAPTER_WAV_H_
#define _TY_DSP_ADAPTER_WAV_H_

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct _wav_riff_t {
    char id[5];               //ID:"RIFF"
    int  size;                //file_len - 8
    char type[5];             //type:"WAVE"
} ty_wav_riff_s;


typedef struct _wav_format_t {
    char  id[5];              //ID:"fmt"
    int   size;
    short compression_code;
    short channels;
    int   samples_per_sec;
    int   avg_bytes_per_sec;
    short block_align;
    short bits_per_sample;
} ty_wav_format_s;


typedef struct _wav_fact_t {
    char id[5];
    int  size;
} ty_wav_fact_s;


typedef struct _wav_data_t {
    char id[5];
    int  size;
} ty_wav_data_s;


typedef struct _wav_t {
    FILE*           fp;
    ty_wav_riff_s   riff;
    ty_wav_format_s format;
    ty_wav_fact_s   fact;
    ty_wav_data_s   data;
    int             file_size;
    int             data_offset;
    int             data_size;
} ty_dsp_audio_wav_s;


ty_dsp_audio_wav_s *ty_dsp_audio_wav_open(char *file_name);


void ty_dsp_audio_wav_close(ty_dsp_audio_wav_s **wav);


void ty_dsp_audio_wav_dump(ty_dsp_audio_wav_s *wav);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
