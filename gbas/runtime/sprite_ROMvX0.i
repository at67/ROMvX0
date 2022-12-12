; do *NOT* use register4 to register7 during time slicing
spriteId            EQU     register0
spriteXY            EQU     register1
spatternId          EQU     register3
spritesXYpos        EQU     register0
spritesAdrHt        EQU     register2
spritesCount        EQU     register3
spriteEnable        EQU     register12
spritesXposLut      EQU     register13
spritesYposLut      EQU     register14
spritesLut          EQU     register15

    
%SUB                moveSprite
moveSprite          LDWI    _spritesXposLut_
                    ADDW    spriteId
                    POKEA   spriteXY
                    LDWI    _spritesYposLut_
                    ADDW    spriteId
                    POKEA   spriteXY + 1
                    RET
%ENDS


%SUB                enableSprite
enableSprite        LDWI    _spritesHeightLut_
                    ADDW    spriteId
                    STW     memAddr
                    PEEK
                    XORBA   spriteEnable
                    POKE    memAddr
                    RET
%ENDS


%SUB                animateSprite
animateSprite       MOVQW   giga_sysFn, SYS_SpritePattern_vX_134
                    LDWI    _spritesLut_
                    ADDW    spriteId
                    PEEKA   giga_sysArg2
                    LDARRW  spatternId, _patternsLut_
                    STW     giga_sysArg4
                    DEEKA   giga_sysArg0
                    SYS     134
                    RET
%ENDS


%SUB                initSprites
initSprites         MOVQW   giga_sysFn, SYS_MemCopyByte_vX_40
                    LDI     giga_sysArg0
                    DOKEI+  _spritesXposLut_
                    DOKEI+  _spritesTmpLut_ + 0                 ; index 0
                    DOKEI+  0x0401                              ; src step = 1, dst step = 4
                    LD      spritesCount
                    SYS     40

                    LDI     giga_sysArg0
                    DOKEI+  _spritesYposLut_
                    DOKEI+  _spritesTmpLut_ + 1                 ; index 1
                    DOKEI+  0x0401                              ; src step = 1, dst step = 4
                    LD      spritesCount
                    SYS     40
                    RET
%ENDS


%SUB                drawSprites
drawSprites         LDI     giga_sysFn
                    DOKEI+  SYS_DrawSprite_vX_140
                    DOKEI+  _vtX
                    LDWI    _spritesTmpLut_
                    STW     spritesLut
                    PEEKA+  spritesXYpos                        ; x pos
                    PEEKA+  spritesXYpos + 1                    ; y pos
                    PEEKA+  spritesAdrHt                        ; height
                    PEEKA+  spritesAdrHt + 1                    ; data
                    SYS     146
                    RET
%ENDS

%SUB                restoreSprites
restoreSprites      STW     spritesLut                          ; _spritesTmpLut_ + numSprites*4 - 2
                    PEEKA+  spritesAdrHt                        ; height
                    PEEKA+  spritesAdrHt + 1                    ; data

%if SPRITES_WAITVB
                    WAITVV  frameCountPrev                      ; more robust WAITVB
%endif

                    LDWI    SYS_RestoreSprite_vX_124
                    STW     giga_sysFn
                    SYS     124
                    RET
%ENDS

%SUB                sortSpritesLut
                    ; sort _spritesLut_ using sorted indices
sortSpritesLut      LDI     giga_sysFn
                    DOKEI+  SYS_SortViaIndices_vX_44
                    DOKEI+  _spritesIndicesLut_
                    DOKEI+  _spritesLut_
                    DOKEI+  _spritesTmpLut_ + 3                 ; index 3
                    MOVQB   giga_sysArg6, 4                     ; dst step
                    LD      spritesCount
                    SYS     44
                    
                    ; sort _spritesXposLut_ using sorted indices
                    LDI     giga_sysArg0
                    DOKEI+  _spritesIndicesLut_
                    DOKEI+  _spritesXposLut_
                    DOKEI+  _spritesTmpLut_ + 0                 ; index 0
                    MOVQB   giga_sysArg6, 4                     ; dst step
                    LD      spritesCount
                    SYS     44

                    ; sort _spritesHeightLut_ using sorted indices
                    LDI     giga_sysArg0
                    DOKEI+  _spritesIndicesLut_
                    DOKEI+  _spritesHeightLut_
                    DOKEI+  _spritesTmpLut_ + 2                 ; index 2
                    MOVQB   giga_sysArg6, 4                     ; dst step
                    LD      spritesCount
                    SYS     44
                    RET
