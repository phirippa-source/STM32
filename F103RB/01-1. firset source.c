#include "stm32f10x.h"
void delay_ms(uint16_t t);

int main()
{
    RCC->APB2ENR |= 0x7C; //Enable the clocks for GPIO ports 
    GPIOA->CRL = 0x44344444; //PA5 as outputs
    while(1)
    {       
        GPIOA->ODR |= (1<<5) ; //Set PA5
        delay_ms(1000); //wait 1000ms
        GPIOA->ODR &= ~(1<<5); //Clear PA5
        delay_ms(1000); //wait 1000ms
    }
}
void delay_ms(uint16_t t)
{
    volatile unsigned long l = 0;
    for(uint16_t i = 0; i < t; i++)
        for(l = 0; l < 3271; l++);
}
