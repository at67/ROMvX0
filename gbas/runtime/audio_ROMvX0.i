midiId              EQU     register0
audioAddr           EQU     register1
waveType            EQU     register2
chnCount            EQU     register3
midiNote            EQU     register4                           ; register4 to register7 are the only free registers during time slicing
midiCommand         EQU     register5
midiPtr             EQU     register6
sndChannel          EQU     register8
sndFrequency        EQU     register9
sndVolume           EQU     register10
sndWaveType         EQU     register11
musicStream         EQU     register8
musicNote           EQU     register9
musicCommand        EQU     register10
musicPtr            EQU     register11


%SUB                resetAudio
resetAudio          LDI     0
                    FREQI   0                                   ; zero freq 0
                    FREQI   1                                   ; zero freq 1
                    FREQI   2                                   ; zero freq 2
                    FREQI   3                                   ; zero freq 3
                    OSCZ    0                                   ; turn off osc 0, (suppresses audio glitches)
                    OSCZ    1                                   ; turn off osc 1, (suppresses audio glitches)
                    OSCZ    2                                   ; turn off osc 2, (suppresses audio glitches)
                    OSCZ    3                                   ; turn off osc 3, (suppresses audio glitches)
                    LD      waveType + 1
                    MODI   0                                    ; vol = 0, wav = <wavType + 1> for channel 0
                    MODI   1                                    ; vol = 0, wav = <wavType + 1> for channel 1
                    MODI   2                                    ; vol = 0, wav = <wavType + 1> for channel 2
                    MODI   3                                    ; vol = 0, wav = <wavType + 1> for channel 3
                    RET
%ENDS

%SUB                resetMidi
resetMidi           PUSH
                    MOVQW   midiDelay, 1                        ; instant MIDI startup
                    CALLI   resetAudio
                    POP
                    RET
%ENDS

%SUB                resetMusic
resetMusic          PUSH
                    CALLI   resetAudio
                    POP
                    RET
%ENDS

%SUB                setMidiStream
setMidiStream       ARRVW   midiId, _midisLut_
                    DEEK
                    RET
%ENDS

%SUB                playMidi
playMidi            LDW     midiStream
                    BEQ     playM_exit0                         ; 0x0000 = stop
                    MOVQB   giga_soundTimer, 5                  ; keep pumping soundTimer
                    DECWA   midiDelay
                    BLE     playM_start
                    
playM_exit0         RET

playM_start         PUSH

playM_process       PEEKV+  midiStream                          ; get midi stream byte
                    ST      midiCommand
                    ANDI    0xF0                    
                    XORI    0x90                                ; check for start note
                    BNE     playM_endnote

                    CALLI   midiStartNote                       ; start note
                    BRA     playM_process
                    
playM_endnote       XORI    0x10                                ; check for end note
                    BNE     playM_segment
                    FREQM   midiCommand                         ; end note
                    BRA     playM_process

playM_segment       XORI    0x50                                ; check for new segment
                    BNE     playM_delay

                    DEEKV   midiStream                          ; midi score
                    STW     midiStream                          ; 0xD0 new midi segment address
                    BEQ     playM_exit1                         ; 0x0000 = stop
                    BRA     playM_process
    
playM_delay         LD      midiCommand
                    STW     midiDelay
                    
playM_exit1         POP
                    RET
%ENDS

%SUB                playMidiVol
playMidiVol         LDW     midiStream
                    BEQ     playMV_exit0                        ; 0x0000 = stop
                    MOVQB   giga_soundTimer, 5                  ; keep pumping soundTimer
                    DECWA   midiDelay
                    BLE     playMV_start

playMV_exit0        RET

playMV_start        PUSH

playMV_process      PEEKV+  midiStream                          ; get midi stream byte
                    ST      midiCommand
                    ANDI    0xF0
                    XORI    0x90                                ; check for start note
                    BNE     playMV_endnote
                    
                    CALLI   midiStartNote                       ; start note
                    CALLI   midiSetVolume                       ; set note volume
                    BRA     playMV_process
                    
