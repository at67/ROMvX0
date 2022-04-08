; do *NOT* use register4 to register7 during time slicing
textStr             EQU     register0
textNum             EQU     register0
textBak             EQU     register0
textHex             EQU     register1
textSpc             EQU     register1
textLen             EQU     register2
textOfs             EQU     register3
textChr             EQU     register8
textFont            EQU     register9
textSlice           EQU     register10
scanLine            EQU     register11
digitMult           EQU     register12
digitIndex          EQU     register13
clearLoop           EQU     register14
    
    
%SUB                clearCursorRow
                    ; clears the top giga_yfont lines of pixels in preparation of text scrolling
clearCursorRow      PUSH
                    LDWI    SYS_SetMemory_v2_54
                    STW     giga_sysFn                          ; setup fill memory SYS routine
                    LD      fgbgColour
                    ST      giga_sysArg1                        ; fill value
                    LDWI    giga_videoTable
                    PEEK
                    ST      giga_sysArg3                        ; row0 high byte address
                    LDI     giga_yfont

clearCR_loopy       ST      clearLoop                    
                    LDI     giga_xres
                    ST      giga_sysArg0
                    LDI     0
                    ST      giga_sysArg2                        ; low start address
                    SYS     54                                  ; fill memory
                    INC     giga_sysArg3                        ; next line
                    LD      clearLoop
                    SUBI    1
                    BNE     clearCR_loopy
                    CALLI   printInit                           ; re-initialise the SYS registers
                    POP
                    RET
%ENDS

%SUB                printInit
printInit           LDWI    SYS_VDrawBits_134
                    STW     giga_sysFn
                    LDW     fgbgColour
                    STW     giga_sysArg0
                    LD      cursorXY + 1                        ; xy = peek(256+2*y)*256 + x
                    LSLW
                    INC     giga_vAC + 1
                    PEEK
                    ST      giga_sysArg4 + 1
                    LD      cursorXY
                    ST      giga_sysArg4                        ; xy position
                    RET
%ENDS
                    
%SUB                printText
                    ; prints text string pointed to by the accumulator
printText           PUSH
                    STW     textStr
                    CALLI   printInit

                    ; first byte is length
printT_char         INC     textStr                             ; next char
                    LDW     textStr             
                    PEEK
                    BEQ     printT_exit                         ; check for terminating zero
                    CALLI   printChar
                    BRA     printT_char
                    
printT_exit         POP
                    RET
%ENDS   

%SUB                printLeft
                    ; prints left sub string pointed to by the accumulator
printLeft           PUSH
                    CALLI   printInit
                    LD      textLen
                    BEQ     printL_exit
    
printL_char         ST      textLen
                    INC     textStr                             ; next char
                    LDW     textStr             
                    PEEK
                    CALLI   printChar

                    LD      textLen
                    SUBI    1
                    BNE     printL_char
printL_exit         POP
                    RET
%ENDS   

%SUB                printRight
                    ; prints right sub string pointed to by the accumulator
printRight          PUSH
                    CALLI   printInit
                    LDW     textStr
                    PEEK                                        ; text length
                    ADDW    textStr
                    SUBW    textLen
                    STW     textStr                             ; text offset
                    LD      textLen
                    BEQ     printR_exit
    
printR_char         ST      textLen
                    INC     textStr                             ; next char
                    LDW     textStr             
                    PEEK
                    CALLI   printChar

                    LD      textLen
                    SUBI    1
                    BNE     printR_char
printR_exit         POP
                    RET
%ENDS   

%SUB                printMid
                    ; prints sub string pointed to by the accumulator
printMid            PUSH
                    CALLI   printInit
                    LDW     textStr
                    ADDW    textOfs
                    STW     textStr                             ; textStr += textOfs
                    LD      textLen
                    BEQ     printM_exit
    
printM_char         ST      textLen
                    INC     textStr                             ; next char
                    LDW     textStr             
                    PEEK
                    CALLI   printChar

                    LD      textLen
                    SUBI    1
                    BNE     printM_char
printM_exit         POP
                    RET
%ENDS   

%SUB                printLower
                    ; prints lower case version of textStr
