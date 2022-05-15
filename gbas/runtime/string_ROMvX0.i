; do *NOT* use register4 to register7 during time slicing
strChr              EQU     register0
strAddr             EQU     register0
strHex              EQU     register1
strLen              EQU     register2
strDstLen           EQU     register0
strFinish           EQU     register0
strSrcAddr          EQU     register1
strDstAddr          EQU     register2
strSrcLen           EQU     register3
strOffset           EQU     register8
strSrcAddr2         EQU     register9
strTmpAddr          EQU     register10
strLutAddr          EQU     register11
strBakAddr          EQU     register12
strSrcLen2          EQU     register13
strInteger          EQU     register0
strDigit            EQU     register1
strMult             EQU     register3


%SUB                stringChr
                    ; create a char string, (parameter in strChr)
stringChr           STW     strDstAddr
                    LDI     1
                    POKEV+  strDstAddr                          ; set destination buffer length
                    LD      strChr
                    POKEV+  strDstAddr                          ; copy char
                    LDW     strDstAddr
                    POKEI   0                                   ; terminating 0
                    RET
%ENDS

%SUB                stringSpc
                    ; create a spc string, (parameter in strLen)
stringSpc           LD      strLen
                    BLE     stringS_exit
                    CMPI    strLen, 94
                    BGT     stringS_exit
                    POKEV+  strAddr                             ; set destination buffer length
                    
stringS_loop        LDI     32
                    POKEV+  strAddr                             ; copy char
                    DBNE    strLen, stringS_loop
                    LDW     strAddr
                    POKEI   0                                   ; terminating 0
                    
stringS_exit        RET
%ENDS

%SUB                stringHex
                    ; creates a hex string at strAddr of strLen digits from strHex
stringHex           LDWI    SYS_LSRW4_50                        ; shift right by 4 SYS routine
                    STW     giga_sysFn
                    MOVWA   strAddr, strTmpAddr                 ; store string start
                    LD      strLen
                    BLE     stringH_done
                    POKE    strAddr                             ; length byte
                    ADDI    1
                    ADDW    strAddr
                    STW     strAddr                             ; offset by length byte and zero delimeter
                    POKEI   0                                   ; zero delimiter
                    
stringH_loop        DECWA   strAddr
                    SUBW    strTmpAddr                          ; start at LSD and finish at MSD
                    BEQ     stringH_done
                    ANDBK   strHex, 0x0F
                    SUBI    10
                    BLT     stringH_skip
                    ADDI    7
                    
stringH_skip        ADDI    0x3A
                    POKE    strAddr
                    LDW     strHex
                    SYS     50
                    STW     strHex                              ; next nibble
                    BRA     stringH_loop
                    
stringH_done        RET
%ENDS

%SUB                stringCopy
                    ; copy one string to another
stringCopy          STW     strDstAddr

stringCp_loop       PEEKV+  strSrcAddr
                    POKEV+  strDstAddr
                    BNE     stringCp_loop                       ; copy char until terminating char
                    RET
%ENDS

%SUB                stringCmp
                    ; compares two strings: returns 0 for smaller, 1 for equal and 2 for larger
stringCmp           STW     strSrcAddr2
                    PEEKV+  strSrcAddr
                    STW     strSrcLen                           ; save str length
                    PEEKV+  strSrcAddr2
                    STW     strSrcLen2                          ; save str length
                    
stringC_loop        PEEKV+  strSrcAddr
                    BEQ     stringC_equal                       ; this assumes your strings are valid, (i.e. valid length and terminating bytes)
                    STW     strChr
                    PEEKV+  strSrcAddr2
                    SUBW    strChr
                    BLT     stringC_larger
                    BGT     stringC_smaller
                    BRA     stringC_loop

stringC_smaller     LDI     0
                    RET
                    
stringC_equal       LDW     strSrcLen
                    SUBW    strSrcLen2
                    BLT     stringC_smaller
                    BGT     stringC_larger                      ; if strings are equal, choose based on length
                    LDI     1
                    RET
                    
stringC_larger      LDI     2
                    RET
%ENDS

%SUB                stringAdd
                    ; adds two strings together, (internal sub)
stringAdd           MOVWA   strDstAddr, strTmpAddr
                    XORW    strSrcAddr
                    BNE     stringA_diff
                    PEEKV   strDstAddr                          ; if src = dst then skip first copy
                    STW     strDstLen
                    ADDW    strDstAddr
                    STW     strDstAddr                          ; skip length byte and point to end of dst
                    INC     strDstAddr
                    BRA     stringA_copy1

