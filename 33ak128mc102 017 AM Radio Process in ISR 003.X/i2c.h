#define I2C1_SDA_LAT LATBbits.LATB3
#define I2C1_SDA_CNPU CNPUBbits.CNPUB3
#define I2C1_SDA_TRIS TRISBbits.TRISB3
#define I2C1_SDA_ANSEL ANSELBbits.ANSELB3

#define I2C1_SCL_LAT LATBbits.LATB4
#define I2C1_SCL_CNPU CNPUBbits.CNPUB4
#define I2C1_SCL_TRIS TRISBbits.TRISB4
#define I2C1_SCL_ANSEL ANSELBbits.ANSELB4

#define I2C1_BAUD_RATE (400000UL)
#define I2C1_PBCLK_SPEED STANDARD_PERIPHERAL_CLOCK_FREQ
#define I2C1_TPGOB (200e-9) // Fudge factor in nanoseconds for BRG calcs from datasheet, "delay" parameter

void I2C1Init(void);
void I2C1RegWriteU8(uint8_t u8I2CAddr,uint8_t u8RegAddr,uint8_t u8Data);
uint8_t I2C1RegReadU8(uint8_t u8I2CAddr,uint8_t u8RegAddr);
void I2C1RegWriteAU8(uint8_t u8I2CAddr,uint8_t u8RegAddr,uint8_t *pu8Data,uint32_t u32Count);
void I2C1RegReadAU8(uint8_t u8I2CAddr,uint8_t u8RegAddr,uint8_t *pu8Data,uint32_t u32Count);

