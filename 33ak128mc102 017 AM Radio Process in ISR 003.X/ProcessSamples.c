#include <xc.h>
#include <math.h>

#include "clock.h"
#include "main.h"
#include "OptimisedDSP.h"
#include "Debug.h"

static float _fLOPhase=0.0;
static float _fLOPhaseInc=0.0;
static float _fAGCGain=1.0F; // Starting point

uint16_t _au16DAC1Samples[DAC1_NUM_SAMPLES];
CIC_IQ_HISTORY_STRUCT _aciciqhs[LOG2_DECIMATION_RATE];

void SetFrequency(float fFrequency)
{
    _fLOPhaseInc=(float)(PI_MUL_2 * fFrequency / ADC1CH0_SAMPLE_FREQ);
}

float GetAGC(void)
{
    return _fAGCGain;
}

static void __attribute__((optimize("-O3"))) QuadratureFIRFilter(float fISample,float fQSample,float *pfFilteredISample,float *pfFilteredQSample)
{
    static float afISampleHistory[NUM_FIR_FILTER_COEFFS];
    static float afQSampleHistory[NUM_FIR_FILTER_COEFFS];    
    // 6kHz filter for AM, from CIC_compensation_filter.m, M = 3, R = 128, N = 1, Fs = 4,000,000, Fp = 6000, FIR order = 15
    static const float _afFilterCoeffs[NUM_FIR_FILTER_COEFFS]=
    {
        -7.9342e-07F,
        0.0068218F,
        0.010264F,
        -0.018895F,
        -0.061575F,
        -0.011419F,
        0.18922F,
        0.39652F,
        0.39652F,
        0.18922F,
        -0.011419F,
        -0.061575F,
        -0.018895F,
        0.010264F,
        0.0068218F,
        -7.9342e-07F
    };
    float const *pfFilterCoeff=_afFilterCoeffs;
    float *pfISample=afISampleHistory;
    float *pfQSample=afQSampleHistory;
    float fIFilteredSample=0.0F;
    float fQFilteredSample=0.0F;

    // First time in, the coeffs are multiplied by the latest; 800ns with -O3
    // This could be optimised by implementing a circular buffer rather than
    // moving samples within the buffer
    for (uint8_t u8=0;u8<NUM_FIR_FILTER_COEFFS;u8++)
    {
        float fFilterCoeff = *pfFilterCoeff++;
        float fINext=*pfISample; // Save the next I sample from history, or else it'll be overwritten
        float fQNext=*pfQSample; // Save the next Q sample from history, or else it'll be overwritten

        fIFilteredSample += fISample * fFilterCoeff;
        fQFilteredSample += fQSample * fFilterCoeff;

        *pfISample++=fISample; // Stick our latest pair of samples into the downsample history array
        *pfQSample++=fQSample; // Stick our latest pair of samples into the downsample history array

        fISample=fINext; // get those next latest pair of samples we saved earlier
        fQSample=fQNext; // get those next latest pair of samples we saved earlier
    }
    
    *pfFilteredISample=fIFilteredSample;
    *pfFilteredQSample=fQFilteredSample;
}

static float __attribute__((optimize("-O3"))) DemodulateAndAGC(float fISample,float fQSample)
{
    const float fAGCSetpoint=2000.0;
    static float fOutput = fAGCSetpoint; // Needs to be static as it's fed back
    float fAGCError;
    float const fAGCErrorGainTooLow=0.0000005; // Slower AGC when input signal too low
    float const fAGCErrorGainTooHigh=0.00001; // Faster AGC when input signal too large
    float fAGCErrorGain;
    float fDemodulatedResult;
 
    fDemodulatedResult = sqrt(fISample*fISample+fQSample*fQSample); // * fAGCGain;
    fAGCError = fAGCSetpoint - fabs(fOutput);
    if (fAGCError<0.0)
    {
        fAGCErrorGain = fAGCErrorGainTooHigh; // Fast when signal too high
    }
    else
    {
        fAGCErrorGain = fAGCErrorGainTooLow; // Slow when signal too low
    }
    _fAGCGain += fAGCErrorGain * fAGCError;
    fOutput = fDemodulatedResult * _fAGCGain;
    
    return fOutput;
}

void __attribute__((optimize("-O3"))) ProcessSamples(int32_t *pn32Samples)
{
    static uint16_t *pu16DAC=_au16DAC1Samples;
    const uint16_t *pu16DACEnd=_au16DAC1Samples+DAC1_NUM_SAMPLES;
    const uint16_t *pu16DACHalf=_au16DAC1Samples+DAC1_NUM_SAMPLES/2U;
    float fFilteredISample;
    float fFilteredQSample;
    float fDemodulatedOutput;
        
    DBG3Set();
    _fLOPhase=LOMixDecimateBuffer(_fLOPhase,_fLOPhaseInc,pn32Samples,_aciciqhs,CIC_SCALE_MULTIPLIER);
//    DBG3Clear();

    // Keep _fLOPhase within 2 * PI to avoid mantissa truncation and excessive inaccuracies when adding relatively small incremental values
    if (_fLOPhase >= PI_MUL_2)
    {
        float fIntSub=((int)(_fLOPhase * ( 1.0/PI_MUL_2 ) ) ) * PI_MUL_2;
        _fLOPhase -= fIntSub;
    }              
    
    // FIR compensation filter for CIC
//    DBG1Set();
    QuadratureFIRFilter(_aciciqhs[6].fISOut,_aciciqhs[6].fQSOut,&fFilteredISample,&fFilteredQSample);
//    DBG1Clear();
//    DBG3Clear();

    // Demodulate & AGC
//    DBG2Set();
    fDemodulatedOutput = DemodulateAndAGC(fFilteredISample,fFilteredQSample);
//    DBG2Clear();

//    DAC1DATbits.DACDAT = 0x800 - ((int32_t)(fDemodulatedOutput) >> 1U); 
    *pu16DAC++=fDemodulatedOutput;
    if (pu16DAC==pu16DACHalf)
    {
        if (!CCP1CON1bits.ON)
        {
            CCP1CON1bits.ON=1;
        }
    }
    else
    {
        if (pu16DAC==pu16DACEnd)
        {
            pu16DAC=_au16DAC1Samples;
        }
    }
    DBG3Clear();
}

