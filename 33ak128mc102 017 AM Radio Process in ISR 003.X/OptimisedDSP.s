    .text
    
    .equ pn32ADCSample,W0              ; Pointer to signed ADC samples
    .equ paciciqhs,W1                  ; Pointer to polyphase non-recursive CIC history structures
    .equ wfScaleMultiplier,W11         ; Storage for the scale muliplier to apply to the I & Q results 1 / 2 ^ (CICOrder * Log2(DecimationRate)) = 1 / 2 ^ 21 = 1 / 4194304
    .equ wTemp0,W4                     ; Very short term floating point temp register
    .equ wTemp1,W5                     ; Very short term floating point temp register
    .equ wTemp2,W6                     ; Very short term floating point temp register
    .equ wADCOffset,W7                 ; Storage for constant to adjust an unsigned ADC sample to a signed sample

    .equ fLOPhase,F0
    .equ fLOPhaseInc,F1
    .equ fISample,F2                   ; In-phase downmix sample; also temporarily used for ILO and transferring between Phase1 CIC stage and next CIC stage.
    .equ fQSample,F3                   ; Quadrature phase downmix sample
    .equ fTemp0,F4                     ; Very short term floating point temp register
    .equ fTemp1,F5                     ; Short term floating point temp register
    .equ fTemp2,F6                     ; Short term floating point temp register
    .equ fThree,F7                     ; Floating point register with contant 3.0F
    
    
    ; First four stages of the polyphase CIC are held as long as possible to avoid RAM access stalls and delays
    .equ fIS0P0Hist,F8
    .equ fIS0P1Hist,F9
    .equ fIS0Out,   F10
    .equ fQS0P0Hist,F11
    .equ fQS0P1Hist,F12
    .equ fQS0Out,   F13
    
    .equ fIS1P0Hist,F14
    .equ fIS1P1Hist,F15
    .equ fIS1Out,   F16
    .equ fQS1P0Hist,F17
    .equ fQS1P1Hist,F18
    .equ fQS1Out,   F19
    
    .equ fIS2P0Hist,F20
    .equ fIS2P1Hist,F21
    .equ fIS2Out,   F22
    .equ fQS2P0Hist,F23
    .equ fQS2P1Hist,F24
    .equ fQS2Out,   F25
    
    .equ fIS3P0Hist,F26
    .equ fIS3P1Hist,F27
    .equ fIS3Out,   F28
    .equ fQS3P0Hist,F29
    .equ fQS3P1Hist,F30
    .equ fQS3Out,   F31
    
    ; Last three stages of the polyphase CIC are loaded in as required
    .equ fIS4P0Hist,F8
    .equ fIS4P1Hist,F9
    .equ fIS4Out,   F10
    .equ fQS4P0Hist,F11
    .equ fQS4P1Hist,F12
    .equ fQS4Out,   F13
    
    .equ fIS5P0Hist,F14
    .equ fIS5P1Hist,F15
    .equ fIS5Out,   F16
    .equ fQS5P0Hist,F17
    .equ fQS5P1Hist,F18
    .equ fQS5Out,   F19
    
    .equ fIS6P0Hist,F20
    .equ fIS6P1Hist,F21
    .equ fIS6Out,   F22
    .equ fQS6P0Hist,F23
    .equ fQS6P1Hist,F24
    .equ fQS6Out,   F25
    
    ; Word offsets into CIC history array
    .equ OffsetIS0P0Hist,  0
    .equ OffsetIS0P1Hist,  1
    .equ OffsetIS0Out,     2
    .equ OffsetQS0P0Hist,  3
    .equ OffsetQS0P1Hist,  4
    .equ OffsetQS0Out,     5
    
    .equ OffsetIS1P0Hist,  6
    .equ OffsetIS1P1Hist,  7
    .equ OffsetIS1Out,     8
    .equ OffsetQS1P0Hist,  9
    .equ OffsetQS1P1Hist, 10
    .equ OffsetQS1Out,    11
    
    .equ OffsetIS2P0Hist, 12
    .equ OffsetIS2P1Hist, 13
    .equ OffsetIS2Out,    14
    .equ OffsetQS2P0Hist, 15
    .equ OffsetQS2P1Hist, 16
    .equ OffsetQS2Out,    17
    
    .equ OffsetIS3P0Hist, 18
    .equ OffsetIS3P1Hist, 19
    .equ OffsetIS3Out,    20
    .equ OffsetQS3P0Hist, 21
    .equ OffsetQS3P1Hist, 22
    .equ OffsetQS3Out,    23
    
    .equ OffsetIS4P0Hist, 24
    .equ OffsetIS4P1Hist, 25
    .equ OffsetIS4Out,    26
    .equ OffsetQS4P0Hist, 27
    .equ OffsetQS4P1Hist, 28
    .equ OffsetQS4Out,    29
    
    .equ OffsetIS5P0Hist, 30
    .equ OffsetIS5P1Hist, 31
    .equ OffsetIS5Out,    32
    .equ OffsetQS5P0Hist, 33
    .equ OffsetQS5P1Hist, 34
    .equ OffsetQS5Out,    35
    
    .equ OffsetIS6P0Hist, 36
    .equ OffsetIS6P1Hist, 37
    .equ OffsetIS6Out,    38
    .equ OffsetQS6P0Hist, 39
    .equ OffsetQS6P1Hist, 40
    .equ OffsetQS6Out,    41
        
    
    .macro ResetPerfCounters
	bset.b HPCCON+1,#5 
    .endm
    
    .macro Copy32 Src,Dst
	mov.l \Src,W13
	mov.l W13,\Dst
    .endm
    
    .macro CopyPerfCounters
	Copy32 HPCCNTL0,__u32CPUCycles
	Copy32 HPCCNTL1,__u32FPUWriteStall
	Copy32 HPCCNTL2,__u32FPUInstructionStall
	Copy32 HPCCNTL3,__u32FPUReadStall
	Copy32 HPCCNTL4,__u32WriteStageStall
	Copy32 HPCCNTL5,__u32AddressStageHazard
	Copy32 HPCCNTL6,__u32AddressStageReadStall
	Copy32 HPCCNTL7,__u32AddressStageStall    
    .endm
    
    .macro PushXCScratchRegisters
	push W0
	push W1
	push W2
	push W3
	push W4
	push W5
	push W6
	push W7

	push.l F0
	push.l F1
	push.l F2
	push.l F3
	push.l F4
	push.l F5
	push.l F6
	push.l F7    
    .endm

    .macro PopXCScratchRegisters
	pop.l F7
	pop.l F6
	pop.l F5
	pop.l F4
	pop.l F3
	pop.l F2
	pop.l F1
	pop.l F0

	pop W7
	pop W6
	pop W5
	pop W4
	pop W3
	pop W2
	pop W1
	pop W0    
    .endm
    
    .macro DebugGPIO0Set
	bset.b LATD,#0
    .endm
    
    .macro DebugGPIO0Clear
	bclr.b LATD,#0
    .endm
    
    .macro DebugFPUCICRegs ; Clobbers W13
	mov.l #__uDebugFPUCICRegs,W13
	mov.l F8,[W13++]
	mov.l F9,[W13++]
	mov.l F10,[W13++]
	mov.l F11,[W13++]
	mov.l F12,[W13++]
	mov.l F13,[W13++]
	mov.l F14,[W13++]
	mov.l F15,[W13++]
	mov.l F16,[W13++]
	mov.l F17,[W13++]
	mov.l F18,[W13++]
	mov.l F19,[W13++]
	mov.l F20,[W13++]
	mov.l F21,[W13++]
	mov.l F22,[W13++]
	mov.l F23,[W13++]
	mov.l F24,[W13++]
	mov.l F25,[W13++]
	mov.l F26,[W13++]
	mov.l F27,[W13++]
	mov.l F28,[W13++]
	mov.l F29,[W13++]
	mov.l F30,[W13++]
	mov.l F31,[W13++]
    .endm
    
    .macro DebugFPUMinRegs ; Clobbers W13
	mov.l #__uDebugFPUMinRegs,W13
	mov.l F0,[W13++]
	mov.l F1,[W13++]
	mov.l F2,[W13++]
	mov.l F3,[W13++]
	mov.l F4,[W13++]
	mov.l F5,[W13++]
	mov.l F6,[W13++]
	mov.l F7,[W13++]
    .endm
    
    .macro DebugFPUAllRegs
	DebugFPUMinRegs
	DebugFPUCICRegs
    .endm
    
    .macro DebugBreak
	break
    .endm
    
    .macro DebugSaveFPURegInit
        mov #__afDebug,W12; ; Debug sample output
    .endm
    
    .macro DebugSaveFPUReg f
	mov.l \f,[W12++]
    .endm
    
    .macro Stage0Push
	push.l fIS0P0Hist
	push.l fIS0P1Hist
	push.l fIS0Out
	push.l fQS0P0Hist
	push.l fQS0P1Hist
	push.l fQS0Out
    .endm
    
    .macro Stage0Pop
	pop.l fQS0Out
	pop.l fQS0P1Hist
	pop.l fQS0P0Hist
	pop.l fIS0Out
	pop.l fIS0P1Hist
	pop.l fIS0P0Hist    
    .endm
    
    .macro Stage1Push
	push.l fIS1P0Hist
	push.l fIS1P1Hist
	push.l fIS1Out
	push.l fQS1P0Hist
	push.l fQS1P1Hist
	push.l fQS1Out
    .endm
    
    .macro Stage1Pop
	pop.l fQS1Out
	pop.l fQS1P1Hist
	pop.l fQS1P0Hist
	pop.l fIS1Out
	pop.l fIS1P1Hist
	pop.l fIS1P0Hist    
    .endm
    
    .macro Stage2Push
	push.l fIS2P0Hist
	push.l fIS2P1Hist
	push.l fIS2Out
	push.l fQS2P0Hist
	push.l fQS2P1Hist
	push.l fQS2Out
    .endm
    
    .macro Stage2Pop
	pop.l fQS2Out
	pop.l fQS2P1Hist
	pop.l fQS2P0Hist
	pop.l fIS2Out
	pop.l fIS2P1Hist
	pop.l fIS2P0Hist    
    .endm
    
    .macro Stage0FPULoad
	mov.l [paciciqhs + 4 * OffsetIS0P0Hist],fIS0P0Hist    ; Stage 0 Phase 0 in-phase CIC history
	mov.l [paciciqhs + 4 * OffsetIS0P1Hist],fIS0P1Hist    ; Stage 0 Phase 1 in-phase CIC history
	mov.l [paciciqhs + 4 * OffsetIS0Out],   fIS0Out       ; Stage 0 Phase 0 in-phase ouptut
	mov.l [paciciqhs + 4 * OffsetQS0P0Hist],fQS0P0Hist    ; Stage 0 Phase 0 quadrature CIC history
	mov.l [paciciqhs + 4 * OffsetQS0P1Hist],fQS0P1Hist    ; Stage 0 Phase 1 quadrature CIC history
	mov.l [paciciqhs + 4 * OffsetQS0Out],   fQS0Out       ; Stage 0 Phase 0 quadrature ouptut
    .endm

    .macro Stage1FPULoad
	mov.l [paciciqhs + 4 * OffsetIS1P0Hist],fIS1P0Hist    ; Stage 1 Phase 0 in-phase CIC history
	mov.l [paciciqhs + 4 * OffsetIS1P1Hist],fIS1P1Hist    ; Stage 1 Phase 1 in-phase CIC history
	mov.l [paciciqhs + 4 * OffsetIS1Out],   fIS1Out       ; Stage 1 Phase 0 in-phase ouptut
	mov.l [paciciqhs + 4 * OffsetQS1P0Hist],fQS1P0Hist    ; Stage 1 Phase 0 quadrature CIC history
	mov.l [paciciqhs + 4 * OffsetQS1P1Hist],fQS1P1Hist    ; Stage 1 Phase 1 quadrature CIC history
	mov.l [paciciqhs + 4 * OffsetQS1Out],   fQS1Out       ; Stage 1 Phase 0 quadrature ouptut
    .endm

    .macro Stage2FPULoad
	mov.l [paciciqhs + 4 * OffsetIS2P0Hist],fIS2P0Hist    ; Stage 2 Phase 0 in-phase CIC history
	mov.l [paciciqhs + 4 * OffsetIS2P1Hist],fIS2P1Hist    ; Stage 2 Phase 1 in-phase CIC history
	mov.l [paciciqhs + 4 * OffsetIS2Out],   fIS2Out       ; Stage 2 Phase 0 in-phase ouptut
	mov.l [paciciqhs + 4 * OffsetQS2P0Hist],fQS2P0Hist    ; Stage 2 Phase 0 quadrature CIC history
	mov.l [paciciqhs + 4 * OffsetQS2P1Hist],fQS2P1Hist    ; Stage 2 Phase 1 quadrature CIC history
	mov.l [paciciqhs + 4 * OffsetQS2Out],   fQS2Out       ; Stage 2 Phase 0 quadrature ouptut
    .endm

    .macro Stage3FPULoad
	mov.l [paciciqhs + 4 * OffsetIS3P0Hist],fIS3P0Hist    ; Stage 3 Phase 0 in-phase CIC history
	mov.l [paciciqhs + 4 * OffsetIS3P1Hist],fIS3P1Hist    ; Stage 3 Phase 1 in-phase CIC history
	mov.l [paciciqhs + 4 * OffsetIS3Out],   fIS3Out       ; Stage 3 Phase 0 in-phase ouptut
	mov.l [paciciqhs + 4 * OffsetQS3P0Hist],fQS3P0Hist    ; Stage 3 Phase 0 quadrature CIC history
	mov.l [paciciqhs + 4 * OffsetQS3P1Hist],fQS3P1Hist    ; Stage 3 Phase 1 quadrature CIC history
	mov.l [paciciqhs + 4 * OffsetQS3Out],   fQS3Out       ; Stage 3 Phase 0 quadrature ouptut
    .endm

    .macro Stage4FPULoad
	mov.l [paciciqhs + 4 * OffsetIS4P0Hist],fIS4P0Hist    ; Stage 4 Phase 0 in-phase CIC history
	mov.l [paciciqhs + 4 * OffsetIS4P1Hist],fIS4P1Hist    ; Stage 4 Phase 1 in-phase CIC history
	mov.l [paciciqhs + 4 * OffsetIS4Out],   fIS4Out       ; Stage 4 Phase 0 in-phase ouptut
	mov.l [paciciqhs + 4 * OffsetQS4P0Hist],fQS4P0Hist    ; Stage 4 Phase 0 quadrature CIC history
	mov.l [paciciqhs + 4 * OffsetQS4P1Hist],fQS4P1Hist    ; Stage 4 Phase 1 quadrature CIC history
	mov.l [paciciqhs + 4 * OffsetQS4Out],   fQS4Out       ; Stage 4 Phase 0 quadrature ouptut
    .endm

    .macro Stage5FPULoad
	mov.l [paciciqhs + 4 * OffsetIS5P0Hist],fIS5P0Hist    ; Stage 5 Phase 0 in-phase CIC history
	mov.l [paciciqhs + 4 * OffsetIS5P1Hist],fIS5P1Hist    ; Stage 5 Phase 1 in-phase CIC history
	mov.l [paciciqhs + 4 * OffsetIS5Out],   fIS5Out       ; Stage 5 Phase 0 in-phase ouptut
	mov.l [paciciqhs + 4 * OffsetQS5P0Hist],fQS5P0Hist    ; Stage 5 Phase 0 quadrature CIC history
	mov.l [paciciqhs + 4 * OffsetQS5P1Hist],fQS5P1Hist    ; Stage 5 Phase 1 quadrature CIC history
	mov.l [paciciqhs + 4 * OffsetQS5Out],   fQS5Out       ; Stage 5 Phase 0 quadrature ouptut
    .endm

    .macro Stage6FPULoad
	mov.l [paciciqhs + 4 * OffsetIS6P0Hist],fIS6P0Hist    ; Stage 6 Phase 0 in-phase CIC history
	mov.l [paciciqhs + 4 * OffsetIS6P1Hist],fIS6P1Hist    ; Stage 6 Phase 1 in-phase CIC history
	mov.l [paciciqhs + 4 * OffsetIS6Out],   fIS6Out       ; Stage 6 Phase 0 in-phase ouptut
	mov.l [paciciqhs + 4 * OffsetQS6P0Hist],fQS6P0Hist    ; Stage 6 Phase 0 quadrature CIC history
	mov.l [paciciqhs + 4 * OffsetQS6P1Hist],fQS6P1Hist    ; Stage 6 Phase 1 quadrature CIC history
	mov.l [paciciqhs + 4 * OffsetQS6Out],   fQS6Out       ; Stage 6 Phase 0 quadrature ouptut
    .endm


    
    
    .Macro Stage0FPUSave
	mov.l fIS0P0Hist,[paciciqhs + 4 * OffsetIS0P0Hist]    ; Stage 0 Phase 0 in-phase CIC history    
	mov.l fIS0P1Hist,[paciciqhs + 4 * OffsetIS0P1Hist]    ; Stage 0 Phase 1 in-phase CIC history
	mov.l fIS0Out,   [paciciqhs + 4 * OffsetIS0Out]       ; Stage 0 in-phase ouptut
	mov.l fQS0P0Hist,[paciciqhs + 4 * OffsetQS0P0Hist]    ; Stage 0 Phase 0 quadrature CIC history
	mov.l fQS0P1Hist,[paciciqhs + 4 * OffsetQS0P1Hist]    ; Stage 0 Phase 1 quadrature CIC history
	mov.l fQS0Out,   [paciciqhs + 4 * OffsetQS0Out]       ; Stage 0 quadrature ouptut    
    .endm
    
    .Macro Stage1FPUSave
	mov.l fIS1P0Hist,[paciciqhs + 4 * OffsetIS1P0Hist]    ; Stage 1 Phase 0 in-phase CIC history    
	mov.l fIS1P1Hist,[paciciqhs + 4 * OffsetIS1P1Hist]    ; Stage 1 Phase 1 in-phase CIC history
	mov.l fIS1Out,   [paciciqhs + 4 * OffsetIS1Out]       ; Stage 1 in-phase ouptut
	mov.l fQS1P0Hist,[paciciqhs + 4 * OffsetQS1P0Hist]    ; Stage 1 Phase 0 quadrature CIC history
	mov.l fQS1P1Hist,[paciciqhs + 4 * OffsetQS1P1Hist]    ; Stage 1 Phase 1 quadrature CIC history
	mov.l fQS1Out,   [paciciqhs + 4 * OffsetQS1Out]       ; Stage 1 quadrature ouptut    
    .endm
    
    .Macro Stage2FPUSave
	mov.l fIS2P0Hist,[paciciqhs + 4 * OffsetIS2P0Hist]    ; Stage 2 Phase 0 in-phase CIC history    
	mov.l fIS2P1Hist,[paciciqhs + 4 * OffsetIS2P1Hist]    ; Stage 2 Phase 1 in-phase CIC history
	mov.l fIS2Out,   [paciciqhs + 4 * OffsetIS2Out]       ; Stage 2 in-phase ouptut
	mov.l fQS2P0Hist,[paciciqhs + 4 * OffsetQS2P0Hist]    ; Stage 2 Phase 0 quadrature CIC history
	mov.l fQS2P1Hist,[paciciqhs + 4 * OffsetQS2P1Hist]    ; Stage 2 Phase 1 quadrature CIC history
	mov.l fQS2Out,   [paciciqhs + 4 * OffsetQS2Out]       ; Stage 2 quadrature ouptut    
    .endm
    
    .Macro Stage3FPUSave
	mov.l fIS3P0Hist,[paciciqhs + 4 * OffsetIS3P0Hist]    ; Stage 3 Phase 0 in-phase CIC history    
	mov.l fIS3P1Hist,[paciciqhs + 4 * OffsetIS3P1Hist]    ; Stage 3 Phase 1 in-phase CIC history
	mov.l fIS3Out,   [paciciqhs + 4 * OffsetIS3Out]       ; Stage 3 in-phase ouptut
	mov.l fQS3P0Hist,[paciciqhs + 4 * OffsetQS3P0Hist]    ; Stage 3 Phase 0 quadrature CIC history
	mov.l fQS3P1Hist,[paciciqhs + 4 * OffsetQS3P1Hist]    ; Stage 3 Phase 1 quadrature CIC history
	mov.l fQS3Out,   [paciciqhs + 4 * OffsetQS3Out]       ; Stage 3 quadrature ouptut    
    .endm
    
    .Macro Stage4FPUSave
	mov.l fIS4P0Hist,[paciciqhs + 4 * OffsetIS4P0Hist]    ; Stage 4 Phase 0 in-phase CIC history    
	mov.l fIS4P1Hist,[paciciqhs + 4 * OffsetIS4P1Hist]    ; Stage 4 Phase 1 in-phase CIC history
	mov.l fIS4Out,   [paciciqhs + 4 * OffsetIS4Out]       ; Stage 4 in-phase ouptut
	mov.l fQS4P0Hist,[paciciqhs + 4 * OffsetQS4P0Hist]    ; Stage 4 Phase 0 quadrature CIC history
	mov.l fQS4P1Hist,[paciciqhs + 4 * OffsetQS4P1Hist]    ; Stage 4 Phase 1 quadrature CIC history
	mov.l fQS4Out,   [paciciqhs + 4 * OffsetQS4Out]       ; Stage 4 quadrature ouptut    
    .endm
    
    .Macro Stage5FPUSave
	mov.l fIS5P0Hist,[paciciqhs + 4 * OffsetIS5P0Hist]    ; Stage 5 Phase 0 in-phase CIC history    
	mov.l fIS5P1Hist,[paciciqhs + 4 * OffsetIS5P1Hist]    ; Stage 5 Phase 1 in-phase CIC history
	mov.l fIS5Out,   [paciciqhs + 4 * OffsetIS5Out]       ; Stage 5 in-phase ouptut
	mov.l fQS5P0Hist,[paciciqhs + 4 * OffsetQS5P0Hist]    ; Stage 5 Phase 0 quadrature CIC history
	mov.l fQS5P1Hist,[paciciqhs + 4 * OffsetQS5P1Hist]    ; Stage 5 Phase 1 quadrature CIC history
	mov.l fQS5Out,   [paciciqhs + 4 * OffsetQS5Out]       ; Stage 5 quadrature ouptut    
    .endm
    
    .Macro Stage6FPUSave
	mov.l fIS6P0Hist,[paciciqhs + 4 * OffsetIS6P0Hist]    ; Stage 6 Phase 0 in-phase CIC history    
	mov.l fIS6P1Hist,[paciciqhs + 4 * OffsetIS6P1Hist]    ; Stage 6 Phase 1 in-phase CIC history
	mov.l fIS6Out,   [paciciqhs + 4 * OffsetIS6Out]       ; Stage 6 in-phase ouptut
	mov.l fQS6P0Hist,[paciciqhs + 4 * OffsetQS6P0Hist]    ; Stage 6 Phase 0 quadrature CIC history
	mov.l fQS6P1Hist,[paciciqhs + 4 * OffsetQS6P1Hist]    ; Stage 6 Phase 1 quadrature CIC history
	mov.l fQS6Out,   [paciciqhs + 4 * OffsetQS6Out]       ; Stage 6 quadrature ouptut    
    .endm
    
   
   
    
    ; *** GetNextDownmixedIQSamplePair
    ; Inputs...
    ;   pn32ADCSample: W register pointing to current ADC sample
    ;   fLOPhase: FPU register
    ;   fLOPhaseInc: FPU register
    ; Outputs...
    ;   fISample: FPU register, result = ADC sample * cos(fLOPhase)
    ;   fQSample: FPU register, result = ADC sample * sin(fLOPhase)
    ; Other functionality...
    ;   fLOPhase += fLOPhaseInc
    ;   pn32ADCSample++
    ;   fThree is assigned to 3.0F
    ; Clobbered...
    ;   fTemp0
    ; Notes...
    ;   Interim fILO/fQLO vlues use the same registers as fISample/fQSample outputs
    .macro GetNextDownmixedIQSamplePair pn32ADCSample,fLOPhase,fLOPhaseInc,fISample,fQSample
