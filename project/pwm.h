#ifndef PWM_H
#define PWM_H

void backlight_set(int brightness); /* 0~100 % */
void backlight_enable(int en);

void motor_set(int strength); /* 0~100 % */
void motor_enable(int en);

void buzzer_enable(int en);

#endif /* PWM_H */