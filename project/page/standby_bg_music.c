#include "standby_bg_music.h"
#include "Amy_music_player.h"
#include "Amy_file_manager.h"
#include <stddef.h>
#include "stdio.h"
static bool g_running = false;

static void bg_music_on_track_finished(void)
{
    if (!g_running)
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
    if (g_running)
        return 0;
    Amy_fm_refresh(FM_DIR_MUSIC);
    int total = Amy_fm_get_count(FM_DIR_MUSIC);
    if (total <= 0)
        return -1;
    g_running = true;
    Amy_music_player_play(0);
    Amy_music_player_set_volume(volume);
    Amy_music_player_set_finish_cb(bg_music_on_track_finished);
    return 0;
}

void standby_bg_music_stop(void)
{
    if (!g_running)
        return;
    g_running = false;
    Amy_music_player_set_finish_cb(NULL);
    Amy_music_player_stop();
}

bool standby_bg_music_is_running(void)
{
    return g_running;
}
