#include <xc.h>
#define FCY FAST_PERIPHERAL_CLOCK_FREQ
#include <libpic30.h>

#include "clock.h"
#include "i2c.h"
#include "lcd.h"

void LCDInit(void)
{
	LCD_LAT_RESET=0; // Assert LCD Reset
	LCD_TRIS_RESET=0; // LCD Reset
	__delay_ms(40);
	LCD_LAT_RESET=1;
	__delay_ms(40);
	LCD_LAT_RESET=0;
	__delay_ms(40);
	LCD_LAT_RESET=1;
	__delay_ms(40);
	
	I2C1RegWriteU8(LCD_I2C_ADDR,0x00,0x38);
    __delay_us(30);
	I2C1RegWriteU8(LCD_I2C_ADDR,0x00,0x39);
    __delay_us(30);
	I2C1RegWriteU8(LCD_I2C_ADDR,0x00,0x14);
    __delay_us(30);
	I2C1RegWriteU8(LCD_I2C_ADDR,0x00,0x70);
    __delay_us(30);
	I2C1RegWriteU8(LCD_I2C_ADDR,0x00,0x56);
    __delay_us(30);
	I2C1RegWriteU8(LCD_I2C_ADDR,0x00,0x6C);
    __delay_us(30);
	I2C1RegWriteU8(LCD_I2C_ADDR,0x00,0x38);
    __delay_us(30);
	I2C1RegWriteU8(LCD_I2C_ADDR,0x00,0x0C);
    __delay_us(30);
	I2C1RegWriteU8(LCD_I2C_ADDR,0x00,0x01); // Clear display
	__delay_ms(2);
}

void LCDPutC(char c)
{
	I2C1RegWriteU8(LCD_I2C_ADDR,0x40,c);
}

void LCDPutU32(unsigned long u32,int8_t n8NumDigits)
{
	unsigned long u32Div=1000000000UL;

	n8NumDigits-=10;
	while (u32Div!=0)
	{
		uint8_t u8=(uint8_t)(u32/u32Div);
		unsigned long u32a=u8*u32Div;

		u32-=u32a;
		u32Div/=10;
		if (n8NumDigits++>=0)
		{
			LCDPutC(u8+'0');
		}				
	}
}

void LCDClear(void)
{
	I2C1RegWriteU8(LCD_I2C_ADDR,0x00,0x01);
	__delay_ms(10);
}

void LCDCursor(uint8_t u8x,uint8_t u8y)
{
	I2C1RegWriteU8(LCD_I2C_ADDR,0x00,0x80 | (uint8_t)((u8y & 1)<<6) | (u8x & 0x3F)); // Set DDRAM address
}

void LCDPutS(const char *psz)
{
	char c;

	while ((c=*psz++)!='\0')
	{
		LCDPutC(c);
	}
}