printLower          PUSH
                    CALLI   printInit
    
printLo_next        INC     textStr                             ; next char, (skips length byte)
                    LDW     textStr
                    PEEK
                    BEQ     printLo_exit
                    ST      textChr
                    SUBI    65
                    BLT     printLo_char
                    LD      textChr
                    SUBI    90
                    BGT     printLo_char
                    LD      textChr                             ; >= 65 'A' and <= 90 'Z'
                    ADDI    32
                    ST      textChr
                    
printLo_char        LD      textChr
                    CALLI   printChar
                    BRA     printLo_next
                    
printLo_exit        POP
                    RET
%ENDS

%SUB                printUpper
                    ; prints upper case version of textStr
printUpper          PUSH
                    CALLI   printInit
    
printUp_next        INC     textStr                             ; next char, (skips length byte)
                    LDW     textStr
                    PEEK
                    BEQ     printUp_exit
                    ST      textChr
                    SUBI    97
                    BLT     printUp_char
                    LD      textChr
                    SUBI    122
                    BGT     printUp_char
                    LD      textChr                             ; >= 97 'a' and <= 122 'z'
                    SUBI    32
                    ST      textChr
                    
printUp_char        LD      textChr
                    CALLI   printChar
                    BRA     printUp_next
                    
printUp_exit        POP
                    RET
%ENDS

%SUB                printDigit
                    ; prints single digit in textNum
printDigit          PUSH
                    STW     digitMult
                    LDW     textNum
printD_index        SUBW    digitMult
                    BLT     printD_cont
                    STW     textNum
                    INC     digitIndex
                    BRA     printD_index
    
printD_cont         LD      digitIndex
                    BEQ     printD_exit
                    ORI     0x30
                    CALLI   printChar
                    LDI     0x30
                    ST      digitIndex
printD_exit         POP
                    RET
%ENDS   
    
%SUB                printInt16
                    ; prints 16bit int in textNum
printInt16          PUSH
                    STW     textNum
                    CALLI   printInit
                    LDI     0
                    ST      digitIndex
                    LDW     textNum
                    BGE     printI16_pos
                    LDI     0x2D
                    CALLI   printChar
                    LDI     0
                    SUBW    textNum
                    STW     textNum    
    
printI16_pos        LDWI    10000
                    CALLI   printDigit
                    LDWI    1000
                    CALLI   printDigit
                    LDI     100
                    CALLI   printDigit
                    LDI     10
                    CALLI   printDigit
                    LD      textNum
                    ORI     0x30
                    CALLI   printChar
                    POP
                    RET
%ENDS

%SUB                printChr
                    ; prints char in textChr for standalone calls
printChr            PUSH
                    ST      textChr
                    CALLI   printInit
                    LD      textChr
                    CALLI   printChar
                    POP
                    RET
%ENDS

%SUB                printSpc
                    ; prints textSpc spaces
printSpc            PUSH
                    BEQ     printS_exit
                    ST      textSpc
                    CALLI   printInit
                    
printS_loop         LDI     32
                    CALLI   printChar
                    LD      textSpc
                    SUBI    1
                    ST      textSpc
                    BNE     printS_loop
                    
printS_exit         POP
                    RET
%ENDS

%SUB                printHex
                    ; print textLen hex digits in textHex, (textStr, textHex, textLen = strAddr, strHex, strLen in string::stringHex)
printHex            PUSH
                    LDWI    textWorkArea
                    STW     strAddr
                    CALLI   stringHex
                    LDW     strAddr
                    CALLI   printText
                    POP
                    RET
%ENDS

%SUB                printChar
                    ; prints char in textChr
printChar           SUBI    130                                 ; char can't be bigger than 130
                    BGT     printC_exit
                    ADDI    130
                    SUBI    82
                    BGE     printC_text82
                    ADDI    50
                    BLT     printC_exit                         ; char<32 exit
                    STW     textChr                             ; char-32
                    LDWI    giga_text32                         ; text font slice base address for chars 32-81
                    BRA     printC_font
                    
printC_text82       STW     textChr                             ; char-82
                    LDWI    giga_text82                         ; text font slice base address for chars 82+
                    
