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
                    MOVB    fgbgColour, giga_sysArg1            ; fill value
                    LDWI    giga_videoTable
                    PEEKA   giga_sysArg3                        ; row0 high byte address
                    MOVQW   clearLoop, giga_yfont

clearCR_loopy       MOVQB   giga_sysArg0, giga_xres
                    MOVQB   giga_sysArg2, 0                     ; low start address
                    SYS     54                                  ; fill memory
                    INC     giga_sysArg3                        ; next line
                    DBNE    clearLoop, clearCR_loopy
                    CALLI   printInit                           ; re-initialise the SYS registers
                    POP
                    RET
%ENDS

%SUB                clearCursorRow4x6
                    ; clears the top giga_yfont lines of pixels in preparation of text scrolling
clearCursorRow4x6   PUSH
                    LDWI    SYS_SetMemory_v2_54
                    STW     giga_sysFn                          ; setup fill memory SYS routine
                    MOVB    fgbgColour, giga_sysArg1            ; fill value
                    LDWI    giga_videoTable
                    PEEKA   giga_sysArg3                        ; row0 high byte address
                    MOVQW   clearLoop, 6

clearCR46_loopy     MOVQB   giga_sysArg0, giga_xres
                    MOVQB   giga_sysArg2, 0                     ; low start address
                    SYS     54                                  ; fill memory
                    INC     giga_sysArg3                        ; next line
                    DBNE    clearLoop, clearCR46_loopy
                    CALLI   printInit                           ; re-initialise the SYS registers
                    POP
                    RET
%ENDS

%SUB                printInit
printInit           ANDBK   miscFlags, MISC_ENABLE_FNT4X6_BIT
                    JNE     printInit4x6                        ; is fnt4x6 enabled flag?
                    LDWI    SYS_VDrawBits_134
                    STW     giga_sysFn
                    MOVWA   fgbgColour, giga_sysArg0
                    LDW     cursorXY
                    VTBL    giga_sysArg4                        ; convert xy to vtable address
                    RET
%ENDS

%SUB                printInit4x6
printInit4x6        MOVWA   fgbgColour, giga_sysArg0
                    LDW     cursorXY
                    VTBL    giga_sysArg4                        ; convert xy to vtable address
                    RET
%ENDS

%SUB                printText
                    ; prints text string pointed to by the accumulator
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
printLeft           LD      textLen
                    BEQ     printL_exit
                    PUSH
                    CALLI   printInit
                    INC     textStr                             ; skip length
    
printL_char         PEEKV+  textStr             
                    CALLI   printChar
                    DBNE    textLen, printL_char
                    POP
printL_exit         RET
%ENDS   

%SUB                printRight
                    ; prints right sub string pointed to by the accumulator
printRight          LD      textLen
                    BEQ     printR_exit
                    PUSH
                    CALLI   printInit
                    PEEKV   textStr                             ; text length
                    ADDW    textStr
                    SUBW    textLen
                    STW     textStr                             ; text offset
                    INC     textStr                             ; skip length
    
printR_char         PEEKV+  textStr             
                    CALLI   printChar
                    DBNE    textLen, printR_char
                    POP
printR_exit         RET
%ENDS   

%SUB                printMid
                    ; prints sub string pointed to by the accumulator
printMid            LD      textLen
                    BEQ     printM_exit
                    PUSH
                    CALLI   printInit
                    ADDVW   textOfs, textStr, textStr           ; textStr += textOfs
                    INC     textStr                             ; skip length
                    
printM_char         PEEKV+  textStr             
                    CALLI   printChar
                    DBNE    textLen, printM_char
                    POP
printM_exit         RET
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
                    ADDBI   textChr, textChr, 32                ; >= 65 'A' and <= 90 'Z'
                    
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
                    SUBBI   textChr, textChr, 32                ; >= 97 'a' and <= 122 'z'
                    
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
printChar           STW     textChr
                    CMPII   32, 132                             ; 32 <= textChr < 132
                    BNE     printC_exit
                    ANDBK   miscFlags, MISC_ENABLE_FNT4X6_BIT
                    JNE     printChar4x6                        ; is fnt4x6 enabled flag?
                    FNT6X8  textFont, textChr
                    MOVQB   textSlice, 0x05

printC_slice        LDW     textFont                            ; text font slice base address
                    LUP     0x00                                ; get ROM slice
                    ST      giga_sysArg2
                    SYS     134                                 ; draw vertical slice, SYS_VDrawBits_134, 270 - 134/2 = 0xCB
                    INC     textFont                            ; next vertical slice
                    INC     giga_sysArg4                        ; next x
                    DBNE    textSlice, printC_slice
                    
                    MOVQB   giga_sysArg2, 0
                    SYS     134                                 ; draw last blank slice
                    INC     giga_sysArg4                        ; using sysArg4 as a temporary cursor address for multiple char prints
                    
                    PUSH
                    CALLI   printClip
                    POP
                    
printC_exit         RET
%ENDS

%SUB                printChar4x6
                    ; prints char in textChr
