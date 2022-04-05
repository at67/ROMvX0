; do *NOT* use register4 to register7 during time slicing
graphicsMode        EQU     register0
waitVBlankNum       EQU     register0
waitVBlankTmp       EQU     register1

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

drawPixel_xy        EQU     giga_sysArg6
readPixel_xy        EQU     giga_sysArg6

drawCircle_cx       EQU     register0
drawCircle_cy       EQU     register1
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
drawCircleF_x2      EQU     register2
drawCircleF_cnt     EQU     register3
drawCircleF_cx      EQU     register15
drawCircleF_cy      EQU     register10
drawCircleF_r       EQU     register11
drawCircleF_v       EQU     register8
drawCircleF_w       EQU     register9

drawRect_x1         EQU     register8
drawRect_y1         EQU     register9
drawRect_x2         EQU     register10
drawRect_y2         EQU     register11

drawRectF_x1        EQU     register0
drawRectF_y1        EQU     register1
drawRectF_x2        EQU     register2
drawRectF_y2        EQU     register3
drawRectF_xcnt      EQU     register8
drawRectF_ycnt      EQU     register9

drawPoly_mode       EQU     register14
drawPoly_addr       EQU     giga_sysArg4                        ; TODO: find a better spot for this

    
%SUB                scanlineMode
scanlineMode        LDWI    SYS_SetMode_v2_80
                    STW     giga_sysFn
                    LDW     graphicsMode
                    SYS     80
                    RET
%ENDS   

%SUB                waitVBlanks
waitVBlanks         PUSH

waitVB_loop0        DECW    waitVBlankNum
                    LDW     waitVBlankNum
                    BGE     waitVB_vblank
                    POP
                    RET
    
waitVB_vblank       CALLI   waitVBlank
                    BRA     waitVB_loop0
%ENDS

%SUB                waitVBlank
%if VBLANK_INTERRUPT
waitVBlank          LD      timerPrev                           ; can't use giga_frameCount for VBlanks
%else
waitVBlank          LD      giga_frameCount
%endif
                    XORW    frameCountPrev
                    BEQ     waitVBlank
%if VBLANK_INTERRUPT
                    MOVB    timerPrev, frameCountPrev           ; can't use giga_frameCount for VBlanks
%else
                    MOVB    giga_frameCount, frameCountPrev
%endif
                    RET
%ENDS

%SUB                readPixel
readPixel           STW     readPixel_xy
                    LD      readPixel_xy + 1                    ; pixel = peek(peek(256 + 2*y)*256 + x)
                    LSLW
                    INC     giga_vAC + 1
                    PEEKA   readPixel_xy + 1
                    PEEKV   readPixel_xy
                    RET
%ENDS

%SUB                drawPixel
drawPixel           STW     drawPixel_xy
                    LD      drawPixel_xy + 1                    ; poke peek(256 + 2*y)*256 + x, fg_colour
                    LSLW
                    INC     giga_vAC + 1
                    PEEKA   drawPixel_xy + 1
                    LD      fgbgColour + 1
                    POKE    drawPixel_xy
                    RET
%ENDS   

%SUB                drawHLine
drawHLine           MOVB    drawHLine_x1, giga_sysArg2          ; low start address
                    LD      drawHLine_x2
                    SUBW    drawHLine_x1
                    BGE     drawHL_cont
                    MOVB    drawHLine_x2, giga_sysArg2          ; low start address
                    LD      drawHLine_x1
                    SUBW    drawHLine_x2
                    
drawHL_cont         ADDI    1
                    ST      giga_sysArg0                        ; count
                    MOVB    fgbgColour + 1, giga_sysArg1        ; fill value
                    MOVB    drawHLine_y1, giga_sysArg3
                    ADDBI   giga_sysArg3, 8                     ; high start address
                    LDWI    SYS_SetMemory_v2_54                 ; not zero page sys
                    STW     giga_sysFn
                    SYS     54                                  ; fill memory
                    RET
%ENDS

%SUB                drawVLine
drawVLine           MOVB    drawVLine_y1, giga_sysArg3          ; high start address
                    LD      drawVLine_y2
                    SUBW    drawVLine_y1
                    BGE     drawVL_cont
                    MOVB    drawVLine_y2, giga_sysArg3          ; high start address
                    LD      drawVLine_y1
                    SUBW    drawVLine_y2
                    
