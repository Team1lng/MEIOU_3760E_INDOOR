#ifndef _STANDBY_PHOTO_FRAME_H_
#define _STANDBY_PHOTO_FRAME_H_

#include <stdbool.h>

int standby_photo_frame_start(int interval_sec);
void standby_photo_frame_stop(void);
bool standby_photo_frame_is_running(void);

#endif