printChar4x6        FNT4X6  textFont, textChr
                    MOVQB   giga_sysArg2, 3                     ; 3 LUP's per char
                    LDW     giga_sysArg4
                    PRN4X6  textFont
                    ADDBI   giga_sysArg4, giga_sysArg4, 4       ; using sysArg4 as a temporary cursor address for multiple char prints
                    PUSH
                    CALLI   printClip4x6
                    POP
                    
printC46_exit       RET
%ENDS

%SUB                printClip
printClip           ADDBI   cursorXY, cursorXY, giga_xfont
                    CMPI    cursorXY, giga_xres - giga_xfont    ; last possible char on line
                    BLE     printCl_exit
                    ANDBK   miscFlags, MISC_DISABLE_CLIP_BIT
                    BNE     printCl_exit                        ; is text clipping disabled?
                    PUSH
                    CALLI   newLineScroll                       ; next row, scroll at bottom
                    POP
                    
printCl_exit        RET
%ENDS

%SUB                printClip4x6
printClip4x6        ADDBI   cursorXY, cursorXY, 4
                    CMPI    cursorXY, giga_xres - 4             ; last possible char on line
                    BLE     printCl46_exit
                    ANDBK   miscFlags, MISC_DISABLE_CLIP_BIT
                    BNE     printCl46_exit                      ; is text clipping disabled?
                    PUSH
                    CALLI   newLineScroll4x6                    ; next row, scroll at bottom
                    POP
                    
printCl46_exit      RET
%ENDS

%SUB                newLineScroll
                    ; print from top row to bottom row, then start scrolling 
newLineScroll       ANDBK   miscFlags, MISC_ENABLE_FNT4X6_BIT
                    JNE     newLineScroll4x6                    ; is fnt4x6 enabled flag?
                    LDI     giga_CursorX                        ; cursor x start
                    ST      cursorXY
                    ST      giga_sysArg4
                    ANDBK   miscFlags, MISC_ENABLE_SCROLL_BIT
                    BNE     newLS_cont0                         ; is scroll on or off?
                    RET
                    
newLS_cont0         PUSH
                    ANDBK   miscFlags, MISC_ON_BOTTOM_ROW_BIT
                    BNE     newLS_cont1                         ; is on bottom row flag?
                    ADDBI   cursorXY + 1, cursorXY + 1, giga_yfont
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

%SUB                newLineScroll4x6
                    ; print from top row to bottom row, then start scrolling 
newLineScroll4x6    LDI     0                                   ; cursor x start
                    ST      cursorXY
                    ST      giga_sysArg4
                    ANDBK   miscFlags, MISC_ENABLE_SCROLL_BIT
                    BNE     newLS46_cont0                       ; is scroll on or off?
                    RET
                    
newLS46_cont0       PUSH
                    ANDBK   miscFlags, MISC_ON_BOTTOM_ROW_BIT
                    BNE     newLS46_cont1                       ; is on bottom row flag?
                    ADDBI   cursorXY + 1, cursorXY + 1, 6
                    CMPI    cursorXY + 1, giga_yres
                    BLT     newLS46_exit
                    MOVQB   cursorXY + 1, giga_yres - 6
                    
newLS46_cont1       CALLI   clearCursorRow4x6
                    LDWI    giga_videoTable
                    STW     giga_sysArg2                        ; VTable
                    MOVQB   giga_sysArg0, 6                     ; scroll offset
                    MOVQB   giga_sysArg1, giga_yres             ; scanline count
                    LDWI    SYS_ScrollVTableY_vX_38
                    STW     giga_sysFn
                    SYS     38
                    ORBI    miscFlags, MISC_ON_BOTTOM_ROW_BIT   ; set on bottom row flag
                    
newLS46_exit        CALLI   printInit4x6                        ; re-initialise print
                    POP
                    RET
%ENDS

%SUB                atTextCursor
atTextCursor        ANDBK   miscFlags, MISC_ENABLE_FNT4X6_BIT
                    JNE     atTextCursor4x6                     ; is fnt4x6 enabled flag?
                    CMPI    cursorXY, giga_xres - giga_xfont
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

%SUB                atTextCursor4x6
atTextCursor4x6     CMPI    cursorXY, giga_xres - 4
                    BLE     atTC46_checkY
                    MOVQB   cursorXY, 0
                    
atTC46_checkY       CMPI    cursorXY + 1, giga_yres - 6
                    BLT     atTC46_resbot
                    MOVQB   cursorXY + 1, giga_yres - 6
                    ORBI    miscFlags, MISC_ON_BOTTOM_ROW_BIT   ; set on bottom row flag
                    RET
                    
atTC46_resbot       ANDBI   miscFlags, MISC_ON_BOTTOM_ROW_MSK   ; reset on bottom row flag
                    RET
%ENDS

%SUB                textWidth
textWidth           ANDBK   miscFlags, MISC_ENABLE_FNT4X6_BIT
                    JNE     textWidth4x6                        ; is fnt4x6 enabled flag?
                    CMPI    textLen, 26
                    BLE     textW_mul6
                    LDI     26*6
                    RET

textW_mul6          LD      textLen
                    MULB6
                    RET
%ENDS

%SUB                textWidth4x6
textWidth4x6        CMPI    textLen, 40
                    BLE     textW46_mul4
                    LDI     40*4
                    RET

textW46_mul4        LD      textLen
                    LSLW
                    LSLW
                    RET
%ENDS