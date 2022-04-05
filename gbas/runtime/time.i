timeByte            EQU     register0
timeDigit           EQU     register4
timeArrAddr         EQU     register5
timeStrAddr         EQU     register6
timeDelta           EQU     register7


%SUB                tickTime
tickTime            LD      timerPrev
                    STW     timeDelta
                    LD      giga_frameCount
                    ST      timerPrev                           ; reset timerPrev
                    SUBW    timeDelta
                    BEQ     tickT_exit                          ; exit if we are still on the same frame
                    STW     timeDelta                           ; framCounter - prev
                    BGT     tickT_pos
                    LDWI    256
                    ADDW    timeDelta
                    STW     timeDelta                           ; delta is negative and frameCount wraps around at 256
                    
tickT_pos           ADDW    timerTick
                    STW     timerTick                           ; 1/60 user timer, (max time = 546.116 seconds)
                    LD      timerPrev + 1
                    ADDW    timeDelta
                    ST      timerPrev + 1                       ; 1/60 internal counter
                    SUBI    60
                    BLT     tickT_exit                          ; exit because at least 1 second has not elapsed
                    ST      timerPrev + 1                       ; reset 1/60 internal counter
                    PUSH
                    LDWI    handleTime                          ; handle time every second
                    CALL    giga_vAC
                    POP

tickT_exit          RET                    
%ENDS

%SUB                handleTime
handleTime          LDWI    _timeArray_
                    STW     timeArrAddr
                    PEEK
                    ADDI    1
                    POKE    timeArrAddr                         ; seconds
                    XORI    60
                    BNE     handleT_exit
                    LDI     0
                    POKE    timeArrAddr                         ; reset seconds
                    
                    INC     timeArrAddr
                    LDW     timeArrAddr
                    PEEK
                    ADDI    1
                    POKE    timeArrAddr                         ; minutes
                    XORI    60
                    BNE     handleT_exit
                    LDI     0
                    POKE    timeArrAddr                         ; reset minutes
                    
                    INC     timeArrAddr
                    LDW     timeArrAddr
                    PEEK
                    ADDI    1
                    POKE    timeArrAddr                         ; hours
handleT_mode        XORI    24                                  ; [handleT_mode + 1] = 12 hour/24 hour
                    BNE     handleT_exit
handleT_epoch       LDI     0                                   ; [handleT_epoch + 1] = start hour
                    POKE    timeArrAddr                         ; reset hours

handleT_exit        RET                    
%ENDS

%SUB                timeDigits
timeDigits          LDW     timeByte

timeD_index         SUBI    10
                    BLT     timeD_cont
                    STW     timeByte
                    INC     timeDigit                           ; calculate 10's digit
                    BRA     timeD_index
    
timeD_cont          LD      timeDigit
                    ORI     0x30
                    POKE    timeStrAddr                         ; store 10's digit
                    INC     timeStrAddr
                    LD      timeByte
                    ORI     0x30
                    POKE    timeStrAddr                         ; store 1's digit
                    INC     timeStrAddr                         ; skip colon
                    INC     timeStrAddr                         ; next 10's digit
                    LDI     0x30
                    ST      timeDigit                           ; reset 10's digit
                    RET
%ENDS

%SUB                timeString
                    ; create a time string
timeString          PUSH
                    LDI     0
                    STW     timeDigit
                    LDWI    _timeString_ + 1
                    STW     timeStrAddr                         ; skip length byte

                    LDWI    _timeArray_
                    STW     timeArrAddr
                    ADDI    2
                    PEEK                                        ; hours
                    STW     timeByte
                    LDWI    timeDigits
                    CALL    giga_vAC

                    LDW     timeArrAddr
                    ADDI    1
                    PEEK                                        ; minutes
                    STW     timeByte
                    LDWI    timeDigits
                    CALL    giga_vAC

                    LDW     timeArrAddr
                    PEEK                                        ; seconds
                    STW     timeByte
                    LDWI    timeDigits
                    CALL    giga_vAC
                    POP
                    RET
%ENDS
