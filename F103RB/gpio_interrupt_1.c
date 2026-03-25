#include <stm32f10x.h>
void EXTI4_IRQHandler(void) ;

int main(void)
{
    RCC->APB2ENR |= (0xFC | 1); /* Enable clocks for GPIO ports and AFIO */
	  
    GPIOA->CRL = 0x44344444; /* PA5 output, PA4 input */

    AFIO->EXTICR[1] = 0<<0; /* EXTI4 = 0 (PA4 for EXTI4) */
    EXTI->RTSR = (1<<4);    /* interrupt on rising edge */
    EXTI->IMR  = (1<<4);    /* enable interrupt EXTI4 */
    EXTI->PR   = (1<<4);    /* clear pending flag */
    NVIC_EnableIRQ(EXTI4_IRQn); /* enable the EXTI4 IRQ interrupt */
		  
    while(1);
}

void EXTI4_IRQHandler(void) /* interrupt handler for EXTI4 */
{
    EXTI->PR = (1<<4);      /* clear the Pending flag */
    GPIOA->ODR ^= (1<<5);   /* toggle PA5 */
}
