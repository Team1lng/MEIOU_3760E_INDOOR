#ifndef _AMY_FILE_MANAGER_H_
#define _AMY_FILE_MANAGER_H_

typedef enum {
    FM_DIR_MUSIC,
    FM_DIR_PHOTO,
    FM_DIR_TOTAL
} fm_dir_type_e;

typedef struct {
    char name[256];
    unsigned long size;
} fm_file_info_t;

int Amy_fm_init(void);
int Amy_fm_get_count(fm_dir_type_e dir);
int Amy_fm_get_file(fm_dir_type_e dir, int index, fm_file_info_t *info);
int Amy_fm_refresh(fm_dir_type_e dir);
int Amy_fm_delete(fm_dir_type_e dir, int index);
int Amy_fm_delete_all(fm_dir_type_e dir);

#endif
