#include <xc.h>

#include "clock.h"
#include "i2c.h"

inline static void I2C1Close(void) // Pinched from MPLAB C32 v2.02 legacy plib
{
    I2C1CON1bits.ON = 0;
}

inline static void I2C1Open(uint32_t u32Config,uint32_t u32BRG) // Pinched from MPLAB C32 v2.02 legacy plib
{
    I2C1LBRGbits.I2CLBRG = u32BRG;
    I2C1HBRGbits.I2CHBRG = u32BRG;
    I2C1CON2=0;
    I2C1CON1 = u32Config;
}

inline static void I2C1Start(void) // Pinched from MPLAB C32 v2.02 legacy plib
{
    I2C1CON1bits.SEN = 1;
}

inline static void I2C1Stop(void) // Pinched from MPLAB C32 v2.02 legacy plib
{
    I2C1CON1bits.PEN = 1;
}

inline static void I2C1Restart(void) // Pinched from MPLAB C32 v2.02 legacy plib
{
    I2C1CON1bits.RSEN = 1;
}

static void I2C1Idle(void) // Pinched from MPLAB C32 v2.02 legacy plib
{
    while(I2C1CON1bits.SEN || I2C1CON1bits.PEN || I2C1CON1bits.RSEN || I2C1CON1bits.RCEN || I2C1CON1bits.ACKEN || I2C1STAT1bits.TRSTAT);    
}

static void I2C1Ack(void)
{
	I2C1CON1bits.ACKDT = 0;       /* Acknowledge data bit, 0 = ACK */
	I2C1CON1bits.ACKEN = 1;       /* Ack data enabled */
}

static void I2C1Nak(void)
{
	I2C1CON1bits.ACKDT = 1;       /* Acknowledge data bit, 1 = NAK */
	I2C1CON1bits.ACKEN = 1;       /* Ack data enabled */
}


static uint8_t I2C1MasterRead(void) // Pinched from MPLAB C32 v2.02 legacy plib
{
    I2C1CON1bits.RCEN = 1;
    while (I2C1CON1bits.RCEN)
    {
        Nop();
    }
    I2C1STAT1bits.I2COV = 0;
    return I2C1RCV;   
}

static int I2C1MasterWrite(uint8_t u8Data) // Pinched from MPLAB C32 v2.02 legacy plib
{
    I2C1TRNbits.I2CTRN = u8Data;

    if(I2C1STAT1bits.IWCOL)        /* If write collision occurs,return -1 */
        return -1;
    else
	{
		while( I2C1STAT1bits.TBF );   // wait until write cycle is complete         
		I2C1Idle();                 // ensure module is idle

		if ( I2C1STAT1bits.ACKSTAT ) // test for ACK condition received
             return ( -2 );
        else 
			return ( 0 );              
    }     
}

void I2C1Init(void)
{
#ifdef I2C1_SDA_ANSEL
    I2C1_SDA_ANSEL=0;
#endif    
    I2C1_SDA_LAT=1;
    I2C1_SDA_CNPU=1;
    I2C1_SDA_TRIS=0;

#ifdef I2C1_SCL_ANSEL
    I2C1_SCL_ANSEL=0;
#endif    
    I2C1_SCL_LAT=1;
    I2C1_SCL_CNPU=1;
    I2C1_SCL_TRIS=0;    
    
	I2C1Close();
	// Configure I2C1 0x2C ~400kHz @FCY=40MHz, disable slew rate control if necessary(Silicon errata 80531c section 9)
	I2C1Open
    (
        _I2C1CON1_ON_MASK |
        _I2C1CON1_DISSLW_MASK,
        (uint32_t)( (1.0 / ( 2.0 * I2C1_BAUD_RATE ) - I2C1_TPGOB ) * I2C1_PBCLK_SPEED - 3) // DS 70005539B Equation 20-1
    );    
}

void I2C1RegWriteU8(uint8_t u8I2CAddr,uint8_t u8RegAddr,uint8_t u8Data)
{

	I2C1Start();
	I2C1Idle();
	
	I2C1MasterWrite((u8I2CAddr<<1) | 0);
	I2C1Idle();
	while (I2C1STAT1bits.ACKSTAT)
	{
		Nop();
	}
	I2C1MasterWrite(u8RegAddr);
	I2C1Idle();
	while (I2C1STAT1bits.ACKSTAT)
	{
		Nop();
	}
	I2C1MasterWrite(u8Data);
	I2C1Idle();
	while (I2C1STAT1bits.ACKSTAT)
	{
		Nop();
	}

	I2C1Stop();
	I2C1Idle();

	Nop();    
}

uint8_t I2C1RegReadU8(uint8_t u8I2CAddr,uint8_t u8RegAddr)
{
	uint8_t u8Ret;

	I2C1Start();
	I2C1Idle();
	
	I2C1MasterWrite((u8I2CAddr<<1) | 0);
	I2C1Idle();
	while (I2C1STAT1bits.ACKSTAT)
	{
		Nop();
	}
	I2C1MasterWrite(u8RegAddr);
	I2C1Idle();
	while (I2C1STAT1bits.ACKSTAT)
	{
		Nop();
	}

    I2C1Restart();
	I2C1Idle();
	
	I2C1MasterWrite((u8I2CAddr<<1) | 1);
	I2C1Idle();
	while (I2C1STAT1bits.ACKSTAT)
	{
		Nop();
	}

	u8Ret=I2C1MasterRead();
    
    I2C1Nak(); // Nak as it's the last byte to be received
    I2C1Idle();

	I2C1Stop();
	I2C1Idle();
	Nop();

	return u8Ret;    
}

void I2C1RegWriteAU8(uint8_t u8I2CAddr,uint8_t u8RegAddr,uint8_t *pu8Data,uint32_t u32Count)
{
    I2C1Start();
    I2C1Idle();    
    
    I2C1MasterWrite((u8I2CAddr<<1) | 0);
    I2C1Idle();
	while (I2C1STAT1bits.ACKSTAT)
	{
		Nop();
	}   

    I2C1MasterWrite(u8RegAddr);
    I2C1Idle();
	while (I2C1STAT1bits.ACKSTAT)
	{
		Nop();
	}
    
    while (u32Count--)
    {
        I2C1MasterWrite(*pu8Data++);
        I2C1Idle();
        while (I2C1STAT1bits.ACKSTAT)
        {
            Nop();
        }
    }
    I2C1Stop();
    I2C1Idle();    
}

void I2C1RegReadAU8(uint8_t u8I2CAddr,uint8_t u8RegAddr,uint8_t *pu8Data,uint32_t u32Count)
{
    I2C1Start();
    I2C1Idle();
    
    I2C1MasterWrite((u8I2CAddr<<1) | 0);
    I2C1Idle();
	while (I2C1STAT1bits.ACKSTAT)
	{
		Nop();
	}
    
    I2C1MasterWrite(u8RegAddr);
    I2C1Idle();
	while (I2C1STAT1bits.ACKSTAT)
	{
		Nop();
	}    

    I2C1Restart();
    I2C1Idle();
    
    I2C1MasterWrite((u8I2CAddr<<1) | 1);
    I2C1Idle();
	while (I2C1STAT1bits.ACKSTAT)
	{
		Nop();
	}
    
    while (u32Count--)
    {
        *pu8Data++=I2C1MasterRead();

        if (u32Count!=0)
        {
            I2C1Ack();
        }
        else
        {
            I2C1Nak(); // Nak as it's the last byte to be received
        }
        I2C1Idle();
    }
    I2C1Stop();
    I2C1Idle();
}
