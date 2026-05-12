#include "stm32f10x.h"

#define PC_BAUD        115200U
#define HC05_BAUD      115200U   // HC-05 ?? ?? ??? ?? ?

#define USART2_PCLK_HZ 36000000U
#define USART1_PCLK_HZ 72000000U

#define PC_UART        USART2
#define HC05_UART      USART1

static void PC_UART_Init(void);
static void HC05_UART_Init(void);
static uint16_t USART_BRR(uint32_t pclk_hz, uint32_t baud);
static void USART_WriteChar(USART_TypeDef *USARTx, uint8_t ch);

int main(void) {
    uint8_t ch;

    PC_UART_Init();       // USART2: ST-LINK Virtual COM Port, COM25
    HC05_UART_Init();     // USART1: HC-05

    while (1) {
        // PC USB terminal -> Nucleo USART2 -> HC-05
        if (PC_UART->SR & USART_SR_RXNE) {
            ch = (uint8_t)(PC_UART->DR & 0xFF);
            USART_WriteChar(HC05_UART, ch);
        }

        // HC-05 -> Nucleo USART1 -> PC USB terminal
        if (HC05_UART->SR & USART_SR_RXNE) {
            ch = (uint8_t)(HC05_UART->DR & 0xFF);
            USART_WriteChar(PC_UART, ch);
        }
    }
}

static void HC05_UART_Init(void) {
    // GPIOA, AFIO, USART1 clock enable
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN |
                    RCC_APB2ENR_AFIOEN |
                    RCC_APB2ENR_USART1EN;

    // PA9: USART1_TX, Alternate Function Push-Pull, 50 MHz
    GPIOA->CRH &= ~(0xFU << ((9U - 8U) * 4U));
    GPIOA->CRH |=  (0xBU << ((9U - 8U) * 4U));

    // PA10: USART1_RX, Input floating
    GPIOA->CRH &= ~(0xFU << ((10U - 8U) * 4U));
    GPIOA->CRH |=  (0x4U << ((10U - 8U) * 4U));

    // USART1: HC-05 UART
    USART1->CR1 = 0x0000;
    USART1->CR2 = 0x0000;
    USART1->CR3 = 0x0000;

    USART1->BRR = USART_BRR(USART1_PCLK_HZ, HC05_BAUD);

    USART1->CR1 |= USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
}

static void PC_UART_Init(void) {
    // GPIOA, AFIO clock enable
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN |
                    RCC_APB2ENR_AFIOEN;

    // USART2 clock enable
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    // PA2: USART2_TX, Alternate Function Push-Pull, 50 MHz
    GPIOA->CRL &= ~(0xFU << (2U * 4U));
    GPIOA->CRL |=  (0xBU << (2U * 4U));

    // PA3: USART2_RX, Input floating
    GPIOA->CRL &= ~(0xFU << (3U * 4U));
    GPIOA->CRL |=  (0x4U << (3U * 4U));

    // USART2: PC terminal
    USART2->CR1 = 0x0000;
    USART2->CR2 = 0x0000;
    USART2->CR3 = 0x0000;

    USART2->BRR = USART_BRR(USART2_PCLK_HZ, PC_BAUD);

    USART2->CR1 |= USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
}

static uint16_t USART_BRR(uint32_t pclk_hz, uint32_t baud) {
    return (uint16_t)((pclk_hz + (baud / 2U)) / baud);
}

static void USART_WriteChar(USART_TypeDef *USARTx, uint8_t ch) {
    while (!(USARTx->SR & USART_SR_TXE)) {
        // wait
    }

    USARTx->DR = ch;
}
