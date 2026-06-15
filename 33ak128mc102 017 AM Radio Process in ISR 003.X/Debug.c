// Debug support objects & fucntions

#include <xc.h>
#include <stdio.h>

#include "Debug.h"

// PMU storage
uint32_t _u32CPUCycles=0U;
uint32_t _u32FPUWriteStall=0U;
uint32_t _u32FPUInstructionStall=0U;
uint32_t _u32FPUReadStall=0U;
uint32_t _u32WriteStageStall=0U;
uint32_t _u32AddressStageHazard=0U;
uint32_t _u32AddressStageReadStall=0U;
uint32_t _u32AddressStageStall=0U;

// Unions & structures to aid debugging because MPLAB X debugger won't display FPU register values in IEEE 754 format
volatile union
{
    float af[8];
    struct
    {
        float fLOPhase;
        float fLOPhaseInc;
        float fISample;
        float fQSample;
        float fTemp0;
        float fTemp1;
        float fTemp2;
        float fThree;
    } s;
} _uDebugFPUMinRegs;

volatile union
{
    float af[24];
    struct
    {
        float fIS0P0Hist;
        float fIS0P1Hist;
        float fIS0Out;
        float fQS0P0Hist;
        float fQS0P1Hist;
        float fQS0Out;
        
        float fIS1P0Hist;
        float fIS1P1Hist;
        float fIS1Out;
        float fQS1P0Hist;
        float fQS1P1Hist;
        float fQS1Out;
        
        float fIS2P0Hist;
        float fIS2P1Hist;
        float fIS2Out;
        float fQS2P0Hist;
        float fQS2P1Hist;
        float fQS2Out;
        
        float fIS3P0Hist;
        float fIS3P1Hist;
        float fIS3Out;
        float fQS3P0Hist;
        float fQS3P1Hist;
        float fQS3Out;
    } s;    
} _uDebugFPUCICRegs;

volatile float _afDebug[2048]; // Somewhere to stream values for debug analysis

// Debug functions for assembler functions to call
void PrintFloat(float f)
{
    printf("%f ",f);
}

void PrintU32(uint32_t u32)
{
    printf("0x%08lX ",u32);
}

void PrintNewLine(void)
{
    printf("\r\n");
}
