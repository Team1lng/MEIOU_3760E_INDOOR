#include "page/page_tabel.h"
#include "page/page_common.h"
#include "Amy_audio_play.h"
#include "boot_time.h"
#include <signal.h>
#include <unistd.h>

static audio_ring_resource ring_code_param[] = {
    {RING_KEY_MP3_OFFSET, RING_KEY_MP3_SIZE},
    {RING_1_MP3_OFFSET, RING_1_MP3_SIZE},
    {RING_2_MP3_OFFSET, RING_2_MP3_SIZE},
    {RING_3_MP3_OFFSET, RING_3_MP3_SIZE},
    {RING_4_MP3_OFFSET, RING_4_MP3_SIZE},
    {RING_5_MP3_OFFSET, RING_5_MP3_SIZE},
};
static void signal_cb(int sig)
{
  AMY_LOG_ERROR("--------------%d\n", sig);
}

int main(int argv, char **argc)
{
  // signal(SIGINT, signal_cb);
  signal(SIGKILL, signal_cb);
  signal(SIGTERM, signal_cb);
  signal(SIGPIPE, SIG_IGN);

  boot_time_init();
  lv_resource_init(ttf, TTF_FONT_DYC_TTF_OFFSET, TTF_FONT_DYC_TTF_SIZE);
  Amy_sdk_init(ring_code_param, sizeof(ring_code_param) / sizeof(audio_ring_resource), &ttf);
  boot_time_tick("Amy_sdk_init");
  boot_time_tick("enter power page");
  return Amy_lvgl_run(lv_page(power));
}