; do *NOT* use register4 to register7 during time slicing
intSrcA             EQU     register0
intSrcB             EQU     register1
intSrcX             EQU     register2
intSwap             EQU     register3
intSrcAddr          EQU     register8
intDigit            EQU     register9
intResult           EQU     register10
bcdLength           EQU     register8
bcdSrcAddr          EQU     register9
bcdDstAddr          EQU     register10
bcdSrcData          EQU     register11
bcdDstData          EQU     register11                          ; alias to make code less confusing
bcdCarry            EQU     register12
bcdBorrow           EQU     register12                          ; alias to make code less confusing
bcdValue            EQU     register0
bcdDigit            EQU     register1
bcdMult             EQU     register2


%SUB                integerMin
integerMin          STW     intSrcB
                    LDW     intSrcA
                    SUBW    intSrcB
                    BLE     integerMi_A
                    LDW     intSrcB
                    RET

integerMi_A         LDW     intSrcA
                    RET
%ENDS

%SUB                integerMax
integerMax          STW     intSrcB
                    LDW     intSrcA
                    SUBW    intSrcB
                    BGE     integerMa_A
                    LDW     intSrcB
                    RET

integerMa_A         LDW     intSrcA
                    RET
%ENDS

%SUB                integerClamp
integerClamp        STW     intSrcB
                    LDW     intSrcX
                    SUBW    intSrcA
                    BGE     integerCl_X
                    BRA     integerCl_A0

integerCl_X         MOVWA   intSrcX, intSrcA

integerCl_A0        LDW     intSrcA
                    SUBW    intSrcB
                    BLE     integerCl_A1
                    LDW     intSrcB
                    RET

integerCl_A1        LDW     intSrcA
                    RET
%ENDS

%SUB                integerStr
                    ; converts a string to a +/- integer, assumes string pointer is pointing to first char and not the string length, (no overflow or underflow checks)
integerStr          STW     intSrcAddr
                    MOVQW   intResult, 0
                    PEEKV   intSrcAddr
                    SUBI    45                                  ; -ve
                    BNE     integerS_loop
                    INC     intSrcAddr                          ; skip -ve

integerS_loop       PEEKV   intSrcAddr
                    SUBI    48                                  ; str[i] - '0'
                    BLT     integerS_neg
                    STW     intDigit
                    SUBI    9
                    BGT     integerS_neg
                    LDW     intResult
                    MULW10
                    ADDW    intDigit
                    STW     intResult                           ; result = result*10 + digit
                    INC     intSrcAddr
                    BRA     integerS_loop
          
integerS_neg        ABSVW   intResult
                    LDW     intResult
                    RET
%ENDS

                    ; bcd values are stored unpacked lsd to msd
%SUB                bcdAdd
bcdAdd              ST      bcdLength
                    MOVQW   bcdCarry, 0
                    
bcdA_loop           PEEKV   bcdDstAddr                          ; expects unpacked byte values 0 to 9
                    STW     bcdDstData
                    PEEKV+  bcdSrcAddr                          ; expects unpacked byte values 0 to 9
                    ADDW    bcdDstData
                    ADDW    bcdCarry
                    STW     bcdDstData
                    SUBI    10                                  ; no handling of values > 9
                    BLT     bcdA_nc
                    STW     bcdDstData
                    LDI     1
                    BRA     bcdA_cont
          
bcdA_nc             LDI     0
                    
bcdA_cont           STW     bcdCarry
          
                    LDW     bcdDstData
                    POKEV+  bcdDstAddr                          ; modifies dst bcd value
                    DBNE    bcdLength, bcdA_loop
                    RET
%ENDS

                    ; bcd values are stored unpacked lsd to msd
%SUB                bcdSub
bcdSub              ST      bcdLength
                    MOVQW   bcdBorrow, 0
                    
bcdS_loop           PEEKV+  bcdSrcAddr                          ; expects unpacked byte values 0 to 9
                    STW     bcdSrcData
                    PEEKV   bcdDstAddr                          ; expects unpacked byte values 0 to 9
                    SUBW    bcdSrcData
                    SUBW    bcdBorrow
                    STW     bcdDstData
                    BGE     bcdS_nb
                    ADDI    10
                    STW     bcdDstData
                    LDI     1
                    BRA     bcdS_cont
          
bcdS_nb             LDI     0
                    
bcdS_cont           STW     bcdBorrow
          
                    LDW     bcdDstData
                    POKEV+  bcdDstAddr                          ; modifies dst bcd value
                    DBNE    bcdLength, bcdS_loop                ; expects src and dst lengths to be equal
                    RET
%ENDS

%SUB                bcdDigits
bcdDigits           STW     bcdMult
                    LDW     bcdValue

bcdD_index          SUBW    bcdMult
                    BLT     bcdD_cont
                    STW     bcdValue
                    INC     bcdDigit                            ; calculate digit
                    BRA     bcdD_index
    
bcdD_cont           LDW     bcdDstAddr
                    POKEA   bcdDigit                            ; store digit
                    DECWA   bcdDstAddr
                    MOVQB   bcdDigit, 0                         ; reset digit
                    RET                                         ; returns bcdDstAddr
%ENDS

%SUB                bcdInt
                    ; create a bcd value from a +ve int, (max 42767)
bcdInt              STW     bcdValue
                    PUSH
                    ADDVI   bcdDstAddr, bcdDstAddr, 4           ; bcdDstAddr must point to >= 5 digit bcd value
                    MOVQW   bcdDigit, 0
                    LDWI    10000
                    CALLI   bcdDigits
                    LDWI    1000
                    CALLI   bcdDigits
                    LDI     100
                    CALLI   bcdDigits
                    LDI     10
                    CALLI   bcdDigits                           ; returns bcdDstAddr in vAC
                    POKEA   bcdValue
                    POP
                    RET
%ENDS

                    ; bcd values are stored unpacked lsd to msd
                    ; cmp expects addrs to be pointing to msd!
%SUB                bcdCmp
bcdCmp              ST      bcdLength

bcdCmp_loop         PEEKV   bcdDstAddr                          ; expects unpacked byte values 0 to 9
                    STW     bcdDstData
                    PEEKV   bcdSrcAddr                          ; expects unpacked byte values 0 to 9
                    SUBW    bcdDstData
                    BGT     bcdC_gt
                    BLT     bcdC_lt
                    DECWA   bcdDstAddr
                    DECWA   bcdSrcAddr
                    DBNE    bcdLength, bcdCmp_loop              ; expects src and dst lengths to be equal
                    LDI     0
                    RET

bcdC_gt             LDI     1
                    RET
                    
bcdC_lt             LDNI    1
                    RET                    
%ENDS

%SUB                bcdCpy
bcdCpy              ST      bcdLength

bcdCpy_loop         PEEKV+  bcdSrcAddr
                    POKEV+  bcdDstAddr
                    DBNE    bcdLength, bcdCpy_loop              ; expects src and dst lengths to be equal
                    RET
%ENDS