printC_font         STW     textFont
                    LD      textChr
                    LSLW    
                    LSLW    
                    ADDW    textChr
                    ADDW    textFont
                    STW     textFont                            ; char*5 + textFont
                    LDI     0x05

printC_slice        ST      textSlice
                    LDW     textFont                            ; text font slice base address
                    LUP     0x00                                ; get ROM slice
                    ST      giga_sysArg2
                    SYS     134                                 ; draw vertical slice, SYS_VDrawBits_134, 270 - 134/2 = 0xCB
                    INC     textFont                            ; next vertical slice
                    INC     giga_sysArg4                        ; next x
                    LD      textSlice
                    SUBI    1
                    BNE     printC_slice
                    
                    ST      giga_sysArg2                        ; result of printC_slice is 0
                    SYS     134                                 ; draw last blank slice
                    INC     giga_sysArg4                        ; using sysArg4 as a temporary cursor address for multiple char prints
                    
                    PUSH
                    CALLI   printClip
                    POP
                    
printC_exit         RET
%ENDS

%SUB                printClip
printClip           LD      cursorXY
                    ADDI    giga_xfont
                    ST      cursorXY
                    SUBI    giga_xres - giga_xfont              ; last possible char on line
                    BLE     printCl_exit
                    LD      miscFlags
                    ANDI    MISC_DISABLE_CLIP_BIT
                    BNE     printCl_exit                        ; is text clipping disabled?
                    PUSH
                    CALLI   newLineScroll                       ; next row, scroll at bottom
                    POP
                    
printCl_exit        RET
%ENDS
    
%SUB                newLineScroll
                    ; print from top row to bottom row, then start scrolling 
newLineScroll       LDI     giga_CursorX                        ; cursor x start
                    ST      cursorXY
                    ST      giga_sysArg4
                    LD      miscFlags
                    ANDI    MISC_ENABLE_SCROLL_BIT
                    BNE     newLS_cont0                         ; is scroll on or off?
                    RET
                    
newLS_cont0         PUSH
                    LD      miscFlags
                    ANDI    MISC_ON_BOTTOM_ROW_BIT
                    BNE     newLS_cont1                         ; is on bottom row flag?
                    LD      cursorXY + 1
                    ADDI    giga_yfont
                    ST      cursorXY + 1
                    SUBI    giga_yres
                    BLT     newLS_exit
                    LDI     giga_yres - giga_yfont
                    ST      cursorXY + 1
                    
newLS_cont1         CALLI   clearCursorRow
                    LDWI    giga_videoTable
                    STW     scanLine
    
newLS_scroll        LDW     scanLine
                    PEEK
                    ADDI    giga_yfont
                    SUBI    0x7F
                    BLE     newLS_adjust
                    ADDI    0x88
                    
newLS_adjust        ADDI    0x7F
                    POKE    scanLine
                    INC     scanLine                            ; scanline pointers are 16bits
                    INC     scanLine
                    LD      scanLine
                    SUBI    0xF0                                ; scanline pointers end at 0x01EE
                    BLT     newLS_scroll
                    LD      miscFlags
                    ORI     MISC_ON_BOTTOM_ROW_BIT
                    ST      miscFlags                           ; set on bottom row flag
                    
newLS_exit          CALLI   printInit                           ; re-initialise the SYS registers
                    POP
                    RET
%ENDS   

%SUB                atTextCursor
atTextCursor        LD      cursorXY
                    SUBI    giga_xres - giga_xfont
                    BLE     atTC_checkY
                    LDI     0
                    ST      cursorXY
                    
atTC_checkY         LD      cursorXY + 1
                    SUBI    giga_yres - giga_yfont
                    BLT     atTC_resbot
                    LDI     giga_yres - giga_yfont
                    ST      cursorXY + 1
                    LD      miscFlags
                    ORI     MISC_ON_BOTTOM_ROW_BIT
                    ST      miscFlags                           ; set on bottom row flag
                    RET
                    
atTC_resbot         LD      miscFlags
                    ANDI    MISC_ON_BOTTOM_ROW_MSK
                    ST      miscFlags                           ; reset on bottom row flag
                    RET
%ENDS
