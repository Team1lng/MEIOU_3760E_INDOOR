#include "pwm.h"
#include "Amy_log.h"
#include "Amy_sdk.h"
#include "pwm_ctrl.h"

/* 背光 PWM */
#define BL_PWM_CHIP (0)
#define BL_PWM_CHANNEL (0)
#define BL_PWM_PERIOD_NS (1000000) /* 1ms -> 1kHz */
/* 背光最小占空比（%），设为0时仍保持此亮度，防止屏幕全灭 */
#define BL_PWM_MIN_PERCENT 5 /* 根据实际屏幕调整 */
/* ------------------------------------------------------------------ */

void backlight_set(int brightness)
{
  /* brightness: 0~100 (%)，实际输出范围 BL_PWM_MIN_PERCENT~100 */

  /* 1. 限幅 */
  if (brightness < 0)
    brightness = 0;
  if (brightness > 100)
    brightness = 100;

  /* 2. 将 0~100 映射到 BL_PWM_MIN_PERCENT~100
   *    实际% = MIN + (100 - MIN) * brightness / 100
   */
  unsigned int actual_percent = BL_PWM_MIN_PERCENT + (unsigned int)((100 - BL_PWM_MIN_PERCENT) * brightness / 100);

  /* 3. 换算为纳秒 */
  unsigned long duty_ns = (unsigned long)(BL_PWM_PERIOD_NS / 100) * actual_percent;

  Amy_pwm_set_duty(BL_PWM_CHIP, BL_PWM_CHANNEL, duty_ns);
}

void backlight_enable(int en)
{
  Amy_pwm_enable(BL_PWM_CHIP, BL_PWM_CHANNEL, en ? 1 : 0);
}

/* ------------------------------------------------------------------ */

void Amy_pwm_ctrl_init(void)
{
  /* 背光 */
  Amy_pwm_init(BL_PWM_CHIP, BL_PWM_CHANNEL, BL_PWM_PERIOD_NS);
  Amy_pwm_set_duty(BL_PWM_CHIP, BL_PWM_CHANNEL, 0);
}