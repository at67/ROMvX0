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
        BLT     _label_ + 2
        LDWI    _label
_label_ CALL    giga_vAC        
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
        BGT     _label_ + 2
        LDWI    _label
_label_ CALL    giga_vAC
%ENDM

%MACRO  ForNextFarDec _var _label _end
        LDW     _var
        SUBI    1
        STW     _var
        SUBI    _end
        BLT     _label_ + 2
        LDWI    _label
_label_ CALL    giga_vAC
%ENDM

%MACRO  ForNextFarAdd _var _label _end _step
        LDW     _var
        ADDI    _step
        STW     _var
        SUBI    _end
        BGT     _label_ + 2
        LDWI    _label
_label_ CALL    giga_vAC
%ENDM

%MACRO  ForNextFarSub _var _label _end _step
        LDW     _var
        SUBI    _step
        STW      _var
        SUBI    _end
        BLT     _label_ + 2
        LDWI    _label
_label_ CALL    giga_vAC
%ENDM

%MACRO  ForNextFarVarAdd _var _label _vEnd _vStep
        LDW     _var
        ADDW    _vStep
        STW     _var
        SUBW    _vEnd
        BGT     _label_ + 2
        LDWI    _label
_label_ CALL    giga_vAC
%ENDM

%MACRO  ForNextFarVarSub _var _label _vEnd _vStep
        LDW     _var
        SUBW    _vStep
        STW     _var
        SUBW    _vEnd
        BLT     _label_ + 2
        LDWI    _label
_label_ CALL    giga_vAC
%ENDM

%MACRO  Return
        POP
        RET
%ENDM

%MACRO  GotoNumeric
        LDWI    gotoNumericLabel
        CALL    giga_vAC
%ENDM

%MACRO  GosubNumeric
        LDWI    gosubNumericLabel
        CALL    giga_vAC
%ENDM

%MACRO  ResetVideoTable
        LDWI    resetVideoTable
        CALL    giga_vAC
%ENDM

%MACRO  ResetVideoFlags
        LDWI    resetVideoFlags
        CALL    giga_vAC
%ENDM

%MACRO  ClearScreen
        LDWI    clearScreen
        CALL    giga_vAC
%ENDM

%MACRO  ClearRect
        LDWI    clearRect
        CALL    giga_vAC
%ENDM

%MACRO  ClearVertBlinds
        LDWI    clearVertBlinds
        CALL    giga_vAC
%ENDM

%MACRO  AtTextCursor
        LDWI    atTextCursor
        CALL    giga_vAC
%ENDM

%MACRO  TextWidth
        STW     textLen
        CALLI   textWidth
%ENDM

%MACRO  Input
        STW     inpLutAddr
        LDWI    input
        CALL    giga_vAC
%ENDM

%MACRO  NewLine
        LDWI    newLineScroll
        CALL    giga_vAC
%ENDM

%MACRO  PrintChr _chr
        LDI     _chr
        ST      textChr
        LDWI    printChr
        CALL    giga_vAC
%ENDM

%MACRO  PrintAcChr
        ST      textChr
        LDWI    printChr
        CALL    giga_vAC
%ENDM

%MACRO  PrintVarChr _var
        LD      _var
        ST      textChr
        LDWI    printChr
        CALL    giga_vAC
%ENDM

%MACRO  PrintSpc
        ST      textSpc
        LDWI    printSpc
        CALL    giga_vAC
%ENDM

%MACRO  PrintHex
        LDWI    printHex
        CALL    giga_vAC
%ENDM

%MACRO  PrintString _str
        LDWI    _str
        STW     textStr
        LDWI    printText
        CALL    giga_vAC
%ENDM

%MACRO  PrintAcString
        STW     textStr
        LDWI    printText
        CALL    giga_vAC
%ENDM

%MACRO  PrintVarString _var
        LDW     _var
        STW     textStr
        LDWI    printText
        CALL    giga_vAC
%ENDM

%MACRO  PrintAcLeft
        LDWI    printLeft
        CALL    giga_vAC
%ENDM

