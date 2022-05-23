; do *NOT* use register4 to register7 during time slicing
mathX               EQU     giga_sysArg0
mathY               EQU     giga_sysArg2
mathSum             EQU     register12
mathRem             EQU     register12
mathMask            EQU     register13
mathSign            EQU     register14
mathQuot            EQU     register15
mathShift           EQU     register15
mathBase            EQU     register10
mathPow             EQU     register11
mathResult          EQU     register14


%SUB                sign
sign                SGNW
                    RET
%ENDS

%SUB                absolute
absolute            ABSW
                    RET
%ENDS

%SUB                power16bit
                    ; accumulator = mathX ** mathY, (result 16bit)
power16bit          LDW     mathX
                    BNE     power16_cont0
                    LDI     0
                    RET

power16_cont0       SUBI    1
                    BNE     power16_cont1
                    LDI     1
                    RET
                    
power16_cont1       LDW     mathY
                    BNE     power16_cont2
                    LDI     1
                    RET

power16_cont2       SUBI    1
                    BNE     power16_cont3
                    LDW     mathX                               ; return mathX
                    RET
                    
power16_cont3       MOVWA   mathX, mathBase
                    MOVWA   mathY, mathPow
                    MOVQW   mathResult, 1
                    PUSH
                    CALLI   power16bitExt
                    POP
                    RET
%ENDS

%SUB                power16bitExt
power16bitExt       PUSH
                    LDW     mathPow
                    
power16E_loop       ANDI    1
                    BEQ     power16E_skip
                    MOVWA   mathBase, mathX
                    MOVWA   mathResult, mathY
                    CALLI   multiply16bit
                    STW     mathResult                          ; mathResult = mathBase * mathResult                    
                    
power16E_skip       MOVWA   mathBase, mathX
                    STW     mathY
                    CALLI   multiply16bit
                    STW     mathBase                            ; mathBase = mathBase * mathBase
                    LSRV    mathPow                             ; mathPow = mathPow / 2
                    LDW     mathPow
                    BNE     power16E_loop                       ; while mathPow > 0
                    POP
                    LDW     mathResult
                    RET
%ENDS

%SUB                multiply16bit
                    ; accumulator = mathX * mathY, (result 16bit)
multiply16bit       MOVQW   giga_sysFn, SYS_Multiply_s16_vX_66
                    MOVQW   giga_sysArg4, 0                     ; mathSum
                    MOVQW   giga_sysArg6, 1                     ; mathMask
                    SYS     66
                    RET
%ENDS

%SUB                multiply16bit_1
                    ; accumulator = mathX * mathY, (result 16bit)
multiply16bit_1     MOVQW   mathSum, 0
                    LDW     mathX
                    BEQ     multiply161_exit                    ; if x=0 then return 0
                    LDW     mathY
                    BEQ     multiply161_exit                    ; if y=0 then return
                    
multiply161_loop    ANDI    1
                    BEQ     multiply161_skip
                    ADDVW   mathX, mathSum, mathSum             ; mathSum += mathX
                    
multiply161_skip    LSLV    mathX                               ; mathX = mathX <<1
                    LSRV    mathY                               ; mathY = mathY >>1
                    LDW     mathY
                    BNE     multiply161_loop

multiply161_exit    LDW     mathSum
                    RET
%ENDS
    
%SUB                multiply16bit_2
                    ; accumulator = mathX * mathY, (result 16bit)
multiply16bit_2     MOVQW   mathSum, 0
                    MOVQW   mathMask, 1
                    
multiply162_loop    ANDW    mathY
                    BEQ     multiply162_skip
                    ADDVW   mathX, mathSum, mathSum
                    
multiply162_skip    LSLV    mathX
                    LSLV    mathMask
                    LDW     mathMask
                    BNE     multiply162_loop
                    LDW     mathSum
                    RET
%ENDS   

%SUB                divide16bit
                    ; accumulator:mathRem = mathX / mathY, (results 16bit)
divide16bit         LDW     giga_sysArg0                        ; mathX
                    XORW    giga_sysArg2                        ; mathY
                    STW     mathSign
                    LDW     giga_sysArg0
                    BGE     divide16_pos0
                    NEGW    giga_sysArg0
                    
divide16_pos0       LDW     giga_sysArg2                     
                    BGE     divide16_pos1
                    NEGW    giga_sysArg2
                    
divide16_pos1       MOVQW   giga_sysFn, SYS_Divide_s16_vX_80
                    MOVQW   giga_sysArg4, 0                     ; mathRem
                    MOVQW   giga_sysArg6, 1                     ; mathMask
                    SYS     80
                    LDW     mathSign
                    BGE     divide16_exit
                    NEGW    giga_sysArg0
                    
divide16_exit       LDW     giga_sysArg0
                    RET
%ENDS

%SUB                divide16bit_1
                    ; accumulator:mathRem = mathX / mathY, (results 16bit)
