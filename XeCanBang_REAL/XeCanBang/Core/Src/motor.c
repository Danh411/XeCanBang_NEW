#include "motor.h"

static TIM_HandleTypeDef *pwm;

void Motor_Init(TIM_HandleTypeDef *htim_pwm)
{
    pwm = htim_pwm;
    __HAL_TIM_SET_COMPARE(pwm, TIM_CHANNEL_1, 0);
    __HAL_TIM_SET_COMPARE(pwm, TIM_CHANNEL_2, 0);
    HAL_TIM_PWM_Start(pwm, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(pwm, TIM_CHANNEL_2);
    HAL_GPIO_WritePin(STBY_GPIO_Port, STBY_Pin, GPIO_PIN_SET);   // bat driver
}

void Motor_Set(int motor, float duty)
{
    int inv = (motor == 0) ? MOTOR_A_INVERT : MOTOR_B_INVERT;
    if (inv) duty = -duty;

    if (duty >  MOTOR_MAX_DUTY) duty =  MOTOR_MAX_DUTY;
    if (duty < -MOTOR_MAX_DUTY) duty = -MOTOR_MAX_DUTY;

    // IN1=1,IN2=0: quay thuan | IN1=0,IN2=1: quay nguoc | 0,0: tha troi (stop)
    GPIO_PinState in1 = GPIO_PIN_RESET, in2 = GPIO_PIN_RESET;
    if (duty > 0.0f)      in1 = GPIO_PIN_SET;
    else if (duty < 0.0f) in2 = GPIO_PIN_SET;

    float mag = (duty < 0.0f) ? -duty : duty;
    uint32_t ccr = (uint32_t)(mag * PWM_ARR);

    if (motor == 0) {
        HAL_GPIO_WritePin(AIN1_GPIO_Port, AIN1_Pin, in1);
        HAL_GPIO_WritePin(AIN2_GPIO_Port, AIN2_Pin, in2);
        __HAL_TIM_SET_COMPARE(pwm, TIM_CHANNEL_1, ccr);
    } else {
        HAL_GPIO_WritePin(BIN1_GPIO_Port, BIN1_Pin, in1);
        HAL_GPIO_WritePin(BIN2_GPIO_Port, BIN2_Pin, in2);
        __HAL_TIM_SET_COMPARE(pwm, TIM_CHANNEL_2, ccr);
    }
}

void Motor_StopAll(void)
{
    Motor_Set(0, 0.0f);
    Motor_Set(1, 0.0f);
}
