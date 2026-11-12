#ifdef CONFIG_MEDIA_TKL_SUPPORT

#include "ty_dsp_adapter_wav.h"


ty_dsp_audio_wav_s *ty_dsp_audio_wav_open(char *file_name)
{
    ty_dsp_audio_wav_s *wav = NULL;
    char buffer[256];
    int  read_len = 0;
    int  offset = 0;

    if(NULL == file_name) {
        printf("file_name is NULL\n");
        return NULL;
    }

    wav = (ty_dsp_audio_wav_s *)malloc(sizeof(ty_dsp_audio_wav_s));
    if(NULL == wav) {
        printf("malloc wav failedly\n");
        return NULL;
    }

    bzero(wav, sizeof(ty_dsp_audio_wav_s));

    wav->fp = fopen(file_name, "r");
    if(NULL == wav->fp) {
        printf("fopen %s failedly\n", file_name);
        free(wav);
        return NULL;
    }

    //handle RIFF WAVE chunk
    read_len = fread(buffer, 1, 12, wav->fp);
    if(read_len < 12) {
        printf("error wav file\n");
        ty_dsp_audio_wav_close(&wav);
        return NULL;
    }

    if(strncasecmp("RIFF", buffer, 4)) {
        printf("no riff, error wav file\n");
        ty_dsp_audio_wav_close(&wav);
        return NULL;
    }

    memcpy(wav->riff.id, buffer, 4);
    wav->riff.size = *(int *)(buffer + 4);
    if(strncasecmp("WAVE", buffer + 8, 4)) {
        printf("no wave, error wav file\n");
        ty_dsp_audio_wav_close(&wav);
        return NULL;
    }

    memcpy(wav->riff.type, buffer + 8, 4);
    wav->file_size = wav->riff.size + 8;

    offset += 12;

    while(1) 
    {
        char id_buffer[5] = {0};
        int  tmp_size = 0;

        read_len = fread(buffer, 1, 8, wav->fp);
        if(read_len < 8) {
            printf("error wav file\n");
            ty_dsp_audio_wav_close(&wav);
            return NULL;
        }
        memcpy(id_buffer, buffer, 4);
        tmp_size = *(int *)(buffer + 4);

        if(0 == strncasecmp("FMT", id_buffer, 3)) {
            memcpy(wav->format.id, id_buffer, 3);
            wav->format.size = tmp_size;
            read_len = fread(buffer, 1, tmp_size, wav->fp);
            if(read_len < tmp_size) {
                printf("error wav file\n");
                ty_dsp_audio_wav_close(&wav);
                return NULL;
            }

            void* ptr = (void*)buffer;

            wav->format.compression_code  = *(short *)ptr;
            wav->format.channels          = *(short *)(ptr + 2);
            wav->format.samples_per_sec   = *(int *)(ptr + 4);
            wav->format.avg_bytes_per_sec = *(int *)(ptr + 8);
            wav->format.block_align       = *(short *)(ptr + 12);
            wav->format.bits_per_sample   = *(short *)(ptr + 14);
        } else if(0 == strncasecmp("DATA", id_buffer, 4)) {
            memcpy(wav->data.id, id_buffer, 4);
            wav->data.size = tmp_size;
            offset += 8;
            wav->data_offset = offset;
            wav->data_size = wav->data.size;
            break;
        } else {
            printf("unhandled chunk: %s, size: %d\n", id_buffer, tmp_size);
            fseek(wav->fp, tmp_size, SEEK_CUR);
        }

        offset += 8 + tmp_size;
    }

    return wav;
}

void ty_dsp_audio_wav_close(ty_dsp_audio_wav_s **wav)
{
    ty_dsp_audio_wav_s *tmp_wav = NULL;

    if(NULL == wav) {
        return;
    }

    tmp_wav = *wav;
    if(NULL == tmp_wav) {
        return;
    }

    if(NULL != tmp_wav->fp) {
        fclose(tmp_wav->fp);
    }

    free(tmp_wav);
    *wav = NULL;

    return;
}

void ty_dsp_audio_wav_dump(ty_dsp_audio_wav_s *wav)
{
    printf("file length: %d\n", wav->file_size);

    printf("\nRIFF WAVE Chunk\n");
    printf("id: %s\n", wav->riff.id);
    printf("size: %d\n", wav->riff.size);
    printf("type: %s\n", wav->riff.type);

    printf("\nFORMAT Chunk\n");
    printf("id: %s\n", wav->format.id);
    printf("size: %d\n", wav->format.size);
    printf("formattag: 0x%02x\n", wav->format.compression_code);

    if(wav->format.compression_code == 0) {
        printf("compression: Unknown\n");
    } else if(wav->format.compression_code == 1) {
        printf("compression: PCM/uncompressed\n");
    } else if(wav->format.compression_code == 2) {
        printf("compression: Microsoft ADPCM\n");
    } else if(wav->format.compression_code == 6) {
        printf("compression: ITU G.711 a-law\n");
    } else if(wav->format.compression_code == 7) {
        printf("compression: ITU G.711 Âµ-law\n");
    } else if(wav->format.compression_code == 17) {
        printf("compression: IMA ADPCM\n");
    } else if(wav->format.compression_code == 20) {
        printf("compression: ITU G.723 ADPCM (Yamaha)\n");
    } else if(wav->format.compression_code == 49) {
        printf("compression: GSM 6.10\n");
    } else if(wav->format.compression_code == 64) {
        printf("compression: ITU G.721 ADPCM\n");
    } else if(wav->format.compression_code == 69) {
        printf("compression: G.726\n");
    } else if(wav->format.compression_code == 80) {
        printf("compression: MPEG\n");
    } else {
        printf("compression: Unknown\n");
    }

    printf("channels: %d\n", wav->format.channels);
    printf("samples: %d\n", wav->format.samples_per_sec);
    printf("avg_bytes_per_sec: %d\n", wav->format.avg_bytes_per_sec);
    printf("block_align: %d\n", wav->format.block_align);
    printf("bits_per_sample: %d\n", wav->format.bits_per_sample);

    printf("\nDATA Chunk\n");
    printf("id: %s\n", wav->data.id);
    printf("size: %d\n", wav->data.size);
    printf("data offset: %d\n", wav->data_offset);

    return;
}

#endif
