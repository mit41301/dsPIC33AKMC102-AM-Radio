// 20250817 Works: AM radio receiver with LCD display and rotary encoder tuning.
//
//   Channel spacing is set for ITU regions 1 & 3, i.e. 9kHz.
//   LW & MW bands are set to the UK allocation.
//   You can change this within in the two CLC interrupts.
//
//   8MHz crystal with 2 x 22pF (OSCO/RC0/RP33 & OSCI/RC1/RP34)
//   MCLR, PGC3/RC2/RP35, PGD/RC3/RP36 for programming/debugging
//   ACM0802A I2C LCD (not used other than to say "Hello" on I2C
//     Reset (RC4/RP37)
//     SCL1 (RB4/RP21) 4.7k pull-up
//     SDA1 (RB4/RP21) 4.7k pull-up
//   Rotary encoder generates interrupts but otherwise unused
//     Phase A 10k pull-up, 1k series, 100nF to ground (RD0/RP49/CLCINA)
//     Phase B 10k pull-up, 1k series, 100nF to ground (RD1/RP50/CLCINB)
//   4.7k + 4.7k + 10uF + 10nF for Vdd/2 for voltage reference
//   1 x Opamp used (OA2)
//     20k feedback OA2OUT (RB0/RP17) to OA2IN- (RB1/RP18)
//     2k + 100nF in series input on OA2IN- (RB1/RP18)
//       coupled to BNC with 47 ohm to ground, Mini Whip source antenna
//     OA2IN+ (RB2/RP19) to Vdd/2 voltage reference
//   ADC1Ch0 input single-ended on AD1AN1 (RA4/RP5/pin 7)
//     Final sample rate after aggregation is controlled by SCCP2/DMA1
//     4x oversampled giving 13 bit resolution
//     Note in errata DS80001139B that ADC doesn't sign extend
//     Samples are converted to signed by subtracting 4096 from every sample
//   SCCP2 controls the ADC1Ch0 sampling rate (4MSa/s)
//   DMA1 is used for ADC1Ch0 streaming
//     Note that DMA1 is triggered from SCCP2 rather than ADC1Ch0 interrupt
//       because ADC interrupts generate multiple triggers when using
//       oversampled ADC.
//   DACOUT1 audio output (RA1/RP2)
//     Sample rate is controlled by SCCP1/DMA2
//   SCCP1 controls the DAC1 sampling rate, ADC sampling rate / decimation rate
//     in this implementation, 4MSa/s / 128 = 31.25kSa/s.
//   DMA2 is triggered by SCCP1, writing to DAC1's DAC1DAT field
//
//   DSP Processing Overview
//     Almost the entire DSP processing is performed in single precision 
//       floating point. Only the ADC input samples and DAC output samples are 
//       in integer.
//     The logical flow is:
//       ADC Samples
//       Mix with Quadrature LO (direct conversion)
//       Low pass filter
//       Decimate
//       Demodulate & AGC
//
//   DSP Processing flow detail...
//     Ping/pong ADC DMA buffer, produces 128 samples at a time for processing.
//     Each unsigned integer sample is 13 bits at 4MSa/s from ADC.
//     Each sample is corrected to signed.
//     Each signed sample is converted to float.
//     Each float sample is multiplied  with sin(Phi) and cos(Phi) to give I 
//       and Q mixed with the frequency in question.
//     Phi is incremented commensurate with desired operating frequency, ready
//       for next sample.
//     I & Q Samples filtered and decimated by 128 with a seven stage polyphase 
//       deconstructed non-recursive 3rd-order CIC filter to give 31.25kSa/s 
//       (see Richard Lyons "Understanding Digital Signal Processing" 2nd 
//       edition section 13.24).
//     I & Q filtered outputs scaled (due to filter gain).
//     Phi is checked for value over 2 * PI, and adjusted as necessary to
//       avoid future excessive frequency inaccuracies.
//     31.25kSa/s I & Q stream further filtered with a 16 tap 6kHz low pass FIR 
//       compensation filter. See Matlab code referenced here:
//       https://www.dsprelated.com/blogimages/RickLyons/CIC_digital_filters-Ver_3-Lyons.pdf
//       (also runs in Octave).
//     FIR result samples demodulated with simple SQRT(I^2 + Q^2).
//     AGC applied giving audio output.
//     Convert to 12 bit unsigned integer for DAC
//     Result fed to DAC DMA managed ping/pong buffer.
//
//  1 RA0 RP1 U1TX DBG out 921.6kbps   28 MCLR
//  2 RA1 RP2 DACOUT1 audio output     27 Vdd
//  3 AVss                             26 Vss
//  4 AVdd                             25 RD3 RP52 DBG3: DSP processing active
//  5 RA2 RP3                          24 RD2 RP51 DBG2
//  6 RA3 RP4                          23 RD1 RP50 CLCINB Encoder input B
//  7 RA4 RP5 AD1AN1 tie to OA2OUT     22 RD0 RP49 CLCINA Encoder input A
//  8 Vss                              21 RC4 RP37 ACM0802 LCD Reset
//  9 AVdd                             20 RC3 RP36 PGD3
// 10 RB0 RP17 OA2OUT tie to AD1AN1    19 RC2 RP35 PGC3
// 11 RB1 RP18 OA2IN-                  18 Vdd
// 12 RB2 RP19 OA2IN+ Vdd/2            17 Vss
// 13 RB3 RP20 SDA1                    16 RC1 RP34 OSCI 8MHz Xtal, 22pF to Vss
// 14 RB4 RP21 SCL1                    15 RC0 RP33 OSCO 8MHz Xtal, 22pF to Vss

