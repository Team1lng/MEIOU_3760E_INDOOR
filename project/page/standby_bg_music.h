#ifndef _STANDBY_BG_MUSIC_H_
#define _STANDBY_BG_MUSIC_H_

#include <stdbool.h>

int standby_bg_music_start(int volume);
void standby_bg_music_stop(void);
bool standby_bg_music_is_running(void);

#endif
