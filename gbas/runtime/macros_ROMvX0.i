%MACRO  LoopCounterTo1 _counter _label
        DBNE    _counter, _label
%ENDM

%MACRO  LoopCounterTo0 _counter _label
        DBGE    _counter, _label
%ENDM

%MACRO  LoopCounter _counter _label
        DBNE    _counter, _label
%ENDM

%MACRO  LoopCounter1 _counter _label
        DBGE    _counter, _label
%ENDM

%MACRO  LutPeek _lut _index
        LDWM    _lut
        ADDW    _index
        PEEK
%ENDM

%MACRO  LutDeek _lut _index _scratch
        LDWM    _lut
        DEEKR   _index
%ENDM

%MACRO  ForNextInc _var _label _end
        CMPI    _var, _end
        INC     _var
        JNE     _label
%ENDM

%MACRO  ForNextIncZero _var _label
        INCWA   _var
        JLE     _label
%ENDM

%MACRO  ForNextIncOne _var _label
        INCWA   _var
        XORI    2
        JNE     _label
%ENDM

%MACRO  ForNextDec _var _label _end
        CMPI    _var, _end
        DEC     _var
        JNE     _label
%ENDM

%MACRO  ForNextDBNE _var _label
        DBNE    _var, _label
%ENDM

%MACRO  ForNextDJNE _var _label
        DJNE    _var, _label
%ENDM

%MACRO  ForNextDecOne _var _label
        DBNE    _var, _label
%ENDM

%MACRO  ForNextFarDecOne _var _label
        DJNE    _var, _label
%ENDM

%MACRO  ForNextDBGE _var _label
        DBGE    _var, _label
%ENDM

%MACRO  ForNextDJGE _var _label
        DJGE    _var, _label
%ENDM

%MACRO  ForNextDecZero _var _label
        DBGE    _var, _label
%ENDM

%MACRO  ForNextFarDecZero _var _label
        DJGE    _var, _label
%ENDM

%MACRO  ForNextFarInc _var _label _end
        CMPI    _var, _end
        INC     _var
        JNE     _label
%ENDM

%MACRO  ForNextFarDec _var _label _end
        CMPI    _var, _end
        DEC     _var
        JNE     _label
%ENDM

%MACRO  ForNextVarInc _var _label _vEnd
        INCWA   _var
        SUBW    _vEnd
        JLE     _label
%ENDM

%MACRO  ForNextVarDec _var _label _vEnd
        DECWA   _var
        SUBW    _vEnd
        JGE     _label
%ENDM

%MACRO  ForNextAddZero _var _label _end _step
        ADDVI   _var, _var, _step
        JLE     _label
%ENDM

%MACRO  ForNextSubZero _var _label _end _step
        SUBVI   _var, _var, _step
        JGE     _label
%ENDM

%MACRO  ForNextAdd _var _label _end _step
        ADDVI   _var, _var, _step
        SUBI    _end
        JLE     _label
%ENDM

%MACRO  ForNextSub _var _label _end _step
        SUBVI   _var, _var, _step
        SUBI    _end
        JGE     _label
%ENDM

%MACRO  ForNextFarAdd _var _label _end _step
        ADDVI   _var, _var, _step
        SUBI    _end
        JLE     _label
%ENDM

%MACRO  ForNextFarSub _var _label _end _step
        SUBVI   _var, _var, _step
        SUBI    _end
        JGE     _label
%ENDM

%MACRO  ForNextVarAdd _var _label _vEnd _vStep
        ADDVW   _vStep, _var, _var
        SUBW    _vEnd
        JLE     _label
%ENDM

%MACRO  ForNextVarSub _var _label _vEnd _vStep
        SUBVW   _var, _vStep, _var
        SUBW    _vEnd
        JGE     _label
%ENDM

%MACRO  ForNextFarVarAdd _var _label _vEnd _vStep
        ADDVW   _vStep, _var, _var
        SUBW    _vEnd
        JLE     _label
%ENDM