#include <xc.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#define FCY FAST_PERIPHERAL_CLOCK_FREQ
#include <libpic30.h>

#include "clock.h"
#include "i2c.h"
#include "lcd.h"
#include "OptimisedDSP.h"
#include "Debug.h"
#include "main.h"
#include "ProcessSamples.h"

//#define U1_BAUD_RATE (115200UL)
//#define U1_BAUD_RATE (460800UL)
#define U1_BAUD_RATE (921600UL)

//   60000 // MSF
//   77500 // DCF77
//  198000 // BBC Radio 4
//  558000 // Punjab Radio
//  648000 // Radio Caroline
//  693000 // BBC Radio 5
//  837000 // BBC Asian
//  873000 // BBC Norfolk
//  909000 // BBC Radio 5
// 1053000 // Talk Sport
// 1089000 // Talk Sport
// 1107000 // Talk Sport
// 1449000 // BBC Asian

static volatile int _nFrequency=198000;
static volatile bool _bFrequencyChanged=true;

static int32_t _an32ADC1Ch0Samples[ADC1CH0_NUM_SAMPLES];

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

// Rotary encoder clockwise
void __attribute__ ( ( interrupt, no_auto_psv ) ) _CLC1PInterrupt (void)
{
    IFS7bits.CLC1PIF=0;
    
    // UK Long wave and Medium wave allocations:
    // LW is 144 to 252kHz
    // MW is 531 to 1602kHz
    _nFrequency+=9000; // ITU Regions 1 & 3 channel spacing
    if (_nFrequency>1602000)
    {
        _nFrequency=144000;
    }
    else
    {
        if (_nFrequency>252000 && _nFrequency<531000)
        {
            _nFrequency=531000;
        }
    }
    _bFrequencyChanged=true;
}

// Rotary encoder anticlockwise
void __attribute__ ( ( interrupt, no_auto_psv ) ) _CLC2PInterrupt (void)
{
    IFS7bits.CLC2PIF=0;
    
    _nFrequency-=9000; // IARU Regions 1 & 3 channel spacing
    if (_nFrequency<144000)
    {
        _nFrequency=1602000;
    }
    else
    {
        if (_nFrequency<531000 && _nFrequency>252000)
        {
            _nFrequency=252000;
        }
    }
    _bFrequencyChanged=true;
}