playMV_endnote      XORI    0x10                                ; check for end note
                    BNE     playMV_segment
                    FREQM   midiCommand                         ; end note
                    BRA     playMV_process

playMV_segment      XORI    0x50                                ; check for new segment
                    BNE     playMV_delay

                    DEEKV   midiStream                          ; midi score
                    STW     midiStream                          ; 0xD0 new midi segment address
                    BEQ     playMV_exit1                        ; 0x0000 = stop
                    BRA     playMV_process
    
playMV_delay        LD      midiCommand
                    STW     midiDelay

playMV_exit1        POP
                    RET
%ENDS

%SUB                midiStartNote
midiStartNote       PEEKV+  midiStream                          ; midi note
                    MIDI                                        ; ROM note
                    FREQM   midiCommand                         ; freq address 0x01FC <-> 0x04FC
                    RET

midiSetVolume       PEEKV+  midiStream
                    VOLM    midiCommand                         ; wavA address 0x01FA <-> 0x04FA
                    RET
%ENDS

%SUB                midiGetNote
midiGetNote         LD      musicNote
                    MIDI
                    STW     musicNote
                    RET
%ENDS

%SUB                playMusic
playMusic           PUSH
                    
playN_process       PEEKV+  musicStream                         ; get music stream byte
                    ST      musicCommand
                    ANDI    0xF0
                    XORI    0x90                                ; check for start note
                    BNE     playN_endnote
                    PEEKV+  musicStream                         ; get music note
                    MIDI                                        ; get midi note from ROM
                    FREQM   musicCommand
                    BRA     playN_process
                    
playN_endnote       XORI    0x10                                ; check for end note
                    BNE     playN_segment
                    FREQM   musicCommand         			    ; end note
                    BRA     playN_process

playN_segment       XORI    0x50                                ; check for new segment
                    BNE     playN_delay
                    DEEKV   musicStream                         ; music stream
                    STW     musicStream                         ; 0xD0 new music segment address
                    BNE     playN_process                       ; 0x0000 = stop
                    POP
                    RET

playN_delay         LD      musicCommand
                    ST      giga_soundTimer                     ; keep pumping soundTimer
                    STW     waitVBlankNum
                    CALLI   waitVBlanks
                    BRA     playN_process
%ENDS

%SUB                musicGetNote
musicGetNote        LD      musicNote
                    NOTE
                    RET
%ENDS

%SUB                musicPlayNote
musicPlayNote       FREQM   musicCommand                        ; set channel freq
                    RET
%ENDS

%SUB                soundAllOff
soundAllOff         LDI     0
                    FREQI   0                                   ; turn off channel 0
                    FREQI   1                                   ; turn off channel 1
                    FREQI   2                                   ; turn off channel 2
                    FREQI   3                                   ; turn off channel 3
                    RET
%ENDS

%SUB                soundOff
soundOff            LDI     0
                    FREQA   sndChannel + 1                      ; turn off channel
                    RET
%ENDS

%SUB                soundOn
soundOn             LSRB    sndFrequency                        ; right shift low byte of sndFrequency by 1
                    LDW     sndFrequency                        ; format = high:8 low:07, (bit 7 of low byte = 0)
                    FREQA   sndChannel + 1                      ; turn on channel
                    RET
%ENDS

%SUB                soundOnV
soundOnV            LSRB    sndFrequency                        ; right shift low byte of sndFrequency by 1
                    LDW     sndFrequency                        ; format = high:8 low:07, (bit 7 of low byte = 0)
                    FREQA   sndChannel + 1                      ; turn on channel
                    LD      sndVolume
                    VOLA    sndChannel + 1
                    LD      sndWaveType
                    MODA    sndChannel + 1
                    RET
%ENDS

%SUB                soundMod
soundMod            MOVQB   sndChannel, 0xFA
                    LDW     sndWaveType                         ; format = high:waveX low:waveA
                    DOKE    sndChannel                          ; set modulation
                    RET
%ENDS