;	mov.l [\pn32ADCSample++],fTemp0            ; fTemp0 = sample (not yet converted to float)
	add.l wADCOffset,[\pn32ADCSample++],wTemp0 ; wTemp0 = signed sample (not yet converted to float)
	mov.l wTemp0,fTemp0                        ; fTemp0 = signed sample (not yet converted to float)
	cos.s \fLOPhase,\fISample                  ; fISample = cos(fLOPhase)
	li2f.s fTemp0,fTemp0                       ; fTemp0 = sample converted to float
	sin.s \fLOPhase,\fQSample                  ; fQSample = sin(fLOPhase)
	add.s \fLOPhase,\fLOPhaseInc,\fLOPhase     ; fLOPhase+=fLOPhaseInc
	mul.s fTemp0,\fISample,\fISample           ; fISample = sample * cos(fLOPhase)
	nop                                        ; *** mul FPU instruction stall
	nop                                        ; *** mul FPU instruction stall    
	mul.s fTemp0,\fQSample,\fQSample           ; fQSample = sample * sin(fLOPhase)
	nop                                        ; *** mul FPU instruction stall    
    .endm
    
    
    ; *** CICPolyphaseFastPhase0 fIIn,fQIn,fIP0Hist,fQP0Hist,fIOut,fQOut
    ; Implement 3 + z^-1 for both I and Q
    ; fIIn: floating point register holding in-phase Phase 0 sample
    ; fQIn: floating point register holding quadrature Phase 0 sample
    ; fIP0Hist: floating point register holding in-phase Phase 0 CIC History 
    ; fQP0Hist: floating point register holding quadrature Phase 0 CIC History 
    ; fIOut is floating point register holding Phase 0 in-phase downsample output
    ; fQOut is floating point register holding Phase 0 quadrature downsample output
    ; Input requirements...
    ;   fThree is floating point register already holding 3.0F floating point constant
    ; Output
    ;   fIOut: floating point register holding in-phase output
    ;   fQOut: floating point register holding quadrature output
    ; Other functionality
    ;   New I & Q processed samples are stored back into fIP0Hist & fQP0Hist
    ; Clobbered...
    ;   fTemp0
    .macro CICPolyphaseFastPhase0 fIIn,fQIn,fIP0Hist,fQP0Hist,fIOut,fQOut
	; In-phase
	mov.s \fIP0Hist,\fIOut                      ; fIOut = fIP0Hist
	mov.s \fIIn,\fIP0Hist                       ; fIP0Hist = fIIn
	mov.s \fQP0Hist,\fQOut                      ; fQOut = fQP0Hist ; ***** Moved from Quadrature section to reduce stalls
	mac.s fThree,\fIIn,\fIOut                   ; fIout += 3 * fIIn
