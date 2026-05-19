#include <stm32f10x.h>
#include <stdint.h>

/* HTU21D 7-bit I2C address */
#define HTU21D_ADDR        0x40

/* Temperature measurement command: No Hold Master Mode */
#define HTU21D_TEMP_CMD    0xF3

/* Simple timeout value for I2C waiting loops */
#define I2C_TIMEOUT        100000U

/* Global variables for debugging */
volatile float HTU21D_Temperature = 0.0f;
volatile uint16_t HTU21D_RawTemp = 0;
volatile uint8_t HTU21D_Error = 0;

/* Function prototypes */
void I2C1_Init(void);
void delay_ms(uint16_t ms);

uint8_t HTU21D_ReadTemperature(void);

static uint8_t I2C1_WaitReady(void);
static uint8_t I2C1_Start(void);
static void    I2C1_Stop(void);
static uint8_t I2C1_SendAddress(uint8_t addr_rw);
static uint8_t I2C1_WriteByte(uint8_t data);
static uint8_t I2C1_ReadByte(uint8_t ack, uint8_t *data);

int main(void)
{
    I2C1_Init();

    /* Wait for HTU21D to become ready after power-up */
    delay_ms(100);

    while (1)
    {
        /*
           Read temperature from HTU21D.

           If HTU21D_Error is 0:
           - HTU21D_Temperature has the converted temperature value.
           - HTU21D_RawTemp has the raw temperature data.

           You can check these variables in the debugger Watch window.
        */
        HTU21D_Error = HTU21D_ReadTemperature();

        delay_ms(1000);
    }
}

/*
   Initialize I2C1.

   STM32F103RB I2C1 pins:
   PB6 = I2C1_SCL
   PB7 = I2C1_SDA

   Important:
   I2C uses open-drain lines.
   External pull-up resistors are required.

   Recommended:
   PB6/SCL -- 4.7k or 10k -- 3.3V
   PB7/SDA -- 4.7k or 10k -- 3.3V
*/
void I2C1_Init(void)
{
    /* Enable GPIOB and AFIO clocks */
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN | RCC_APB2ENR_AFIOEN;

    /* Enable I2C1 clock */
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;

    /*
       Configure PB6 and PB7 as alternate function open-drain.

       MODE = 11: Output mode, max speed 50 MHz
       CNF  = 11: Alternate function open-drain
       0xF  = 1111b
    */
    GPIOB->CRL &= ~((0xFU << (6 * 4)) | (0xFU << (7 * 4)));
    GPIOB->CRL |=  ((0xFU << (6 * 4)) | (0xFU << (7 * 4)));

    /* Reset I2C1 peripheral */
    RCC->APB1RSTR |=  RCC_APB1RSTR_I2C1RST;
    RCC->APB1RSTR &= ~RCC_APB1RSTR_I2C1RST;

    /* Disable I2C1 before setting timing registers */
    I2C1->CR1 &= ~I2C_CR1_PE;

    /*
       I2C timing configuration.

       PCLK1 = 36 MHz
       Target I2C speed = 100 kHz

       CR2   = 36  : I2C peripheral clock frequency in MHz
       CCR   = 180 : 36 MHz / (2 * 180) = 100 kHz
       TRISE = 37  : Standard mode maximum rise time setting
    */
    I2C1->CR2   = 36;
    I2C1->CCR   = 180;
    I2C1->TRISE = 37;

    /* Enable I2C1 */
    I2C1->CR1 |= I2C_CR1_PE;
}

/*
   Wait until the I2C bus is free.

   Return:
   1 = bus is ready
   0 = timeout
*/
static uint8_t I2C1_WaitReady(void) {
	uint32_t timeout = I2C_TIMEOUT;

	while (I2C1->SR2 & I2C_SR2_BUSY) {
		// Communication ongoing on the bus
    if (timeout-- == 0)
			return 0;
  }
  return 1;	// No communication on the bus
}

/*
   Generate START condition.

   Return:
   1 = success
   0 = timeout
*/
static uint8_t I2C1_Start(void){
    uint32_t timeout = I2C_TIMEOUT;
	
    /* Clear old acknowledge failure flag */
    I2C1->SR1 &= ~I2C_SR1_AF;

    /* Generate START condition */
    I2C1->CR1 |= I2C_CR1_START;

    /* Wait until START condition is generated */
    while ((I2C1->SR1 & I2C_SR1_SB) == 0)
    {
        if (timeout-- == 0)
            return 0;
    }

    return 1;
}

/*
   Generate STOP condition.
*/
static void I2C1_Stop(void) {
    I2C1->CR1 |= I2C_CR1_STOP;
}

