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
                    LDWI    _spritesXposLut_
                    STW     giga_sysArg0
                    LDWI    _spritesTmpLut_ + 0                 ; index 0
                    STW     giga_sysArg2
                    LDWI    0x0401                              ; src step = 1, dst step = 4
                    STW     giga_sysArg4
                    LD      spritesCount
                    SYS     40

                    LDWI    _spritesYposLut_
                    STW     giga_sysArg0
                    LDWI    _spritesTmpLut_ + 1                 ; index 1
                    STW     giga_sysArg2
                    LDWI    0x0401                              ; src step = 1, dst step = 4
                    STW     giga_sysArg4
                    LD      spritesCount
                    SYS     40
                    RET
%ENDS


%SUB                drawSprites
drawSprites         LDWI    _spritesXposTmpLut_
                    STW     spritesXposLut
                    PEEKA   spritesXYpos
                    LDWI    _spritesYposTmpLut_
                    STW     spritesYposLut
                    PEEKA   spritesXYpos + 1
                    LDWI    _spritesTmpLut_
                    STW     spritesLut
                    PEEKA   spritesAdrHt + 1
                    MOVQW   giga_sysFn, SYS_DrawSprite_vX_132

drawSpr_loop        MOVQB    spritesAdrHt, _SH
                    SYS     132
                    DBNE    spritesCount, drawSpr_loop
                    RET
%ENDS

                    ; constant height sprites
%SUB                drawSpritesH
drawSpritesH        LDWI    SYS_DrawSpriteH_vX_140
                    STW     giga_sysFn
                    LDWI    _vtX
                    STW     giga_sysArg0
                    LDWI    _spritesTmpLut_
                    STW     spritesLut
                    PEEKA+  spritesXYpos                        ; x pos
                    PEEKA+  spritesXYpos + 1                    ; y pos
                    PEEKA+  spritesAdrHt                        ; height
                    PEEKA+  spritesAdrHt + 1                    ; data
                    SYS     136
                    RET
%ENDS

                    ; constant height sprites
%SUB                restoreSpritesH
restoreSpritesH     STW     spritesLut                          ; _spritesTmpLut_ + numSprites*4 - 2
                    PEEKA+  spritesAdrHt                        ; height
                    PEEKA+  spritesAdrHt + 1                    ; data
            
                    MOVQW   giga_sysFn, SYS_WaitVBlank_vX_28
                    SYS     28
                    
                    LDWI    SYS_RestoreSprite_vX_126
                    STW     giga_sysFn
                    SYS     124
                    RET
%ENDS

%SUB                sortSpritesLut
                    ; sort _spritesLut_ using sorted indices
sortSpritesLut      MOVQW   giga_sysFn, SYS_SortViaIndices_vX_44
                    LDWI    _spritesIndicesLut_
                    STW     giga_sysArg0
                    LDWI    _spritesLut_
                    STW     giga_sysArg2
                    LDWI    _spritesTmpLut_ + 3                 ; index 3
                    STW     giga_sysArg4
                    MOVQB    giga_sysArg6, 4                     ; dst step
                    LD      spritesCount
                    SYS     44
                    
                    ; sort _spritesXposLut_ using sorted indices
                    LDWI    _spritesIndicesLut_
                    STW     giga_sysArg0
                    LDWI    _spritesXposLut_
                    STW     giga_sysArg2
                    LDWI    _spritesTmpLut_ + 0                 ; index 0
                    STW     giga_sysArg4
                    MOVQB    giga_sysArg6, 4                     ; dst step
                    LD      spritesCount
                    SYS     44

                    ; sort _spritesHeightLut_ using sorted indices
                    LDWI    _spritesIndicesLut_
                    STW     giga_sysArg0
                    LDWI    _spritesHeightLut_
                    STW     giga_sysArg2
                    LDWI    _spritesTmpLut_ + 2                 ; index 2
                    STW     giga_sysArg4
                    MOVQB    giga_sysArg6, 4                     ; dst step
                    LD      spritesCount
                    SYS     44
                    RET
%ENDS


%SUB                sortSprites
sortSprites         LDWI    SYS_FillByteSeq_vX_36
                    STW     giga_sysFn
                    LDWI    _spritesIndicesLut_
                    STW     giga_sysArg0                        ; dst address
                    MOVQB    giga_sysArg2, 0                     ; offset
                    MOVQB    giga_sysArg3, 1                     ; step
                    MOVB    spritesCount, giga_sysArg4          ; num sprites
                    SYS     36                                  ; reset indices

                    ; init y
                    MOVQW   giga_sysFn, SYS_MemCopyByte_vX_40
                    LDWI    _spritesYposLut_
                    STW     giga_sysArg0
                    LDWI    _spritesYposTmpLut_
                    STW     giga_sysArg2
                    LDWI    0x0101                              ; src step = 1, dst step = 1
                    STW     giga_sysArg4
                    LD      spritesCount                        ; num sprites
                    SYS     40
                    
                    ; sort y and indices
                    MOVQW   giga_sysFn, SYS_SortSprites_vX_62
                    LDWI    _spritesYposTmpLut_
                    STW     giga_sysArg0
                    LDWI    0x0101
                    STW     giga_sysArg2                        ; i:j
                    LDWI    _spritesYposTmpLut_ + 1
                    PEEKA   giga_sysArg4                        ; key0
                    MOVB    spritesCount, giga_sysArg5          ; num sprites
                    LDWI    _spritesIndicesLut_
                    STW     giga_sysArg6
                    LDWI    _spritesIndicesLut_ + 1
                    PEEKA   0x82                                ; key1
                    SYS     62
                    
                    MOVQW   giga_sysFn, SYS_MemCopyByte_vX_40
                    LDWI    _spritesYposTmpLut_
                    STW     giga_sysArg0
                    LDWI    _spritesTmpLut_ + 1                 ; index 1
                    STW     giga_sysArg2
                    LDWI    0x0401                              ; src step = 1, dst step = 4
                    STW     giga_sysArg4
                    LD      spritesCount
                    SYS     40
                    RET
%ENDS