;	nop                                        ; *** mac FPU instruction stall
;	nop                                        ; *** mac FPU instruction stall    
	
	; Quadrature
;	mov.s \fQP0Hist,\fQOut                      ; fQOut = fQP0Hist ; ***** Moved to In-phase section to reduce stalls
	mov.s \fQIn,\fQP0Hist                       ; fQP0Hist = fQIn
	mac.s fThree,\fQIn,\fQOut                   ; fQOut += 3 * fQIn
    .endm    
    
    ; *** CICPolyphaseFastPhase1AndAddPhase0 fIIn,fQIn,fIP0In,FIQ0In,fIP1Hist,fQP1Hist,fIOut,fQOut
    ; Implement 1 + 3z^-1 for both I and Q, and add in previous Phase 0 I & Q results
    ; fIIn: floating point register holding in-phase Phase 1 sample: can also be used as FIPOut
    ; fQIn: floating point register holding quadrature Phase 1 sample: can also be used as FQPOut
    ; fIP0In: floating point register holding in-phase Phase 0 downsample from Phase 0's output
    ; fQP0In: floating point register holding quadrature Phase 0 downsample from Phase 0's output
    ; fIP1Hist: floating point register holding in-phase Phase 1 CIC History 
    ; fQP1Hist: floating point register holding quadrature Phase 1 CIC History 
    ; fIOut is floating point register holding Phase 1 in-phase downsample output
    ; fQOut is floating point register holding Phase 1 quadrature downsample output
    ; Input requirements...
    ;   fThree is floating point register already holding 3.0F floating point constant
    ; Output
    ;   fIOut: floating point register holding in-phase output
    ;   fQOut: floating point register holding quadrature output
    ; Other functionality
    ;   New I & Q processed samples are stored back into fIP0Hist & fQP0Hist
    ; Clobbered...
    ;   fTemp0 (fTemp0 can be used for fQOut as fQOut is the last register to be asssigned)
    .macro CICPolyphaseFastPhase1AndAddPhase0 fIIn,fQIn,fIP0In,FIQ0In,fIP1Hist,fQP1Hist,fIOut,fQOut
	; In-phase
	mov.s \fIP1Hist,fTemp0                    ; fTemp0 = fIP1Hist
	mul.s \fIIn,fThree,\fIP1Hist              ; fIP1Hist = 3 * fIIn
	nop                                        ; *** mul FPU instruction stall    
	add.s fTemp0,\fIIn,fTemp0                 ; fTemp0 = fTemp0 + fIIn		
	nop                                        ; *** add FPU instruction stall    
	nop                                        ; *** add FPU instruction stall    
	add.s fTemp0,\fIP0In,\fIOut               ; fIOut = Polyphase sum of last two processed phases
	; Quadrature
	mov.s \fQP1Hist,fTemp0                    ; fTemp0 = fQP1Hist
	mul.s \fQIn,fThree,\fQP1Hist              ; fQP1Hist = 3 * fMixQ
	nop                                        ; *** mul FPU instruction stall    
	add.s fTemp0,\fQIn,fTemp0                 ; fTemp0 = fTemp0 + fMixQ	
	add.s fTemp0,\FIQ0In,\fQOut               ; fQOut = Polyphase sum of last two processed phases

    .endm
    
    
    .macro ProcessStage0 fIOut,fQOut
	; Phase 0
	GetNextDownmixedIQSamplePair pn32ADCSample,fLOPhase,fLOPhaseInc,fISample,fQSample
	CICPolyphaseFastPhase0 fISample,fQSample,fIS0P0Hist,fQS0P0Hist,fTemp1,fTemp2

	; Phase 1
	GetNextDownmixedIQSamplePair pn32ADCSample,fLOPhase,fLOPhaseInc,fISample,fQSample
	CICPolyphaseFastPhase1AndAddPhase0 fISample,fQSample,fTemp1,fTemp2,fIS0P1Hist,fQS0P1Hist,\fIOut,\fQOut
    .endm
    
    .macro ProcessStage fIP0In,fQP0In,fIP1In,fQP1In,fIP0Hist,fQP0Hist,fIP1Hist,fQP1Hist,fIP0Out,fQP0Out,fIOut,fQOut
	CICPolyphaseFastPhase0 \fIP0In,\fQP0In,\fIP0Hist,\fQP0Hist,\fIP0Out,\fQP0Out
	CICPolyphaseFastPhase1AndAddPhase0 \fIP1In,\fQP1In,\fIP0Out,\fQP0Out,\fIP1Hist,\fQP1Hist,\fIOut,\fQOut
    .endm
    
    .macro ProcessStage1 fIOut,fQOut
	ProcessStage0 fIS0Out,fQS0Out
	ProcessStage0 fISample,fQSample
	ProcessStage fIS0Out,fQS0Out,fISample,fQSample,fIS1P0Hist,fQS1P0Hist,fIS1P1Hist,fQS1P1Hist,fTemp1,fTemp2,\fIOut,\fQOut
    .endm
    
    .macro ProcessStage2 fIOut,fQOut
	ProcessStage1 fIS1Out,fQS1Out
	ProcessStage1 fISample,fQSample
	ProcessStage fIS1Out,fQS1Out,fISample,fQSample,fIS2P0Hist,fQS2P0Hist,fIS2P1Hist,fQS2P1Hist,fTemp1,fTemp2,\fIOut,\fQOut
    .endm
    
    .macro ProcessStage3 fIOut,fQOut
	ProcessStage2 fIS2Out,fQS2Out
	ProcessStage2 fISample,fQSample
	ProcessStage fIS2Out,fQS2Out,fISample,fQSample,fIS3P0Hist,fQS3P0Hist,fIS3P1Hist,fQS3P1Hist,fTemp1,fTemp2,\fIOut,\fQOut
    .endm
    
    .macro ProcessStage4 fIOut,fQOut
	ProcessStage3 fIS3Out,fQS3Out
	ProcessStage3 fISample,fQSample

	; Need to temporaily save away stage 0 FPU registers so we can re-use them for stage 4
	Stage0FPUSave
	Stage4FPULoad
	
	ProcessStage fIS3Out,fQS3Out,fISample,fQSample,fIS4P0Hist,fQS4P0Hist,fIS4P1Hist,fQS4P1Hist,fTemp1,fTemp2,\fIOut,\fQOut

	Stage4FPUSave
	Stage0FPULoad

    .endm
    
    .macro ProcessStage5 fIOut,fQOut
	ProcessStage4 fIS4Out,fQS4Out
	ProcessStage4 fISample,fQSample
	
	; Need to temporaily save away stages 0 & 1 FPU registers so we can re-use them for stages 4 & 5
	Stage0FPUSave
	Stage1FPUSave
	Stage4FPULoad	
	Stage5FPULoad	
	
	ProcessStage fIS4Out,fQS4Out,fISample,fQSample,fIS5P0Hist,fQS5P0Hist,fIS5P1Hist,fQS5P1Hist,fTemp1,fTemp2,\fIOut,\fQOut
	
	Stage5FPUSave
	Stage4FPUSave
	Stage1FPULoad
	Stage0FPULoad
    .endm
    
    .macro ProcessStage6 fIOut,fQOut
	ProcessStage5 fIS5Out,fQS5Out
	ProcessStage5 fISample,fQSample
	
	; Need to temporaily save away stages 1 & 2 FPU registers so we can re-use them for stages 5 & 6
	Stage1FPUSave
	Stage2FPUSave
	Stage5FPULoad	
	Stage6FPULoad	

	ProcessStage fIS5Out,fQS5Out,fISample,fQSample,fIS6P0Hist,fQS6P0Hist,fIS6P1Hist,fQS6P1Hist,fTemp1,fTemp2,\fIOut,\fQOut

	Stage6FPUSave
	Stage5FPUSave
	Stage2FPULoad
	Stage1FPULoad
    .endm

    .global _fnDebugFPUMinRegs
