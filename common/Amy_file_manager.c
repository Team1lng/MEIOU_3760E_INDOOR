#include "Amy_file_manager.h"
#include "Amy_tfcard.h"
#include "Amy_log.h"
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>

#define MUSIC_DIR TFCARD_MOUNT_PATH "/music"
#define PHOTO_DIR TFCARD_MOUNT_PATH "/picture"
#define MUSIC_MAX 100
#define PHOTO_MAX 1000

static fm_file_info_t g_music_files[MUSIC_MAX];
static int g_music_count = 0;

static fm_file_info_t g_photo_files[PHOTO_MAX];
static int g_photo_count = 0;

static bool is_music_ext(const char *name)
{
    const char *dot = strrchr(name, '.');
    if (dot == NULL)
        return false;
    return strcasecmp(dot, ".mp3") == 0;
}

static bool is_photo_ext(const char *name)
{
    const char *dot = strrchr(name, '.');
    if (dot == NULL)
        return false;
    return (strcasecmp(dot, ".jpg") == 0 ||
            strcasecmp(dot, ".jpeg") == 0 ||
            strcasecmp(dot, ".png") == 0 ||
            strcasecmp(dot, ".bmp") == 0);
}

static int scan_dir(const char *dir_path, fm_file_info_t *files, int max,
                    bool (*filter)(const char *name))
{
    DIR *dir = opendir(dir_path);
    if (dir == NULL)
        return 0;

    int count = 0;
    struct dirent *ent;
    char full_path[512];

    while ((ent = readdir(dir)) != NULL && count < max)
    {
        if (ent->d_name[0] == '.')
            continue;
        if (!filter(ent->d_name))
            continue;

        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, ent->d_name);

        struct stat st;
        if (stat(full_path, &st) != 0)
            continue;
        if (!S_ISREG(st.st_mode))
            continue;

        strncpy(files[count].name, ent->d_name, sizeof(files[count].name) - 1);
        files[count].name[sizeof(files[count].name) - 1] = '\0';
        files[count].size = (unsigned long)st.st_size;
        count++;
    }

    closedir(dir);
    return count;
}

static const char *fm_dir_path(fm_dir_type_e dir)
{
    switch (dir)
    {
    case FM_DIR_MUSIC:
        return MUSIC_DIR;
    case FM_DIR_PHOTO:
        return PHOTO_DIR;
    default:
        return NULL;
    }
}

static fm_file_info_t *fm_cache(fm_dir_type_e dir, int *count)
{
    switch (dir)
    {
    case FM_DIR_MUSIC:
        *count = g_music_count;
        return g_music_files;
    case FM_DIR_PHOTO:
        *count = g_photo_count;
        return g_photo_files;
    default:
        *count = 0;
        return NULL;
    }
}

int Amy_fm_init(void)
{
    if (Amy_tfcard_status() != SD_STATE_MOUNT)
    {
        AMY_LOG_WARNING("fm: sd card not mounted");
        return -1;
    }

    g_music_count = scan_dir(MUSIC_DIR, g_music_files, MUSIC_MAX, is_music_ext);
    AMY_LOG_INFO("fm: music count=%d", g_music_count);

    g_photo_count = scan_dir(PHOTO_DIR, g_photo_files, PHOTO_MAX, is_photo_ext);
    AMY_LOG_INFO("fm: photo count=%d", g_photo_count);

    return 0;
}

int Amy_fm_get_count(fm_dir_type_e dir)
{
    switch (dir)
    {
    case FM_DIR_MUSIC:
        return g_music_count;
    case FM_DIR_PHOTO:
        return g_photo_count;
    default:
        return -1;
    }
}

int Amy_fm_get_file(fm_dir_type_e dir, int index, fm_file_info_t *info)
{
    if (info == NULL)
        return -1;

    fm_file_info_t *src = NULL;
    int count = 0;

    switch (dir)
    {
    case FM_DIR_MUSIC:
        src = g_music_files;
        count = g_music_count;
        break;
    case FM_DIR_PHOTO:
        src = g_photo_files;
        count = g_photo_count;
        break;
    default:
        return -1;
    }

    if (index < 0 || index >= count)
        return -1;

    memcpy(info, &src[index], sizeof(fm_file_info_t));
    return 0;
}

int Amy_fm_delete(fm_dir_type_e dir, int index)
{
    const char *dir_path = fm_dir_path(dir);
    if (dir_path == NULL)
        return -1;

    int count = 0;
    fm_file_info_t *files = fm_cache(dir, &count);
    if (files == NULL || index < 0 || index >= count)
        return -1;

    char full_path[512];
    snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, files[index].name);
    if (unlink(full_path) != 0)
    {
        AMY_LOG_ERROR("fm: delete %s failed", full_path);
        return -1;
    }

    Amy_fm_refresh(dir);
    return 0;
}

int Amy_fm_delete_all(fm_dir_type_e dir)
{
    const char *dir_path = fm_dir_path(dir);
    if (dir_path == NULL)
        return -1;

    int count = 0;
    fm_file_info_t *files = fm_cache(dir, &count);

    char full_path[512];
    for (int i = 0; i < count; i++)
    {
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, files[i].name);
        if (unlink(full_path) != 0)
            AMY_LOG_ERROR("fm: delete %s failed", full_path);
    }

    Amy_fm_refresh(dir);
    return 0;
}

int Amy_fm_refresh(fm_dir_type_e dir)
{
    if (Amy_tfcard_status() != SD_STATE_MOUNT)
    {
        /* 拔卡/未挂载时清空对应缓存，避免重新进入页面残留旧媒体列表 */
        if (dir == FM_DIR_MUSIC)
            g_music_count = 0;
        else if (dir == FM_DIR_PHOTO)
            g_photo_count = 0;
        AMY_LOG_WARNING("fm: sd card not mounted");
        return -1;
    }

    switch (dir)
    {
    case FM_DIR_MUSIC:
        g_music_count = scan_dir(MUSIC_DIR, g_music_files, MUSIC_MAX, is_music_ext);
        AMY_LOG_INFO("fm: music refresh count=%d", g_music_count);
        return 0;
    case FM_DIR_PHOTO:
        g_photo_count = scan_dir(PHOTO_DIR, g_photo_files, PHOTO_MAX, is_photo_ext);
        AMY_LOG_INFO("fm: photo refresh count=%d", g_photo_count);
        return 0;
    default:
        return -1;
    }
}
