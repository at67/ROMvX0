%MACRO  LoopCounterTo1 _counter _label
        LD      _counter
        SUBI    0x01
        ST      _counter
        BGT     _label
%ENDM

%MACRO  LoopCounterTo0 _counter _label
        LD      _counter
        SUBI    0x01
        ST      _counter
        BGE     _label
%ENDM

%MACRO  LoopCounter _counter _label
        LD      _counter
        SUBI    0x01
        ST      _counter
        BNE     _label
%ENDM

%MACRO  LoopCounter1 _counter _label
        LD      _counter
        SUBI    0x01
        ST      _counter
        BGE     _label
%ENDM

%MACRO  LutPeek _lut _index
        LDWI    _lut
        DEEK
        ADDW    _index
        PEEK
%ENDM

%MACRO  LutDeek _lut _index _scratch
        LDWI    _lut
        DEEK
        ADDW    _index
        ADDW    _index
        DEEK
%ENDM

%MACRO  ForNextInc _var _label _end
        INC     _var
        LD      _var
        SUBI    _end
        BLE     _label
%ENDM

%MACRO  ForNextDec _var _label _end
        LDW     _var
        SUBI    1
        STW     _var
        SUBI    _end
        BGE     _label
%ENDM

%MACRO  ForNextDecOne _var _label
        LDW     _var
        SUBI    1
        STW     _var
        BNE     _label
%ENDM

%MACRO  ForNextFarDecOne _var _label
        LDW     _var
        SUBI    1
        STW     _var
        BLE     _label_ + 3
_label_ CALLI   _label
%ENDM

%MACRO  ForNextDecZero _var _label
        LDW     _var
        SUBI    1
        STW     _var
        BGE     _label
%ENDM

%MACRO  ForNextFarDecZero _var _label
        LDW     _var
        SUBI    1
        STW     _var
        BLT     _label_ + 3
_label_ CALLI   _label
%ENDM

%MACRO  ForNextAdd _var _label _end _step
        LDW     _var
        ADDI    _step
        STW     _var
        SUBI    _end
        BLE     _label
%ENDM

%MACRO  ForNextSub _var _label _end _step
        LDW     _var
        SUBI    _step
        STW     _var
        SUBI    _end
        BGE     _label
%ENDM

%MACRO  ForNextVarAdd _var _label _vEnd _vStep
        LDW     _var
        ADDW    _vStep
        STW     _var
        SUBW    _vEnd
        BLE     _label
%ENDM

%MACRO  ForNextVarSub _var _label _vEnd _vStep
        LDW     _var
        SUBW    _vStep
        STW     _var
        SUBW    _vEnd
        BGE     _label
%ENDM

%MACRO  ForNextFarInc _var _label _end
        INC     _var
        LD      _var
        SUBI    _end
        BGT     _label_ + 3
_label_ CALLI   _label
%ENDM

%MACRO  ForNextFarDec _var _label _end
        LDW     _var
        SUBI    1
        STW     _var
        SUBI    _end
        BLT     _label_ + 3
_label_ CALLI   _label
%ENDM

%MACRO  ForNextFarAdd _var _label _end _step
        LDW     _var
        ADDI    _step
        STW     _var
        SUBI    _end
        BGT     _label_ + 3
_label_ CALLI   _label
%ENDM

%MACRO  ForNextFarSub _var _label _end _step
        LDW     _var
        SUBI    _step
        STW     _var
        SUBI    _end
        BLT     _label_ + 3
_label_ CALLI   _label
%ENDM

%MACRO  ForNextFarVarAdd _var _label _vEnd _vStep
        LDW     _var
        ADDW    _vStep
        STW     _var
        SUBW    _vEnd
        BGT     _label_ + 3
_label_ CALLI   _label
%ENDM

%MACRO  ForNextFarVarSub _var _label _vEnd _vStep
        LDW     _var
        SUBW    _vStep
        STW     _var
        SUBW    _vEnd
        BLT     _label_ + 3
_label_ CALLI   _label
%ENDM

%MACRO  Return
        POP
        RET
%ENDM

%MACRO  GotoNumeric
        CALLI   gotoNumericLabel
%ENDM

%MACRO  GosubNumeric
        CALLI   gosubNumericLabel
%ENDM