_fnDebugFPUMinRegs:
    DebugFPUMinRegs
    return
    
    .global _fnDebugFPUCICRegs
_fnDebugFPUCICRegs:
    DebugFPUCICRegs
    return
	
    .global _fnDebugFPUAllRegs
_fnDebugFPUAllRegs:
    DebugFPUAllRegs
    return
    
    .global _fnDebugFlushCICHistory
_fnDebugFlushCICHistory:  
    mov #__aciciqhs,W13
    
    mov.l fIS0P0Hist,[W13++]       ; Stage 0 Phase 0 in-phase CIC history
    mov.l fIS0P1Hist,[W13++]       ; Stage 0 Phase 1 in-phase CIC history
    mov.l fIS0Out,[W13++]          ; Stage 0 Phase 0 in-phase ouptut
    mov.l fQS0P0Hist,[W13++]       ; Stage 0 Phase 0 quadrature CIC history
    mov.l fQS0P1Hist,[W13++]       ; Stage 0 Phase 1 quadrature CIC history
    mov.l fQS0Out,[W13++]          ; Stage 0 Phase 0 quadrature ouptut
    
    mov.l fIS1P0Hist,[W13++]       ; Stage 1 Phase 0 in-phase CIC history
    mov.l fIS1P1Hist,[W13++]       ; Stage 1 Phase 1 in-phase CIC history
    mov.l fIS1Out,[W13++]          ; Stage 1 Phase 0 in-phase ouptut
    mov.l fQS1P0Hist,[W13++]       ; Stage 1 Phase 0 quadrature CIC history
    mov.l fQS1P1Hist,[W13++]       ; Stage 1 Phase 1 quadrature CIC history
    mov.l fQS1Out,[W13++]          ; Stage 1 Phase 0 quadrature ouptut
    
    mov.l fIS2P0Hist,[W13++]       ; Stage 2 Phase 0 in-phase CIC history
    mov.l fIS2P1Hist,[W13++]       ; Stage 2 Phase 1 in-phase CIC history
    mov.l fIS2Out,[W13++]          ; Stage 2 Phase 0 in-phase ouptut
    mov.l fQS2P0Hist,[W13++]       ; Stage 2 Phase 0 quadrature CIC history
    mov.l fQS2P1Hist,[W13++]       ; Stage 2 Phase 1 quadrature CIC history
    mov.l fQS2Out,[W13++]          ; Stage 2 Phase 0 quadrature ouptut
    
    mov.l fIS3P0Hist,[W13++]       ; Stage 3 Phase 0 in-phase CIC history
    mov.l fIS3P1Hist,[W13++]       ; Stage 3 Phase 1 in-phase CIC history
    mov.l fIS3Out,[W13++]          ; Stage 3 Phase 0 in-phase ouptut
    mov.l fQS3P0Hist,[W13++]       ; Stage 3 Phase 0 quadrature CIC history
    mov.l fQS3P1Hist,[W13++]       ; Stage 3 Phase 1 quadrature CIC history
    mov.l fQS3Out,[W13++]          ; Stage 3 Phase 0 quadrature ouptut
    
    return
    
    ; prints fTemp0
    .global _fnDebugPrintFloat
