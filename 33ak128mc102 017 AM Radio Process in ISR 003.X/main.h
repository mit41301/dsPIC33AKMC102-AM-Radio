#define ADC1CH0_SAMPLE_FREQ (4000000.0)
#define DAC1_SAMPLE_FREQ (ADC1CH0_SAMPLE_FREQ/128U)
#define SCCP2_FREQ (ADC1CH0_SAMPLE_FREQ)
#define SCCP1_FREQ (DAC1_SAMPLE_FREQ)

#define SCCP3_FREQ (10U) // This is the LCD display's update rate in Hz

#define PI_MUL_2 (M_PI * 2.0)
#define CIC_ORDER (3U)
#define DECIMATION_RATE (128U)
#define LOG2_DECIMATION_RATE (7U)
#define CIC_SCALE_MULTIPLIER (1.0 / (1ULL << (CIC_ORDER * LOG2_DECIMATION_RATE)))
#define NUM_FIR_FILTER_COEFFS (16U)  

#define ADC1CH0_NUM_SAMPLES (256U) // Twice the decimation rate, bottom half ping, top half pong
#define DAC1_NUM_SAMPLES (256U) // Bottom half ping, top half pong
