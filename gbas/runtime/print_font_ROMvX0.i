; do *NOT* use register4 to register7 during time slicing
textStr             EQU     register0
textNum             EQU     register0
textBak             EQU     register0
textHex             EQU     register1
textSpc             EQU     register1
textLen             EQU     register2
textOfs             EQU     register3
textChr             EQU     register8
scanLine            EQU     register11
digitMult           EQU     register12
digitIndex          EQU     register13
clearLoop           EQU     register14
fontId              EQU     register9
fontAddrs           EQU     register10
fontBase            EQU     register11
fontPosXY           EQU     register15


%SUB                clearCursorRow
                    ; clears the top giga_yfont lines of pixels in preparation of text scrolling
clearCursorRow      PUSH
                    LDWI    SYS_SetMemory_v2_54
                    STW     giga_sysFn                          ; setup fill memory SYS routine
                    MOVB    fgbgColour, giga_sysArg1            ; fill value
                    LDWI    giga_videoTable
                    PEEKA   giga_sysArg3                        ; row0 high byte address
                    MOVQB   clearLoop, giga_yfont

clearCR_loopy       MOVQB   giga_sysArg0, giga_xres
                    MOVQB   giga_sysArg2, 0                     ; low start address
                    SYS     54                                  ; fill memory
                    INC     giga_sysArg3                        ; next line
                    DBNE    clearLoop, clearCR_loopy
                    CALLI   printInit                           ; re-initialise the SYS registers
                    POP
                    RET
%ENDS

%SUB                printInit
printInit           LDWI    SYS_Sprite6_v3_64
                    STW     giga_sysFn
                    LD      cursorXY + 1                        ; xy = peek(256+2*y)*256 + x
                    LSLW
                    INC     giga_vAC + 1
                    PEEKA   fontPosXY + 1
                    MOVB    cursorXY, fontPosXY                 ; xy position
                    RET
%ENDS

%SUB                printText
                    ; prints text string pointed to by textStr
printText           PUSH
                    STW     textStr
                    CALLI   printInit
                    INC     textStr                             ; skip length

printT_char         PEEKV+  textStr             
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
                    INC     textStr                             ; skip length
                    
printL_char         PEEKV+  textStr             
                    CALLI   printChar
                    DBNE    textLen, printL_char
                    
printL_exit         POP
                    RET
%ENDS   

%SUB                printRight
                    ; prints right sub string pointed to by the accumulator
printRight          PUSH
                    CALLI   printInit
                    PEEKV   textStr                             ; text length
                    ADDW    textStr
                    SUBW    textLen
                    STW     textStr                             ; text offset
                    LD      textLen
                    BEQ     printR_exit
                    INC     textStr                             ; skip length
                    
printR_char         PEEKV+  textStr             
                    CALLI   printChar
                    DBNE    textLen, printR_char
                    
printR_exit         POP
                    RET
%ENDS   

%SUB                printMid
                    ; prints sub string pointed to by the accumulator
printMid            PUSH
                    CALLI   printInit
                    ADDVW   textOfs, textStr, textStr           ; textStr += textOfs
                    LD      textLen
                    BEQ     printM_exit
                    INC     textStr                             ; skip length
    
printM_char         PEEKV+  textStr             
                    CALLI   printChar
                    DBNE    textLen, printM_char
                    
printM_exit         POP
                    RET
%ENDS   

%SUB                printLower
                    ; prints lower case version of textStr
printLower          PUSH
                    CALLI   printInit
                    INC     textStr                             ; skip length
    
printLo_next        PEEKV+  textStr
                    BEQ     printLo_exit
                    ST      textChr
                    SUBI    65
                    BLT     printLo_char
                    CMPI    textChr, 90
                    BGT     printLo_char
                    ADDBI   textChr, 32                         ; >= 65 'A' and <= 90 'Z'
                    
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
                    INC     textStr                             ; skip length
    
printUp_next        PEEKV+  textStr
                    BEQ     printUp_exit
                    ST      textChr
                    SUBI    97
                    BLT     printUp_char
                    CMPI    textChr, 122
                    BGT     printUp_char
                    SUBBI   textChr, 32                             ; >= 97 'a' and <= 122 'z'
                    
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
                    MOVQB   digitIndex, 0x30