/*
   Send slave address with R/W bit.
   addr_rw:
   - 0x80 = HTU21D address + Write bit
   - 0x81 = HTU21D address + Read bit

   Return:
   1 = ACK received
   0 = NACK or timeout
*/
static uint8_t I2C1_SendAddress(uint8_t addr_rw) {
	volatile uint32_t dummy;
  uint32_t timeout = I2C_TIMEOUT;
  /* Clear old acknowledge failure flag */
  I2C1->SR1 &= ~I2C_SR1_AF;
  /* Send slave address */
  I2C1->DR = addr_rw;

  while (1) {
		// ADDR flag is set when the slave address is acknowledged.
		if (I2C1->SR1 & I2C_SR1_ADDR){
			// Clear ADDR flag by reading SR1 followed by SR2.
			dummy = I2C1->SR1;
			dummy = I2C1->SR2;
			(void)dummy;

			return 1;
		}

		// AF flag is set when the slave does not acknowledge.
		if (I2C1->SR1 & I2C_SR1_AF) {
				I2C1->SR1 &= ~I2C_SR1_AF;
				return 0;
		}

		if (timeout-- == 0)
				return 0;
	}
}

/*
   Write one byte to I2C bus.

   Return:
   1 = success
   0 = NACK or timeout
*/
static uint8_t I2C1_WriteByte(uint8_t data){
    uint32_t timeout = I2C_TIMEOUT;
    /* Write data to data register */
    I2C1->DR = data;
    /* Wait until byte transfer is finished.
       BTF means the byte has been transferred.*/
    while ((I2C1->SR1 & I2C_SR1_BTF) == 0) {
        if (I2C1->SR1 & I2C_SR1_AF) {
            I2C1->SR1 &= ~I2C_SR1_AF;
            return 0;
        }

        if (timeout-- == 0)
            return 0;
    }
    return 1;
}

/* Read one byte from I2C bus.
   ack:
   - 1 = send ACK after receiving this byte
   - 0 = send NACK after receiving this byte

   Return:
   1 = success
   0 = timeout
*/
static uint8_t I2C1_ReadByte(uint8_t ack, uint8_t *data) {
    uint32_t timeout = I2C_TIMEOUT;

    if (ack) {
        /* ACK means: I want to read more bytes */
        I2C1->CR1 |= I2C_CR1_ACK;
    } else {
        /* NACK means: this is the last byte */
        I2C1->CR1 &= ~I2C_CR1_ACK;
    }

    /* Wait until one byte is received */
    while ((I2C1->SR1 & I2C_SR1_RXNE) == 0) {
        if (timeout-- == 0)
            return 0;
    }

    *data = (uint8_t)I2C1->DR;
    return 1;
}

/* Read temperature from HTU21D using No Hold Master Mode.
   Sequence:
   1. Send temperature measurement command 0xF3.
   2. Send STOP.
   3. Wait for conversion.
   4. Send START again.
   5. Read MSB, LSB, and checksum.
   6. Convert raw data to temperature in Celsius.
   Return:
   0  = success
   1~10 = error code
*/
uint8_t HTU21D_ReadTemperature(void) {
    uint8_t msb, lsb, checksum;
    uint16_t raw;

    /* Step 1:
       Send temperature measurement command. */
    if (!I2C1_WaitReady()) return 1;
    if (!I2C1_Start()) return 2;
    if (!I2C1_SendAddress((HTU21D_ADDR << 1) | 0)){
        I2C1_Stop();
        return 3;
    }
    if (!I2C1_WriteByte(HTU21D_TEMP_CMD)) {
        I2C1_Stop();
        return 4;
    }

    /* Step 2:
       In No Hold Master Mode, the master sends STOP
       and waits before reading the result. */
    I2C1_Stop();

    /* Step 3:
       Wait for temperature conversion to finish.
       100 ms is simple and safe for this educational example. */
    delay_ms(100);

    /* Step 4:
       Read three bytes from HTU21D.
       Byte 1 = temperature MSB
       Byte 2 = temperature LSB + status bits
       Byte 3 = checksum */
    if (!I2C1_WaitReady()) return 5;
    /* Enable ACK before reading multiple bytes */
    I2C1->CR1 |= I2C_CR1_ACK;
    if (!I2C1_Start()) return 6;
    if (!I2C1_SendAddress((HTU21D_ADDR << 1) | 1)) {
        I2C1_Stop();   return 7;
    }
    if (!I2C1_ReadByte(1, &msb)) {
        I2C1_Stop();   return 8;
    }
    if (!I2C1_ReadByte(1, &lsb)) {
        I2C1_Stop();   return 9;
    }
    if (!I2C1_ReadByte(0, &checksum)){
        I2C1_Stop();   return 10;
    }
    I2C1_Stop();

    /* The lower 2 bits are status bits.
       Clear them before using the conversion formula.
       Do not shift right.
       The HTU21D datasheet formula uses this 16-bit raw value directly.
    */
    raw = (((uint16_t)msb << 8) | lsb) & 0xFFFC;
    HTU21D_RawTemp = raw;

    /* Convert raw data to Celsius.
       Temperature [C] = -46.85 + 175.72 * raw / 65536 */
    HTU21D_Temperature = -46.85f + 175.72f * raw / 65536.0f;

    /* The checksum byte is read but not checked in this simple example.
       It can be used later for data validation. */
    (void)checksum;
    return 0;
}

/*
   Simple blocking delay.

   This is not an accurate hardware-timer delay.
   It is enough for a basic sensor example.
*/
void delay_ms(uint16_t ms)
{
    volatile uint32_t i;

    while (ms--)
    {
        for (i = 0; i < 3500; i++)
        {
        }
    }
}
