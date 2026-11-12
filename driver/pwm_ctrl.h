#ifndef PWM_CTRL_H
#define PWM_CTRL_H

int Amy_pwm_init(int chip, int channel, unsigned int period_ns);
int Amy_pwm_deinit(int chip, int channel);
int Amy_pwm_set_period(int chip, int channel, unsigned long period_ns);
int Amy_pwm_set_duty(int chip, int channel, unsigned long duty_ns);
int Amy_pwm_enable(int chip, int channel, int en);
void Amy_pwm_ctrl_init(void);
#endif /* PWM_CTRL_H */