typedef struct
{
    float fIP0Hist;      // I Z^-1 history Phase 0
    float fIP1Hist;      // I Z^-1 history Phase 1
    float fISOut;   // I filter output from Phase 0
    float fQP0Hist;      // Q Z^-1 history Phase 0
    float fQP1Hist;      // Q Z^-1 history Phase 1
    float fQSOut;   // Q filter output from Phase 0
} CIC_IQ_HISTORY_STRUCT;

extern float LOMixDecimateBuffer(float fLOPhase,float fLOPhaseInc,int32_t *panADCSamples,CIC_IQ_HISTORY_STRUCT *_paciciqhs,float fScaleMultiplier);
