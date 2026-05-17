#include "stm32f10x.h"

#define AIN1_PIN    0      // PA0 -> AIN1
#define AIN2_PIN    1      // PA1 -> AIN2
#define PWMA_PIN    6      // PA6 -> TIM3_CH1 -> PWMA
#define STBY_PIN    7      // PA7 -> STBY

#define DUTY_FIXED  70     // Fixed duty cycle: 70%

void Motor_Init(void);
void Motor_SetPWM(uint16_t pwm_top, uint8_t duty);
void delay_ms(uint32_t ms);

int main(void)
{
    uint8_t i;

    /*
        PWM_TOP values

        10000 -> 100Hz
        2000  -> 500Hz
        1000  -> 1kHz
        200   -> 5kHz
        100   -> 10kHz
        50    -> 20kHz
    */
    uint16_t pwm_top_list[6] = {
        10000,
        2000,
        1000,
        200,
        100,
        50
    };

    Motor_Init();

    while (1)
    {
        for (i = 0; i < 6; i++)
        {
            Motor_SetPWM(pwm_top_list[i], DUTY_FIXED);
            delay_ms(2000);
        }
    }
}

void Motor_Init(void)
{
    // Enable GPIOA, AFIO, and TIM3 clocks
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_AFIOEN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

    // Clear PA0, PA1, PA6, PA7 configuration bits
    GPIOA->CRL &= ~(
        (0xFU << (AIN1_PIN * 4)) |
        (0xFU << (AIN2_PIN * 4)) |
        (0xFU << (PWMA_PIN * 4)) |
        (0xFU << (STBY_PIN * 4))
    );

    // PA0, PA1, PA7: output push-pull
    // PA6: alternate function output push-pull
    GPIOA->CRL |= (
        (0x3U << (AIN1_PIN * 4)) |
        (0x3U << (AIN2_PIN * 4)) |
        (0xBU << (PWMA_PIN * 4)) |
        (0x3U << (STBY_PIN * 4))
    );

    // Motor forward direction
    // STBY = 1, AIN1 = 1, AIN2 = 0
    GPIOA->BSRR = (1U << STBY_PIN) |
                  (1U << AIN1_PIN) |
                  (1U << (AIN2_PIN + 16));

    /*
        TIM3_CH1 PWM setup

        TIM3 clock = 72MHz
        PSC = 72 - 1
        Timer counter clock = 1MHz
    */
    TIM3->PSC  = 72 - 1;
    TIM3->ARR  = 1000 - 1;     // Initial PWM frequency: 1kHz
    TIM3->CCR1 = 0;

    // PWM mode 1 on channel 1
    TIM3->CCMR1 &= ~((0x3U << 0) | (0x7U << 4));
    TIM3->CCMR1 |=  (0x6U << 4);    // OC1M = 110, PWM mode 1
    TIM3->CCMR1 |=  (1U << 3);      // OC1PE = 1, CCR1 preload enable

    TIM3->CCER &= ~(1U << 1);       // CC1P = 0, active high
    TIM3->CCER |=  (1U << 0);       // CC1E = 1, enable CH1 output

    TIM3->CR1  |=  (1U << 7);       // ARPE = 1, ARR preload enable

    TIM3->EGR  |=  (1U << 0);       // UG = 1, update registers
    TIM3->CR1  |=  (1U << 0);       // CEN = 1, start timer
}

void Motor_SetPWM(uint16_t pwm_top, uint8_t duty)
{
    uint32_t ccr_value;

    if (duty > 100)
    {
        duty = 100;
    }

    /*
        Timer counter clock = 1MHz

        PWM frequency = 1MHz / pwm_top
        ARR = pwm_top - 1
        CCR1 = pwm_top * duty / 100
    */

    ccr_value = ((uint32_t)pwm_top * duty) / 100;

    TIM3->ARR  = pwm_top - 1;
    TIM3->CCR1 = ccr_value;

    // Apply new ARR and CCR1 values
    TIM3->EGR |= (1U << 0);
}

void delay_ms(uint32_t ms)
{
    SysTick->LOAD = 72000 - 1;      // 72MHz -> 1ms
    SysTick->VAL  = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |
                    SysTick_CTRL_ENABLE_Msk;

    while (ms--)
    {
        while ((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) == 0);
    }

    SysTick->CTRL = 0;
}