drawVL_cont         ADDI    1
                    ST      giga_sysArg0                        ; count
                    MOVB    fgbgColour + 1, giga_sysArg1        ; fill value
                    MOVB    drawVLine_x1, giga_sysArg2
                    ADDBI   giga_sysArg3, 8                     ; high start address
                    MOVQW   giga_sysFn, SYS_DrawVLine_vX_66     ; zero page sys
                    SYS     66                                  ; draw vertical line
                    RET
%ENDS

%SUB                drawLine
drawLine            PUSH                                        ; matches drawLineLoop's POP
                    LDI     1
                    STW     drawLine_dx1
                    STW     drawLine_dx2
                    STW     drawLine_dy1
                    MOVQW   drawLine_dy2, 0
    
                    LDW     drawLine_x2                         ; sx = x2 - x1
                    SUBW    drawLine_x1
                    STW     drawLine_sx
                    ANDBK   drawLine_sx+1, 0x80
                    BEQ     drawL_dy
                    LDNI    1
                    STW     drawLine_dx1        
                    STW     drawLine_dx2                        ; dx1 = dx2 = (sx & 0x8000) ? -1 : 1
                    NEGW    drawLine_sx                         ; sx = (sx & 0x8000) ? 0 - sx : sx
                    
drawL_dy            LDW     drawLine_y2
                    SUBW    drawLine_y1
                    STW     drawLine_sy
                    STW     drawLine_h                          ; h = sy
                    ANDBK   drawLine_h+1, 0x80
                    BEQ     drawL_ext
                    
                    LDNI    1
                    STW     drawLine_dy1                        ; dy1 = (sy & 0x8000) ? -1 : 1
                    NEGW    drawLine_sy                         ; sy = (sy & 0x8000) ? 0 - sy : sy
                    LDW     drawLine_sy
                    SUBW    drawLine_sx
                    BLE     drawL_ext           
                    LDW     drawLine_dy1
                    STW     drawLine_dy2                        ; if(sx < sy) dy2 = -1
    
drawL_ext           CALLI   drawLineExt
%ENDS   
                    
%SUB                drawLineExt
drawLineExt         MOVB    drawLine_x1, drawLine_xy1
                    MOVB    drawLine_y1, drawLine_xy1 + 1
                    ADDBI   drawLine_xy1 + 1, 8                 ; xy1 = x1 | ((y1+8)<<8)
                    
                    MOVB    drawLine_x2, drawLine_xy2
                    MOVB    drawLine_y2, drawLine_xy2 + 1
                    ADDBI   drawLine_xy2 + 1, 8                 ; xy2 = x2 | ((y2+8)<<8)
                    
                    LDW     drawLine_sy
                    SUBW    drawLine_sx
                    BLE     drawL_num
                    MOVQW   drawLine_dx2, 0                     ; if(sx < sy) dx2 = 0
                    XCHGB    drawLine_sx, drawLine_sy            ; swap sx and sy, high bytes are always 0
                    LDW     drawLine_h
                    BLE     drawL_num
                    MOVQW   drawLine_dy2, 1                     ; if(h > 0) dy2 = 1
    
drawL_num           LD      drawLine_sx
                    LSRB    giga_vAC
                    ADDI    1
                    STW     drawLine_num                        ; numerator = sx>>1
                    STW     drawLine_count                      ; for(count=sx>>1; counti>=0; --i)
                    
                    MOVB    drawLine_dy1, giga_vAC + 1
                    MOVQB   giga_vAC, 0
                    ADDW    drawLine_dx1
                    STW     drawLine_dxy1                       ; dxy1 = dx1 + (dy1<<8)
    
                    MOVB    drawLine_dy2, giga_vAC + 1
                    MOVQB   giga_vAC, 0
                    ADDW    drawLine_dx2
                    STW     drawLine_dxy2                       ; dxy2 = dx2 + (dy2<<8)
                    
                    MOVQW   giga_sysFn, SYS_DrawLine_vX_86      ; self starting sys call, performs
                    SYS     86                                  ; the inner loop of drawLineLoop
                    POP                                         ; matches drawLine's PUSH
                    ;CALLI   drawLineLoop
                    RET