%MACRO  ResetVideoTable
        CALLI   resetVideoTable
%ENDM

%MACRO  ResetVideoFlags
        CALLI   resetVideoFlags
%ENDM

%MACRO  ClearScreen
        CALLI   clearScreen
%ENDM

%MACRO  ClearRect
        CALLI   clearRect
%ENDM

%MACRO  ClearVertBlinds
        CALLI   clearVertBlinds
%ENDM

%MACRO  AtTextCursor
        CALLI   atTextCursor
%ENDM

%MACRO  TextWidth
        STW     textLen
        CALLI   textWidth
%ENDM

%MACRO  Input
        CALLI   input
%ENDM

%MACRO  NewLine
        CALLI   newLineScroll
%ENDM

%MACRO  PrintChr _chr
        LDI     _chr
        CALLI   printChr
%ENDM

%MACRO  PrintAcChr
        CALLI   printChr
%ENDM

%MACRO  PrintVarChr _var
        LD      _var
        CALLI   printChr
%ENDM

%MACRO  PrintSpc
        CALLI   printSpc
%ENDM

%MACRO  PrintHex
        CALLI   printHex
%ENDM

%MACRO  PrintString _str
        LDWI    _str
        CALLI   printText
%ENDM

%MACRO  PrintAcString
        CALLI   printText
%ENDM

%MACRO  PrintVarString _var
        LDW     _var
        CALLI   printText
%ENDM

%MACRO  PrintAcLeft
        CALLI   printLeft
%ENDM

%MACRO  PrintAcRight
        CALLI   printRight
%ENDM

%MACRO  PrintAcMid
        CALLI   printMid
%ENDM

%MACRO  PrintAcLower
        CALLI   printLower
%ENDM

%MACRO  PrintAcUpper
        CALLI   printUpper
%ENDM

%MACRO  PrintInt16 _int
        LDWI    _int
        CALLI   printInt16
%ENDM

%MACRO  PrintAcInt16
        CALLI   printInt16
%ENDM

%MACRO  PrintVarInt16 _var
        LDW     _var
        CALLI   printInt16
%ENDM

%MACRO  ReadIntVar
        CALLI   readIntVar
%ENDM

%MACRO  ReadStrVar
        CALLI   readStrVar
%ENDM

%MACRO  StringChr
        CALLI   stringChr
%ENDM

%MACRO  StringSpc
        STW     strAddr
        CALLI   stringSpc
%ENDM

%MACRO  StringHex
        STW     strAddr
        CALLI   stringHex
%ENDM

%MACRO  StringCopy
        CALLI   stringCopy
%ENDM

%MACRO  StringCmp
        CALLI   stringCmp
%ENDM

%MACRO  StringConcat
        CALLI   stringConcat
%ENDM

%MACRO  StringConcatLut
        CALLI   stringConcatLut
%ENDM

%MACRO  StringLeft
        CALLI   stringLeft
%ENDM

%MACRO  StringRight
        CALLI   stringRight
%ENDM

%MACRO  StringMid
        CALLI   stringMid
%ENDM

%MACRO  StringLower
        STW     strDstAddr
        CALLI   stringLower
%ENDM

%MACRO  StringUpper
        STW     strDstAddr
        CALLI   stringUpper
%ENDM

%MACRO  StringInt
        CALLI   stringInt
%ENDM

%MACRO  IntegerStr
        CALLI   integerStr
%ENDM

%MACRO  Absolute
        CALLI   absolute
%ENDM

%MACRO  Sign
        CALLI   sign
%ENDM

%MACRO  IntMin
        CALLI   integerMin
%ENDM

%MACRO  IntMax
        CALLI   integerMax
%ENDM

%MACRO  IntClamp
        CALLI   integerClamp
%ENDM

%MACRO  IntCondii id i0 i1
        BEQ     _id_
        LDWI    i0
        BRA     _id_ + 3
_id_    LDWI    i1
%ENDM

%MACRO  IntCondvi id v i
        BEQ     _id_
        LDW     v
        BRA     _id_ + 3
_id_    LDWI    i
%ENDM

%MACRO  IntCondiv id i v
        BEQ     _id_
        LDWI    i
        BRA     _id_ + 2
_id_    LDW     v
%ENDM

