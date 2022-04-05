; do *NOT* use register4 to register7 during time slicing
blitId              EQU     register0
blitXY              EQU     register1
blitAddrs           EQU     register2

    
%SUB                drawBlit_
drawBlit_           LDWI    _blitsLut_
                    ADDW    blitId
                    ADDW    blitId
                    DEEKA   blitAddrs                     ; get blit address table
                    
drawB_loop          DEEK+   blitAddrs
                    BEQ     drawB_exit
                    STW     giga_sysArg0
                    DEEK+   blitAddrs
                    ADDW    blitXY
                    SYS     64
                    BRA     drawB_loop
                    
drawB_exit          RET
%ENDS

%SUB                drawBlit
drawBlit            PUSH
                    LDWI    SYS_Sprite6_v3_64
                    STW     giga_sysFn
                    CALLI   drawBlit_
                    POP
                    RET
%ENDS

%SUB                drawBlitX
drawBlitX           PUSH
                    LDWI    SYS_Sprite6x_v3_64
                    STW     giga_sysFn
                    CALLI   drawBlit_
                    POP
                    RET
%ENDS

%SUB                drawBlitY
drawBlitY           PUSH
                    LDWI    SYS_Sprite6y_v3_64
                    STW     giga_sysFn
                    CALLI   drawBlit_
                    POP
                    RET
%ENDS

%SUB                drawBlitXY
drawBlitXY          PUSH
                    LDWI    SYS_Sprite6xy_v3_64
                    STW     giga_sysFn
                    CALLI   drawBlit_
                    POP
                    RET
%ENDS

%SUB                getBlitLUT
getBlitLUT          LDWI    _blitsLut_
                    ADDW    blitId
                    ADDW    blitId
                    DEEK
                    DEEK
                    RET
%ENDS