%ENDS

%SUB                drawLineLoop
drawLineLoop        LD      fgbgColour + 1
                    POKE    drawLine_xy1                        ; plot start pixel
                    POKE    drawLine_xy2                        ; plot end pixel, (meet in middle)
                    
                    ADDVW   drawLine_sy, drawLine_num           ; numerator += sy
                    SUBW    drawLine_sx
                    BLE     drawL_flip                          ; if(numerator <= sx) goto flip
                    STW     drawLine_num                        ; numerator -= sx
                    
                    ADDVW   drawLine_dxy1, drawLine_xy1         ; xy1 += dxy1
                    SUBVW   drawLine_dxy1, drawLine_xy2         ; xy2 -= dxy1
                    BRA     drawL_count
                    
drawL_flip          ADDVW   drawLine_dxy2, drawLine_xy1         ; xy1 += dxy2
                    SUBVW   drawLine_dxy2, drawLine_xy2         ; xy2 -= dxy2
                    
drawL_count         DBNE    drawLine_count, drawLineLoop
                    POP                                         ; matches drawLine's PUSH
                    RET
%ENDS   
    
%SUB                drawLineSlow
drawLineSlow        PUSH
                    MOVQW   drawLine_u, 1                   ; u = 1
                    LDW     drawLine_x2
                    SUBW    drawLine_x1                     ; dx = x2 - x1
                    BGE     drawLS_dxp
                    NEGW    drawLine_u                      ; u = -1
                    NEGW    giga_vAC                        ; dx = x1 - x2
                    
drawLS_dxp          STW     drawLine_dx
                    LDWI    256
                    STW     drawLine_v                      ; v = 256
                    LDW     drawLine_y2
                    SUBW    drawLine_y1                     ; dy = y2 - y1
                    BGE     drawLS_dyp
                    NEGW    drawLine_v                      ; v = -256
                    NEGW    giga_vAC                        ; sy = y1 - y2
                    
drawLS_dyp          STW     drawLine_dy
                    
                    MOVB    drawLine_x1, drawLine_addr
                    MOVB    drawLine_y1, drawLine_addr + 1
                    ADDBI   drawLine_addr + 1, 8
                    LDW     drawLine_dx
                    SUBW    drawLine_dy
                    BGE     drawLS_noswap
                    XCHGW   drawLine_dx, drawLine_dy
                    XCHGW   drawLine_u, drawLine_v
                    
drawLS_noswap       LDI     0
                    SUBW    drawLine_dx
                    STW     drawLine_ddx
                    MOVB    drawLine_dx, drawLine_cnt
                    INC     drawLine_cnt
                    LSLV    drawLine_dx
                    LSLV    drawLine_dy
                    CALLI   drawLineSlowLoop
%ENDS

%SUB                drawLineSlowSwap
drawLineSlowSwap    XCHGW   drawLine_dx, drawLine_dy
                    XCHGW   drawLine_u, drawLine_v
                    RET
%ENDS

%SUB                drawLineSlowLoop
drawLineSlowLoop    LD      fgbgColour + 1
                    POKE    drawLine_addr
                    ADDVW   drawLine_dy, drawLine_ddx
                    BLE     drawLLS_xy
                    SUBW    drawLine_dx
                    STW     drawLine_ddx
                    ADDVW   drawLine_v, drawLine_addr

drawLLS_xy          ADDVW   drawLine_u, drawLine_addr
                    DBNE    drawLine_cnt, drawLineSlowLoop

                    POP
                    RET
%ENDS

%SUB                drawVTLine
drawVTLine          PUSH                                        ; matches drawVTLineLoop's POP
                    LDI     1
                    STW     drawLine_dx1
                    STW     drawLine_dx2
                    STW     drawLine_dy1
                    MOVQW   drawLine_dy2, 0
    
                    LDW     drawLine_x2                         ; sx = x2 - x1
                    SUBW    drawLine_x1
                    STW     drawLine_sx
                    ANDBK   drawLine_sx + 1, 0x80
                    BEQ     drawVTL_dy
                    LDNI    1
                    STW     drawLine_dx1        
                    STW     drawLine_dx2                        ; dx1 = dx2 = (sx & 0x8000) ? -1 : 1
                    NEGW    drawLine_sx                         ; sx = (sx & 0x8000) ? 0 - sx : sx
                    