%MACRO  IntCondvv id v0 v1
        BEQ     _id_
        LDW     v0
        BRA     _id_ + 2
_id_    LDW     v1
%ENDM

%MACRO  Rand
        CALLI   rand16bit
%ENDM

%MACRO  RandMod
        STW     mathY
        CALLI   randMod16bit
%ENDM

%MACRO  Lsl4bit
        CALLI   lsl4bit
%ENDM

%MACRO  Lsl8bit
        CALLI   lsl8bit
%ENDM

%MACRO  Lsr1bit
        CALLI   lsr1bit
%ENDM

%MACRO  Lsr2bit
        CALLI   lsr2bit
%ENDM

%MACRO  Lsr3bit
        CALLI   lsr3bit
%ENDM

%MACRO  Lsr4bit
        CALLI   lsr4bit
%ENDM

%MACRO  Lsr5bit
        CALLI   lsr5bit
%ENDM

%MACRO  Lsr6bit
        CALLI   lsr6bit
%ENDM

%MACRO  Lsr7bit
        CALLI   lsr7bit
%ENDM

%MACRO  Lsr8bit
        CALLI   lsr8bit
%ENDM

%MACRO  Asr1bit
        CALLI   asr1bit
%ENDM

%MACRO  Asr2bit
        CALLI   asr2bit
%ENDM

%MACRO  Asr3bit
        CALLI   asr3bit
%ENDM

%MACRO  Asr4bit
        CALLI   asr4bit
%ENDM

%MACRO  Asr5bit
        CALLI    asr5bit
%ENDM

%MACRO  Asr6bit
        CALLI    asr6bit
%ENDM

%MACRO  Asr7bit
        CALLI   asr7bit
%ENDM

%MACRO  Asr8bit
        CALLI   asr8bit
%ENDM

%MACRO  ScanlineMode
        CALLI   scanlineMode
%ENDM

%MACRO  WaitVBlanks
        CALLI   waitVBlanks
%ENDM

%MACRO  WaitVBlank
        CALLI   waitVBlank
%ENDM

%MACRO  GetVBlank
        LD      giga_videoY
        ANDI    1
%ENDM

%MACRO  ReadPixel
        CALLI   readPixel
%ENDM

%MACRO  DrawPixel
        CALLI   drawPixel
%ENDM

%MACRO  DrawLine
        CALLI   drawLine
%ENDM

%MACRO  DrawVTLine
        CALLI   drawVTLine
%ENDM

%MACRO  DrawHLine
        CALLI   drawHLine
%ENDM

%MACRO  DrawVLine
        CALLI   drawVLine
%ENDM

%MACRO  DrawCircle
        CALLI   drawCircle
%ENDM

%MACRO  DrawCircleF
        CALLI   drawCircleF
%ENDM

%MACRO  DrawRect
        CALLI   drawRect
%ENDM

%MACRO  DrawRectF
        CALLI   drawRectF
%ENDM

%MACRO  DrawPoly
        CALLI   drawPoly
%ENDM

%MACRO  DrawPolyRel
        CALLI   drawPolyRel
%ENDM

%MACRO  SetPolyRelFlipX
        CALLI   setPolyRelFlipX
%ENDM

%MACRO  SetPolyRelFlipY
        CALLI   setPolyRelFlipY
%ENDM

%MACRO  AtLineCursor
        CALLI   atLineCursor
%ENDM

%MACRO  SetMidiStream
        STW     midiId
        CALLI   setMidiStream
%ENDM

%MACRO  PlayMidi
        STW     midiStream
        CALLI   resetMidi
%ENDM

%MACRO  PlayMidiV
        STW     midiStream
        CALLI   resetMidi
%ENDM

%MACRO  GetMidiNote
        CALLI   midiGetNote
%ENDM

%MACRO  PlayMusic
        STW     musicStream
        CALLI   resetMusic
        CALLI   playMusic
%ENDM

%MACRO  GetMusicNote
        CALLI   musicGetNote
%ENDM

%MACRO  TimeString
        CALLI   timeString
%ENDM

%MACRO  DrawBlit
        CALLI   drawBlit
%ENDM

%MACRO  DrawBlitX
        CALLI   drawBlitX
%ENDM

%MACRO  DrawBlitY
        CALLI   drawBlitY