stringA_diff        INC     strSrcAddr
                    INC     strDstAddr                          ; skip lengths
                    MOVQW   strDstLen, 0
                    
stringA_copy0       PEEKV+  strSrcAddr                          ; assumes strSrcAddr is a valid string <= 94 length
                    BEQ     stringA_copy1
                    POKEV+  strDstAddr
                    INC     strDstLen
                    BRA     stringA_copy0
                    
stringA_copy1       CMPI    strDstLen, 94
                    BGE     stringA_exit                        ; maximum destination length reached
                    INC     strSrcAddr2                         ; skips length first time
                    PEEKV   strSrcAddr2
                    BEQ     stringA_exit                        ; copy char until terminating char
                    POKEV+  strDstAddr                          ; copy char
                    INC     strDstLen
                    BRA     stringA_copy1

stringA_exit        LDW     strTmpAddr
                    POKEA   strDstLen                           ; save concatenated string length
                    LDW     strDstAddr
                    POKEI   0                                   ; terminating zero
                    RET
%ENDS

%SUB                stringConcat
                    ; concatenates multiple strings together
stringConcat        PUSH
                    STW     strDstAddr
                    CALLI   stringAdd
                    POP
                    RET
%ENDS

%SUB                stringConcatLut
                    ; concatenates multiple strings together using a LUT of string addresses
stringConcatLut     PUSH
                    STW     strDstAddr
                    DEEKV+  strLutAddr
                    BEQ     stringCCL_exit
                    STW     strSrcAddr
                    MOVWA   strDstAddr, strBakAddr
                    
stringCCL_loop      DEEKV+  strLutAddr
                    BEQ     stringCCL_exit
                    STW     strSrcAddr2
                    CALLI   stringAdd
                    MOVWA   strBakAddr, strDstAddr
                    STW     strSrcAddr
                    BRA     stringCCL_loop
                    
stringCCL_exit      POP
                    RET
%ENDS

%SUB                stringLeft
                    ; copies sub string from left hand side of source string to destination string
stringLeft          STW     strDstAddr
                    LD      strDstLen
                    BEQ     stringL_exit                        ; exit if left length <= 0
                    POKE    strDstAddr                          ; destination length
                    PEEKV   strSrcAddr                          ; get source length
                    BEQ     stringL_exit                        ; exit if source length = 0
                    STW     strSrcLen
                    SUBW    strDstLen
                    BGE     stringL_skip                        ; is left length <= source length
                    LD      strSrcLen
                    STW     strDstLen
                    POKE    strDstAddr                          ; new destination length
                    
stringL_skip        INC     strSrcAddr                          ; skip lengths
                    INC     strDstAddr
                    
stringL_loop        PEEKV+  strSrcAddr
                    POKEV+  strDstAddr                          ; copy char
                    DBNE    strDstLen, stringL_loop             ; until finished
                    
stringL_exit        LDW     strDstAddr
                    POKEI   0                                   ; terminating 0
                    RET
%ENDS

%SUB                stringRight
                    ; copies sub string from right hand side of source string to destination string
stringRight         STW     strDstAddr
                    LD      strDstLen
                    BEQ     stringR_exit                        ; exit if right length = 0
                    POKE    strDstAddr                          ; destination length
                    PEEKV   strSrcAddr                          ; get source length
                    BEQ     stringR_exit                        ; exit if source length = 0
                    STW     strSrcLen
                    SUBW    strDstLen
                    BGE     stringR_skip                        ; length <= srcLength
                    LD      strSrcLen
                    STW     strDstLen
                    POKE    strDstAddr                          ; new destination length
                    LDI     0
                    
stringR_skip        ADDW    strSrcAddr
                    STW     strSrcAddr                          ; copy from (source address + (source length - right length)) to destination address
                    INC     strSrcAddr                          ; skip lengths
                    INC     strDstAddr
                    
stringR_loop        PEEKV+  strSrcAddr
                    POKEV+  strDstAddr                          ; copy char
                    DBNE    strDstLen, stringR_loop             ; until finished
                    
stringR_exit        LDW     strDstAddr
                    POKEI   0                                   ; terminating 0
                    RET
%ENDS