%ENDS


%SUB                sortSprites
sortSprites         LDI     giga_sysFn
                    DOKEI+  SYS_FillByteSeq_vX_36
                    DOKEI+  _spritesIndicesLut_                 ; dst address
                    DOKEI+  0x0100                              ; step : offset
                    MOVB    spritesCount, giga_sysArg4          ; num sprites
                    SYS     36                                  ; reset indices

                    ; init y
                    LDI     giga_sysFn
                    DOKEI+  SYS_MemCopyByte_vX_40
                    DOKEI+  _spritesYposLut_
                    DOKEI+  _spritesYposTmpLut_
                    DOKEI+  0x0101                              ; src step : dst step
                    LD      spritesCount                        ; num sprites
                    SYS     40
                    
                    ; sort y and indices
                    LDI     giga_sysFn
                    DOKEI+  SYS_SortSprites_vX_62
                    DOKEI+  _spritesYposTmpLut_
                    DOKEI+  0x0101                              ; i : j
                    LDWI    _spritesYposTmpLut_ + 1
                    PEEKA   giga_sysArg4                        ; key0
                    MOVB    spritesCount, giga_sysArg5          ; num sprites
                    LDWI    _spritesIndicesLut_
                    STW     giga_sysArg6
                    LDWI    _spritesIndicesLut_ + 1
                    PEEKA   0x82                                ; key1
                    SYS     62
                    
                    LDI     giga_sysFn
                    DOKEI+  SYS_MemCopyByte_vX_40
                    DOKEI+  _spritesYposTmpLut_
                    DOKEI+  _spritesTmpLut_ + 1                 ; index 1
                    DOKEI+  0x0401                              ; src step = 1, dst step = 4
                    LD      spritesCount
                    SYS     40
                    RET
%ENDS

%SUB                mergeSpritesLut
mergeSpritesLut     LDI     0xB0
                    DOKEI+  _spritesXposLut_
                    DOKEI+  _spritesYposLut_
                    DOKEI+  _spritesHeightLut_
                    DOKEI+  _spritesLut_
                    MOVWA   spritesCount, giga_sysArg4
                    MERGE4  _spritesTmpLut_
                    RET
%ENDS

%SUB                mergeSpritesLutOld
mergeSpritesLutOld  LDI     giga_sysFn
                    DOKEI+  SYS_MemCopyByte_vX_40
                    DOKEI+  _spritesXposLut_
                    DOKEI+  _spritesTmpLut_ + 0                 ; index 0
                    DOKEI+  0x0401                              ; src step = 1, dst step = 4
                    LD      spritesCount
                    SYS     40

                    LDI     giga_sysArg0
                    DOKEI+  _spritesYposLut_
                    DOKEI+  _spritesTmpLut_ + 1                 ; index 1
                    DOKEI+  0x0401                              ; src step = 1, dst step = 4
                    LD      spritesCount
                    SYS     40

                    LDI     giga_sysArg0
                    DOKEI+  _spritesHeightLut_
                    DOKEI+  _spritesTmpLut_ + 2                 ; index 2
                    DOKEI+  0x0401                              ; src step = 1, dst step = 4
                    LD      spritesCount
                    SYS     40

                    LDI     giga_sysArg0
                    DOKEI+  _spritesLut_
                    DOKEI+  _spritesTmpLut_ + 3                 ; index 3
                    DOKEI+  0x0401                              ; src step = 1, dst step = 4
                    LD      spritesCount
                    SYS     40
                    RET
%ENDS