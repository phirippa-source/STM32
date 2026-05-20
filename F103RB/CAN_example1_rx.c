#include "stm32f10x.h"

volatile uint8_t rx_data = 0;

void GPIO_Init_CAN_LED(void);
void CAN_Init_Simple(void);
void CAN_Filter_AcceptAll(void);

int main(void)
{
    uint32_t id;

    GPIO_Init_CAN_LED();
    CAN_Init_Simple();
    CAN_Filter_AcceptAll();

    while (1)
    {
        // Check if FIFO0 has received message
        if ((CAN1->RF0R & CAN_RF0R_FMP0) != 0)
        {
            // Read standard ID
            id = (CAN1->sFIFOMailBox[0].RIR >> 21) & 0x7FF;

            // Read first data byte
            rx_data = CAN1->sFIFOMailBox[0].RDLR & 0xFF;

            // Release FIFO0
            CAN1->RF0R |= CAN_RF0R_RFOM0;

            // If ID is 0x123, toggle LED
            if (id == 0x123)
            {
                GPIOA->ODR ^= (1U << 5);
            }
        }
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

void CAN_Filter_AcceptAll(void)
{
    // Enter filter initialization mode
    CAN1->FMR |= CAN_FMR_FINIT;

    // Deactivate filter 0
    CAN1->FA1R &= ~(1U << 0);

    // Filter 0: mask mode
    CAN1->FM1R &= ~(1U << 0);

    // Filter 0: 32-bit scale
    CAN1->FS1R |= (1U << 0);

    // Assign filter 0 to FIFO0
    CAN1->FFA1R &= ~(1U << 0);

    // Accept all IDs
    CAN1->sFilterRegister[0].FR1 = 0x00000000;
    CAN1->sFilterRegister[0].FR2 = 0x00000000;

    // Activate filter 0
    CAN1->FA1R |= (1U << 0);

    // Exit filter initialization mode
    CAN1->FMR &= ~CAN_FMR_FINIT;
}