%MACRO  PrintAcRight
        LDWI    printRight
        CALL    giga_vAC
%ENDM

%MACRO  PrintAcMid
        LDWI    printMid
        CALL    giga_vAC
%ENDM

%MACRO  PrintAcLower
        LDWI    printLower
        CALL    giga_vAC
%ENDM

%MACRO  PrintAcUpper
        LDWI    printUpper
        CALL    giga_vAC
%ENDM

%MACRO  PrintInt16 _int
        LDWI    _int
        STW     textNum    
        LDWI    printInt16
        CALL    giga_vAC
%ENDM

%MACRO  PrintAcInt16
        STW     textNum    
        LDWI    printInt16
        CALL    giga_vAC
%ENDM

%MACRO  PrintVarInt16 _var
        LDW     _var
        STW     textNum    
        LDWI    printInt16
        CALL    giga_vAC
%ENDM

%MACRO  ReadIntVar
        LDWI    readIntVar
        CALL    giga_vAC
%ENDM

%MACRO  ReadStrVar
        LDWI    readStrVar
        CALL    giga_vAC
%ENDM

%MACRO  StringChr
        STW     strDstAddr
        LDWI    stringChr
        CALL    giga_vAC
%ENDM

%MACRO  StringSpc
        STW     strAddr
        LDWI    stringSpc
        CALL    giga_vAC
%ENDM

%MACRO  StringHex
        STW     strAddr
        LDWI    stringHex
        CALL    giga_vAC
%ENDM

%MACRO  StringCopy
        STW     strDstAddr
        LDWI    stringCopy
        CALL    giga_vAC
%ENDM

%MACRO  StringCmp
        STW     strSrcAddr2
        LDWI    stringCmp
        CALL    giga_vAC
%ENDM

%MACRO  StringConcat
        STW     strDstAddr
        LDWI    stringConcat
        CALL    giga_vAC
%ENDM

%MACRO  StringConcatLut
        STW     strDstAddr
        LDWI    stringConcatLut
        CALL    giga_vAC
%ENDM

%MACRO  StringLeft
        STW     strDstAddr
        LDWI    stringLeft
        CALL    giga_vAC
%ENDM

%MACRO  StringRight
        STW     strDstAddr
        LDWI    stringRight
        CALL    giga_vAC
%ENDM

%MACRO  StringMid
        STW     strDstAddr
        LDWI    stringMid
        CALL    giga_vAC
%ENDM

%MACRO  StringLower
        STW     strDstAddr
        LDWI    stringLower
        CALL    giga_vAC
%ENDM

%MACRO  StringUpper
        STW     strDstAddr
        LDWI    stringUpper
        CALL    giga_vAC
%ENDM

%MACRO  StringInt
        STW     strDstAddr
        LDWI    stringInt
        CALL    giga_vAC
%ENDM

%MACRO  IntegerStr
        STW     intSrcAddr
        LDWI    integerStr
        CALL    giga_vAC
%ENDM

%MACRO  Absolute
        STW     mathX
        LDWI    absolute
        CALL    giga_vAC
%ENDM

%MACRO  Sign
        STW     mathX
        LDWI    sign
        CALL    giga_vAC
%ENDM

%MACRO  IntMin
        STW     intSrcB
        LDWI    integerMin
        CALL    giga_vAC
%ENDM

%MACRO  IntMax
        STW     intSrcB
        LDWI    integerMax
        CALL    giga_vAC
%ENDM

%MACRO  IntClamp
        STW     intSrcB
        LDWI    integerClamp
        CALL    giga_vAC
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
        LDWI    rand16bit
        CALL    giga_vAC
%ENDM

%MACRO  RandMod
        STW     mathY
        LDWI    randMod16bit
        CALL    giga_vAC
%ENDM        

%MACRO  Lsl4bit
        LDWI    lsl4bit
        CALL    giga_vAC
%ENDM

%MACRO  Lsl8bit
        LDWI    lsl8bit
        CALL    giga_vAC
%ENDM

%MACRO  Lsr1bit
        LDWI    lsr1bit
        CALL    giga_vAC
