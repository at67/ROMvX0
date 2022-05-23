; do *NOT* use register4 to register7 during time slicing
inpLutAddr          EQU     register0
inpKeyBak           EQU     register0
inpVarsAddr         EQU     register1
inpStrsAddr         EQU     register2
inpTypesAddr        EQU     register3
inpTextAddr         EQU     register8
inpTextOfs          EQU     register9
inpTypeData         EQU     register10
inpPrnXYBak         EQU     register11
inpCursXYBak        EQU     register12
inpCursXYOfs        EQU     register13
inpTextEnd          EQU     register14
inpSndChan          EQU     register15

    
%SUB                input
                    ; inputs numerics and text into vars
input               PUSH
                    STW     inpLutAddr
                    DEEKV+  inpLutAddr
                    STW     inpVarsAddr                         ; vars LUT address
                    DEEKV+  inpLutAddr
                    STW     inpStrsAddr                         ; strings LUT address
                    DEEKV   inpLutAddr
                    STW     inpTypesAddr                        ; types LUT address
                    ANDBI   miscFlags, MISC_ENABLE_SCROLL_MSK   ; disable text scrolling
                    MOVB    giga_serialRaw, serialRawPrev       ; initialise previous keystroke
                    CALLI   inputExt1                           ; doesn't return to here
%ENDS

%SUB                inputExt1
                    ; input extended 1
inputExt1           DEEKV+  inpTypesAddr
                    BEQ     inputE1_exit                        ; exit on LUT delimiter
                    STW     inpTypeData                         ; high byte is max string length, 8th and 7th bits of low byte are newlines, last 6 bits of low byte is type
                    ANDI    0x40
                    BEQ     inputE1_print                       ; check for prefix newline
                    CALLI   inputNewline                        ; registers don't need to be saved yet
                    
inputE1_print       CALLI   saveRegs8_15
                    DEEKV+  inpStrsAddr
                    CALLI   printText                           ; print strings LUT
                    CALLI   loadRegs8_15
                    ANDBK   inpTypeData, 0x80
                    BEQ     inputE1_skip                        ; check for suffix newline
                    CALLI   inputNewline                        ; registers don't need to be saved yet
                    
inputE1_skip        LDWI    textWorkArea + 1
                    STW     inpTextAddr                         ; text work area, treated as a string so skip length
                    MOVQW   inpTextOfs, 0                       ; print text offset
                    LDWI    textWorkArea
                    STW     inpTextEnd                          ; print text end
                    LD      inpTypeData + 1
                    ADDW    inpTextEnd
                    STW     inpTextEnd                          ; text max = textWorkArea + (highByte(inpTypeData) >> 8)
                    
                    MOVWA   cursorXY, inpCursXYBak
                    STW     inpPrnXYBak
                    CALLI   inputExt2                           ; doesn't return to here

inputE1_exit        ORBI    miscFlags, MISC_ENABLE_SCROLL_BIT   ; enable text scrolling
                    POP
                    RET
%ENDS

%SUB                inputExt2
                    ; input extended 2
inputExt2           CALLI   saveRegs8_15
                    LDI     127
                    STW     textChr
                    CALLI   inputCursor
                    CALLI   loadRegs8_15
                    CALLI   inputKeys
                    BEQ     inputExt2                           ; loop until return key pressed

                    CALLI   inputExt1                           ; doesn't return to here
%ENDS

%SUB                inputCursor
                    ; draws cursor
inputCursor         PUSH
                    XORI    127
                    BNE     inputC_skip                         ; don't flash cursor if char != 127
                    ANDBK   giga_jiffiesTick, 32
                    BNE     inputC_skip
                    MOVQW   textChr, 32                         ; alternate between 32 and 127

inputC_skip         MOVWA   inpCursXYBak, cursorXY              ; restore cursor position after the printChr
                    LDW     textChr
                    CALLI   printChr
                    POP
                    RET

inputBeep           CMPI    inpKeyBak, 0xFF
                    BEQ     inputB_exit
                    LDWI    8200
                    LSRB    giga_vAC
                    FREQI   0
                    MOVQB   giga_soundTimer, 2
                    
inputB_exit         RET
%ENDS

%SUB                inputKeys
                    ; saves key press into string work area buffer
inputKeys           PUSH
                    MOVB    giga_serialRaw, inpKeyBak           ; save keystroke
                    LD      serialRawPrev
                    SUBBA   inpKeyBak
                    BEQ     inputK_exit                         ; if keystroke hasn't changed exit
                    CALLI   inputBeep
                    MOVB    inpKeyBak, serialRawPrev            ; save as previous keystroke
                    CMPI    inpKeyBak, 127
                    BGT     inputK_exit
                    BNE     inputK_ret
                    CALLI   inputDelete                         ; delete key, doesn't return to here
                    
inputK_ret          CMPI    inpKeyBak, 10
                    BNE     inputK_char
                    CALLI   inputReturn                         ; return key, doesn't return to here
                    
