#include "stm32f10x.h"

/* UART baud rates */
#define PC_BAUD        115200U
#define HC05_BAUD       38400U

/* Peripheral clocks */
#define USART2_PCLK_HZ 36000000U   // USART2: APB1 clock
#define USART1_PCLK_HZ 72000000U   // USART1: APB2 clock

/* UART aliases */
#define PC_UART        USART2
#define HC05_UART      USART1

/* Function prototypes */
static void PC_UART_Init(void);
static void HC05_UART_Init(void);

static uint16_t USART_BRR(uint32_t pclk_hz, uint32_t baud);
static uint8_t  USART_RxReady(USART_TypeDef *USARTx);
static uint8_t  USART_ReadChar(USART_TypeDef *USARTx);
static void     USART_WriteChar(USART_TypeDef *USARTx, uint8_t ch);
static void     USART_WriteString(USART_TypeDef *USARTx, const char *str);

int main(void) {
	uint8_t ch;

	PC_UART_Init();       // USART2: PC terminal
	HC05_UART_Init();     // USART1: HC-05

	USART_WriteString(PC_UART, "HC-05 AT command bridge ready\r\n");
	USART_WriteString(PC_UART, "Type AT command and press Enter\r\n\r\n");

	while (1) {
		// PC -> Nucleo USART2 -> USART1 -> HC-05
		if (USART_RxReady(PC_UART)) {
				ch = USART_ReadChar(PC_UART);

				USART_WriteChar(HC05_UART, ch);  // send to HC-05
				USART_WriteChar(PC_UART, ch);    // echo to PC terminal
		}
		
		// HC-05 -> USART1 -> USART2 -> PC
		if (USART_RxReady(HC05_UART)) {
				ch = USART_ReadChar(HC05_UART);

				USART_WriteChar(PC_UART, ch);    // send to PC terminal
		}
	}
}

/*
 * USART1 for HC-05
 *
 * PA9  = USART1_TX -> HC-05 RXD
 * PA10 = USART1_RX <- HC-05 TXD
 */
static void HC05_UART_Init(void) {
	/* Enable GPIOA, AFIO, USART1 clocks */
	RCC->APB2ENR |= RCC_APB2ENR_IOPAEN |
									RCC_APB2ENR_AFIOEN |
									RCC_APB2ENR_USART1EN;

	/*
	 * PA9: USART1_TX
	 * Alternate Function Push-Pull, 50 MHz
	 * MODE9 = 11, CNF9 = 10 -> 0xB
	 */
	GPIOA->CRH &= ~(0xFU << ((9U - 8U) * 4U));
	GPIOA->CRH |=  (0xBU << ((9U - 8U) * 4U));

	/*
	 * PA10: USART1_RX
	 * Input floating
	 * MODE10 = 00, CNF10 = 01 -> 0x4
	 */
	GPIOA->CRH &= ~(0xFU << ((10U - 8U) * 4U));
	GPIOA->CRH |=  (0x4U << ((10U - 8U) * 4U));

	/* USART1: 38400, 8N1 */
	USART1->CR1 = 0x0000;
	USART1->CR2 = 0x0000;
	USART1->CR3 = 0x0000;

	USART1->BRR = USART_BRR(USART1_PCLK_HZ, HC05_BAUD);

	USART1->CR1 |= USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
}

/*
 * USART2 for PC terminal through ST-LINK Virtual COM Port
 *
 * PA2 = USART2_TX
 * PA3 = USART2_RX
 */
static void PC_UART_Init(void) {
	/* Enable GPIOA and AFIO clocks */
	RCC->APB2ENR |= RCC_APB2ENR_IOPAEN |
									RCC_APB2ENR_AFIOEN;

	/* Enable USART2 clock */
	RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

	/*
	 * PA2: USART2_TX
	 * Alternate Function Push-Pull, 50 MHz
	 * MODE2 = 11, CNF2 = 10 -> 0xB
	 */
	GPIOA->CRL &= ~(0xFU << (2U * 4U));
	GPIOA->CRL |=  (0xBU << (2U * 4U));

	/*
	 * PA3: USART2_RX
	 * Input floating
	 * MODE3 = 00, CNF3 = 01 -> 0x4
	 */
	GPIOA->CRL &= ~(0xFU << (3U * 4U));
	GPIOA->CRL |=  (0x4U << (3U * 4U));

	/* USART2: 115200, 8N1 */
	USART2->CR1 = 0x0000;
	USART2->CR2 = 0x0000;
	USART2->CR3 = 0x0000;

	USART2->BRR = USART_BRR(USART2_PCLK_HZ, PC_BAUD);

	USART2->CR1 |= USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
}

/*
 * BRR calculation
 *
 * STM32F103 USART BRR can be calculated approximately as:
 *
 * BRR = USART clock / baud rate
 *
 * This function adds baud/2 for rounding.
 */
static uint16_t USART_BRR(uint32_t pclk_hz, uint32_t baud) {
	return (uint16_t)((pclk_hz + (baud / 2U)) / baud);
}

static uint8_t USART_RxReady(USART_TypeDef *USARTx) {
	return (USARTx->SR & USART_SR_RXNE) ? 1U : 0U;
}

static uint8_t USART_ReadChar(USART_TypeDef *USARTx) {
	return (uint8_t)(USARTx->DR & 0xFFU);
}

static void USART_WriteChar(USART_TypeDef *USARTx, uint8_t ch) {
	while (!(USARTx->SR & USART_SR_TXE)) {
			/* wait */
	}

	USARTx->DR = ch;
}

static void USART_WriteString(USART_TypeDef *USARTx, const char *str) {
	while (*str) {
			USART_WriteChar(USARTx, (uint8_t)*str);
			str++;
	}
}
