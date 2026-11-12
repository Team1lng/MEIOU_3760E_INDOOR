#include "boot_time.h"
#include "Amy_log.h"
#include <sys/time.h>

static struct timeval s_t0;
static struct timeval s_last;

void boot_time_init(void)
{
  gettimeofday(&s_t0, NULL);
  s_last = s_t0;
}

void boot_time_tick(const char *name)
{
  struct timeval now;
  gettimeofday(&now, NULL);

  long long delta = (now.tv_sec - s_last.tv_sec) * 1000
                  + (now.tv_usec - s_last.tv_usec) / 1000;
  long long total = (now.tv_sec - s_t0.tv_sec) * 1000
                  + (now.tv_usec - s_t0.tv_usec) / 1000;
  s_last = now;

  AMY_LOG_INFO("[BOOT] %-28s +%lld ms (total %lld ms)\n", name, delta, total);
}
