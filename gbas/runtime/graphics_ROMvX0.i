; do *NOT* use register4 to register7 during time slicing
graphicsMode        EQU     register0
waitVBlankNum       EQU     register0

drawHLine_x1        EQU     register0
drawHLine_y1        EQU     register1
drawHLine_x2        EQU     register2

drawVLine_x1        EQU     register0
drawVLine_y1        EQU     register1
drawVLine_y2        EQU     register2

drawLine_x1         EQU     register0
drawLine_y1         EQU     register1
drawLine_x2         EQU     register2
drawLine_y2         EQU     register3
drawLine_xy1        EQU     register0
drawLine_xy2        EQU     register1
drawLine_dxy1       EQU     register2
drawLine_dxy2       EQU     register3
drawLine_dx1        EQU     register8
drawLine_dy1        EQU     register9
drawLine_dx2        EQU     register10
drawLine_dy2        EQU     register11
drawLine_sx         EQU     register12
drawLine_sy         EQU     register13
drawLine_h          EQU     register14
drawLine_num        EQU     register15
drawLine_count      EQU     register14
drawLine_dx         EQU     register2
drawLine_dy         EQU     register3
drawLine_u          EQU     register8
drawLine_v          EQU     register9
drawLine_addr       EQU     register10
drawLine_ddx        EQU     register11
drawLine_cnt        EQU     register12
drawLine_swp        EQU     register13
drawLine_xy         EQU     register7

drawPixel_xy        EQU     register0
readPixel_xy        EQU     register0

drawCircle_cycx     EQU     register0
drawCircle_r        EQU     register2
drawCircle_a        EQU     register3
drawCircle_d        EQU     register8
drawCircle_x        EQU     register9
drawCircle_y        EQU     register10
drawCircle_ch0      EQU     register11
drawCircle_ch1      EQU     register12
drawCircle_ch2      EQU     register13
drawCircle_ch3      EQU     register14

drawCircleF_x1      EQU     register0
drawCircleF_y1      EQU     register1
drawCircleF_cnt     EQU     register3
drawCircleF_cx      EQU     register15
drawCircleF_cy      EQU     register10
drawCircleF_r       EQU     register11
drawCircleF_v       EQU     register8
drawCircleF_w       EQU     register9

drawRect_x1         EQU     register7
drawRect_y1         EQU     register10
drawRect_x2         EQU     register11
drawRect_y2         EQU     register16

drawRectF_x1        EQU     register0
drawRectF_y1        EQU     register1
drawRectF_x2        EQU     register2
drawRectF_y2        EQU     register3
drawRectF_xcnt      EQU     register8
drawRectF_ycnt      EQU     register9

drawPoly_mode       EQU     register14
drawPoly_addr       EQU     register7

    
%SUB                scanlineMode
scanlineMode        LDWI    SYS_SetMode_v2_80
                    STW     giga_sysFn
                    LDW     graphicsMode
                    SYS     80
                    RET
%ENDS   

%SUB                waitVBlanks
waitVBlanks         DECWA   waitVBlankNum
                    BGE     waitVB_vblank
                    RET
    
waitVB_vblank       PUSH
                    CALLI   waitVBlank
                    POP
                    BRA     waitVBlanks
%ENDS

%SUB                waitVBlank
waitVBlank          LD      giga_jiffiesTick
                    XORW    frameCountPrev
                    BEQ     waitVBlank
                    LD      giga_jiffiesTick
                    STW     frameCountPrev
                    RET
%ENDS

%SUB                readPixel
                    ; dummy
readPixel           RET
%ENDS

%SUB                drawPixel
                    ; dummy
drawPixel           RET
%ENDS   

%SUB                drawHLine
drawHLine           CNVXY   drawHLine_x1, drawHLine_y1, giga_sysArg2
                    LD      drawHLine_x2
                    SUBW    drawHLine_x1
                    BGE     drawHL_cont
                    CNVXY   drawHLine_x2, drawHLine_y1, giga_sysArg2
                    LD      drawHLine_x1
                    SUBW    drawHLine_x2
                    
drawHL_cont         ADDI    1
                    PACKVW  giga_vAC, fgbgColour + 1, giga_sysArg0
                    LDWI    SYS_SetMemory_v2_54                         ; not zero page sys
                    STW     giga_sysFn
                    SYS     54                                          ; fill memory
                    RET
%ENDS

