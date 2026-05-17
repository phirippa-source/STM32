#include "stm32f10x.h"

#define AIN1_PIN    0      // PA0 -> AIN1
#define AIN2_PIN    1      // PA1 -> AIN2
#define PWMA_PIN    6      // PA6 -> TIM3_CH1 -> PWMA
#define STBY_PIN    7      // PA7 -> STBY

#define PWM_TOP     1000   // PWM Duty ?? ???

void Motor_Init(void);
void Motor_SetDuty(uint8_t duty);
void delay_ms(uint32_t ms);

int main(void) {
    Motor_Init();

    while (1) {
        Motor_SetDuty(100);   // Duty 100%
        delay_ms(2000);

        Motor_SetDuty(80);    // Duty 80%
        delay_ms(2000);
    
				Motor_SetDuty(50);    // Duty 50%
        delay_ms(2000);
		}
}

void Motor_Init(void) {
    // GPIOA, AFIO, TIM3 clock enable
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_AFIOEN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
    GPIOA->CRL &= ~(
        (0xFU << (AIN1_PIN * 4)) | (0xFU << (AIN2_PIN * 4)) | (0xFU << (PWMA_PIN * 4)) | (0xFU << (STBY_PIN * 4))
    );

    GPIOA->CRL |= (
        (0x3U << (AIN1_PIN * 4)) |   // PA0 Output Push-Pull
        (0x3U << (AIN2_PIN * 4)) |   // PA1 Output Push-Pull
        (0xBU << (PWMA_PIN * 4)) |   // PA6 Alternate Function Output Push-Pull
        (0x3U << (STBY_PIN * 4))     // PA7 Output Push-Pull
    );

    
    //    STBY = 1, AIN1 = 1, AIN2 = 0   
    GPIOA->BSRR = (1U << STBY_PIN) | (1U << AIN1_PIN) | (1U << (AIN2_PIN + 16));

    
    //    TIM3_CH1 PWM setup
    //    TIM3 Clock = 72MHz, PSC = 72 - 1  -> 1MHz, ARR = 1000 - 1 -> PWM frequency 1kHz
    TIM3->PSC = 72 - 1;
    TIM3->ARR = PWM_TOP - 1;
    TIM3->CCR1 = 0;

    // PWM Mode 1, CH1 
    TIM3->CCMR1 &= ~(0x7U << 4);
    TIM3->CCMR1 |=  (0x6U << 4);    // OC1M = 110, PWM Mode 1
    TIM3->CCMR1 |=  (1U << 3);      // OC1PE = 1

    TIM3->CCER |=  (1U << 0);       // CC1E = 1, CH1 enable
    TIM3->CR1  |=  (1U << 7);       // ARPE = 1

    TIM3->EGR  |=  (1U << 0);       // UG = 1
    TIM3->CR1  |=  (1U << 0);       // CEN = 1, timer start
}

void Motor_SetDuty(uint8_t duty) {
    if (duty > 100) {
        duty = 100;
    }
    TIM3->CCR1 = (PWM_TOP * duty) / 100;
} 

void delay_ms(uint32_t ms) {
    SysTick->LOAD = 72000 - 1;   // 72MHz --> 1ms
    SysTick->VAL  = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |
                    SysTick_CTRL_ENABLE_Msk;

    while (ms--)
    {
        while ((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) == 0);
    }

    SysTick->CTRL = 0;
}
