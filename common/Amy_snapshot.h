#ifndef __AMY_SNAPHOT_H__
#define __AMY_SNAPHOT_H__

typedef struct snapshot_context_t snapshot_context_t;


int Amy_snapshot_start(char* data, int pixel_w, int pixel_h, int valid_w, int valid_h, int(*finish_cb)(const char* data, int size));
#endif
