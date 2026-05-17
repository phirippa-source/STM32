#include "stm32f10x.h"

#define AIN1_PIN    0      // PA0 -> AIN1
#define AIN2_PIN    1      // PA1 -> AIN2
#define PWMA_PIN    6      // PA6 -> TIM3_CH1 -> PWMA
#define STBY_PIN    7      // PA7 -> STBY

#define PWM_TOP     100    // 10kHz PWM when timer counter clock is 1MHz

void Motor_Init(void);
void Motor_SetDuty(uint8_t duty);
void Motor_Forward_100(void);
void Motor_Stop(void);
void Motor_ShortBrake(void);
void delay_ms(uint32_t ms);

int main(void)
{
    Motor_Init();

    while (1)
    {
        // Run forward for 2 seconds
        Motor_Forward_100();
        delay_ms(2000);

        // Stop: motor terminals are open
        Motor_Stop();
        delay_ms(2000);

        // Run forward again for 2 seconds
        Motor_Forward_100();
        delay_ms(2000);

        // Short brake: motor terminals are shorted
        Motor_ShortBrake();
        delay_ms(2000);
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

    /*
        TIM3_CH1 PWM setup

        TIM3 clock = 72MHz
        PSC = 72 - 1       -> timer counter clock = 1MHz
        ARR = 100 - 1      -> PWM frequency = 10kHz
    */
    TIM3->PSC  = 72 - 1;
    TIM3->ARR  = PWM_TOP - 1;
    TIM3->CCR1 = 0;

    // PWM mode 1 on channel 1
    TIM3->CCMR1 &= ~((0x3U << 0) | (0x7U << 4));
    TIM3->CCMR1 |=  (0x6U << 4);    // OC1M = 110, PWM mode 1
    TIM3->CCMR1 |=  (1U << 3);      // OC1PE = 1

    TIM3->CCER &= ~(1U << 1);       // CC1P = 0, active high
    TIM3->CCER |=  (1U << 0);       // CC1E = 1, enable CH1 output

    TIM3->CR1  |=  (1U << 7);       // ARPE = 1
    TIM3->EGR  |=  (1U << 0);       // UG = 1
    TIM3->CR1  |=  (1U << 0);       // CEN = 1

    // Enable TB6612FNG
    GPIOA->BSRR = (1U << STBY_PIN);
}

void Motor_SetDuty(uint8_t duty)
{
    if (duty > 100)
    {
        duty = 100;
    }

    TIM3->CCR1 = (PWM_TOP * duty) / 100;
}

void Motor_Forward_100(void)
{
    /*
        Forward:
        STBY = 1
        AIN1 = 1
        AIN2 = 0
        PWMA = 100% duty
    */
    GPIOA->BSRR = (1U << STBY_PIN) |
                  (1U << AIN1_PIN) |
                  (1U << (AIN2_PIN + 16));

    Motor_SetDuty(100);
}

void Motor_Stop(void)
{
    /*
        Stop:
        STBY = 1
        AIN1 = 0
        AIN2 = 0
        PWMA = HIGH

        In this mode, motor output terminals are open.
        The motor coasts down by inertia.
    */
    GPIOA->BSRR = (1U << STBY_PIN) |
                  (1U << (AIN1_PIN + 16)) |
                  (1U << (AIN2_PIN + 16));

    Motor_SetDuty(100);
}

void Motor_ShortBrake(void)
{
    /*
        Short brake:
        STBY = 1
        AIN1 = 1
        AIN2 = 1

        In this mode, motor output terminals are shorted.
        The motor stops faster.
    */
    GPIOA->BSRR = (1U << STBY_PIN) |
                  (1U << AIN1_PIN) |
                  (1U << AIN2_PIN);

    Motor_SetDuty(100);
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
