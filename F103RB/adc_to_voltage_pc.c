#include "stm32f10x.h"

#define PC_BAUD        115200U
#define PCLK1_HZ       36000000U

#define ADC_VREF       3.30f      // Use measured VDDA value if possible
                                  // Example: 3.27f, 3.29f, 3.30f

void delay_ms(uint16_t t);
void USART2_Init_PC(void);
void USART2_SendChar(char c);
void USART2_SendString(const char *s);
void USART2_SendUInt32(uint32_t value);
void USART2_SendFloat(float value, uint8_t digits);

void delay_ms(uint16_t t) {
    volatile unsigned long l = 0;
    for(uint16_t i = 0; i < t; i++)
        for(l = 0; l < 3271; l++);
}

void USART2_Init_PC(void) {
    /*
     * USART2 is used to send data from the Nucleo-F103RB board to the PC.
     *
     * PA2 = USART2_TX
     * PA3 = USART2_RX
     *
     * On the Nucleo-F103RB board, USART2 is connected to the ST-LINK
     * Virtual COM Port, so data can be observed on the PC terminal.
     */

    // Enable GPIOA, AFIO, and USART2 clocks
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    // Use default USART2 pins: PA2 = TX, PA3 = RX
    AFIO->MAPR &= ~AFIO_MAPR_USART2_REMAP;

    /*
     * Configure PA2 as USART2_TX.
     *
     * MODE2[1:0] = 11 : Output mode, max speed 50 MHz
     * CNF2[1:0]  = 10 : Alternate function output push-pull
     *
     * 1011b = 0xB
     */
    GPIOA->CRL &= ~(0xFU << (4U * 2U));
    GPIOA->CRL |=  (0xBU << (4U * 2U));

    /*
     * Configure PA3 as USART2_RX.
     *
     * MODE3[1:0] = 00 : Input mode
     * CNF3[1:0]  = 01 : Floating input
     *
     * 0100b = 0x4
     */
    GPIOA->CRL &= ~(0xFU << (4U * 3U));
    GPIOA->CRL |=  (0x4U << (4U * 3U));

    /*
     * Baud rate setting.
     *
     * In this project:
     *   PCLK1 = 36 MHz
     *   Baud rate = 115200 bps
     *
     * BRR = 36,000,000 / 115,200 ˜ 313 = 0x0139
     */
    USART2->BRR = 0x0139;

    // Enable transmitter, receiver, and USART2
    USART2->CR1 |= USART_CR1_TE;
    USART2->CR1 |= USART_CR1_RE;
    USART2->CR1 |= USART_CR1_UE;
}

void USART2_SendChar(char c) {
    /*
     * Wait until transmit data register is empty.
     * TXE = 1 means USART2->DR is ready for new data.
     */
    while ((USART2->SR & USART_SR_TXE) == 0) {
        ;
    }

    // Write one character to the data register
    USART2->DR = c;
}

void USART2_SendString(const char *s) {
    /*
     * Send a null-terminated string.
     */
    while (*s) {
        USART2_SendChar(*s++);
    }
}

void USART2_SendUInt32(uint32_t value) {
    /*
     * Convert an unsigned integer value to decimal ASCII characters
     * and send it through USART2.
     */

    char buf[11];    // uint32_t max value: 4294967295, 10 digits
    int i = 0;

    if (value == 0) {
        USART2_SendChar('0');
        return;
    }

    while (value > 0) {
        buf[i++] = (value % 10) + '0';
        value /= 10;
    }

    while (i > 0) {
        USART2_SendChar(buf[--i]);
    }
}

void USART2_SendFloat(float value, uint8_t digits) {
    /*
     * Send a float value as ASCII text.
     *
     * Example:
     *   value = 0.75123, digits = 3
     *   output = "0.751"
     *
     * This function does not use printf().
     */

    uint32_t int_part;
    uint32_t frac_part;
    uint32_t scale = 1;

    if (digits > 6) {
        digits = 6;
    }

    // Handle negative value
    if (value < 0.0f) {
        USART2_SendChar('-');
        value = -value;
    }

    // Make scale value
    // digits = 3 -> scale = 1000
    for (uint8_t i = 0; i < digits; i++) {
        scale *= 10;
    }

    // Separate integer part and fractional part
    int_part = (uint32_t)value;
    frac_part = (uint32_t)((value - (float)int_part) * (float)scale + 0.5f);

    // Handle rounding carry
    // Example: 1.999 with 2 digits -> 2.00
    if (frac_part >= scale) {
        int_part++;
        frac_part -= scale;
    }

    // Send integer part
    USART2_SendUInt32(int_part);

    // Send decimal point and fractional part
    if (digits > 0) {
        USART2_SendChar('.');

        // Print leading zeros
        for (uint32_t div = scale / 10; div > 0; div /= 10) {
            USART2_SendChar((frac_part / div) % 10 + '0');
        }
    }
}

