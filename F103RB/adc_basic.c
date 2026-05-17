#include "stm32f10x.h"

void delay_ms(uint16_t t) {
	volatile unsigned long l = 0;
	for(uint16_t i = 0; i < t; i++)
			for(l = 0; l < 3271; l++);
}

int main(void) {
	volatile uint16_t adc_value = 0;
	RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
	RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
	GPIOA->CRL	&= ~(0xF << (4U * 1U));	// PA1 : Input, Analog
	
	// ADC prescaler is already configured in system_stm32f10x.c
  // Example: PCLK2 = 72MHz, ADCPRE = /6, ADCCLK = 12MHz
	// RCC->CFGR &= ~RCC_CFGR_ADCPRE;
  // RCC->CFGR |=  RCC_CFGR_ADCPRE_DIV6;    // 72MHz / 6 = 12MHz
	
	ADC1->CR2		&= ~ADC_CR2_ALIGN;					// ALIGN = 0, right align
	ADC1->CR2		|= ADC_CR2_ADON;						// ADON = 1
	delay_ms(1);
	
	
  // cal
	ADC1->CR2 |= ADC_CR2_RSTCAL;
  while (ADC1->CR2 & ADC_CR2_RSTCAL) {;}
	ADC1->CR2		|= ADC_CR2_CAL;
	while( ADC1->CR2 & ADC_CR2_CAL) {;}
	
	// sampling time for channel 1, SMP1[2:0] = ADC1_SMPR2[5:3]
  ADC1->SMPR2 &= ~(7U << 3);     // Clear SMP1[2:0]
  ADC1->SMPR2 |=  (7U << 3);     // SMP1 = b111, 239.5 ADC cycles		
		
	// sequence setup
	ADC1->SQR1 	&= ~(0xFU << 20);					// L = 0, sequence length = 1
	ADC1->SQR3	&= ~(0x1F);								// clear SQ1[4:0]
	ADC1->SQR3	|= 0x1;										// sequence = {ADC1_IN1}, ADC1_IN1 = PA1

		
	while(1) {
		// conversion start
		ADC1->CR2 |= ADC_CR2_ADON;
		while( ( ADC1->SR & ADC_SR_EOC ) == 0);
		adc_value = (ADC1->DR) & 0xFFF;
		delay_ms(1000);
	}
}

