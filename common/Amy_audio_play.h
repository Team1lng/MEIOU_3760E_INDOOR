#ifndef _AMY_AUDIO_PLAY_H_
#define _AMY_AUDIO_PLAY_H_
typedef struct audio_ring_resource
{
    unsigned long offset;
    unsigned long size;
} audio_ring_resource;

int Amy_audio_play_init(const char *mtd_name, const audio_ring_resource *resource, int cnt);
int Amy_audio_play(int index, int volume, int interrupt_enable, void (*start_cb)(int), int (*finish_cb)(int));
int Amy_audio_play_stop(void);
#endif
