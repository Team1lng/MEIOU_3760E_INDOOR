#include "pwm_ctrl.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "fcntl.h"
#include "Amy_log.h"
#include "Amy_sdk.h"

#define PWM_EXPORT_BASE "/sys/class/pwm/pwmchip"
#define PWM_BASE "/sys/class/pwm/pwmchip"

/* ------------------------------------------------------------------ */
/*  内部辅助：向 sysfs 节点写字符串                                     */
/* ------------------------------------------------------------------ */
static int pwm_sysfs_write(const char *path, const char *value)
{
  int fd = open(path, O_WRONLY);
  if (fd < 0)
  {
    AMY_LOG_ERROR("open %s failed\n", path);
    return -1;
  }
  int len = strlen(value);
  if (write(fd, value, len) != len)
  {
    AMY_LOG_ERROR("write %s failed\n", path);
    close(fd);
    return -1;
  }
  close(fd);
  return 0;
}

/* ------------------------------------------------------------------ */
/*  export / unexport                                                   */
/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
/*  export / unexport                                                   */
/* ------------------------------------------------------------------ */
int Amy_pwm_init(int chip, int channel, unsigned int period_ns)
{
  char path[64] = {0};
  char buf[32] = {0};

  /* 1. 先检查节点是否已经存在，不存在才 export */
  snprintf(path, sizeof(path), "%s%d/pwm%d", PWM_EXPORT_BASE, chip, channel);
  if (access(path, F_OK) != 0)
  {
    /* 节点不存在，执行 export */
    snprintf(path, sizeof(path), "%s%d/export", PWM_EXPORT_BASE, chip);
    snprintf(buf, sizeof(buf), "%d", channel);
    if (pwm_sysfs_write(path, buf) < 0)
    {
      AMY_LOG_ERROR("export pwmchip%d channel%d failed\n", chip, channel);
      return -1;
    }
    Amy_msleep(100);
  }
  else
  {
    AMY_LOG_INFO("pwmchip%d channel%d already exported, skip\n", chip, channel);
  }

  /* 2. 先禁用，再设置周期（防止运行中修改参数报错） */
  snprintf(path, sizeof(path), "%s%d/pwm%d/enable", PWM_EXPORT_BASE, chip, channel);
  pwm_sysfs_write(path, "0");

  /* 3. 设置周期 */
  snprintf(path, sizeof(path), "%s%d/pwm%d/period", PWM_EXPORT_BASE, chip, channel);
  snprintf(buf, sizeof(buf), "%u", period_ns);
  if (pwm_sysfs_write(path, buf) < 0)
  {
    AMY_LOG_ERROR("set pwmchip%d channel%d period(%uns) failed\n", chip, channel, period_ns);
    return -1;
  }

  AMY_LOG_INFO("pwmchip%d channel%d init ok, period=%uns\n", chip, channel, period_ns);
  return 0;
}

int Amy_pwm_deinit(int chip, int channel)
{
  char path[64] = {0};
  char buf[10] = {0};

  snprintf(path, sizeof(path), "%s%d/unexport", PWM_EXPORT_BASE, chip);
  snprintf(buf, sizeof(buf), "%d", channel);
  if (pwm_sysfs_write(path, buf) < 0)
  {
    AMY_LOG_ERROR("unexport pwmchip%d channel%d failed\n", chip, channel);
    return -1;
  }
  return 0;
}

/* ------------------------------------------------------------------ */
/*  设置周期（单位：纳秒）                                              */
/* ------------------------------------------------------------------ */
int Amy_pwm_set_period(int chip, int channel, unsigned long period_ns)
{
  char path[64] = {0};
  char buf[32] = {0};

  snprintf(path, sizeof(path), "%s%d/pwm%d/period", PWM_BASE, chip, channel);
  snprintf(buf, sizeof(buf), "%lu", period_ns);
  if (pwm_sysfs_write(path, buf) < 0)
  {
    AMY_LOG_ERROR("set pwmchip%d channel%d period:%lu failed\n", chip, channel, period_ns);
    return -1;
  }
  return 0;
}

/* ------------------------------------------------------------------ */
/*  设置占空比（单位：纳秒）                                            */
/* ------------------------------------------------------------------ */
int Amy_pwm_set_duty(int chip, int channel, unsigned long duty_ns)
{
  char path[64] = {0};
  char buf[32] = {0};

  snprintf(path, sizeof(path), "%s%d/pwm%d/duty_cycle", PWM_BASE, chip, channel);
  snprintf(buf, sizeof(buf), "%lu", duty_ns);
  if (pwm_sysfs_write(path, buf) < 0)
  {
    AMY_LOG_ERROR("set pwmchip%d channel%d duty_cycle:%lu failed\n", chip, channel, duty_ns);
    return -1;
  }
  return 0;
}

/* ------------------------------------------------------------------ */
/*  使能 / 禁用                                                         */
/* ------------------------------------------------------------------ */
int Amy_pwm_enable(int chip, int channel, int en)
{
  char path[64] = {0};

  snprintf(path, sizeof(path), "%s%d/pwm%d/enable", PWM_BASE, chip, channel);
  if (pwm_sysfs_write(path, en ? "1" : "0") < 0)
  {
    AMY_LOG_ERROR("%s pwmchip%d channel%d failed\n",
                  en ? "enable" : "disable", chip, channel);
    return -1;
  }
  return 0;
}