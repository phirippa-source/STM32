#include "stm32f10x.h"

void delay_ms(uint16_t ms);
void GPIO_Init_CAN_LED(void);
void CAN_Init_Simple(void);
void CAN_Send_Byte(uint8_t data);

int main(void)
{
    uint8_t count = 0;

    GPIO_Init_CAN_LED();
    CAN_Init_Simple();

    while (1)
    {
        CAN_Send_Byte(count);

        // Toggle LED after sending
        GPIOA->ODR ^= (1U << 5);

        count++;

        delay_ms(1000);
    }
}

void GPIO_Init_CAN_LED(void)
{
    // Enable GPIOA and AFIO clock
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;

    // PA5: Output push-pull, 2 MHz
    GPIOA->CRL &= ~(0xFU << (5 * 4));
    GPIOA->CRL |=  (0x2U << (5 * 4));

    // PA11: CAN_RX, input floating
    GPIOA->CRH &= ~(0xFU << ((11 - 8) * 4));
    GPIOA->CRH |=  (0x4U << ((11 - 8) * 4));

    // PA12: CAN_TX, alternate function push-pull, 50 MHz
    GPIOA->CRH &= ~(0xFU << ((12 - 8) * 4));
    GPIOA->CRH |=  (0xBU << ((12 - 8) * 4));
}

void CAN_Init_Simple(void)
{
    // Enable CAN1 clock
    RCC->APB1ENR |= RCC_APB1ENR_CAN1EN;

    // Request CAN initialization mode
    CAN1->MCR |= CAN_MCR_INRQ;

    // Wait until CAN enters initialization mode
    while ((CAN1->MSR & CAN_MSR_INAK) == 0);

    // Exit sleep mode
    CAN1->MCR &= ~CAN_MCR_SLEEP;

    // CAN bit timing
    // APB1 clock = 36 MHz
    // CAN bitrate = 36 MHz / [4 * (1 + 12 + 5)] = 500 kbps
    CAN1->BTR = 0;
    CAN1->BTR |= (3U << 0);      // Prescaler = 4
    CAN1->BTR |= (11U << 16);    // BS1 = 12 tq
    CAN1->BTR |= (4U << 20);     // BS2 = 5 tq
    CAN1->BTR |= (0U << 24);     // SJW = 1 tq

    // Leave initialization mode
    CAN1->MCR &= ~CAN_MCR_INRQ;

    // Wait until CAN enters normal mode
    while (CAN1->MSR & CAN_MSR_INAK);
}

void CAN_Send_Byte(uint8_t data)
{
    // Wait until transmit mailbox 0 is empty
    while ((CAN1->TSR & CAN_TSR_TME0) == 0);

    // Standard ID = 0x123
    // Data frame
    CAN1->sTxMailBox[0].TIR = (0x123U << 21);

    // Data length = 1 byte
    CAN1->sTxMailBox[0].TDTR = 1;

    // Put data byte into first data byte
    CAN1->sTxMailBox[0].TDLR = data;

    // Request transmission
    CAN1->sTxMailBox[0].TIR |= CAN_TI0R_TXRQ;
}

void delay_ms(uint16_t ms)
{
    volatile uint32_t i;

    while (ms--)
    {
        for (i = 0; i < 8000; i++);
    }
}