_fnDebugPrintFloat:
    PushXCScratchRegisters
    
    mov.s fTemp0,F0
    call _PrintFloat
    
    PopXCScratchRegisters

    return

    ; prints W13 in hex
    .global _fnDebugPrintU32
_fnDebugPrintU32:
    PushXCScratchRegisters
    
    mov.l W13,W0
    call _PrintU32
    
    PopXCScratchRegisters

    return
    
    
    ; prints New line
    .global _fnDebugPrintNewLine
_fnDebugPrintNewLine:
    PushXCScratchRegisters
    
    call _PrintNewLine
    
    PopXCScratchRegisters

    return
    
    .global _fnDebugCICPolyphaseFastStage0Phase0
_fnDebugCICPolyphaseFastStage0Phase0:
;.macro CICPolyphaseFastPhase0 fIIn,    fQIn,    fIP0Hist,  fQP0Hist,  fIOut, fQOut
	; In-phase
	mov.s fIS0P0Hist,fTemp1                      ; fTemp0 = fIP0Hist  ; fTemp1!!
	mov.s fISample,fIS0P0Hist                       ; fIP0Hist = fIIn
	mac.s fThree,fISample,fTemp1                   ; fIout = 3 * fIIn + fTemp0  ; fTemp1!!

	; Quadrature
	mov.s fQS0P0Hist,fTemp2                      ; fTemp0 = fQP0Hist ; fTemp2!!!
	mov.s fQSample,fQS0P0Hist                      ; fQP0Hist = fQIn
	mac.s fThree,fQSample,fTemp2                   ; fQOut = 3 * fQIn + fTemp0   ; fTemp2!!!
	
	return
	
    .global _fnCICPolyphaseFastStage0Phase1AndAddPhase0
