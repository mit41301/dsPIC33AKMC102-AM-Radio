#include <xc.h>
#include <stdio.h>
#include <math.h>

#define FCY CLKGEN1_FREQ

#include "clock.h"

#define U1_BAUD_RATE (115200UL)
//#define U1_BAUD_RATE (460800UL)
//#define U1_BAUD_RATE (921600UL)


void __attribute__((interrupt)) _StackErrorTrap(void) 
{
    while (1)
    {
        Nop();
    }
}

void __attribute__((interrupt)) _BusErrorTrap(void) 
{
    while (1)
    {
        Nop();
    }
}

void __attribute__((interrupt)) _IllegalInstructionTrap(void) 
{
    while (1)
    {
        Nop();
    }
}

void __attribute__((interrupt)) _AddressErrorTrap(void) 
{
    while (1)
    {
        Nop();
    }
}

void __attribute__((interrupt)) _MathErrorTrap(void) 
{
    while (1)
    {
        Nop();
    }
}

void __attribute__((interrupt)) _GeneralTrap(void) 
{
    while (1)
    {
        Nop();
    }
}

static void GPIOInit(void)
{
    // RA0/RP1 is U1TX, RA1/RP2 is DACOUT1, RA2/RP3 is OA1OUT, RA3/RP4 is OA1IN-, RA4/RP5 is OA1IN+
    LATA=0b00000001; 
    CNPUA=0b00000000;
    ANSELA=0b00011110;
    TRISA=0b00011110;
    RPOR0bits.RP1R=0;
    RPOR0bits.RP2R=0;
    RPOR0bits.RP3R=0;
    RPOR0bits.RP4R=0;
    RPOR1bits.RP5R=0;    
    
    // RB0/RP17 is OA2OUT, RB1/RP18 is OA2IN-, RB2/RP19 is OA2IN+, RB3/RP20 is SDA1, RB4/RP21 is SCL1
    LATB=0b00000000;
    CNPUB=0b00011000;
    ANSELB=0b00000111;
    TRISB=0b00011111;
    RPOR4bits.RP17R=0;
    RPOR4bits.RP18R=0;
    RPOR4bits.RP19R=0;
    RPOR4bits.RP20R=0;
    RPOR5bits.RP21R=0;
    
    // RC0/RP33 is OSCO, RC1/RP34 is OSCI, RC2/RP35 is PGC3, RC3/RP36 is PGD3, RC4/RP37 is GPIO
    LATC=0b00000000;
    CNPUC=0b00000000;
//    ANSELC=0b00000011; // Device has no ANSELC register
    TRISC=0b00000011;
    RPOR8bits.RP33R=0;
    RPOR8bits.RP34R=0;
    RPOR8bits.RP35R=0;
    RPOR8bits.RP36R=0;
    RPOR9bits.RP37R=0;
    
    // RD0 is CLCINA, RD1 is CLCINB, RD2 is CLC1OUT, RD3 is CLC2OUT
    LATD=0b00000000;
    CNPUD=0b00000011;
//    ANSELD=0b00000000; // Device has no ANSELD register
    TRISD=0b00000011;
    RPOR12bits.RP49R=0;
    RPOR12bits.RP50R=0;
    RPOR12bits.RP51R=0;
    RPOR12bits.RP52R=0;
}  

static void U1Init(void)
{
    // Configure pin RA0/RP1/U1TX
    LATAbits.LATA0=1;
    TRISAbits.TRISA0=0;
    RPOR0bits.RP1R=9; // 9 => U1TX (DS70005539C Table 11-13 pp480-481)
    
    U1CONbits.BRGS=1; //  1 => High speed legacy mode (div-by-4 for BRG calcs)
    U1BRGbits.BRG=(uint32_t)(STANDARD_PERIPHERAL_CLOCK_FREQ/4.0/U1_BAUD_RATE+0.4999-1);
    U1CONbits.TXEN=1;
    U1CONbits.ON=1;
}

// Override weak write() function for stdout on UART U1
int write(int handle, void *buffer, unsigned int len) // To redirect stdout
{
    int i;

    for (i = 0; i < len; i++)
    {
        char c=*(char*) buffer++;
        
        while (!U1STATbits.TXMTIF)
        {
            Nop();
        }
        U1TXB=c;
    }
    return (len);
}

int main(void)
{
    uint32_t u32RCONSave=RCON;
    
    u32RCONSave=RCON;
    RCON=0;
        
    ClkInit();
    GPIOInit();
    U1Init();

    printf("\r\n\nRCON = 0x%08lX",u32RCONSave);      
    
    for (float f = 0.0F; f < M_PI * 2.0f; f += 0.2f)
	{
		printf("\r\nPhi=%f, sin(Phi)=%+12.8f, cos(Phi)=%+12.8f",f,(float)sin(f),(float)cos(f));
	}
    while (1)
    {
        Nop();
    }
	return 0;
    
    
}