%ENDM

%MACRO  Lsr2bit
        LDWI    lsr2bit
        CALL    giga_vAC
%ENDM

%MACRO  Lsr3bit
        LDWI    lsr3bit
        CALL    giga_vAC
%ENDM

%MACRO  Lsr4bit
        LDWI    lsr4bit
        CALL    giga_vAC
%ENDM

%MACRO  Lsr5bit
        LDWI    lsr5bit
        CALL    giga_vAC
%ENDM

%MACRO  Lsr6bit
        LDWI    lsr6bit
        CALL    giga_vAC
%ENDM

%MACRO  Lsr7bit
        LDWI    lsr7bit
        CALL    giga_vAC
%ENDM

%MACRO  Lsr8bit
        LDWI    lsr8bit
        CALL    giga_vAC
%ENDM

%MACRO  Asr1bit
        LDWI    asr1bit
        CALL    giga_vAC
%ENDM

%MACRO  Asr2bit
        LDWI    asr2bit
        CALL    giga_vAC
%ENDM

%MACRO  Asr3bit
        LDWI    asr3bit
        CALL    giga_vAC
%ENDM

%MACRO  Asr4bit
        LDWI    asr4bit
        CALL    giga_vAC
%ENDM

%MACRO  Asr5bit
        LDWI    asr5bit
        CALL    giga_vAC
%ENDM

%MACRO  Asr6bit
        LDWI    asr6bit
        CALL    giga_vAC
%ENDM

%MACRO  Asr7bit
        LDWI    asr7bit
        CALL    giga_vAC
%ENDM

%MACRO  Asr8bit
        LDWI    asr8bit
        CALL    giga_vAC
%ENDM

%MACRO  ScanlineMode
        LDWI    scanlineMode
        CALL    giga_vAC
%ENDM

%MACRO  WaitVBlanks
        LDWI    waitVBlanks
        CALL    giga_vAC
%ENDM

%MACRO  WaitVBlank
        LDWI    waitVBlank
        CALL    giga_vAC
%ENDM

%MACRO  GetVBlank
        LD      giga_videoY
        ANDI    1
%ENDM

%MACRO  ReadPixel
        LDWI    readPixel
        CALL    giga_vAC
%ENDM

%MACRO  DrawPixel
        LDWI    drawPixel
        CALL    giga_vAC
%ENDM

%MACRO  DrawLine
        LDWI    drawLine
        CALL    giga_vAC
%ENDM

%MACRO  DrawVTLine
        LDWI    drawVTLine
        CALL    giga_vAC
%ENDM

%MACRO  DrawHLine
        LDWI    drawHLine
        CALL    giga_vAC
%ENDM

%MACRO  DrawVLine
        LDWI    drawVLine
        CALL    giga_vAC
%ENDM

%MACRO  DrawCircle
        LDWI    drawCircle
        CALL    giga_vAC
%ENDM

%MACRO  DrawCircleF
        LDWI    drawCircleF
        CALL    giga_vAC
%ENDM

%MACRO  DrawRect
        LDWI    drawRect
        CALL    giga_vAC
%ENDM

%MACRO  DrawRectF
        LDWI    drawRectF
        CALL    giga_vAC
%ENDM

%MACRO  DrawPoly
        LDWI    drawPoly
        CALL    giga_vAC
%ENDM

%MACRO  DrawPolyRel
        LDWI    drawPolyRel
        CALL    giga_vAC
%ENDM

%MACRO  SetPolyRelFlipX
        LDWI    setPolyRelFlipX
        CALL    giga_vAC
%ENDM

%MACRO  SetPolyRelFlipY
        LDWI    setPolyRelFlipY
        CALL    giga_vAC
%ENDM

%MACRO  AtLineCursor
        LDWI    atLineCursor
        CALL    giga_vAC
%ENDM

%MACRO  SetMidiStream
        STW     midiId
        LDWI    setMidiStream
        CALL    giga_vAC
%ENDM

%MACRO  PlayMidi
        STW     midiStream
        LDWI    resetMidi
        CALL    giga_vAC
