#include "standby_bg_music.h"
#include "Amy_music_player.h"
#include "Amy_file_manager.h"
#include "Amy_lvgl.h"
#include "Amy_tfcard.h"
#include <stddef.h>
#include "stdio.h"
static bool g_running = false;
static bool g_finish_pending;
static Amy_mutex_t g_finish_mutex = PTHREAD_MUTEX_INITIALIZER;
static lv_timer_t *g_finish_timer;

static void bg_music_on_track_finished(void)
{
    /* Completion is retained even when the shared UI event queue is full. */
    Amy_mutex_lock(&g_finish_mutex);
    if (g_running)
        g_finish_pending = true;
    Amy_mutex_unlock(&g_finish_mutex);
}

static void bg_music_finish_timer_cb(lv_timer_t *timer)
{
    Amy_mutex_lock(&g_finish_mutex);
    bool finished = g_running && g_finish_pending;
    g_finish_pending = false;
    Amy_mutex_unlock(&g_finish_mutex);
    if (!finished || Amy_tfcard_status() != SD_STATE_MOUNT)
        return;

    int total = Amy_music_player_get_track_count();
    if (total <= 0)
        return;

    int cur = Amy_music_player_get_current_index();
    int next = (cur + 1 < total) ? cur + 1 : 0;
    Amy_music_player_play(next);
}

int standby_bg_music_start(int volume)
{
    if (Amy_tfcard_status() != SD_STATE_MOUNT)
        return -1;
    if (g_running)
        return 0;
    Amy_fm_refresh(FM_DIR_MUSIC);
    int total = Amy_fm_get_count(FM_DIR_MUSIC);
    if (total <= 0)
        return -1;
    g_finish_timer = lv_timer_create(bg_music_finish_timer_cb, 100, NULL);
    if (!g_finish_timer)
        return -1;
    Amy_mutex_lock(&g_finish_mutex);
    g_finish_pending = false;
    g_running = true;
    Amy_mutex_unlock(&g_finish_mutex);
    Amy_music_player_play(0);
    Amy_music_player_set_volume(volume);
    Amy_music_player_set_finish_cb(bg_music_on_track_finished);
    return 0;
}

void standby_bg_music_stop(void)
{
    Amy_mutex_lock(&g_finish_mutex);
    if (!g_running)
    {
        Amy_mutex_unlock(&g_finish_mutex);
        return;
    }
    g_running = false;
    g_finish_pending = false;
    Amy_mutex_unlock(&g_finish_mutex);
    if (g_finish_timer)
    {
        lv_timer_del(g_finish_timer);
        g_finish_timer = NULL;
    }
    Amy_music_player_set_finish_cb(NULL);
    Amy_music_player_stop();
}

bool standby_bg_music_is_running(void)
{
    return g_running;
}
