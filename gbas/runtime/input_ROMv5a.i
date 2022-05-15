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

    
%SUB                input
                    ; inputs numerics and text into vars
input               PUSH
                    STW     inpLutAddr
                    DEEK
                    STW     inpVarsAddr         ; vars LUT address
                    CALLI   initCursorTimer
                    LDW     inpLutAddr
                    ADDI    2
                    DEEK
                    STW     inpStrsAddr         ; strings LUT address
                    LDW     inpLutAddr
                    ADDI    4
                    DEEK
                    STW     inpTypesAddr        ; types LUT address

                    LD      miscFlags
                    ANDI    MISC_ENABLE_SCROLL_MSK
                    ST      miscFlags           ; disable text scrolling
                    
                    LD      giga_serialRaw
                    ST      serialRawPrev       ; initialise previous keystroke
                    CALLI   inputExt1           ; doesn't return to here
%ENDS

%SUB                inputExt1
                    ; input extended 1
inputExt1           LDW     inpTypesAddr
                    DEEK
                    BEQ     inputE1_exit        ; exit on LUT delimiter
                    STW     inpTypeData         ; high byte is max string length, 8th and 7th bits of low byte are newlines, last 6 bits of low byte is type
                    ANDI    0x40
                    BEQ     inputE1_print       ; check for prefix newline
                    CALLI   inputNewline        ; registers don't need to be saved yet
                    
inputE1_print       CALLI   saveRegs8_15
                    LDW     inpStrsAddr
                    DEEK
                    CALLI   printText           ; print strings LUT
                    CALLI   loadRegs8_15
                    LD      inpTypeData
                    ANDI    0x80
                    BEQ     inputE1_skip        ; check for suffix newline
                    CALLI   inputNewline        ; registers don't need to be saved yet
                    
inputE1_skip        LDWI    textWorkArea + 1
                    STW     inpTextAddr         ; text work area, treated as a string so skip length
                    LDI     0
                    STW     inpTextOfs          ; print text offset
                    LDWI    textWorkArea
                    STW     inpTextEnd          ; print text end
                    LD      inpTypeData + 1
                    ADDW    inpTextEnd
                    STW     inpTextEnd          ; text max = textWorkArea + (highByte(inpTypeData) >> 8)
                    
                    LDW     cursorXY
                    STW     inpCursXYBak
                    STW     inpPrnXYBak
                    CALLI   inputExt2           ; doesn't return to here

inputE1_exit        LD      miscFlags
                    ORI     MISC_ENABLE_SCROLL_BIT
                    ST      miscFlags           ; enable text scrolling
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
                    BEQ     inputExt2           ; loop until return key pressed

                    INC     inpVarsAddr
                    INC     inpVarsAddr
                    INC     inpStrsAddr
                    INC     inpStrsAddr
                    INC     inpTypesAddr
                    INC     inpTypesAddr
                    CALLI   inputExt1           ; doesn't return to here
%ENDS

%SUB                inputCursor
                    ; draws cursor
inputCursor         PUSH
                    XORI    127
                    BNE     inputC_skip         ; don't flash cursor if char != 127
                    CALLI   getCursorFlash
                    BNE     inputC_skip
                    LDI     32
                    STW     textChr             ; alternate between 32 and 127

inputC_skip         LDW     inpCursXYBak
                    STW     cursorXY            ; restore cursor position after the printChr
                    LDW     textChr
                    CALLI   printChr
                    POP
                    RET
%ENDS

%SUB                inputKeys
                    ; saves key press into string work area buffer
inputKeys           PUSH
                    LD      giga_serialRaw
                    STW     inpKeyBak           ; save keystroke
                    LD      serialRawPrev
                    SUBW    inpKeyBak
                    BEQ     inputK_exit         ; if keystroke hasn't changed exit
                    LD      inpKeyBak
                    ST      serialRawPrev       ; save as previous keystroke
                    SUBI    127
                    BGT     inputK_exit
                    BNE     inputK_ret
                    CALLI   inputDelete         ; delete key, doesn't return to here
                    
inputK_ret          LD      inpKeyBak
                    SUBI    10
                    BNE     inputK_char
                    CALLI   inputReturn         ; return key, doesn't return to here
                    
inputK_char         LDW     inpTextEnd
                    SUBW    inpTextAddr
                    BEQ     inputK_exit         ; text string bounds, (check after delete and return keys)
                    LD      inpKeyBak
                    SUBI    32
                    BLT     inputK_exit
                    LD      inpKeyBak
                    POKE    inpTextAddr         ; set char
                    INC     inpTextAddr
                    LDI     0
                    POKE    inpTextAddr         ; set new end of text string
                    LD      inpCursXYBak
                    SUBI    giga_xres - 11
                    BLT     inputK_advance      ; cursor max bounds
                    INC     inpTextOfs
                    LDI     0
                    BRA     inputK_print
                    