divide16bit_1       LDW     mathX
                    XORW    mathY
                    STW     mathSign
                    LDW     mathX
                    BGE     divide161_pos0
                    NEGW    mathX
                    
divide161_pos0      LDW     mathY                     
                    BGE     divide161_pos1
                    NEGW    mathY
                    
divide161_pos1      MOVQW   mathRem, 0
                    LDI     1
    
divide161_loop      STW     mathMask
                    LSLV    mathRem
                    LDW     mathX
                    BGE     divide161_incr
                    INC     mathRem
                    
divide161_incr      LSLV    mathX
                    LDW     mathRem
                    SUBW    mathY
                    BLT     divide161_incx
                    STW     mathRem
                    INC     mathX
                    
divide161_incx      LDW     mathMask
                    LSLW
                    BNE     divide161_loop
                    LDW     mathSign
                    BGE     divide161_exit
                    LDI     0
                    SUBW    mathX
                    RET
                    
divide161_exit      LDW     mathX
                    RET
%ENDS   

%SUB                divide16bit_2
                    ; accumulator:mathRem = mathX / mathY, (results 16bit)
divide16bit_2       LDI     0
                    STW     mathQuot
                    STW     mathRem
                    
                    LDW     mathX
                    XORW    mathY
                    STW     mathSign
                    LDW     mathX
                    BGE     divide162_pos0
                    NEGW    mathX
                    
divide162_pos0      LDW     mathY
                    BGE     divide162_pos1
                    NEGW    mathY
                    
divide162_pos1      LDWI    0x8000
                    
divide162_loop      STW     mathMask
                    BEQ     divide162_exit
                    LSLV    mathRem
                    LDW     mathX
                    ANDW    mathMask
                    BEQ     divide162_skip1
                    INC     mathRem
                    
divide162_skip1     LDW     mathRem
                    SUBW    mathY
                    BLT     divide162_skip2
                    STW     mathRem
                    LDW     mathQuot
                    ORW     mathMask
                    STW     mathQuot
                    
divide162_skip2     LSRV    mathMask
                    LDW     mathMask
                    BRA     divide162_loop

divide162_exit      LDW     mathSign
                    BLT     divide162_sgn
                    LDW     mathQuot
                    RET
                    
divide162_sgn       LDI     0
                    SUBW    mathQuot
                    RET
%ENDS

%SUB                sqrt16bit
sqrt16bit           STW     mathX
                    MOVQW   mathResult, 0
                    LDWI    0x4000
                    STW     mathShift
                    
sqrt16_loop         ADDVW   mathResult, mathShift, mathSum
                    LSRV    mathResult
                    LDW     mathSum
                    SUBW    mathX
                    BGT     sqrt16_skip
                    ADDVW   mathShift, mathResult, mathResult
                    SUBVW   mathX, mathSum, mathX
                    
sqrt16_skip         LSRV    mathShift
                    LSRV    mathShift
                    LDW     mathShift
                    BNE     sqrt16_loop
                    LDW     mathResult
                    RET
%ENDS
    
%SUB                rand16bit
rand16bit           LDWI    SYS_Random_34
                    STW     giga_sysFn
                    SYS     34
                    RET
%ENDS

%SUB                randMod16bit
randMod16bit        PUSH
                    LDWI    SYS_Random_34
                    STW     giga_sysFn
                    SYS     34
                    STW     giga_sysArg0                        ; mathX
                    CALLI   divide16bit
                    LDW     giga_sysArg4                        ; mathRem
                    POP
                    RET
%ENDS

%SUB                randFastMod16bit
randFastMod16bit    PUSH
                    LDWI    SYS_Random_34
                    STW     giga_sysFn
                    SYS     34
                    STW     giga_sysArg0                        ; mathX
                    LD      giga_sysArg1
                    SUBW    giga_sysArg2                        ; mathY
                    BLT     randfm16_mod
                    MOVB    giga_sysArg3, giga_sysArg1          ; dividend.hi <= divisor
randfm16_mod        CALLI   fastMod168bit
                    POP
                    RET
%ENDS

%SUB                fastMul8bit
fastMul8bit         MOVQW   giga_sysArg0, 0
                    MOVQB   giga_sysArg4, 8
                    LDWI    SYS_Multiply_u8_vX_48
                    STW     giga_sysFn
                    SYS     48
                    LDW     giga_sysArg0
                    RET
%ENDS

%SUB                fastDiv168bit
fastDiv168bit       LDW     giga_sysArg0                        ; mathX
                    XORW    giga_sysArg2                        ; mathY
                    STW     mathSign
                    LDW     giga_sysArg0
                    BGE     fastD168_pos0
                    NEGW    giga_sysArg0
                    
fastD168_pos0       LDW     giga_sysArg2                     
                    BGE     fastD168_pos1
                    NEGW    giga_sysArg2