%SUB                stringMid
                    ; copies length sub string from source offset to destination string
stringMid           STW     strDstAddr
                    LD      strDstLen
                    BEQ     stringM_exit                        ; exit if right length = 0
                    POKE    strDstAddr                          ; destination length
                    PEEKV   strSrcAddr                          ; get source length
                    BEQ     stringM_exit                        ; exit if source length = 0
                    STW     strSrcLen
                    SUBW    strOffset                           
                    SUBW    strDstLen
                    BGE     stringM_skip                        ; length + offset <= srcLength
                    LD      strSrcLen
                    SUBW    strOffset
                    BLE     stringM_exit
                    STW     strDstLen
                    POKE    strDstAddr                          ; new destination length
                    
stringM_skip        ADDVW   strOffset, strSrcAddr               ; copy from (source address + (source length - right length)) to destination address
                    INC     strSrcAddr                          ; skip lengths
                    INC     strDstAddr

stringM_loop        PEEKV+  strSrcAddr
                    POKEV+  strDstAddr                          ; copy char
                    DBNE    strDstLen, stringM_loop             ; until finished
                    
stringM_exit        LDW     strDstAddr
                    POKEI   0                                   ; terminating 0
                    RET
%ENDS

%SUB                stringLower
                    ; creates a lower case string
stringLower         PEEKV+  strSrcAddr
                    POKEV+  strDstAddr                          ; dst length = src length
                    
stringLo_next       PEEKV+  strSrcAddr
                    BEQ     stringLo_exit
                    ST      strChr
                    SUBI    65
                    BLT     stringLo_char
                    CMPI    strChr, 90
                    BGT     stringLo_char
                    ADDBI   strChr, 32                          ; >= 65 'A' and <= 90 'Z'
                    
stringLo_char       LD      strChr
                    POKEV+  strDstAddr                          ; lower case char
                    BRA     stringLo_next
                    
stringLo_exit       POKE    strDstAddr                          ; terminating 0
                    RET
%ENDS

%SUB                stringUpper
                    ; creates an upper case string
stringUpper         PEEKV+  strSrcAddr
                    POKEV+  strDstAddr                          ; dst length = src length
    
stringUp_next       PEEKV+  strSrcAddr
                    BEQ     stringUp_exit
                    ST      strChr
                    SUBI    97
                    BLT     stringUp_char
                    CMPI    strChr, 122
                    BGT     stringUp_char
                    SUBBI   strChr, 32                          ; >= 97 'a' and <= 122 'z'
                    
stringUp_char       LD      strChr
                    POKEV+  strDstAddr                          ; upper case char
                    BRA     stringUp_next
                    
stringUp_exit       POKE    strDstAddr                          ; terminating 0
                    RET
%ENDS

%SUB                stringDigit
stringDigit         STW     strMult
                    LDW     strInteger
                    
stringD_index       SUBW    strMult
                    BLT     stringD_cont
                    STW     strInteger
                    INC     strDigit                            ; calculate digit
                    BRA     stringD_index
    
stringD_cont        LD      strDigit
                    BEQ     stringD_exit                        ; leading zero supressed
                    ORI     0x30
                    POKEV+  strTmpAddr                          ; store digit
                    MOVQB   strDigit, 0x30                      ; reset digit
                    
stringD_exit        RET
%ENDS

%SUB                stringInt
                    ; create a string from an int
stringInt           PUSH
                    STW     strDstAddr
                    MOVQW   strDigit, 0
                    MOVWA   strDstAddr, strTmpAddr
                    INC     strTmpAddr                          ; skip length byte
                    LDW     strInteger
                    BGE     stringI_pos
                    LDI     0x2D
                    POKEV+  strTmpAddr                          ; -ve sign
                    NEGW    strInteger                          ; +ve number

stringI_pos         LDWI    10000
                    CALLI   stringDigit
                    LDWI    1000
                    CALLI   stringDigit
                    LDI     100
                    CALLI   stringDigit
                    LDI     10
                    CALLI   stringDigit
                    ORBK    strInteger, 0x30
                    POKE    strTmpAddr                          ; 1's digit
                    LDW     strTmpAddr
                    SUBW    strDstAddr
                    POKE    strDstAddr                          ; length byte
                    INCWA   strTmpAddr
                    POKEI   0                                   ; terminating 0                    
                    POP
                    RET
%ENDS