drawVTL_dy          LDW     drawLine_y2
                    SUBW    drawLine_y1
                    STW     drawLine_sy
                    STW     drawLine_h                          ; h = sy
                    ANDBK   drawLine_sy + 1, 0x80
                    BEQ     drawVTL_ext
                    
                    LDNI    1
                    STW     drawLine_dy1                        ; dy1 = (sy & 0x8000) ? -1 : 1
                    NEGW    drawLine_sy
                    LDW     drawLine_sy                         ; sy = (sy & 0x8000) ? 0 - sy : sy
                    SUBW    drawLine_sx
                    BLE     drawVTL_ext           
                    LDW     drawLine_dy1
                    STW     drawLine_dy2                        ; if(sx < sy) dy2 = -1
    
drawVTL_ext         CALLI   drawVTLineExt
%ENDS   
                    
%SUB                drawVTLineExt
drawVTLineExt       MOVB    drawLine_x1, drawLine_xy1
                    MOVB    drawLine_y1, drawLine_xy1 + 1       ; xy1 = x1 | (y1<<8)
                    MOVB    drawLine_x2, drawLine_xy2
                    MOVB    drawLine_y2, drawLine_xy2 + 1       ; xy2 = x2 | (y2<<8)

                    LDW     drawLine_sy
                    SUBW    drawLine_sx
                    BLE     drawVTL_num
                    MOVQW   drawLine_dx2, 0                     ; if(sx < sy) dx2 = 0
                    LDI     drawLine_sx
                    XCHGB    drawLine_sy                         ; swap sx with sy
                    LDW     drawLine_h
                    BLE     drawVTL_num
                    MOVQW   drawLine_dy2, 1                     ; if(h > 0) dy2 = 1
    
drawVTL_num         LD      drawLine_sx
                    LSRB    giga_vAC
                    ADDI    1
                    STW     drawLine_num                        ; numerator = sx>>1
                    STW     drawLine_count                      ; for(count=sx>>1; counti>=0; --i)

                    MOVB    drawLine_dy1, giga_vAC + 1
                    MOVQB   giga_vAC, 0
                    ADDW    drawLine_dx1
                    STW     drawLine_dxy1                       ; dxy1 = dx1 + (dy1<<8)
    
                    MOVB    drawLine_dy2, giga_vAC + 1
                    MOVQB   giga_vAC, 0
                    ADDW    drawLine_dx2
                    STW     drawLine_dxy2                       ; dxy2 = dx2 + (dy2<<8)
                    CALLI   drawVTLineLoop
%ENDS

%SUB                drawVTLineLoop
drawVTLineLoop      LDW     drawLine_xy1
                    CALLI   drawPixel                           ; plot start pixel

                    LDW     drawLine_xy2
                    CALLI   drawPixel                           ; plot end pixel, (meet in middle)
                    
                    ADDVW   drawLine_sy, drawLine_num           ; numerator += sy
                    SUBW    drawLine_sx
                    BLE     drawVTL_flip                        ; if(numerator <= sx) goto flip
                    STW     drawLine_num                        ; numerator -= sx
                    
                    ADDVW   drawLine_dxy1, drawLine_xy1         ; xy1 += dxy1
                    SUBVW   drawLine_dxy1, drawLine_xy2         ; xy2 -= dxy1
                    BRA     drawVTL_count
                    
drawVTL_flip        ADDVW   drawLine_dxy2, drawLine_xy1         ; xy1 += dxy2
                    SUBVW   drawLine_dxy2, drawLine_xy2         ; xy2 -= dxy2
                    
drawVTL_count       DBNE    drawLine_count, drawVTLineLoop
                    POP                                         ; matches drawVTLine's PUSH
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
                    LDW     drawCircle_r
                    STW     drawCircle_y
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
                    DECW    drawCircle_y