int main(void) {
    volatile uint16_t adc_value = 0;
    float voltage = 0.0f;

    /*
     * Enable ADC1 and GPIOA clocks.
     *
     * ADC1 is connected to APB2.
     * GPIOA is also connected to APB2.
     */
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;

    /*
     * Initialize USART2 for PC communication.
     */
    USART2_Init_PC();

    /*
     * Configure PA1 as analog input.
     *
     * PA1 = ADC1_IN1
     *
     * MODE1[1:0] = 00 : Input mode
     * CNF1[1:0]  = 00 : Analog input mode
     */
    GPIOA->CRL &= ~(0xFU << (4U * 1U));

    /*
     * ADC prescaler is already configured in system_stm32f10x.c.
     *
     * Example:
     *   PCLK2  = 72 MHz
     *   ADCPRE = /6
     *   ADCCLK = 12 MHz
     *
     * If ADC prescaler is not configured elsewhere, enable the following code.
     */
    /*
    RCC->CFGR &= ~RCC_CFGR_ADCPRE;
    RCC->CFGR |=  RCC_CFGR_ADCPRE_DIV6;
    */

    /*
     * ADC result alignment.
     *
     * ALIGN = 0 : Right alignment
     * ADC1->DR[11:0] contains the 12-bit ADC result.
     */
    ADC1->CR2 &= ~ADC_CR2_ALIGN;

    /*
     * Turn on ADC1.
     *
     * In STM32F1:
     *   The first write of ADON = 1 turns on the ADC.
     *   After ADC is already on, writing ADON = 1 again starts conversion.
     */
    ADC1->CR2 |= ADC_CR2_ADON;
    delay_ms(1);

    /*
     * Reset ADC calibration register.
     */
    ADC1->CR2 |= ADC_CR2_RSTCAL;
    while (ADC1->CR2 & ADC_CR2_RSTCAL) {
        ;
    }

    /*
     * Start ADC calibration.
     */
    ADC1->CR2 |= ADC_CR2_CAL;
    while (ADC1->CR2 & ADC_CR2_CAL) {
        ;
    }

    /*
     * Set sampling time for channel 1.
     *
     * PA1 = ADC1_IN1
     * SMP1[2:0] is located at ADC1_SMPR2[5:3].
     *
     * SMP1 = b111 means sampling time = 239.5 ADC cycles.
     */
    ADC1->SMPR2 &= ~(7U << 3);
    ADC1->SMPR2 |=  (7U << 3);

    /*
     * Set regular sequence length.
     *
     * L[3:0] = 0 means 1 conversion.
     */
    ADC1->SQR1 &= ~(0xFU << 20);

    /*
     * Set first conversion in regular sequence.
     *
     * SQ1 = 1 means:
     *   First conversion reads ADC channel 1.
     *
     * ADC channel 1 = ADC1_IN1 = PA1
     */
    ADC1->SQR3 &= ~(0x1FU << 0);
    ADC1->SQR3 |=  (1U << 0);

    USART2_SendString("ADC voltage test start\r\n");

    while(1) {
        /*
         * Start ADC conversion.
         *
         * In STM32F1, writing ADON = 1 again starts regular conversion
         * when ADC is already turned on.
         */
        ADC1->CR2 |= ADC_CR2_ADON;

        /*
         * Wait until conversion is complete.
         */
        while ((ADC1->SR & ADC_SR_EOC) == 0) {
            ;
        }

        /*
         * Read 12-bit ADC result.
         *
         * Right-aligned result:
         *   ADC1->DR[11:0] = ADC result
         */
        adc_value = ADC1->DR & 0x0FFF;

        /*
         * Convert ADC integer value to voltage.
         *
         * ADC range:
         *   0    -> 0V
         *   4095 -> ADC_VREF
         *
         * Voltage = adc_value / 4095 * ADC_VREF
         */
        voltage = ((float)adc_value * ADC_VREF) / 4095.0f;

        /*
         * Send ADC integer value and voltage value to PC.
         */
        USART2_SendString("ADC = ");
        USART2_SendUInt32(adc_value);

        USART2_SendString(", Voltage = ");
        USART2_SendFloat(voltage, 3);
        USART2_SendString(" V\r\n");

        delay_ms(200);
    }
}