%ENDM

%MACRO  PlayMidiV
        STW     midiStream
        LDWI    resetMidi
        CALL    giga_vAC
%ENDM

%MACRO  GetMidiNote
        LDWI    midiGetNote
        CALL    giga_vAC
%ENDM

%MACRO  PlayMusic
        STW     musicStream
        LDWI    resetMusic
        CALL    giga_vAC
        LDWI    playMusic
        CALL    giga_vAC
%ENDM

%MACRO  GetMusicNote
        LDWI    musicGetNote
        CALL    giga_vAC
%ENDM

%MACRO  Tick
        LDWI    realTimeStub
        CALL    giga_vAC
%ENDM

%MACRO  TickTime
        LDWI    tickTime
        CALL    giga_vAC
%ENDM

%MACRO  TickMidi
        LDWI    playMidi
        CALL    giga_vAC
%ENDM

%MACRO  TickMidiV
        LDWI    playMidiVol
        CALL    giga_vAC
%ENDM

%MACRO  TimeString
        LDWI    timeString
        CALL    giga_vAC
%ENDM

%MACRO  DrawBlit
        LDWI    drawBlit
        CALL    giga_vAC
%ENDM

%MACRO  DrawBlitX
        LDWI    drawBlitX
        CALL    giga_vAC
%ENDM

%MACRO  DrawBlitY
        LDWI    drawBlitY
        CALL    giga_vAC
%ENDM

%MACRO  DrawBlitXY
        LDWI    drawBlitXY
        CALL    giga_vAC
%ENDM

%MACRO  GetBlitLUT
        LDWI    getBlitLUT
        CALL    giga_vAC
%ENDM

%MACRO  SoundAll
        LDWI    soundAll
        CALL    giga_vAC
%ENDM

%MACRO  SoundAllOff
        LDWI    soundAllOff
        CALL    giga_vAC
%ENDM

%MACRO  SoundOff
        LDWI    soundOff
        CALL    giga_vAC
%ENDM

%MACRO  SoundOn
        LDWI    soundOn
        CALL    giga_vAC
%ENDM

%MACRO  SoundOnV
        LDWI    soundOnV
        CALL    giga_vAC
%ENDM

%MACRO  SoundMod
        LDWI    soundMod
        CALL    giga_vAC
%ENDM

%MACRO  BcdAdd
        ST      bcdLength
        LDWI    bcdAdd
        CALL    giga_vAC
%ENDM

%MACRO  BcdSub
        ST      bcdLength
        LDWI    bcdSub
        CALL    giga_vAC
%ENDM

%MACRO  BcdInt
        STW     bcdValue
        LDWI    bcdInt
        CALL    giga_vAC
%ENDM

%MACRO  BcdCmp
        ST      bcdLength
        LDWI    bcdCmp
        CALL    giga_vAC
%ENDM

%MACRO  BcdCpy
        ST      bcdLength
        LDWI    bcdCpy
        CALL    giga_vAC
%ENDM

%MACRO  JumpFalse _label id
        BNE     _id_ + 2                                ; unique id is used as an internal macro label
        LDWI    _label
_id_    CALL    giga_vAC
%ENDM

%MACRO  JumpTrue _label id
        BEQ     _id_ + 2                                ; unique id is used as an internal macro label
        LDWI    _label
_id_    CALL    giga_vAC
%ENDM

%MACRO  JumpEQ _label id                                ; inverted logic
        BEQ     _id_ + 2
        LDWI    _label
_id_    CALL    giga_vAC
%ENDM

%MACRO  JumpNE _label id                                ; inverted logic
        BNE     _id_ + 2
        LDWI    _label
_id_    CALL    giga_vAC
%ENDM

%MACRO  JumpLE _label id                                ; inverted logic
        BLE     _id_ + 2
        LDWI    _label
_id_    CALL    giga_vAC
%ENDM

%MACRO  JumpGE _label id                                ; inverted logic
        BGE     _id_ + 2
        LDWI    _label
_id_    CALL    giga_vAC
%ENDM