_fnDebugCICPolyphaseFastStage0Phase1AndAddPhase0:
;.macro _fnDebugCICPolyphaseFastStage0Phase1AndAddPhase0 fIIn,    fQIn,    fIP0In,FIQ0In,fIP1Hist,  fQP1Hist,  fIOut, fQOut
	; In-phase
	mov.s fIS0P1Hist,fTemp0                    ; fTemp0 = fIP1Hist
	mul.s fISample,fThree,fIS0P1Hist              ; fIP1Hist = 3 * fIIn
	add.s fTemp0,fISample,fTemp0                 ; fTemp0 = fTemp0 + fIIn		
	add.s fTemp0,fTemp1,fIS0Out               ; fIOut = Polyphase sum of last two processed phases
	
	; Quadrature
	mov.s fQS0P1Hist,fTemp0                    ; fTemp0 = fQP1Hist
	mul.s fQSample,fThree,fQS0P1Hist              ; fQP1Hist = 3 * fMixQ
	add.s fTemp0,fQSample,fTemp0                 ; fTemp0 = fTemp0 + fMixQ	
	add.s fTemp0,fTemp2,fQS0Out               ; fQOut = Polyphase sum of last two processed phases	
    
	return
	
    .global _LOMixDecimateBuffer
_LOMixDecimateBuffer: 
    ; float LOMixDecimateBuffer
    ; (
    ;     float fLOPhase,
    ;     float fLOPhaseInc,
    ;     int32_t *panADCSamples,
    ;     CIC_IQ_HISTORY_STRUCT *_paciciqhs,
    ;     float fScaleMultiplier
    ; );
    ; Called first for I, then again for Q
    ; Parameters
    ;   F0  fLOPhase
    ;   F1  fLOPhaseInc 
    ;   W0  panADCSamples     pointer to array of signed ADCSamples -4096 <= x <= 4095
    ;   W1  paciciqhs         pointer to array of [7] CIC CIC_IQ_HISTORY_STRUCT
    ;	F2  fScaleMultiplier  constant = 1 / ( 2 ^ ( CICOrder * Log2DecimationRate ) ) 
    ;                                  = 1 / ( 2 ^ ( 3 * 7 ) )
    ;                                  = 1 / ( 2 ^ 21 )
    ;                                  = 1 / 2,097,1552
    ; Return
    ;   F0  fLOPhase0
    ;   paciciqhs[6].fIS6Out  decimated and filtered in-phase result
    ;   paciciqhs[6].fQS6Out  decimated and filtered quadrature result
    
    ; Save necessary context (W0-W7 and F0-F7 exempt in XC-DSC C compiler)
    push.l W11
    push.l W12
    push.l W13
    
    push.l F8
    push.l F9
    push.l F10
    push.l F11
    push.l F12
    push.l F13
    push.l F14
    push.l F15
    push.l F16
    push.l F17
    push.l F18
    push.l F19
    push.l F20
    push.l F21
    push.l F22
    push.l F23
    push.l F24
    push.l F25
    push.l F26
    push.l F27
    push.l F28
    push.l F29
    push.l F30
    push.l F31

    mov.l F2,wfScaleMultiplier           ; Save the fScaleMultiplier parameter into a W register for now
    