%SUB                drawVLine
drawVLine           CNVXY   drawVLine_x1, drawVLine_y1, giga_sysArg2
                    LD      drawVLine_y2
                    SUBW    drawVLine_y1
                    BGE     drawVL_cont
                    CNVXY   drawVLine_x1, drawVLine_y2, giga_sysArg2
                    LD      drawVLine_y1
                    SUBW    drawVLine_y2
                    
drawVL_cont         ADDI    1
                    PACKVW  giga_vAC, fgbgColour + 1, giga_sysArg0
                    MOVQW   giga_sysFn, SYS_DrawVLine_vX_66             ; zero page sys
                    SYS     66                                          ; draw vertical line
                    RET
%ENDS

%SUB                drawLine
drawLine            PUSH
                    SUBVW   drawLine_x2, drawLine_x1, drawLine_sx       ; sx = x2 - x1
                    SGNW
                    STW     drawLine_dx1        
                    STW     drawLine_dx2                                ; dx1 = dx2 = sgn(sx)
                    ABSVW   drawLine_sx                                 ; sx = abs(sx)
                    SUBVW   drawLine_y2, drawLine_y1, drawLine_sy       ; sy = y2 - y1
                    STW     drawLine_h                                  ; h = sy
                    SGNW
                    STW     drawLine_dy1                                ; dy1 = sgn(sy)
                    ABSVW   drawLine_sy                                 ; sy = abs(sy)
                    MOVQW   drawLine_dy2, 0
                    LDW     drawLine_sx
                    SUBW    drawLine_sy
                    BGE     drawL_ext                                   ; if(sx < sy) 
                    MOVQW   drawLine_dx2, 0                             ; dx2 = 0
                    XCHGB   drawLine_sx, drawLine_sy                    ; swap sx and sy, high bytes are always 0
                    LDW     drawLine_h
                    SGNW    
                    STW     drawLine_dy2                                ; dy2 = sgn(h)

drawL_ext           CNVXY   drawLine_x1, drawLine_y1, drawLine_xy1      ; xy1 = x1 | ((y1+8)<<8)
                    CNVXY   drawLine_x2, drawLine_y2, drawLine_xy2      ; xy2 = x2 | ((y2+8)<<8)
                    LD      drawLine_sx
                    LSRB    giga_vAC
                    ADDI    1
                    STW     drawLine_num                                ; numerator = sx>>1
                    STW     drawLine_count                              ; for(count=sx>>1; counti>=0; --i)
                    
                    LSL8    drawLine_dy1
                    ADDW    drawLine_dx1
                    STW     drawLine_dxy1                               ; dxy1 = dx1 + (dy1<<8)
    
                    LSL8    drawLine_dy2
                    ADDW    drawLine_dx2
                    STW     drawLine_dxy2                               ; dxy2 = dx2 + (dy2<<8)
                    
                    MOVQW   giga_sysFn, SYS_DrawLine_vX_86              ; self starting sys call, performs
                    SYS     86                                          ; the inner loop of drawLineLoop
                    POP                                                 ; matches drawLine's PUSH
                    RET
%ENDS

%SUB                drawVTLine
drawVTLine          PUSH                                                ; matches drawVTLineLoop's POP
                    SUBVW   drawLine_x2, drawLine_x1, drawLine_sx       ; sx = x2 - x1
                    SGNW
                    STW     drawLine_dx1        
                    STW     drawLine_dx2                                ; dx1 = dx2 = sgn(sx)
                    ABSVW   drawLine_sx                                 ; sx = abs(sx)
                    SUBVW   drawLine_y2, drawLine_y1, drawLine_sy       ; sy = y2 - y1
                    STW     drawLine_h                                  ; h = sy
                    SGNW
                    STW     drawLine_dy1                                ; dy1 = sgn(sy)
                    ABSVW   drawLine_sy                                 ; sy = abs(sy)
                    MOVQW   drawLine_dy2, 0
                    LDW     drawLine_sx
                    SUBW    drawLine_sy
                    BGE     drawVTL_ext                                 ; if(sx < sy) 
                    MOVQW   drawLine_dx2, 0                             ; dx2 = 0
                    XCHGB   drawLine_sx, drawLine_sy                    ; swap sx and sy, high bytes are always 0
                    LDW     drawLine_h
                    SGNW    
                    STW     drawLine_dy2                                ; dy2 = sgn(h)