inputK_advance      LDI     6
                    
inputK_print        STW     inpCursXYOfs        ; advance cursor
                    CALLI   inputPrint          ; doesn't return to here
                    
inputK_exit         LDI     0                   ; keep looping on current var
                    POP
                    RET                    
%ENDS

%SUB                inputIntVar
inputIntVar         PUSH
                    LDWI    textWorkArea + 1
                    STW     intSrcAddr          ; src str address, (skip length)
                    LDW     inpVarsAddr
                    DEEK
                    STW     register12          ; dst int address
                    LDW     intSrcAddr
                    CALLI   integerStr
                    DOKE    register12          ; convert string to integer
                    POP
                    RET
%ENDS

%SUB                inputStrVar
inputStrVar         LDWI    textWorkArea
                    STW     register11          ; src str address
                    LDW     inpVarsAddr
                    DEEK
                    STW     register12          ; dst var address

inputS_copy         LDW     register11
                    PEEK
                    POKE    register12
                    INC     register11
                    INC     register12
                    BNE     inputS_copy         ; copy char until terminating char
                    RET
%ENDS                    

%SUB                inputReturn
inputReturn         CALLI   saveRegs8_15
                    LDI     32
                    STW     textChr
                    CALLI   inputCursor
                    CALLI   loadRegs8_15
                    LDWI    textWorkArea
                    STW     register0
                    LDW     inpTextAddr
                    SUBW    register0
                    SUBI    1
                    POKE    register0           ; text length
                    ADDW    register0
                    ADDI    1
                    STW     register0
                    LDI     0
                    POKE    register0           ; text delimiter
                    LD      inpTypeData         ; check var tye
                    ANDI    0x3F                ; var type is bottom 6 bits
                    SUBI    5                   ; var is string or integer?
                    BNE     inputR_int
                    CALLI   inputStrVar         ; copy string
                    BRA     inputR_exit
                    
inputR_int          CALLI   inputIntVar         ; convert numeric

inputR_exit         LDI     1                   ; return key pressed, next var
                    POP
                    RET
%ENDS

%SUB                inputDelete
inputDelete         LD      inpTextOfs
                    BEQ     inputD_bounds
                    SUBI    1
                    STW     inpTextOfs          ; decrement print text offset
                    LDI     0
                    STW     inpCursXYOfs        ; stationary cursor
                    LDI     0
                    POKE    inpTextAddr         ; delimiter
                    LDW     inpTextAddr
                    SUBI    1
                    STW     inpTextAddr         ; decrement text pointer
                    LDI     32                  
                    POKE    inpTextAddr         ; delete char
                    BRA     inputD_print

inputD_bounds       LDW     inpPrnXYBak
                    SUBW    inpCursXYBak
                    BGE     inputD_exit         ; cursor min bounds
                    LDWI    -6
                    STW     inpCursXYOfs        ; retreat cursor
                    LDI     32                  
                    POKE    inpTextAddr         ; delete cursor
                    INC     inpTextAddr
                    LDI     0
                    POKE    inpTextAddr         ; delimiter
                    LDW     inpTextAddr
                    SUBI    2
                    STW     inpTextAddr         ; decrement text pointer
                    LDI     32                  
                    POKE    inpTextAddr         ; delete char
                    
inputD_print        CALLI   inputPrint          ; doesn't return to here
                    
inputD_exit         LDI     0                   ; keep looping on current var
                    POP
                    RET
%ENDS

%SUB                inputPrint
inputPrint          LDW     inpPrnXYBak
                    STW     cursorXY            ; restore cursor position after the printText
                    CALLI   saveRegs8_15
                    LDWI    textWorkArea
                    ADDW    inpTextOfs
                    CALLI   printText
                    CALLI   loadRegs8_15
                    LDW     inpCursXYBak        ; new cursor position
                    ADDW    inpCursXYOfs
                    STW     inpCursXYBak
                    LDI     0                   ; keep looping on current var
                    POP
                    RET
%ENDS                    

%SUB                inputNewline
inputNewline        PUSH
                    LD      miscFlags
                    ORI     MISC_ENABLE_SCROLL_BIT
                    ST      miscFlags           ; enable text scrolling
                    CALLI   newLineScroll       ; new line
                    LD      miscFlags
                    ANDI    MISC_ENABLE_SCROLL_MSK
                    ST      miscFlags           ; disable text scrolling
                    POP
                    RET
%ENDS