;    DebugSaveFPURegInit
    
    ; This is a constant used in the CIC polyphase filter
    mov.l #0x40400000,fThree             ; Use FPU instruction stall time to load constant for CIC
    
    ; Constant to add to an ADC sample to convert to a signed sample.
    ; Use zero if the ADC samples are already signed
    ; For unsigned to signed conversion, use -(2 ^ (NumBits-1))
;    mov.l #0,wADCOffset ; Use zero if the ADC samples are already signed.
    mov.l #-4096,wADCOffset ; For 13 bit unsigned ADC samples (e.g. 4x oversampling of 12 bits)

    ; Get first four polyphase non-recursive CIC history stages from RAM into FPU registers
    Stage0FPULoad
    Stage1FPULoad
    Stage2FPULoad
    Stage3FPULoad
    
    ResetPerfCounters

    ProcessStage6 fISample,fQSample      ; Stage 6 processes 128 samples
    
    CopyPerfCounters
    
    ; Save first four polyphase non-recursive CIC history stages back to RAM: stages 0, 1 and 2 are _probably_ already done
    Stage3FPUSave
;    Stage2FPUSave
;    Stage1FPUSave
;    Stage0FPUSave
    
    ; Scale results & save them
    mov.l wfScaleMultiplier,fTemp0
    mul.s fISample,fTemp0,fISample
    mul.s fQSample,fTemp0,fQSample
    mov.l fISample,[paciciqhs + 4 * OffsetIS6Out]
    mov.l fQSample,[paciciqhs + 4 * OffsetQS6Out]

    ; Restore context
    pop.l F31
    pop.l F30
    pop.l F29
    pop.l F28
    pop.l F27
    pop.l F26
    pop.l F25
    pop.l F24
    pop.l F23
    pop.l F22
    pop.l F21
    pop.l F20
    pop.l F19
    pop.l F18
    pop.l F17
    pop.l F16
    pop.l F15
    pop.l F14
    pop.l F13
    pop.l F12
    pop.l F11
    pop.l F10
    pop.l F9
    pop.l F8
    
    pop.l W13
    pop.l W12
    pop.l W11

    return;
    
    .end
    