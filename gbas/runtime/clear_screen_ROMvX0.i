; do *NOT* use register4 to register7 during time slicing
xreset              EQU     register0
xcount              EQU     register1
ycount              EQU     register2
treset              EQU     register3
breset              EQU     register8
top                 EQU     register9
bot                 EQU     register10
vtbLines            EQU     register10
vramAddr            EQU     register11
evenAddr            EQU     register12
clsAddress          EQU     register13
clsLines            EQU     register14
varAddress          EQU     register13
varCount            EQU     register14
clrAddress          EQU     register13
clrLines            EQU     register14
clrWidth            EQU     register15


%SUB                resetVars
resetVars           LDI     giga_One
                    SUBW    varAddress
                    STW     varCount
                    LSRB    varCount                            ; (0x0080 - varAddress)/2
                    LDI     0
                    
resetV_loop         DOKE+   varAddress
                    DBNE    varCount, resetV_loop
                    RET
%ENDS

%SUB                resetVideoFlags
resetVideoFlags     MOVQW   cursorXY, giga_CursorX              ; starting cursor position
                    ANDBI   miscFlags, MISC_ON_BOTTOM_ROW_MSK   ; reset on bottom row flag
                    RET
%ENDS
                    
%SUB                resetVideoTable
                    ; resets video table pointers
resetVideoTable     PUSH
                    MOVQW   vramAddr, 8
                    LDWI    giga_videoTable
                    STW     evenAddr
                    MOVQB   vtbLines, 120
    
resetVT_loop        LDW     vramAddr
                    DOKE+   evenAddr
                    INC     vramAddr
                    DBNE    vtbLines, resetVT_loop
                    CALLI   resetVideoFlags
                    POP
                    RET
%ENDS   
    
%SUB                initClearFuncs
initClearFuncs      PUSH
                    CALLI   resetVideoFlags
                    LDWI    SYS_SetMemory_v2_54                 ; setup fill memory SYS routine
                    STW     giga_sysFn
                    MOVB    fgbgColour, giga_sysArg1            ; fill value                   
                    POP
                    RET
%ENDS   

%SUB                clearScreen
                    ; clears the viewable screen
clearScreen         PUSH
                    CALLI   initClearFuncs
                    MOVB    clsAddress + 1, giga_sysArg3
                    MOVQB   clsLines, giga_yres
                    
clearCS_loopy       MOVQB   giga_sysArg0, giga_xres
                    MOVB    clsAddress, giga_sysArg2
                    SYS     54                                  ; fill memory
                    INC     giga_sysArg3                        ; next line
                    DBNE    clsLines, clearCS_loopy
                    POP
                    RET
%ENDS   

%SUB                clearRect
                    ; clears a rectangle on the viewable screen
clearRect           PUSH
                    CALLI   initClearFuncs
                    MOVB    clrAddress + 1, giga_sysArg3
                    
clearR_loop         MOVB    clrWidth, giga_sysArg0
                    MOVB    clrAddress, giga_sysArg2
                    SYS     54                                  ; fill memory
                    INC     giga_sysArg3                        ; next line
                    DBNE    clrLines, clearR_loop
                    POP
                    RET
%ENDS

%SUB                clearVertBlinds
                    ; clears the viewable screen using a vertical blinds effect
clearVertBlinds     PUSH
                    CALLI   initClearFuncs
                    MOVQW   top, giga_vram >> 8
                    MOVQB   clsLines, giga_yres/2
                    
clearVB_loopy       MOVQB   giga_sysArg0, giga_xres
                    MOVQB   giga_sysArg2, 0                     ; low start address
                    MOVB    top, giga_sysArg3                   ; top line
                    SYS     54                                  ; fill memory
    
                    MOVQB   giga_sysArg0, giga_xres
                    MOVQB   giga_sysArg2, 0                     ; low start address
                    LDI     giga_yres - 1 + 16
                    SUBW    top
                    ST      giga_sysArg3                        ; bottom line
                    SYS     54                                  ; fill memory
                    INC     top                                 ; next top line
                    DBNE    clsLines, clearVB_loopy
                    POP
                    RET
%ENDS