inputK_char         LDW     inpTextEnd
                    SUBW    inpTextAddr
                    BEQ     inputK_exit                         ; text string bounds, (check after delete and return keys)
                    CMPI    inpKeyBak, 32
                    BLT     inputK_exit
                    LD      inpKeyBak
                    POKEV+  inpTextAddr                         ; set char
                    LDW     inpTextAddr
                    POKEI   0                                   ; set new end of text string
                    CMPI    inpCursXYBak, giga_xres - 11
                    BLT     inputK_advance                      ; cursor max bounds
                    INC     inpTextOfs
                    LDI     0
                    BRA     inputK_print
                    
inputK_advance      ANDBK   miscFlags, MISC_ENABLE_FNT4X6_BIT
                    CONDII  4, 6                                ; is fnt4x6 enabled flag?
                    
inputK_print        STW     inpCursXYOfs                        ; advance cursor
                    CALLI   inputPrint                          ; doesn't return to here
                    
inputK_exit         LDI     0                                   ; keep looping on current var
                    POP
                    RET                    
%ENDS

%SUB                inputIntVar
inputIntVar         PUSH
                    LDWI    textWorkArea + 1
                    STW     intSrcAddr                          ; src str address, (skip length)
                    DEEKV+  inpVarsAddr
                    STW     register12                          ; dst int address
                    LDW     intSrcAddr
                    CALLI   integerStr
                    DOKE    register12                          ; convert string to integer
                    POP
                    RET
%ENDS

%SUB                inputStrVar
inputStrVar         LDWI    textWorkArea
                    STW     register11                          ; src str address
                    DEEKV+  inpVarsAddr
                    STW     register12                          ; dst var address

inputS_copy         PEEKV+  register11
                    POKEV+  register12
                    BNE     inputS_copy                         ; copy char until terminating char
                    RET
%ENDS                    

%SUB                inputReturn
inputReturn         CALLI   saveRegs8_15
                    MOVQW   textChr, 32
                    CALLI   inputCursor
                    CALLI   loadRegs8_15
                    LDWI    textWorkArea
                    STW     register0
                    LDW     inpTextAddr
                    SUBW    register0
                    SUBI    1
                    POKE    register0                           ; text length
                    ADDW    register0
                    ADDI    1
                    POKEI   0                                   ; text delimiter
                    ANDBK   inpTypeData, 0x3F                   ; var type is bottom 6 bits
                    SUBI    5                                   ; var is string or integer?
                    BNE     inputR_int
                    CALLI   inputStrVar                         ; copy string
                    BRA     inputR_exit
                    
inputR_int          CALLI   inputIntVar                         ; convert numeric

inputR_exit         LDI     1                                   ; return key pressed, next var
                    POP
                    RET
%ENDS

%SUB                inputDelete
inputDelete         LD      inpTextOfs
                    BEQ     inputD_bounds
                    SUBI    1
                    STW     inpTextOfs                          ; decrement print text offset
                    MOVQW   inpCursXYOfs, 0                     ; stationary cursor
                    LDW     inpTextAddr
                    POKEI   0                                   ; delimiter
                    DECWA   inpTextAddr                         ; decrement text pointer
                    POKEI   32                                  ; delete char
                    BRA     inputD_print

inputD_bounds       LDW     inpPrnXYBak
                    SUBW    inpCursXYBak
                    BGE     inputD_exit                         ; cursor min bounds
                    ANDBK   miscFlags, MISC_ENABLE_FNT4X6_BIT
                    BNE     inputD_4x8
                    LDNI    6
                    BRA     inputD_skip

inputD_4x8          LDNI    4

inputD_skip         STW     inpCursXYOfs                        ; retreat cursor
                    LDI     32                  
                    POKEV+  inpTextAddr                         ; delete cursor
                    LDW     inpTextAddr
                    POKEI   0                                   ; delimiter
                    SUBBI   inpTextAddr, inpTextAddr, 2         ; decrement text pointer
                    LDW     inpTextAddr                 
                    POKEI   32                                  ; delete char
                    
inputD_print        CALLI   inputPrint                          ; doesn't return to here
                    
inputD_exit         LDI     0                                   ; keep looping on current var
                    POP
                    RET
%ENDS

%SUB                inputPrint
inputPrint          MOVWA   inpPrnXYBak, cursorXY               ; restore cursor position after the printText
                    CALLI   saveRegs8_15
                    LDWI    textWorkArea
                    ADDW    inpTextOfs
                    CALLI   printText
                    CALLI   loadRegs8_15
                    ADDVW   inpCursXYBak, inpCursXYOfs, inpCursXYBak
                    LDI     0                                   ; keep looping on current var
                    POP
                    RET
%ENDS                    

%SUB                inputNewline
inputNewline        PUSH
                    ORBI    miscFlags, MISC_ENABLE_SCROLL_BIT   ; enable text scrolling
                    CALLI   newLineScroll                       ; new line
                    ANDBI   miscFlags, MISC_ENABLE_SCROLL_MSK   ; disable text scrolling
                    POP
                    RET
%ENDS
