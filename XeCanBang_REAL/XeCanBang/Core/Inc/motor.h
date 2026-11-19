#ifndef MOTOR_H
#define MOTOR_H

#include "stm32f4xx_hal.h"
#include "main.h"

#define PWM_ARR          8399      // khop voi ARR cua TIM9 trong CubeMX (20 kHz)
#define MOTOR_MAX_DUTY   0.90f     // gioi han duty de bao ve motor va TB6612

// Dao chieu quay cua tung motor (0 hoac 1), chinh sau khi thu
#define MOTOR_A_INVERT   1
#define MOTOR_B_INVERT   0

void Motor_Init(TIM_HandleTypeDef *htim_pwm);

// motor: 0 = A (PWMA=PE5, AIN1/AIN2), 1 = B (PWMB=PE6, BIN1/BIN2)
// duty : -1.0 .. +1.0 (dau quyet dinh chieu quay)
void Motor_Set(int motor, float duty);

void Motor_StopAll(void);

#endif
