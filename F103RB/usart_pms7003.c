#include "stm32f10x.h"

/*
    Clock assumptions

    USART1 is connected to APB2.
    USART2 is connected to APB1.

    In a typical STM32F103RB 72 MHz system:
    - APB2 clock = 72 MHz
    - APB1 clock = 36 MHz

    If your clock configuration is different,
    you must change these values.
*/
#define PCLK2_HZ       72000000U
#define PCLK1_HZ       36000000U

/*
    PMS7003 default UART setting:
    - Baud rate : 9600 bps
    - Data bits : 8
    - Parity    : None
    - Stop bit  : 1
*/
#define PMS_BAUD       9600U

/*
    PC serial monitor baud rate.

    USART2 is connected to the ST-LINK Virtual COM Port
    on the Nucleo-F103RB board.
*/
#define PC_BAUD        115200U

/*
    Global variables for storing measured PM values.

    These variables can also be monitored in the uVision Watch window.
*/
uint16_t pm1_0 = 0;
uint16_t pm2_5 = 0;
uint16_t pm10  = 0;


/* Function prototypes */
void USART1_Init(void);
void USART2_Init(void);

uint8_t USART1_ReadByte(void);

void USART2_SendChar(char c);
void USART2_SendString(char *s);
void USART2_SendNumber(uint16_t num);

uint16_t GetWord(uint8_t *buf, uint8_t index);
uint8_t PMS7003_Read(uint8_t *buf);


int main(void)
{
    /*
        PMS7003 sends one data frame of 32 bytes.

        Frame structure:
        byte 0  : 0x42
        byte 1  : 0x4D
        byte 2  : frame length high byte
        byte 3  : frame length low byte
        ...
        byte 30 : checksum high byte
        byte 31 : checksum low byte
    */
    uint8_t buf[32];

    /*
        USART1 receives data from PMS7003.
        USART2 sends the parsed result to the PC.
    */
    USART1_Init();
    USART2_Init();

    /*
        Send a start message to the PC.
        You can see this message in Tera Term or another serial monitor.
    */
    USART2_SendString("PMS7003 start\r\n");

    while (1)
    {
        /*
            Try to read one valid PMS7003 frame.

            PMS7003_Read() returns:
            - 1 if a valid frame is received
            - 0 if the frame is invalid
        */
        if (PMS7003_Read(buf))
        {
            /*
                Extract PM values from the received frame.

                The values below are atmospheric environment values.

                byte 10, 11 : PM1.0
                byte 12, 13 : PM2.5
                byte 14, 15 : PM10

                Each value is stored as two bytes:
                high byte first, low byte second.
            */
            pm1_0 = GetWord(buf, 10);
            pm2_5 = GetWord(buf, 12);
            pm10  = GetWord(buf, 14);

            /*
                Send measured values to the PC through USART2.

                Example output:
                PM1.0=5, PM2.5=8, PM10=12
            */
            USART2_SendString("PM1.0=");
            USART2_SendNumber(pm1_0);

            USART2_SendString(", PM2.5=");
            USART2_SendNumber(pm2_5);

            USART2_SendString(", PM10=");
            USART2_SendNumber(pm10);

            USART2_SendString("\r\n");
        }
    }
}


void USART1_Init(void)
{
    /*
        USART1 is used to receive data from the PMS7003 sensor.

        PMS7003 TXD should be connected to:
        PA10 = USART1_RX

        In this example, USART1_TX is not used.
    */

    /*
        Enable GPIOA clock.

        PA10 belongs to GPIOA, so GPIOA clock must be enabled
        before configuring PA10.
    */
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;

    /*
        Enable USART1 clock.

        USART1 is connected to APB2.
    */
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;

    /*
        Configure PA10 as input floating.

        PA10 is located in GPIOA_CRH because PA10 is pin number 8 or higher.

        For each GPIO pin, 4 bits are used in CRL or CRH.

        PA10 position in CRH:
        (10 - 8) * 4 = 8

        Input floating mode:
        CNF = 01
        MODE = 00
        4-bit value = 0100b = 0x4
    */
    GPIOA->CRH &= ~(0xFU << 8);
    GPIOA->CRH |=  (0x4U << 8);

    /*
        Set USART1 baud rate to 9600 bps.

        For USART1:
        clock source = PCLK2 = 72 MHz

        BRR = 72,000,000 / 9,600 = 7,500

        This simple calculation works well here because
        the result is an integer.
    */
    USART1->BRR = PCLK2_HZ / PMS_BAUD;

    /*
        Enable USART1 receiver and USART1 module.

        RE = Receiver enable
        UE = USART enable
    */
    USART1->CR1 = USART_CR1_RE | USART_CR1_UE;
}