%MACRO  JumpLT _label id                                ; inverted logic
        BLT     _id_ + 2
        LDWI    _label
_id_    CALL    giga_vAC
%ENDM

%MACRO  JumpGT _label id                                ; inverted logic
        BGT     _id_ + 2
        LDWI    _label
_id_    CALL    giga_vAC
%ENDM

%MACRO  InitEqOp
        LDWI    convertEqOp
        STW     convertEqOpAddr
%ENDM
        
%MACRO  InitNeOp
        LDWI    convertNeOp
        STW     convertNeOpAddr
%ENDM

%MACRO  InitLeOp
        LDWI    convertLeOp
        STW     convertLeOpAddr
%ENDM
        
%MACRO  InitGeOp
        LDWI    convertGeOp
        STW     convertGeOpAddr
%ENDM
        
%MACRO  InitLtOp
        LDWI    convertLtOp
        STW     convertLtOpAddr
%ENDM
        
%MACRO  InitGtOp
        LDWI    convertGtOp
        STW     convertGtOpAddr
%ENDM

%MACRO  Init8Array2d
        LDWI    convert8Arr2d
        STW     convert8Arr2dAddr
%ENDM

%MACRO  Init8Array3d
        LDWI    convert8Arr3d
        STW     convert8Arr3dAddr
%ENDM

%MACRO  Init16Array2d
        LDWI    convert16Arr2d
        STW     convert16Arr2dAddr
%ENDM

%MACRO  Init16Array3d
        LDWI    convert16Arr3d
        STW     convert16Arr3dAddr
%ENDM

%MACRO  InitRealTimeStub
        LDWI    realTimeStub
        STW     realTimeStubAddr
%ENDM

%MACRO  SwapByte
        LDWI    swapByte
        CALL    giga_vAC
%ENDM

%MACRO  SwapWord
        LDWI    swapWord
        CALL    giga_vAC
%ENDM

%MACRO  SwapBytes
        LDWI    swapBytes
        CALL    giga_vAC
%ENDM

%MACRO  SwapWords
        LDWI    swapWords
        CALL    giga_vAC
%ENDM

%MACRO  CopyBytes
        LDWI    copyBytes
        CALL    giga_vAC
%ENDM

%MACRO  CopyWords
        LDWI    copyWords
        CALL    giga_vAC
%ENDM

%MACRO  CopyDWords
        LDWI    copyDWords
        CALL    giga_vAC
%ENDM

%MACRO  CopyBytesFar
        LDWI    copyBytesFar
        CALL    giga_vAC
%ENDM

%MACRO  CopyWordsFar
        LDWI    copyWordsFar
        CALL    giga_vAC
%ENDM

%MACRO  CopyDWordsFar
        LDWI    copyDWordsFar
        CALL    giga_vAC
%ENDM

%MACRO  ResetVars _addr
        LDI     _addr
        STW     varAddress
        LDWI    resetVars
        CALL    giga_vAC
%ENDM

%MACRO  ResetMem _memAddr _memCount
        LDWI    _memAddr
        STW     ramAddr0
        LDWI    _memAddr + _memCount
        STW     ramAddr1
        LDWI    resetMem
        CALL    giga_vAC
%ENDM

%MACRO  RomCheck
        LDWI    romCheck
        CALL    giga_vAC
%ENDM

%MACRO  RomExec
        STW     romExec_vLR
        LDWI    romExec
        CALL    giga_vAC
%ENDM

%MACRO  RomRead
        STW     romReadAddr
        LDWI    romRead
        CALL    giga_vAC
%ENDM

%MACRO  Initialise
        LDWI    0x0F20
        STW     fgbgColour                              ; yellow on blue

        LDI     MISC_ENABLE_SCROLL_BIT
        STW     miscFlags                               ; reset flags
        LDI     0
        STW     midiStream                              ; reset MIDI
        LD      giga_frameCount
        STW     frameCountPrev                          ; reset frameCount shadow var

        LDWI    resetVideoFlags
        CALL    giga_vAC
%ENDM