drawVTL_ext         PACKVW  drawLine_x1, drawLine_y1, drawLine_xy1
                    PACKVW  drawLine_x2, drawLine_y2, drawLine_xy2
                    LD      drawLine_sx
                    LSRB    giga_vAC
                    ADDI    1
                    STW     drawLine_num                                ; numerator = sx>>1
                    STW     drawLine_count                              ; for(count=sx>>1; counti>=0; --i)

                    LSL8    drawLine_dy1
                    ADDW    drawLine_dx1
                    STW     drawLine_dxy1                               ; dxy1 = dx1 + (dy1<<8)
    
                    LSL8    drawLine_dy2
                    ADDW    drawLine_dx2
                    STW     drawLine_dxy2                               ; dxy2 = dx2 + (dy2<<8)
                    CALLI   drawVTLineLoop
%ENDS

%SUB                drawVTLineLoop
drawVTLineLoop      LDW     drawLine_xy1
                    STPX    fgbgColour + 1                              ; plot start pixel
                    LDW     drawLine_xy2
                    STPX    fgbgColour + 1                              ; plot start pixel
                    
                    ADDVW   drawLine_sy, drawLine_num, drawLine_num     ; numerator += sy
                    SUBW    drawLine_sx
                    BLE     drawVTL_flip                                ; if(numerator <= sx) goto flip
                    STW     drawLine_num                                ; numerator -= sx
                    
                    ADDVW   drawLine_dxy1, drawLine_xy1, drawLine_xy1   ; xy1 += dxy1
                    SUBVW   drawLine_xy2, drawLine_dxy1, drawLine_xy2   ; xy2 -= dxy1
                    BRA     drawVTL_count
                    
drawVTL_flip        ADDVW   drawLine_dxy2, drawLine_xy1, drawLine_xy1   ; xy1 += dxy2
                    SUBVW   drawLine_xy2, drawLine_dxy2, drawLine_xy2   ; xy2 -= dxy2
                    
drawVTL_count       DBNE    drawLine_count, drawVTLineLoop
                    POP                                                 ; matches drawVTLine's PUSH
                    RET
%ENDS   
    
%SUB                drawCircle
drawCircle          PUSH
                    LDI     0
                    STW     drawCircle_ch0
                    STW     drawCircle_ch1
                    STW     drawCircle_ch2
                    STW     drawCircle_ch3
                    STW     drawCircle_x
                    MOVWA   drawCircle_r, drawCircle_y
                    LDI     1
                    SUBW    drawCircle_r
                    STW     drawCircle_d
                    
drawC_loop          CALLI   drawCircleExt1
                    
                    LDW     drawCircle_d
                    BGE     drawC_skip
                    LDW     drawCircle_x
                    LSLW
                    LSLW
                    ADDW    drawCircle_d
                    ADDI    3
                    STW     drawCircle_d
                    BRA     drawC_cont
                    
drawC_skip          LDW     drawCircle_x
                    SUBW    drawCircle_y
                    LSLW
                    LSLW
                    ADDW    drawCircle_d
                    ADDI    5
                    STW     drawCircle_d
                    DECWA   drawCircle_y

drawC_cont          INCWA   drawCircle_x
                    SUBW    drawCircle_y
                    BLE     drawC_loop

                    POP
                    RET
%ENDS

%SUB                drawCircleExt1
drawCircleExt1      PUSH
                    ADDVB   drawCircle_cycx + 1, drawCircle_y, drawCircle_ch0 + 1
                    SUBVB   drawCircle_cycx + 1, drawCircle_y, drawCircle_ch1 + 1
                    ADDVB   drawCircle_cycx + 1, drawCircle_x, drawCircle_ch2 + 1
                    SUBVB   drawCircle_cycx + 1, drawCircle_x, drawCircle_ch3 + 1

                    LD      drawCircle_cycx
                    ADDW    drawCircle_x
                    ADDW    drawCircle_ch0
                    POKEA   fgbgColour + 1

                    LD      drawCircle_cycx
                    SUBW    drawCircle_x
                    ADDW    drawCircle_ch0
                    POKEA   fgbgColour + 1
                    
                    LD      drawCircle_cycx
                    ADDW    drawCircle_x
                    ADDW    drawCircle_ch1
                    POKEA   fgbgColour + 1

                    LD      drawCircle_cycx
                    SUBW    drawCircle_x
                    ADDW    drawCircle_ch1
                    POKEA   fgbgColour + 1
                    
                    CALLI   drawCircleExt2                              ; doesn't return to here
%ENDS
                    