drawC_cont          INC     drawCircle_x
                    LDW     drawCircle_x
                    SUBW    drawCircle_y
                    BLE     drawC_loop

                    POP
                    RET
%ENDS

%SUB                drawCircleExt1
drawCircleExt1      PUSH
                    LDW     drawCircle_cy
                    ADDW    drawCircle_y
                    ST      drawCircle_ch0 + 1
                    LDW     drawCircle_cy
                    SUBW    drawCircle_y
                    ST      drawCircle_ch1 + 1
                    LDW     drawCircle_cy
                    ADDW    drawCircle_x
                    ST      drawCircle_ch2 + 1
                    LDW     drawCircle_cy
                    SUBW    drawCircle_x
                    ST      drawCircle_ch3 + 1

                    LDW     drawCircle_cx
                    ADDW    drawCircle_x
                    ADDW    drawCircle_ch0
                    STW     drawCircle_a
                    LD      fgbgColour + 1
                    POKE    drawCircle_a

                    LDW     drawCircle_cx
                    SUBW    drawCircle_x
                    ADDW    drawCircle_ch0
                    STW     drawCircle_a
                    LD      fgbgColour + 1
                    POKE    drawCircle_a
                    
                    LDW     drawCircle_cx
                    ADDW    drawCircle_x
                    ADDW    drawCircle_ch1
                    STW     drawCircle_a
                    LD      fgbgColour + 1
                    POKE    drawCircle_a

                    LDW     drawCircle_cx
                    SUBW    drawCircle_x
                    ADDW    drawCircle_ch1
                    STW     drawCircle_a
                    LD      fgbgColour + 1
                    POKE    drawCircle_a
                    
                    CALLI   drawCircleExt2                      ; doesn't return to here
%ENDS
                    
%SUB                drawCircleExt2
drawCircleExt2      LDW     drawCircle_cx
                    ADDW    drawCircle_y
                    ADDW    drawCircle_ch2
                    STW     drawCircle_a
                    LD      fgbgColour + 1
                    POKE    drawCircle_a

                    LDW     drawCircle_cx
                    SUBW    drawCircle_y
                    ADDW    drawCircle_ch2
                    STW     drawCircle_a
                    LD      fgbgColour + 1
                    POKE    drawCircle_a
                    
                    LDW     drawCircle_cx
                    ADDW    drawCircle_y
                    ADDW    drawCircle_ch3
                    STW     drawCircle_a
                    LD      fgbgColour + 1
                    POKE    drawCircle_a

                    LDW     drawCircle_cx
                    SUBW    drawCircle_y
                    ADDW    drawCircle_ch3
                    STW     drawCircle_a
                    LD      fgbgColour + 1
                    POKE    drawCircle_a

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
                    
drawCF_wloop        LDW     drawCircleF_cx
                    SUBW    drawCircleF_r
                    STW     drawCircleF_x1
                    LDW     drawCircleF_cx
                    ADDW    drawCircleF_r
                    STW     drawCircleF_x2
                    SUBW    drawCircleF_x1
                    STW     drawCircleF_cnt
                    LDW     drawCircleF_cy
                    SUBW    drawCircleF_v
                    STW     giga_sysArg3
                    ADDBI   giga_sysArg3, 8                     ; high start address top
                    MOVB    drawCircleF_x1, giga_sysArg2        ; low start address top
                    MOVB    drawCircleF_cnt, giga_sysArg0       ; count top
                    SYS     54                                  ; fill memory
                    LDW     drawCircleF_cy
                    ADDW    drawCircleF_v
                    STW     giga_sysArg3
                    ADDBI   giga_sysArg3, 8                     ; high start address bottom
                    MOVB    drawCircleF_x1, giga_sysArg2        ; low start address bottom
                    MOVB    drawCircleF_cnt, giga_sysArg0       ; count bottom
                    SYS     54                                  ; fill memory
                    LDW     drawCircleF_w
                    ADDW    drawCircleF_v
                    ADDW    drawCircleF_v
                    ADDI    1
                    STW     drawCircleF_w
                    INC     drawCircleF_v
                    