void USART2_Init(void)
{
    /*
        USART2 is used to send data to the PC.

        On the Nucleo-F103RB board:
        PA2 = USART2_TX

        PA2 is connected to the ST-LINK Virtual COM Port,
        so the PC can receive serial data through USB.
    */

    /*
        Enable GPIOA clock.

        PA2 belongs to GPIOA.
    */
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;

    /*
        Enable USART2 clock.

        USART2 is connected to APB1.
    */
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    /*
        Configure PA2 as alternate function push-pull output.

        PA2 is located in GPIOA_CRL because PA2 is pin number 0 to 7.

        PA2 position in CRL:
        2 * 4 = 8

        Alternate function push-pull output, 50 MHz:
        CNF = 10
        MODE = 11
        4-bit value = 1011b = 0xB
    */
    GPIOA->CRL &= ~(0xFU << 8);
    GPIOA->CRL |=  (0xBU << 8);

    /*
        Set USART2 baud rate to 115200 bps.

        For USART2:
        clock source = PCLK1 = 36 MHz

        36,000,000 / 115,200 = 312.5

        The nearest BRR value is approximately 0x0138 or 0x0139.
        0x0138 is commonly acceptable for 115200 bps.
    */
    USART2->BRR = 0x0138;

    /*
        Enable USART2 transmitter and USART2 module.

        TE = Transmitter enable
        UE = USART enable
    */
    USART2->CR1 = USART_CR1_TE | USART_CR1_UE;
}


uint8_t USART1_ReadByte(void)
{
    /*
        Wait until received data is available.

        RXNE means:
        Receive data register not empty.

        When RXNE becomes 1,
        one byte has been received and can be read from USART1->DR.
    */
    while (!(USART1->SR & USART_SR_RXNE));

    /*
        Reading USART1->DR clears the RXNE flag automatically.
    */
    return (uint8_t)USART1->DR;
}


void USART2_SendChar(char c)
{
    /*
        Wait until transmit data register is empty.

        TXE means:
        Transmit data register empty.

        When TXE becomes 1,
        we can write the next byte to USART2->DR.
    */
    while (!(USART2->SR & USART_SR_TXE));

    /*
        Write one character to the transmit data register.
        USART2 will send this character to the PC.
    */
    USART2->DR = c;
}


void USART2_SendString(char *s)
{
    /*
        Send characters one by one until the null character is found.

        A C string ends with '\0'.
    */
    while (*s)
    {
        USART2_SendChar(*s++);
    }
}


void USART2_SendNumber(uint16_t num)
{
    /*
        Convert an unsigned integer value to decimal characters
        and send them through USART2.

        Example:
        num = 123

        The digits are extracted in reverse order:
        3, 2, 1

        Then they are sent in correct order:
        1, 2, 3
    */

    char buf[6];
    int i = 0;

    /*
        Special case for zero.

        Without this block, num = 0 would send nothing.
    */
    if (num == 0)
    {
        USART2_SendChar('0');
        return;
    }

    /*
        Extract decimal digits from the number.

        The largest uint16_t value is 65535,
        so 5 digits are enough.
    */
    while (num > 0)
    {
        buf[i++] = (num % 10) + '0';
        num /= 10;
    }

    /*
        Send digits in reverse order.

        This restores the correct decimal number order.
    */
    while (i > 0)
    {
        USART2_SendChar(buf[--i]);
    }
}


uint16_t GetWord(uint8_t *buf, uint8_t index)
{
    /*
        PMS7003 stores 16-bit values in big-endian format.

        That means:
        first byte  = high byte
        second byte = low byte

        Example:
        buf[index]     = 0x01
        buf[index + 1] = 0xF4

        result = 0x01F4 = 500
    */
    return ((uint16_t)buf[index] << 8) | buf[index + 1];
}


uint8_t PMS7003_Read(uint8_t *buf)
{
    uint8_t i;
    uint16_t sum = 0;
    uint16_t checksum;

    /*
        A valid PMS7003 frame starts with two fixed bytes:

        byte 0 = 0x42
        byte 1 = 0x4D

        First, keep reading bytes until 0x42 is found.
        This helps synchronize with the beginning of a frame.
    */
    do
    {
        buf[0] = USART1_ReadByte();
    } while (buf[0] != 0x42);

    /*
        After 0x42, the next byte must be 0x4D.
        If it is not 0x4D, this is not a valid frame.
    */
    buf[1] = USART1_ReadByte();

    if (buf[1] != 0x4D)
        return 0;

    /*
        Read the remaining 30 bytes.

        We already have:
        buf[0] and buf[1]

        PMS7003 frame size is 32 bytes,
        so we read buf[2] through buf[31].
    */
    for (i = 2; i < 32; i++)
    {
        buf[i] = USART1_ReadByte();
    }

    /*
        Check frame length.

        PMS7003 normally sends frame length = 28.
        This value is stored in buf[2] and buf[3].

        buf[2] = high byte
        buf[3] = low byte
    */
    if (GetWord(buf, 2) != 28)
        return 0;

    /*
        Calculate checksum.

        PMS7003 checksum is the sum of bytes from buf[0] to buf[29].
        The result is compared with the received checksum.
    */
    for (i = 0; i < 30; i++)
    {
        sum += buf[i];
    }

    /*
        The received checksum is stored in the last two bytes:

        buf[30] = checksum high byte
        buf[31] = checksum low byte
    */
    checksum = GetWord(buf, 30);

    /*
        If calculated checksum and received checksum are different,
        the frame is considered invalid.
    */
    if (sum != checksum)
        return 0;

    /*
        If all checks passed, this frame is valid.
    */
    return 1;
}