%SUB                drawCircleExt2
drawCircleExt2      LD      drawCircle_cycx
                    ADDW    drawCircle_y
                    ADDW    drawCircle_ch2
                    POKEA   fgbgColour + 1

                    LD      drawCircle_cycx
                    SUBW    drawCircle_y
                    ADDW    drawCircle_ch2
                    POKEA   fgbgColour + 1
                    
                    LD      drawCircle_cycx
                    ADDW    drawCircle_y
                    ADDW    drawCircle_ch3
                    POKEA   fgbgColour + 1

                    LD      drawCircle_cycx
                    SUBW    drawCircle_y
                    ADDW    drawCircle_ch3
                    POKEA   fgbgColour + 1

                    POP
                    RET
%ENDS

%SUB                drawCircleF
drawCircleF         LDWI    SYS_SetMemory_v2_54
                    STW     giga_sysFn
                    MOVB    fgbgColour + 1, giga_sysArg1
                    LDI     0
                    STW     drawCircleF_v
                    STW     drawCircleF_w
                    
drawCF_wloop        SUBVW   drawCircleF_cx, drawCircleF_r, drawCircleF_x1
                    LDW     drawCircleF_cx
                    ADDW    drawCircleF_r
                    SUBW    drawCircleF_x1
                    STW     drawCircleF_cnt
                    LDW     drawCircleF_cy
                    SUBW    drawCircleF_v
                    CNVXY   drawCircleF_x1, giga_vAC, giga_sysArg2
                    MOVB    drawCircleF_cnt, giga_sysArg0               ; count top
                    SYS     54                                          ; fill memory
                    LDW     drawCircleF_cy
                    ADDW    drawCircleF_v
                    CNVXY   drawCircleF_x1, giga_vAC, giga_sysArg2
                    MOVB    drawCircleF_cnt, giga_sysArg0               ; count bottom
                    SYS     54                                          ; fill memory
                    LDW     drawCircleF_w
                    ADDW    drawCircleF_v
                    ADDW    drawCircleF_v
                    ADDI    1
                    STW     drawCircleF_w
                    INC     drawCircleF_v
                    
drawCF_rloop        BLT     drawCF_wloop
                    LDW     drawCircleF_w
                    SUBW    drawCircleF_r
                    SUBW    drawCircleF_r
                    ADDI    1
                    STW     drawCircleF_w
                    DBNE    drawCircleF_r, drawCF_rloop
                    RET
%ENDS

%SUB                drawRect
drawRect            PUSH
                    MOVWA   drawRect_x1, drawHLine_x1
                    MOVWA   drawRect_y1, drawHLine_y1
                    MOVWA   drawRect_x2, drawHLine_x2
                    CALLI   drawHLine
                    MOVWA   drawRect_y2, drawHLine_y1
                    CALLI   drawHLine

                    MOVWA   drawRect_x1, drawVLine_x1
                    MOVWA   drawRect_y1, drawVLine_y1
                    MOVWA   drawRect_y2, drawVLine_y2
                    CALLI   drawVLine
                    MOVWA   drawRect_x2, drawVLine_x1
                    MOVWA   drawRect_y1, drawVLine_y1
                    MOVWA   drawRect_y2, drawVLine_y2
                    CALLI   drawVLine

                    POP
                    RET
%ENDS

%SUB                drawRectF
drawRectF           LDWI    SYS_SetMemory_v2_54
                    STW     giga_sysFn
                    SUBVW   drawRectF_y2, drawRectF_y1, drawRectF_ycnt ; line count if y2 > y1
                    BGE     drawRFY_cont
                    MOVB    drawRectF_y2, drawRectF_y1
                    NEGW    drawRectF_ycnt                              ; line count if y1 > y2
                    
drawRFY_cont        ADDBI   drawRectF_y1, drawRectF_y1, 8               ; high start address
                    INC     drawRectF_ycnt                              ; line count++ for DBNE
                    LD      drawRectF_x2
                    SUBW    drawRectF_x1
                    BGE     drawRFX_cont                                ; x count if x2 > x1
                    MOVB    drawRectF_x2, drawRectF_x1                  ; low start address
                    NEGW    giga_vAC                                    ; x count if x1 > x2
                    
drawRFX_cont        ADDI    1                                           ; x count++
                    ST      drawRectF_xcnt
                    MOVB    fgbgColour + 1, giga_sysArg1                ; fill value
                    MOVB    drawRectF_y1, giga_sysArg3                  ; high start address
                    
drawRF_loop         MOVB    drawRectF_xcnt, giga_sysArg0                ; x count
                    MOVB    drawRectF_x1, giga_sysArg2                  ; low start address
                    SYS     54                                          ; fill memory
                    INC     giga_sysArg3
                    DBNE    drawRectF_ycnt, drawRF_loop
                    RET
