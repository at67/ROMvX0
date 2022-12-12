; do *NOT* use register4 to register7 during time slicing
blitId              EQU     register0
blitXY              EQU     register1
blitAddrs           EQU     register2
blitScrollY         EQU     register3


%SUB                drawBlit_
drawBlit_           ARRVW   blitId, _blitsLut_
                    DEEKA   blitAddrs                     ; get blit address table
                    BRA     drawB_check
                    
drawB_loop          STW     giga_sysArg0
                    DEEKV+  blitAddrs
                    ADDW    blitXY
                    SYS     64

drawB_check         DEEKV+  blitAddrs
                    BNE     drawB_loop
                    RET
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

%SUB                scrollBlitY
scrollBlitY         LDWI    SYS_Sprite6_v3_64
                    STW     giga_sysFn
                    ARRVW   blitId, _blitsLut_
                    DEEKA   blitAddrs                     ; get blit address table
                    BRA     scrollBY_check
                    
scrollBY_loop       ADDW    blitScrollY
                    STW     giga_sysArg0
                    DEEKV+  blitAddrs
                    ADDW    blitXY
                    SYS     64

scrollBY_check      DEEKV+  blitAddrs
                    BNE     scrollBY_loop
                    RET
%ENDS

%SUB                getBlitLUT
getBlitLUT          LDARRW  blitId, _blitsLut_
                    DEEK
                    RET
%ENDS