drawCF_rloop        LDW     drawCircleF_w
                    BLT     drawCF_wloop
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
                    LDW     drawRect_x1
                    STW     drawHLine_x1
                    LDW     drawRect_y1
                    STW     drawHLine_y1
                    LDW     drawRect_x2
                    STW     drawHLine_x2
                    CALLI   drawHLine
                    LDW     drawRect_y2
                    STW     drawHLine_y1
                    CALLI   drawHLine

                    LDW     drawRect_x1
                    STW     drawVLine_x1
                    LDW     drawRect_y1
                    STW     drawVLine_y1
                    LDW     drawRect_y2
                    STW     drawVLine_y2
                    CALLI   drawVLine
                    LDW     drawRect_x2
                    STW     drawVLine_x1
                    LDW     drawRect_y1
                    STW     drawVLine_y1
                    LDW     drawRect_y2
                    STW     drawVLine_y2
                    CALLI   drawVLine

                    POP
                    RET
%ENDS

%SUB                drawRectF
drawRectF           LDWI    SYS_SetMemory_v2_54
                    STW     giga_sysFn
                    LD      drawRectF_y2
                    SUBW    drawRectF_y1
                    STW     drawRectF_ycnt                      ; line count if y2 > y1
                    BGE     drawRFY_cont
                    MOVB    drawRectF_y2, drawRectF_y1
                    NEGW    drawRectF_ycnt                      ; line count if y1 > y2
                    
drawRFY_cont        ADDBI   drawRectF_y1, 8                     ; high start address
                    INC     drawRectF_ycnt                      ; line count++ for DBNE
                    
                    LD      drawRectF_x2
                    SUBW    drawRectF_x1
                    BGE     drawRFX_cont                        ; x count if x2 > x1
                    MOVB    drawRectF_x2, drawRectF_x1          ; low start address
                    NEGW    giga_vAC                            ; x count if x1 > x2
                    
drawRFX_cont        ADDI    1                                   ; x count++
                    ST      drawRectF_xcnt
                    MOVB    fgbgColour + 1, giga_sysArg1        ; fill value
                    MOVB    drawRectF_y1, giga_sysArg3          ; high start address
                    
drawRF_loop         MOVB    drawRectF_xcnt, giga_sysArg0        ; x count
                    MOVB    drawRectF_x1, giga_sysArg2          ; low start address
                    SYS     54                                  ; fill memory
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
                    PEEK+   drawPoly_addr
                    STW     drawLine_x2
                    SUBI    255
                    BEQ     drawP_exit
                    LDW     drawLine_x2
                    ST      cursorXY
                    PEEK+   drawPoly_addr
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
                    DEEK+   drawPoly_addr
                    STW     drawLine_x2
                    SUBI    255
                    BEQ     drawPR_exit
                    LDW     drawLine_x1
drawPR_x2           ADDW    drawLine_x2                         ;relative X mode
                    STW     drawLine_x2
                    ST      cursorXY
                    DEEK+   drawPoly_addr
                    STW     drawLine_y2
                    LDW     drawLine_y1
drawPR_y2           ADDW    drawLine_y2                         ;relative Y mode
                    STW     drawLine_y2
                    ST      cursorXY + 1
                    CALLI   drawLineSlow
                    BRA     drawPR_loop
                    
drawPR_exit         MOVQB   drawPoly_mode, 0x99                 ;ADDW
                    CALLI   setPolyRelFlipX
                    CALLI   setPolyRelFlipY                     ;reset X and Y modes
                    POP
                    RET
%ENDS

%SUB                setPolyRelFlipX
setPolyRelFlipX     LDWI    drawPR_x2
                    STW     drawPoly_addr
                    LDW     drawPoly_mode
                    POKE    drawPoly_addr
                    RET
%ENDS

%SUB                setPolyRelFlipY
setPolyRelFlipY     LDWI    drawPR_y2
                    STW     drawPoly_addr
                    LD      drawPoly_mode
                    POKE    drawPoly_addr
                    RET
%ENDS

%SUB                parityFill
parityFill          LDWI    SYS_ParityFill_vX_44
                    STW     giga_sysFn
                    MOVB    giga_sysArg2, register0
                    MOVB    giga_sysArg4, register0 + 1
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
                    LD      register0
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