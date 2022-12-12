timeByte            EQU     register0
timeDigit           EQU     register1
timeStrAddr         EQU     register2
timeArrayExt        EQU     register3                           ; reg0-3 are used outside of the interrupt
timeArrayInt        EQU     register4                           ; reg4 is used within the interrupt


%SUB                tickTime
%if TIME_HANDLER
tickTime            LD      giga_jiffiesTick
                    BNE     tickT_exit

                    PUSH
                    CALLI   handleTime                          ; handle time every second
                    POP
                    
tickT_exit          RET
%else
tickTime            RET
%endif

%ENDS

%SUB                handleTime
handleTime          LDWI    _timeArray_
                    STW     timeArrayInt
                    PEEK
                    ADDI    1
                    POKE    timeArrayInt                        ; seconds
                    XORI    60
                    BNE     handleT_exit
                    LDI     0
                    POKEV+  timeArrayInt                        ; reset seconds
                    
                    PEEKV   timeArrayInt
                    ADDI    1
                    POKE    timeArrayInt                        ; minutes
                    XORI    60
                    BNE     handleT_exit
                    LDI     0
                    POKEV+  timeArrayInt                        ; reset minutes
                    
                    PEEKV   timeArrayInt
                    ADDI    1
                    POKE    timeArrayInt                        ; hours
handleT_mode        SUBI    24                                  ; [handleT_mode + 1] = 12 hour/24 hour
                    BLT     handleT_exit
handleT_epoch       LDI     0                                   ; [handleT_epoch + 1] = start hour
                    POKE    timeArrayInt                        ; reset hours

handleT_exit        RET
%ENDS

%SUB                timeDigits
timeDigits          STW     timeByte

timeD_index         SUBI    10
                    BLT     timeD_cont
                    STW     timeByte
                    INC     timeDigit                           ; calculate 10's digit
                    BRA     timeD_index
    
timeD_cont          ORBK    timeDigit, 0x30
                    POKEV+  timeStrAddr                         ; store 10's digit
                    ORBK    timeByte, 0x30
                    POKEV+  timeStrAddr                         ; store 1's digit
                    INC     timeStrAddr                         ; skip colon, next 10's digit
                    MOVQB   timeDigit, 0x30                     ; reset 10's digit
                    RET
%ENDS

%SUB                timeString
                    ; create a time string
timeString          PUSH
                    MOVQW   timeDigit, 0
                    LDWI    _timeString_ + 1
                    STW     timeStrAddr                         ; skip length byte

                    LDWI    _timeArray_ + 2
                    STW     timeArrayExt
                    PEEK                                        ; hours
                    CALLI   timeDigits

                    DEC     timeArrayExt
                    PEEKV   timeArrayExt                        ; minutes
                    CALLI   timeDigits

                    DEC     timeArrayExt
                    PEEKV   timeArrayExt                        ; seconds
                    CALLI   timeDigits
                    POP
                    RET
%ENDS
