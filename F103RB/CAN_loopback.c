#include "stm32f10x.h"

#define LED_PIN     5
#define CAN_ID      0x123
#define CAN_DATA    0x55

void delay_ms(uint16_t ms);
void LED_Init(void);
void CAN_Init_Loopback(void);
void CAN_Send_TestMessage(void);
uint8_t CAN_Receive_Byte(uint8_t *data);

void delay_ms(uint16_t ms)
{
    volatile uint32_t i;

    while (ms--)
    {
        for (i = 0; i < 7200; i++);
    }
}

void LED_Init(void)
{
    // Enable GPIOA clock
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;

    // PA5 output push-pull, 2 MHz
    GPIOA->CRL &= ~(0xFU << (LED_PIN * 4));
    GPIOA->CRL |=  (0x2U << (LED_PIN * 4));
}

void CAN_Init_Loopback(void)
{
    // Enable AFIO clocks
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;

    // Enable CAN clock
    RCC->APB1ENR |= RCC_APB1ENR_CAN1EN;

    // PA11 = CAN_RX, input floating
    GPIOA->CRH &= ~(0xFU << ((11 - 8) * 4));
    GPIOA->CRH |=  (0x4U << ((11 - 8) * 4));

    // PA12 = CAN_TX, alternate function push-pull, 50 MHz
    GPIOA->CRH &= ~(0xFU << ((12 - 8) * 4));
    GPIOA->CRH |=  (0xBU << ((12 - 8) * 4));

    // Exit sleep mode
    CAN1->MCR &= ~CAN_MCR_SLEEP;
    while (CAN1->MSR & CAN_MSR_SLAK);

    // Enter initialization mode
    CAN1->MCR |= CAN_MCR_INRQ;
    while (!(CAN1->MSR & CAN_MSR_INAK));

    // CAN bit timing setting
    // PCLK1 = 36 MHz
    // Prescaler = 18
    // Time quanta = 1 + 13 + 2 = 16 TQ
    // CAN bitrate = 36 MHz / 18 / 16 = 125 kbps
    //
    // Loopback mode enabled
    CAN1->BTR = 0;
    CAN1->BTR |= (17U << 0);      // BRP = 18 - 1
    CAN1->BTR |= (12U << 16);     // TS1 = 13 - 1
    CAN1->BTR |= (1U  << 20);     // TS2 = 2 - 1
    CAN1->BTR |= (0U  << 24);     // SJW = 1 - 1
    CAN1->BTR |= CAN_BTR_LBKM;    // Loopback mode

    // Filter initialization mode
    CAN1->FMR |= CAN_FMR_FINIT;

    // Filter 0 deactivation
    CAN1->FA1R &= ~(1U << 0);
    // Filter 0: 32-bit scale
    CAN1->FS1R |= (1U << 0);
    // Filter 0: identifier mask mode
    CAN1->FM1R &= ~(1U << 0);

    // Filter 0 accepts all IDs
    CAN1->sFilterRegister[0].FR1 = 0x00000000;
    CAN1->sFilterRegister[0].FR2 = 0x00000000;

    // Assign filter 0 to FIFO0
    CAN1->FFA1R &= ~(1U << 0);

    // Activate filter 0
    CAN1->FA1R |= (1U << 0);

    // Leave filter initialization mode
    CAN1->FMR &= ~CAN_FMR_FINIT;

    // Leave initialization mode
    CAN1->MCR &= ~CAN_MCR_INRQ;
    while (CAN1->MSR & CAN_MSR_INAK);
}

void CAN_Send_TestMessage(void)
{
    // Wait until transmit mailbox 0 is empty
    while (!(CAN1->TSR & CAN_TSR_TME0));

    // Standard ID, data frame
    CAN1->sTxMailBox[0].TIR = 0;

    // DLC = 1 byte
    CAN1->sTxMailBox[0].TDTR = 1;

    // Data byte
    CAN1->sTxMailBox[0].TDLR = CAN_DATA;
    CAN1->sTxMailBox[0].TDHR = 0;

    // Standard ID is stored from bit 21
    CAN1->sTxMailBox[0].TIR |= (CAN_ID << 21);

    // Request transmission
    CAN1->sTxMailBox[0].TIR |= CAN_TI0R_TXRQ;
}

uint8_t CAN_Receive_Byte(uint8_t *data)
{
    // Check FIFO0 message pending
    if ((CAN1->RF0R & CAN_RF0R_FMP0) == 0)
    {
        return 0;
    }

    // Read first data byte
    *data = (uint8_t)(CAN1->sFIFOMailBox[0].RDLR & 0xFF);

    // Release FIFO0 output mailbox
    CAN1->RF0R |= CAN_RF0R_RFOM0;

    return 1;
}

int main(void)
{
    uint8_t rx_data;

    LED_Init();
    CAN_Init_Loopback();

    while (1)
    {
        CAN_Send_TestMessage();

        delay_ms(100);

        if (CAN_Receive_Byte(&rx_data))
        {
            if (rx_data == CAN_DATA)
            {
                // Toggle PA5 LED when CAN message is received correctly
                GPIOA->ODR ^= (1U << LED_PIN);
            }
        }

        delay_ms(900);
    }
}