%MACRO  ForNextFarVarSub _var _label _vEnd _vStep
        SUBVW   _var, _vStep, _var
        SUBW    _vEnd
        JGE     _label
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
        ABSW
%ENDM

%MACRO  Sign
        SGNW
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
        RANDW
%ENDM

%MACRO  RandMod
        STW     giga_sysArg2                        ; mathY
        CALLI   randMod16bit
%ENDM

%MACRO  RandFastMod
        STW     giga_sysArg2                        ; mathY
        CALLI   randFastMod16bit
%ENDM

%MACRO  FMul
        CALLI   fastMul8bit
%ENDM

%MACRO  FDiv
        CALLI   fastDiv168bit
%ENDM

%MACRO  FMod
        CALLI   fastMod168bit
%ENDM

%MACRO  FDivMod
        CALLI   fastDivMod168bit
%ENDM

%MACRO  Sqrt
        CALLI   sqrt16bit
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
        ANDBK   giga_videoY, 1
%ENDM

%MACRO  ReadPixel
        LDPX
%ENDM

%MACRO  DrawPixel colour
        STPX    colour
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

%MACRO  ParityFill
        CALLI   parityFill
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

%MACRO  MoveSprite
        CALLI   moveSprite
%ENDM

%MACRO  AnimateSprite
        CALLI   animateSprite
%ENDM

%MACRO  EnableSprite
        CALLI   enableSprite
%ENDM

%MACRO  InitSprites
        CALLI   initSprites
%ENDM

%MACRO  DrawSprites
        CALLI   drawSprites
%ENDM

%MACRO  DrawSpritesH
        CALLI   drawSpritesH
%ENDM

%MACRO  RestoreSprites
        CALLI   restoreSprites
%ENDM

%MACRO  RestoreSpritesH
        CALLI   restoreSpritesH
%ENDM

%MACRO  SortSprites
        CALLI   sortSprites
%ENDM

%MACRO  SortSpritesLut
        CALLI   sortSpritesLut
%ENDM

%MACRO  EnableSprites
        CALLI   enableSprites
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

%MACRO  JumpFalse _label
        JEQ     _label
%ENDM

%MACRO  JumpTrue _label
        JNE     _label
%ENDM

%MACRO  JumpEQ _label                                   ; inverted logic
        JNE     _label
%ENDM

%MACRO  JumpNE _label                                   ; inverted logic
        JEQ     _label
%ENDM

%MACRO  JumpLE _label                                   ; inverted logic
        JGT     _label
%ENDM

%MACRO  JumpGE _label                                   ; inverted logic
        JLT     _label
%ENDM

%MACRO  JumpLT _label                                   ; inverted logic
        JGE     _label
%ENDM

%MACRO  JumpGT _label                                   ; inverted logic
        JLE     _label
%ENDM

%MACRO  SwapByte _var0 _var1
        SWAPB   _var0, _var1
%ENDM

%MACRO  SwapWord _var0 _var1
        SWAPW   _var0, _var1
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
        MOVQW   varAddress, _addr
        CALLI   resetVars
%ENDM

%MACRO  ResetMem _memAddr _memCount
        LDWI    _memAddr
        STW     ramAddr
        LDWI    _memCount
        STW     ramCount
        LDI     0
        CALLI   resetMem
%ENDM
        
%MACRO  ScrollV
        CALLI   scrollV
%ENDM

%MACRO  ScrollRectV
        CALLI   scrollRectV
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
        STW     fgbgColour                          ; yellow on blue

        ORBI    giga_channelMask, 0x03              ; enable 4 channel audio by default
        
        MOVQW   miscFlags, MISC_ENABLE_SCROLL_BIT   ; reset flags
        MOVQW   frameCountPrev, 0                   ; reset frameCount shadow var
        MOVQW   midiStream, 0                       ; reset MIDI
        
        MOVQB   giga_jiffiesTick, 0                 ; reset Jiffies
        MOVQW   giga_secondsTickLo, 0               ; reset Seconds

        CALLI   resetVideoFlags
%ENDM
