#define DBG0Set() do {LATDbits.LATD0=1;} while (0) // TP42
#define DBG1Set() do {LATDbits.LATD1=1;} while (0) // TP40
#define DBG2Set() do {LATDbits.LATD2=1;} while (0) // TP45
#define DBG3Set() do {LATDbits.LATD3=1;} while (0) // TP47

#define DBG0Clear() do {LATDbits.LATD0=0;} while (0)
#define DBG1Clear() do {LATDbits.LATD1=0;} while (0)
#define DBG2Clear() do {LATDbits.LATD2=0;} while (0)
#define DBG3Clear() do {LATDbits.LATD3=0;} while (0)

#define DBG0Toggle() do {LATD ^= 1U << 0U;} while (0)
#define DBG1Toggle() do {LATD ^= 1U << 1U;} while (0)
#define DBG2Toggle() do {LATD ^= 1U << 2U;} while (0)
#define DBG3Toggle() do {LATD ^= 1U << 3U;} while (0)

// PMU storage
extern uint32_t _u32CPUCycles;
extern uint32_t _u32FPUWriteStall;
extern uint32_t _u32FPUInstructionStall;
extern uint32_t _u32FPUReadStall;
extern uint32_t _u32WriteStageStall;
extern uint32_t _u32AddressStageHazard;
extern uint32_t _u32AddressStageReadStall;
extern uint32_t _u32AddressStageStall;