// ADC1Ch0 ISR
void __attribute__ ( ( interrupt, no_auto_psv ) ) _DMA1Interrupt(void)
{
    if(DMA1STATbits.HALF == 1)
    {
        DMA1STATbits.HALF = 0U;
        ProcessSamples(_an32ADC1Ch0Samples);
    }
    if(DMA1STATbits.DONE == 1)
    {
        DMA1STATbits.DONE = 0U;
        ProcessSamples(_an32ADC1Ch0Samples+ADC1CH0_NUM_SAMPLES/2U);
    }

    IFS2bits.DMA1IF = 0;
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

static void PMUInit(void)
{
    HPCCONbits.CLR=1;
    HPSEL0bits.SELECT0=1; // 1 => CPU cycle elapsed (reference)
    HPSEL0bits.SELECT1=3; // 3 => Write stage FPU stall 
    HPSEL0bits.SELECT2=8; // 8 => Address stage FPU instruction stall
    HPSEL0bits.SELECT3=9; // 9 => Address stage FPU read stall
    HPSEL1bits.SELECT4=4; // 4 => Write stage stall
    HPSEL1bits.SELECT5=7; // 7 => Address stage hazard
    HPSEL1bits.SELECT6=10; // 10 => Address stage read stall 
    HPSEL1bits.SELECT7=11; // 11 => Address stage stall 
    HPCCONbits.ON=1;
}

static void OA2Init(void)
{
    // OA2OUT/RB0/RP17
    ANSELBbits.ANSELB0=1;
    TRISBbits.TRISB0=1;
    RPOR4bits.RP17R=0;
    
    // OA2IN-/RA3/RP18
    ANSELBbits.ANSELB1=1;
    TRISBbits.TRISB1=1;
    RPOR4bits.RP18R=0;
    
    // OA2IN+/RB2/RP19
    ANSELBbits.ANSELB2=1;
    TRISBbits.TRISB2=1;
    RPOR4bits.RP19R=0;

    AMP2CON1bits.DIFFCON=0b00; // 0b00 => Use both NMOS and PMOS differential input pair
    AMP2CON1bits.HPEN=1; //  1 => Enables Op Amp High-Power (high bandwidth) mode
    AMP2CON1bits.UGE=0; // 0 => Disables Unity Gain mode
    AMP2CON1bits.OMONEN=1; // 1 => Enables output to ADC
    AMP2CON1bits.AMPEN=1;    
}

static void CLC1Init(void)
{
//    ANSELDbits.ANSD0=0; // Device has no analog function on this pin
    CNPUDbits.CNPUD0=1;
    TRISDbits.TRISD0=1;
    RPINR20bits.CLCINAR=49; // 49 => RD0/RP49

//    ANSELDbits.ANSD1=0; // Device has no analog function on this pin
    CNPUDbits.CNPUD1=1;
    TRISDbits.TRISD1=1;
    RPINR20bits.CLCINBR=50; // 50 => RD1/RP50
    
    // RD2/RP51/CLC1OUT
//    RPOR12bits.RP51R=47; // 47 => CLC1OUT DS70005539C Table 11-13 pp480-481
    
    // MODE JK flip-flop with R; LCPOL non-inverted; LCOUT Low; LCOE enabled; INTN disabled; INTP disabled; ON disabled; G1POL non-inverted; G2POL non-inverted; G3POL inverted; G4POL non-inverted; 
    CLC1CON = 0x40086UL;
    // DS1 CLCINA I/O pin; DS2 CLCINB I/O pin; DS3 CLCINC I/O pin; DS4 CLCIND I/O pin; 
    CLC1SEL = 0x5000UL;
    // G1D1N disabled; G1D1T disabled; G1D2N disabled; G1D2T enabled; G1D3N disabled; G1D3T disabled; G1D4N disabled; G1D4T disabled; G2D1N disabled; G2D1T enabled; G2D2N disabled; G2D2T disabled; G2D3N disabled; G2D3T disabled; G2D4N disabled; G2D4T disabled; G3D1N disabled; G3D1T enabled; G3D2N disabled; G3D2T disabled; G3D3N disabled; G3D3T disabled; G3D4N disabled; G3D4T disabled; G4D1N disabled; G4D1T disabled; G4D2N disabled; G4D2T disabled; G4D3N disabled; G4D3T disabled; G4D4N disabled; G4D4T disabled; 
    CLC1GLS = 0x20208UL;
    
    CLC1CONbits.INTP=1;
    IPC29bits.CLC1NIP=4;
    IFS7bits.CLC1PIF=0;
    IEC7bits.CLC1PIE=1;
    
    CLC1CONbits.ON=1;
}    

static void CLC2Init(void)
{
//    ANSELDbits.ANSD0=0; // Device has no analog function on this pin
    CNPUDbits.CNPUD0=1;
    TRISDbits.TRISD0=1;
    RPINR20bits.CLCINAR=49; // 49 => RD0/RP49

//    ANSELDbits.ANSD1=0; // Device has no analog function on this pin
    CNPUDbits.CNPUD1=1;
    TRISDbits.TRISD1=1;
    RPINR20bits.CLCINBR=50; // 50 => RD1/RP50
    
    // RD3/RP52/CLC2OUT
//    RPOR12bits.RP52R=48; // 48 => CLC2OUT DS70005539C Table 11-13 pp480-481
    
    // MODE JK flip-flop with R; LCPOL non-inverted; LCOUT Low; LCOE enabled; INTN disabled; INTP disabled; ON disabled; G1POL non-inverted; G2POL non-inverted; G3POL inverted; G4POL non-inverted; 
    CLC2CON = 0x40086UL;
    // DS1 CLCINA I/O pin; DS2 CLCINB I/O pin; DS3 CLCINC I/O pin; DS4 CLCIND I/O pin; 
    CLC2SEL = 0x5000UL;
    // G1D1N disabled; G1D1T enabled; G1D2N disabled; G1D2T disabled; G1D3N disabled; G1D3T disabled; G1D4N disabled; G1D4T disabled; G2D1N disabled; G2D1T disabled; G2D2N disabled; G2D2T enabled; G2D3N disabled; G2D3T disabled; G2D4N disabled; G2D4T disabled; G3D1N disabled; G3D1T disabled; G3D2N disabled; G3D2T enabled; G3D3N disabled; G3D3T disabled; G3D4N disabled; G3D4T disabled; G4D1N disabled; G4D1T disabled; G4D2N disabled; G4D2T disabled; G4D3N disabled; G4D3T disabled; G4D4N disabled; G4D4T disabled; 
    CLC2GLS = 0x80802UL;

    CLC2CONbits.INTP=1;
    IPC29bits.CLC2NIP=4;
    IFS7bits.CLC2PIF=0;
    IEC7bits.CLC2PIE=1;
    
    CLC2CONbits.ON=1;
}    

// SCCP2 is ADC1Ch0 Trigger
static void SCCP2Init(void)
{
    // Configure pin RD2/RP51/OCM2
//    RPOR12bits.RP51R=25; // 25 = OCM2 (DS70005539C Table 11-13 pp480-481
//    ASELDbits.ANSD2=0; // No analogue function on RD2
//    TRISDbits.TRISD2=0;
    
    CCP2TMR=0;
    CCP2PR=CLKGEN12_FREQ/SCCP2_FREQ-1;
    CCP2RAbits.CMPA=0;
    CCP2RBbits.CMPB=CLKGEN12_FREQ/SCCP2_FREQ/2U;
    CCP2CON1bits.CLKSEL=0b001; // 0b001 => CLKGEN13
    CCP2CON2bits.OCAEN=1; // 1 => Enable output
    CCP2CON1bits.MOD=0b0101; // 0b0101 => Dual Edge Compare mode ? Buffered
    
    IPC6bits.CCT2IP=4;
    IFS1bits.CCT2IF=0;
//    IEC1bits.CCT2IE=1;    
    
    IPC6bits.CCP2IP=4;
    IFS1bits.CCP2IF=0;
//    IEC1bits.CCP2IE=1;    
    
    CCP2CON1bits.ON=1;
}

static void ADC1Init(void)
{
    // Configure pin RA3/RP4/AD1ANN1
    ANSELAbits.ANSELA3=1;
    TRISAbits.TRISA3=1;
    RPOR0bits.RP4R=0;
    
    // Configure pin RA4/RP5/AD1AN1
    ANSELAbits.ANSELA4=1;
    TRISAbits.TRISA4=1;
    RPOR1bits.RP5R=0;
    
//    AD1CH0CONbits.MODE=0b00; // 0b00 => Single sample triggered by TRG1SRC trigger
    AD1CH0CONbits.MODE=0b11; // 0b11 => Oversampling of multiple samples defined by ACCNUM[1:0] bits
    AD1CH0CONbits.ACCNUM=0b00; // 0b00 => Four samples, 13 bits result in ADnDATAx register
//    AD1CH0CONbits.ACCNUM=0b01; // 0b01 => 16 samples, 14 bits result in ADnDATAx register
//    AD1CH0CONbits.ACCNUM=0b10; // 0b10 => 64 samples, 15 bits result in ADnDATAx register
//    AD1CH0CONbits.ACCNUM=0b11; // 0b11 => 256 samples, 16 bits result in ADnDATAx register
    AD1CH0CONbits.TRG1SRC=0b01101; // 0b01101 => SCCP2 OCMP/ICAP out
    AD1CH0CONbits.TRG2SRC=0b00010; // 0b00010 => Immediate re-trigger request
    
    // Note errata that sign extend doesn't work for DIFF=1
    // Working around this by using fractional representation then scaling
    AD1CH0CONbits.DIFF=0; // 0 => Single ended
//    AD1CH0CONbits.DIFF=1; // 1 => Differential mode
    AD1CH0CONbits.LEFT=0; // 0 => Integer mode
//    AD1CH0CONbits.LEFT=1; // 1 => Fractional mode
    AD1CH0CONbits.PINSEL=1; // 1 => AD1AN1
    AD1CH0CONbits.NINSEL=0; // 0 => AVss
//    AD1CH0CONbits.NINSEL=1; // 1 => AD1ANN1
//    AD1CH0CONbits.SAMC=0; // 0 => 0.5Tad is sampling time
    AD1CH0CONbits.SAMC=1; // 0 => 2.5Tad is sampling time

    AD1CONbits.MODE=0b10; // 0b10 => ADC is on
    AD1CONbits.ON=1;
    while (!AD1CONbits.ADRDY)
    {
        Nop();
    }

    IPC18bits.AD1CH0IP=4;
    IFS4bits.AD1CH0IF=0;
//    IEC4bits.AD1CH0IE=1;
}

// SCCP1 is DAC1's sampling rate
static void SCCP1Init(void)
{
    // Configure pin RD2/RP51/OCM2
//    RPOR12bits.RP51R=24; // 25 = OCM1 (DS70005539C Table 11-13 pp480-481
//    ASELDbits.ANSD2=0; // No analogue function on RD2
//    TRISDbits.TRISD2=0;
    
    CCP1TMR=0;
    CCP1PR=CLKGEN12_FREQ/SCCP1_FREQ-1;
    CCP1RAbits.CMPA=0;
    CCP1RBbits.CMPB=CLKGEN12_FREQ/SCCP1_FREQ/2U;
    CCP1CON1bits.CLKSEL=0b001; // 0b001 => CLKGEN12
    CCP1CON2bits.OCAEN=1; // 1 => Enable output
    CCP1CON1bits.MOD=0b0101; // 0b0101 => Dual Edge Compare mode ? Buffered
    
    IPC6bits.CCT1IP=4;
    IFS1bits.CCT1IF=0;
//    IEC1bits.CCT1IE=1;    
    
    IPC6bits.CCP1IP=4;
    IFS1bits.CCP1IF=0;
//    IEC1bits.CCP1IE=1;    
    
//    CCP1CON1bits.ON=1; // Only enable when we have DAC data ready to send
}

static void DAC1Init(void)
{
    // Configure pin RA1/RP2/DACOUT1
    RPOR0bits.RP2R=0;
    ANSELAbits.ANSELA1=1;
    TRISAbits.TRISA1=1;
    CNPUAbits.CNPUA1=0;
    CNPDAbits.CNPDA1=0;
    
    DAC1DATbits.DACDAT=0x800;
    DAC1CONbits.DACOEN=1;
    DAC1CONbits.DACEN=1;
    
    DACCTRL1bits.ON=1;
}

static void DMAInit(void)
{
    // DMA in MCC is too limited to use
    DMACONbits.PRIORITY=1; // 1 => Round robin
    DMACONbits.ON=1;
    
    DMALOW=0;
    DMAHIGH=0xFFFF;
}

static void DMACh1Init(void)
{
    // DMA1 is for ADC1Ch0
    DMA1CH=0;
    DMA1STAT=0;
    DMA1CHbits.DONEEN=1;
    DMA1CHbits.HALFEN=1;
//    DMA1CHbits.SIZE=0b01; // 0b01 => 16 bits
    DMA1CHbits.SIZE=0b10; // 0b10 => 32 bits
    DMA1CHbits.RELOADS=1;
    DMA1CHbits.RELOADD=1;
    DMA1CHbits.RELOADC=1;
    DMA1CHbits.SAMODE=0b00; // 0b00 => DMAxSRC register remains unchanged after a transfer completion
    DMA1CHbits.DAMODE=0b01; // 0b01 => DMAxDST register is incremented based on SIZE[1:0] after a transfer completion
    DMA1CHbits.TRMODE=0b01; // 0b01 => Repeated One-Shot
//    DMA1SELbits.CHSEL=0x2B; // 0x2B is ADC1AN0 70005539C pp635-637 table 13-2 // *** Note that the ADC triggers multiple times for some reason, so use the CCP that's triggering ADC instead
    DMA1SELbits.CHSEL=0x19; // 0x19 is SCCP2 70005539C pp635-637 table 13-2
    DMA1SRCbits.SADDR=(size_t)&AD1CH0DATA;
    DMA1DSTbits.DADDR=(size_t)&_an32ADC1Ch0Samples[0];
    DMA1CNTbits.CNT=ADC1CH0_NUM_SAMPLES;

    IFS2bits.DMA1IF=0;
    IPC9bits.DMA1IP=1;
    IEC2bits.DMA1IE=1;

    DMA1CHbits.CHEN=1;
}

static void DMACh2Init(void)
{
    // DMA2 is for DAC1
    DMA2CH=0;
    DMA2STAT=0;
    DMA2CHbits.DONEEN=1;
    DMA2CHbits.HALFEN=1;
    DMA2CHbits.SIZE=0b01; // 0b01 => 16 bits
//    DMA2CHbits.SIZE=0b10; // 0b10 => 32 bits
    DMA2CHbits.RELOADS=1;
    DMA2CHbits.RELOADD=1;
    DMA2CHbits.RELOADC=1;
    DMA2CHbits.SAMODE=0b01; // 0b01 => DMAxSRC register is incremented based on SIZE[1:0] after a transfer completion
    DMA2CHbits.DAMODE=0b00; // 0b00 => DMAxDST register remains unchanged after a transfer completion
    DMA2CHbits.TRMODE=0b01; // 0b01 => Repeated One-Shot
    DMA2SELbits.CHSEL=0x18; // 0x18 is SCCP1 70005539C pp635-637 table 13-2
    DMA2SRCbits.SADDR=(size_t)&_au16DAC1Samples[0];
    DMA2DSTbits.DADDR=((size_t)&DAC1DAT) + sizeof(uint16_t); // DACDAT bitfield is top half of DAC1DAT 32 bit word
    DMA2CNTbits.CNT=DAC1_NUM_SAMPLES;

    IFS2bits.DMA2IF=0;
    IPC9bits.DMA2IP=1;
    IEC2bits.DMA2IE=0;

    DMA2CHbits.CHEN=1;
}

// SCCP3 is a timer to flag updating of the display
static void SCCP3Init(void)
{
    CCP3TMR=0;
    CCP3PR=CLKGEN12_FREQ/SCCP3_FREQ-1;
    CCP3CON1bits.CLKSEL=0b001; // 0b001 => CLKGEN12
    CCP3CON1bits.T32=1; // 1 => 32 bit timer
    CCP3CON1bits.MOD=0b0000; // 0b0000 => Timer mode
    
    IPC6bits.CCT1IP=4;
    IFS1bits.CCT1IF=0;
//    IEC1bits.CCT1IE=1;    
    
    IPC6bits.CCP3IP=4;
    IFS1bits.CCP3IF=0;
//    IEC1bits.CCP3IE=1;    
    
    CCP3CON1bits.ON=1; // Only enable when we have DAC data ready to send
}

int main(void)
{
    uint32_t u32RCONSave=RCON;
    
    u32RCONSave=RCON;
    RCON=0;
        
    ClkInit();
    GPIOInit();
    DMAInit();
    U1Init();
    I2C1Init();
    PMUInit();

    printf("\r\n\nRCON = 0x%08lX",u32RCONSave);        
    
    CLC1Init();
    CLC2Init();
    
    DAC1Init();
//    OA1Init(); // Not used
    OA2Init();
    
    LCDInit();

    // ADC DMA & CCP
    DMACh1Init();
    ADC1Init();
    SCCP2Init();

    // DAC DMA & CCP
    DMACh2Init();
    SCCP1Init();
    
    // Periodic timer to update the LCD display
    SCCP3Init();
    
    while (1)
    {
        if (_bFrequencyChanged)
        {
            char sz[20];
            
            SetFrequency(_nFrequency);
            LCDCursor(0,0);
            sprintf(sz,"%dkHz ",_nFrequency/1000);
            LCDPutS(sz);
            
            _bFrequencyChanged=false;
        }
        
        // Periodic time to update the display
        if (IFS1bits.CCT3IF)
        {
            static uint32_t u32AGCLast=0;
            uint32_t u32AGC=GetAGC();
            
            // Only update LCD if it's changed to reduce noise from the I2C bus
            if (u32AGC!=u32AGCLast)
            {
                char sz[20];

                LCDCursor(0,1);
                sprintf(sz,"AGC %4lu",(uint32_t)GetAGC());
                LCDPutS(sz);
                
                u32AGCLast=u32AGC;
            }
            IFS1bits.CCT3IF=0;
        }
        Idle();
    }
    return 0;
}