%ENDM

%MACRO  DrawBlitXY
        CALLI   drawBlitXY
%ENDM

%MACRO  GetBlitLUT
        CALLI   getBlitLUT
%ENDM

%MACRO  SoundAll
        CALLI   soundAll
%ENDM

%MACRO  SoundAllOff
        CALLI   soundAllOff
%ENDM

%MACRO  SoundOff
        CALLI   soundOff
%ENDM

%MACRO  SoundOn
        CALLI   soundOn
%ENDM

%MACRO  SoundOnV
        CALLI   soundOnV
%ENDM

%MACRO  SoundMod
        CALLI   soundMod
%ENDM

%MACRO  BcdAdd
        CALLI   bcdAdd
%ENDM

%MACRO  BcdSub
        CALLI   bcdSub
%ENDM

%MACRO  BcdInt
        CALLI   bcdInt
%ENDM

%MACRO  BcdCmp
        CALLI   bcdCmp
%ENDM

%MACRO  BcdCpy
        CALLI   bcdCpy
%ENDM

%MACRO  JumpFalse _label id
        BNE     _id_ + 3                                ; unique id is used as an internal macro label
_id_    CALLI   _label
%ENDM

%MACRO  JumpTrue _label id
        BEQ     _id_ + 3                                ; unique id is used as an internal macro label
_id_    CALLI   _label
%ENDM

%MACRO  JumpEQ _label id                                ; inverted logic
        BEQ     _id_ + 3
_id_    CALLI   _label
%ENDM

%MACRO  JumpNE _label id                                ; inverted logic
        BNE     _id_ + 3
_id_    CALLI   _label
%ENDM

%MACRO  JumpLE _label id                                ; inverted logic
        BLE     _id_ + 3
_id_    CALLI   _label
%ENDM

%MACRO  JumpGE _label id                                ; inverted logic
        BGE     _id_ + 3
_id_    CALLI   _label
%ENDM

%MACRO  JumpLT _label id                                ; inverted logic
        BLT     _id_ + 3
_id_    CALLI   _label
%ENDM

%MACRO  JumpGT _label id                                ; inverted logic
        BGT     _id_ + 3
_id_    CALLI   _label
%ENDM

%MACRO  SwapByte
        CALLI   swapByte
%ENDM

%MACRO  SwapWord
        CALLI   swapWord
%ENDM

%MACRO  SwapBytes
        CALLI   swapBytes
%ENDM

%MACRO  SwapWords
        CALLI   swapWords
%ENDM

%MACRO  CopyBytes
        CALLI   copyBytes
%ENDM

%MACRO  CopyWords
        CALLI   copyWords
%ENDM

%MACRO  CopyDWords
        CALLI   copyDWords
%ENDM

%MACRO  CopyBytesFar
        CALLI   copyBytesFar
%ENDM

%MACRO  CopyWordsFar
        CALLI   copyWordsFar
%ENDM

%MACRO  CopyDWordsFar
        CALLI   copyDWordsFar
%ENDM

%MACRO  ResetVars _addr
        LDI     _addr
        STW     varAddress
        CALLI   resetVars
%ENDM

%MACRO  ResetMem _memAddr _memCount
        LDWI    _memAddr
        STW     ramAddr0
        LDWI    _memAddr + _memCount
        STW     ramAddr1
        CALLI   resetMem
%ENDM

; Can't use CALLI as this code can be run on ROM's < ROMv5a
%MACRO  RomCheck
        LDWI    romCheck
        CALL    giga_vAC
%ENDM

%MACRO  RomExec
        CALLI   romExec
%ENDM

%MACRO  RomRead
        STW     romReadAddr
        CALLI   romRead
%ENDM

%MACRO  Initialise
        LDWI    0x0F20
        STW     fgbgColour                              ; yellow on blue

        LD      giga_channelMask
        ORI     0x03
        ST      giga_channelMask                        ; enable 4 channel audio by default
        
        LDI     MISC_ENABLE_SCROLL_BIT
        STW     miscFlags                               ; reset flags
        LDI     0
        STW     midiStream                              ; reset MIDI
        LD      giga_frameCount
        STW     frameCountPrev                          ; reset frameCount shadow var
        
        CALLI   resetVideoFlags
%ENDM