fastD168_pos1       MOVQB   giga_sysArg4, 8
                    MOVQW   giga_sysFn, SYS_Divide_u168_vX_56
                    SYS     56
                    LDW     mathSign
                    BGE     fastD168_exit
                    LD      giga_sysArg0
                    NEGW    giga_vAC
                    RET
                    
fastD168_exit       LD      giga_sysArg0
                    RET
%ENDS

%SUB                fastMod168bit
fastMod168bit       MOVQB   giga_sysArg4, 8
                    MOVQW   giga_sysFn, SYS_Divide_u168_vX_56
                    SYS     56
                    LD      giga_sysArg1
                    RET
%ENDS

%SUB                fastDivMod168bit
fastDivMod168bit    MOVQB   giga_sysArg4, 8
                    MOVQW   giga_sysFn, SYS_Divide_u168_vX_56
                    SYS     56
                    LDW     giga_sysArg0
                    RET
%ENDS

%SUB                lsl4bit
lsl4bit             LDWI    SYS_LSLW4_46
                    STW     giga_sysFn
                    LDW     mathShift
                    SYS     46
                    RET
%ENDS   
    
%SUB                lsl8bit
lsl8bit             LDWI    SYS_LSLW8_24
                    STW     giga_sysFn
                    LDW     mathShift
                    SYS     28
                    RET
%ENDS   
    
%SUB                lsr1bit
lsr1bit             LSRV    mathShift
                    LDW     mathShift
                    RET
%ENDS   
    
%SUB                lsr2bit
lsr2bit             LDWI    SYS_LSRW2_52
                    STW     giga_sysFn
                    LDW     mathShift
                    SYS     52
                    RET
%ENDS   
    
%SUB                lsr3bit
lsr3bit             LDWI    SYS_LSRW3_52
                    STW     giga_sysFn
                    LDW     mathShift
                    SYS     52
                    RET
%ENDS   
    
%SUB                lsr4bit
lsr4bit             LDWI    SYS_LSRW4_50
                    STW     giga_sysFn
                    LDW     mathShift
                    SYS     50
                    RET
%ENDS   
    
%SUB                lsr5bit
lsr5bit             LDWI    SYS_LSRW5_50
                    STW     giga_sysFn
                    LDW     mathShift
                    SYS     50
                    RET
%ENDS   
    
%SUB                lsr6bit
lsr6bit             LDWI    SYS_LSRW6_48
                    STW     giga_sysFn
                    LDW     mathShift
                    SYS     48
                    RET
%ENDS   
    
%SUB                lsr7bit
lsr7bit             LDWI    SYS_LSRW7_30
                    STW     giga_sysFn
                    LDW     mathShift
                    SYS     30
                    RET
%ENDS   
    
%SUB                lsr8bit
lsr8bit             LDWI    SYS_LSRW8_24
                    STW     giga_sysFn
                    LDW     mathShift
                    SYS     28
                    RET
%ENDS

%SUB                asr1bit
asr1bit             LSRV    mathShift                           ; shift right 1
                    LDW     mathShift
                    SEXT    0x40                                ; fix sign
                    RET
%ENDS   
    
%SUB                asr2bit
asr2bit             LDWI    SYS_LSRW2_52
                    STW     giga_sysFn
                    LDW     mathShift
                    SYS     52                                  ; shift right 2
                    SEXT    0x20                                ; fix sign
                    RET
%ENDS   
    
%SUB                asr3bit
asr3bit             LDWI    SYS_LSRW3_52
                    STW     giga_sysFn
                    LDW     mathShift
                    SYS     52                                  ; shift right 3
                    SEXT    0x10                                ; fix sign
                    RET

%ENDS   

%SUB                asr4bit
asr4bit             LDWI    SYS_LSRW4_50
                    STW     giga_sysFn
                    LDW     mathShift
                    SYS     50                                  ; shift right 4
                    SEXT    0x08                                ; fix sign
                    RET
%ENDS
                    
%SUB                asr5bit
asr5bit             LDWI    SYS_LSRW5_50
                    STW     giga_sysFn
                    LDW     mathShift
                    SYS     50                                  ; shift right 5
                    SEXT    0x04                                ; fix sign
                    RET
%ENDS   

%SUB                asr6bit
asr6bit             LDWI    SYS_LSRW6_48
                    STW     giga_sysFn
                    LDW     mathShift
                    SYS     48                                  ; shift right 6
                    SEXT    0x02                                ; fix sign
                    RET
%ENDS   
    
%SUB                asr7bit
asr7bit             LDWI    SYS_LSRW7_30
                    STW     giga_sysFn
                    LDW     mathShift
                    SYS     30                                  ; shift right 7
                    SEXT    0x01                                ; fix sign
                    RET
%ENDS   
    
%SUB                asr8bit
asr8bit             LDWI    SYS_LSRW8_24
                    STW     giga_sysFn
                    LDW     mathShift
                    SYS     28                                  ; shift right 8
                    SEXT    0x01                                ; fix sign
                    RET
%ENDS