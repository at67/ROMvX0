; do *NOT* use register4 to register7 during time slicing
realTimeAddr        EQU     register0
realTimeProc0       EQU     register1
realTimeProc1       EQU     register2
realTimeProc2       EQU     register3
numericLabel        EQU     register0
defaultLabel        EQU     register1
lutLabs             EQU     register2
lutAddrs            EQU     register3
lutIndex            EQU     register8
romUser             EQU     register0                           ; user requested romType
romType             EQU     register1                           ; actual romType
romReadAddr         EQU     register0               
romErrAddr          EQU     0x7E                                ; loader happy constant address
vramErrAddr         EQU     0x0101

romErrAddr          DW      vramErrAddr
 

; can't use any new instructions in romCheck
%SUB                romCheck
romCheck            LD      giga_romType
                    ANDI    0xFC
                    STW     romType
                    SUBW    romUser
                    BEQ     romC_return                         ; romType = romUser, so ok
                    LDW     romUser
                    SUBI    0x80                                ; experimental ROM's must match exactly
                    BNE     romC_check
                    LDW     romType
                    SUBW    romUser
                    BEQ     romC_return                         ; romType = romUser, so ok
                    BRA     romC_fail
                    
romC_check          LDW     romType                             ; non experimental ROM
                    SUBW    romUser
                    BGT     romC_return                         ; romType > romUser, so ok
                    
romC_fail           LD      giga_rand0
                    POKE    romErrAddr                          ; random horizontal scroll
                    BRA     romC_fail

romC_return         RET
%ENDS

%SUB                romExec
romExec             STW     giga_vLR
                    MOVQW   giga_sysFn, SYS_Exec_88             ; address < 0x0100 so use MOVQW
                    SYS     88                                  ; doesn't return from here!
%ENDS

%SUB                romRead
romRead             MOVQW   giga_sysFn, SYS_ReadRomDir_v5_80    ; address < 0x0100 so use MOVQW
                    LDW     romReadAddr
                    SYS     80
                    RET
%ENDS

%SUB                getRomType
getRomType          ANDBK   giga_romType, 0xFC
                    RET
%ENDS

%SUB                isRomTypeX
                    ; return zero if false, non zero if true
isRomTypeX          ANDBK   giga_romType, 0x80
                    BEQ     isRomX_false
                    CMPI    giga_romType, 0xF0
                    BGE     isRomX_false
                    RET
                    
isRomX_false        LDI     0
                    RET
%ENDS

%SUB                realTimeStub
                    ; runs real time, (vblank interrupt), code at regular intervals
                    ; self modifying code allows for timer, midi and user procs
realTimeStub        RET                                         ; RET gets replaced by PUSH
                    ALLOC   -2
                    LDW     giga_sysArg6                        ; save sysArg6/7 for ROMvX0
                    STLW    0 
                    
realTimeStub0       CALLI   0x0000                              ; 0x0000 = realTimeProc0

realTimeStub1       BRA     realTS_exit
                    RET                                         ; BRA + RET = realTimeProc1

realTimeStub2       BRA     realTS_exit
                    RET                                         ; BRA + RET = realTimeProc2
                    
realTS_exit         LDLW    0
                    STW     giga_sysArg6                        ; restore sysArg6/7 for ROMvX0
                    ALLOC   2
                    
realTS_rti          POP
                    MOVQB   giga_frameCount, 255                ; (256 - n) = vblank frequency
                    LDWI    &h0400
                    LUP     0x00                                ; RTI
%ENDS

%SUB                setRealTimeProc0
setRealTimeProc0    LDWI    realTimeStub
                    POKEI   0x75                                ; replace RET with PUSH
                    LDWI    realTimeStub0 + 1
                    DOKEA   realTimeProc0                       ; replace 0x0000 with proc
                    RET
%ENDS

%SUB                setRealTimeProc1
setRealTimeProc1    PUSH
                    CALLI   setRealTimeProc0
                    LDWI    realTimeStub1
                    POKEI   0x85                                ; replace BRA with CALLI
                    ADDI    1                                   ; realTimeStub1 + 1
                    DOKEA   realTimeProc1                       ; replace LBL + RET with proc
                    POP
                    RET
%ENDS

%SUB                setRealTimeProc2
setRealTimeProc2    PUSH
                    CALLI   setRealTimeProc1
                    LDWI    realTimeStub2
                    POKEI   0x85                                ; replace BRA with CALLI
                    ADDI    1                                   ; realTimeStub2 + 1
                    DOKEA   realTimeProc2                       ; replace LBL + RET with proc
                    POP
                    RET
%ENDS

%SUB                gotoNumericLabel
                    ; find numeric label and jump to it
gotoNumericLabel    LDWI    _lut_numericLabs
                    STW     lutLabs
                    STW     lutIndex
                    LDWI    _lut_numericAddrs
                    STW     lutAddrs
                    
gotoNL_loop         DEEKV   lutIndex
                    BNE     gotoNL_cont                         ; check for 0
                    LDW     defaultLabel
                    BEQ     gotoNL_exit
                    CALL    defaultLabel                        ; fetch default address and jump, (note we never return from here)
                    
gotoNL_exit         RET
                    
gotoNL_cont         SUBW    numericLabel
                    BEQ     gotoNL_found
                    ADDBI   lutIndex, lutIndex, 2
                    BRA     gotoNL_loop                         ; loop through lut until found or 0
                    
gotoNL_found        LDW     lutIndex
                    SUBW    lutLabs
                    ADDW    lutAddrs
                    DEEK
                    CALL    giga_vAC                            ; fetch label address and jump, (note we never return from here)
%ENDS

%SUB                gosubNumericLabel
                    ; find numeric label and call it, (it had better return or welcome to lala land)
gosubNumericLabel   PUSH
                    LDWI    _lut_numericLabs
                    STW     lutLabs
                    STW     lutIndex
                    LDWI    _lut_numericAddrs
                    STW     lutAddrs
                    
gosubNL_loop        DEEKV   lutIndex
                    BNE     gosubNL_cont                        ; check for 0
                    LDW     defaultLabel
                    BEQ     gosubNL_exit
                    CALL    defaultLabel                        ; fetch default address and call
                    
gosubNL_exit        POP
                    RET
                    
gosubNL_cont        SUBW    numericLabel
                    BEQ     gosubNL_found
                    ADDBI   lutIndex, lutIndex, 2
                    BRA     gosubNL_loop                        ; loop through lut until found or 0
                    
gosubNL_found       LDW     lutIndex
                    SUBW    lutLabs
                    ADDW    lutAddrs
                    DEEK
                    CALL    giga_vAC                            ; fetch label address and call
                    POP
                    RET
%ENDS