printD_exit         POP
                    RET
%ENDS   
    
%SUB                printInt16
                    ; prints 16bit int in textNum
printInt16          PUSH
                    STW     textNum
                    CALLI   printInit
                    MOVQB   digitIndex, 0
                    LDW     textNum
                    BGE     printI16_pos
                    LDI     0x2D
                    CALLI   printChar
                    NEGW    textNum
    
printI16_pos        LDWI    10000
                    CALLI   printDigit
                    LDWI    1000
                    CALLI   printDigit
                    LDI     100
                    CALLI   printDigit
                    LDI     10
                    CALLI   printDigit
                    ORBK    textNum, 0x30
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
                    DBNE    textSpc, printS_loop
                    
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
                    SUBI    32
                    BLT     printC_exit
                    STW     textChr                             ; char-32                    
                    LDWI    _fontId_
                    PEEK
                    ARRW    _fontsLut_                          ; fonts table
                    DEEKA   fontAddrs                           ; get font address table
                    DEEKV+  fontAddrs                           ; get font mapping table
                    BEQ     printC_noMap                        ; no mapping table means font contains all chars 32 -> 127 in the correct order
                    ADDW    textChr
                    PEEKA   textChr                             ; get mapped char
                    
printC_noMap        DEEKV+  fontAddrs
                    STW     fontBase                            ; baseline address, shared by all chars in a font
                    LD      textChr
                    LSLW
                    ADDW    fontAddrs
                    DEEKA   giga_sysArg0                        ; get char address
                    LDW     fontPosXY                           ; XY pos generated in printInit
                    SYS     64                                  ; draw char
                    STW     fontPosXY
                    
                    MOVWA   fontBase, giga_sysArg0
                    LDWI    0x0F00
                    ADDW    cursorXY
                    SYS     64                                  ; draw baseline for char
                    
                    PUSH
                    CALLI   printClip
                    POP
                    
printC_exit         RET
%ENDS

%SUB                printClip
printClip           ADDBI   cursorXY, giga_xfont
                    CMPI    cursorXY, giga_xres - giga_xfont    ; last possible char on line
                    BLE     printCl_exit
                    ANDBK   miscFlags, MISC_DISABLE_CLIP_BIT
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
                    ST      fontPosXY
                    ANDBK   miscFlags, MISC_ENABLE_SCROLL_BIT
                    BNE     newLS_cont0                         ; is scroll on or off?
                    RET
                    
newLS_cont0         PUSH
                    ANDBK   miscFlags, MISC_ON_BOTTOM_ROW_BIT
                    BNE     newLS_cont1                         ; is on bottom row flag?
                    ADDBI   cursorXY + 1, giga_yfont
                    CMPI    cursorXY + 1, giga_yres
                    BLT     newLS_exit
                    MOVQB   cursorXY + 1, giga_yres - giga_yfont
                    
newLS_cont1         CALLI   clearCursorRow
                    LDWI    giga_videoTable
                    STW     giga_sysArg2                        ; VTable
                    MOVQB   giga_sysArg0, giga_yfont            ; scroll offset
                    MOVQB   giga_sysArg1, giga_yres             ; scanline count
                    LDWI    SYS_ScrollVTableY_vX_38
                    STW     giga_sysFn
                    SYS     38
                    ORBI    miscFlags, MISC_ON_BOTTOM_ROW_BIT   ; set on bottom row flag

newLS_exit          CALLI   printInit                           ; re-initialise the SYS registers
                    POP
                    RET
%ENDS   

%SUB                atTextCursor
atTextCursor        CMPI    cursorXY, giga_xres - giga_xfont
                    BLE     atTC_checkY
                    MOVQB   cursorXY, 0
                    
atTC_checkY         CMPI    cursorXY + 1, giga_yres - giga_yfont
                    BLT     atTC_resbot
                    MOVQB   cursorXY + 1, giga_yres - giga_yfont
                    ORBI    miscFlags, MISC_ON_BOTTOM_ROW_BIT   ; set on bottom row flag
                    RET
                    
atTC_resbot         ANDBI   miscFlags, MISC_ON_BOTTOM_ROW_MSK   ; reset on bottom row flag
                    RET
%ENDS