%ENDS

%SUB                drawPoly
drawPoly            PUSH

drawP_loop          LD      cursorXY
                    STW     drawLine_x1
                    LD      cursorXY + 1
                    STW     drawLine_y1
                    PEEKV+  drawPoly_addr
                    STW     drawLine_x2
                    SUBI    255
                    BEQ     drawP_exit
                    MOVB    drawLine_x2, cursorXY
                    PEEKV+  drawPoly_addr
                    STW     drawLine_y2
                    ST      cursorXY + 1
                    CALLI   drawLine
                    BRA     drawP_loop
                    
drawP_exit          POP
                    RET
%ENDS

%SUB                drawPolyRel
drawPolyRel         PUSH

drawPR_loop         LD      cursorXY
                    STW     drawLine_x1
                    LD      cursorXY + 1
                    STW     drawLine_y1
                    DEEKV+  drawPoly_addr
                    STW     drawLine_x2
                    SUBI    255
                    BEQ     drawPR_exit
                    LDW     drawLine_x1
drawPR_x2           ADDW    drawLine_x2                                 ;relative X mode
                    STW     drawLine_x2
                    ST      cursorXY
                    DEEKV+  drawPoly_addr
                    STW     drawLine_y2
                    LDW     drawLine_y1
drawPR_y2           ADDW    drawLine_y2                                 ;relative Y mode
                    STW     drawLine_y2
                    ST      cursorXY + 1
                    CALLI   drawLine
                    BRA     drawPR_loop
                    
drawPR_exit         MOVQB   drawPoly_mode, 0x99                         ;ADDW
                    CALLI   setPolyRelFlipX
                    CALLI   setPolyRelFlipY                             ;reset X and Y modes
                    POP
                    RET
%ENDS

%SUB                setPolyRelFlipX
setPolyRelFlipX     LDWI    drawPR_x2
                    POKEA   drawPoly_mode
                    RET
%ENDS

%SUB                setPolyRelFlipY
setPolyRelFlipY     LDWI    drawPR_y2
                    POKEA   drawPoly_mode
                    RET
%ENDS

%SUB                parityFill
parityFill          LDWI    SYS_ParityFill_vX_44
                    STW     giga_sysFn
                    PACKVW  giga_sysArg2, giga_sysArg4, register0
                    MOVB    giga_sysArg6, register1
                    SYS     44
                    RET
%ENDS

%SUB                atLineCursor
atLineCursor        LD      cursorXY
                    STW     drawLine_x1
                    SUBI    giga_xres
                    BLT     atLC_x1good
                    MOVQW   drawLine_x1, 0
                    
atLC_x1good         LD      cursorXY
                    ADDW    drawLine_x2
                    SUBI    giga_xres
                    BLT     atLC_x2good
                    LDNI    giga_xres
                    
atLC_x2good         ADDI    giga_xres
                    STW     drawLine_x2
                    ST      cursorXY

                    LD      cursorXY + 1
                    STW     drawLine_y1
                    SUBI    giga_yres
                    BLT     atLC_y1good
                    MOVQW   drawLine_y1, giga_yres - 1
                    
atLC_y1good         LD      cursorXY + 1
                    ADDW    drawLine_y2
                    SUBI    giga_yres
                    BLT     atLC_y2good
                    LDNI    1
                    
atLC_y2good         ADDI    giga_yres
                    STW     drawLine_y2
                    ST      cursorXY + 1
                    RET
%ENDS

%SUB                scrollV
scrollV             BGE     scrollV_offs
                    ADDI    0x78
                    
scrollV_offs        ST      giga_sysArg0
                    MOVQB   giga_sysArg1, giga_yres
                    LDWI    giga_videoTable
                    STW     giga_sysArg2
                    LDWI    SYS_ScrollVTableY_vX_38
                    STW     giga_sysFn
                    SYS     38
                    RET
%ENDS

%SUB                scrollRectV
scrollRectV         LDWI    SYS_ScrollRectVTableY_vX_44
                    STW     giga_sysFn
                    LD      register0                                   ; don't replace with ADDBI
                    ADDI    8
                    ST      giga_sysArg4
                    ADDW    register1
                    ST      giga_sysArg5
                    LDWI    giga_videoTable
                    STW     giga_sysArg2
                    LD      register0
                    LSLW
                    ADDW    giga_sysArg2
                    STW     giga_sysArg2
                    MOVB    register1, giga_sysArg1
                    SYS     44
                    RET
%ENDS