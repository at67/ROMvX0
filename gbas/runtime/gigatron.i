; ROM
giga_text32                     EQU     0x0700
giga_text82                     EQU     0x0800
giga_notesTable                 EQU     0x0900
giga_invTable                   EQU     0x0A00

; RAM
giga_vram                       EQU     0x0800
giga_videoTable                 EQU     0x0100
giga_vblankProc                 EQU     0x01F6
giga_videoTop                   EQU     0x01F9
giga_soundChan1                 EQU     0x01FA
giga_soundChan2                 EQU     0x02FA
giga_soundChan3                 EQU     0x03FA
giga_soundChan4                 EQU     0x04FA

; defines
giga_xres                       EQU     160
giga_yres                       EQU     120
giga_xfont                      EQU     6
giga_yfont                      EQU     8

; page 0
giga_Zero                       EQU     0x00
giga_memSize                    EQU     0x01
giga_vSPH                       EQU     0x04
giga_rand0                      EQU     0x06
giga_rand1                      EQU     0x07
giga_rand2                      EQU     0x08
giga_videoY                     EQU     0x09
giga_frameCount                 EQU     0x0E
giga_serialRaw                  EQU     0x0F
giga_buttonState                EQU     0x11
giga_xoutMask                   EQU     0x14
giga_vPC                        EQU     0x16
giga_vAC                        EQU     0x18
giga_vLR                        EQU     0x1A
giga_vSP                        EQU     0x1C
giga_vTMP                       EQU     0x1D
giga_romType                    EQU     0x21
giga_channelMask                EQU     0x21
giga_sysFn                      EQU     0x22
giga_sysArg0                    EQU     0x24
giga_sysArg1                    EQU     0x25
giga_sysArg2                    EQU     0x26
giga_sysArg3                    EQU     0x27
giga_sysArg4                    EQU     0x28
giga_sysArg5                    EQU     0x29
giga_sysArg6                    EQU     0x2A
giga_sysArg7                    EQU     0x2B
giga_soundTimer                 EQU     0x2C
giga_ledState                   EQU     0x2E
giga_ledTempo                   EQU     0x2F
giga_User                       EQU     0x30
giga_One                        EQU     0x80

; ROM types
romTypeValue_ROMv1              EQU     0x1C
romTypeValue_ROMv2              EQU     0x20
romTypeValue_ROMv3              EQU     0x28
romTypeValue_ROMv4              EQU     0x38
romTypeValue_ROMv5a             EQU     0x40
romTypeValue_ROMvX0             EQU     0x80
romTypeValue_SDCARD             EQU     0xF0
romTypeValue_DEVROM             EQU     0xF8

; ROMv1 SYS calls
SYS_Reset_36                    EQU     0x009a
SYS_Exec_88                     EQU     0x00ad
SYS_Out_22                      EQU     0x00f4
SYS_In_24                       EQU     0x00f9
SYS_NextByteIn                  EQU     0x02e9
SYS_Random_34                   EQU     0x04a7
SYS_LSRW7_30                    EQU     0x04b9
SYS_LSRW8_24                    EQU     0x04c6
SYS_LSLW8_24                    EQU     0x04cd
SYS_Draw4_30                    EQU     0x04d4
SYS_VDrawBits_134               EQU     0x04e1
SYS_LSRW1_48                    EQU     0x0600
SYS_LSRW2_52                    EQU     0x0619
SYS_LSRW3_52                    EQU     0x0636
SYS_LSRW4_50                    EQU     0x0652
SYS_LSRW5_50                    EQU     0x066d
SYS_LSRW6_48                    EQU     0x0687
SYS_LSLW4_46                    EQU     0x06a0
SYS_PayloadCopy_34              EQU     0x06e7

; ROMv2 SYS calls
SYS_SetMode_v2_80               EQU     0x0b00
SYS_SetMemory_v2_54             EQU     0x0b03

; ROMv3 SYS calls
SYS_SendSerial1_v3_80           EQU     0x0b06
SYS_Sprite6_v3_64               EQU     0x0c00
SYS_Sprite6x_v3_64              EQU     0x0c40
SYS_Sprite6y_v3_64              EQU     0x0c80
SYS_Sprite6xy_v3_64             EQU     0x0cc0

; ROMv4 SYS calls
SYS_ExpanderControl_v4_40       EQU     0x0b09
SYS_ResetWaveforms_v4_50        EQU     0x0b0f
SYS_ShuffleNoise_v4_46          EQU     0x0b12
SYS_SpiExchangeBytes_v4_134     EQU     0x0b15

; ROMv5 SYS calls
SYS_ReadRomDir_v5_80            EQU     0x00ef

; ROMvX0 SYS calls
SYS_Multiply_s16_vX_66          EQU     0x009e
SYS_Divide_s16_vX_80            EQU     0x00a1
SYS_DrawLine_vX_86              EQU     0x00a4
SYS_WaitVBlank_vX_28            EQU     0x00aa
SYS_SpritePattern_vX_134        EQU     0x00b6
SYS_SortUint8Array_vX_52        EQU     0x00b9
SYS_SortSprites_vX_62           EQU     0x00bc
SYS_SortViaIndices_vX_44        EQU     0x00bf
SYS_MemCopyByte_vX_40           EQU     0x00c2
SYS_MemCopyWord_vX_48           EQU     0x00c5
SYS_MemCopyDWord_vX_58          EQU     0x00c8
SYS_DrawVLine_vX_66             EQU     0x00cb
SYS_DrawSprite_vX_132           EQU     0x00ce
SYS_DrawBullet_vX_140           EQU     0x00d1
SYS_CmpByteBounds_vX_54         EQU     0x00d4
SYS_Divide_u168_vX_56           EQU     0x00d7
SYS_ReadPixel_vX_32             EQU     0x00da
SYS_DrawPixel_vX_30             EQU     0x00dd
SYS_ScanMemoryExt_vX_50         EQU     0x00e3
SYS_ScanMemory_vX_50            EQU     0x00e6
SYS_CopyMemory_vX_80            EQU     0x00e9
SYS_CopyMemoryExt_vX_100        EQU     0x00ec
SYS_Multiply_u8_vX_48           EQU     0x1300
SYS_OffsetVTableY_vX_36         EQU     0x1340
SYS_FillByteSeq_vX_36           EQU     0x1360
SYS_AddInt8Array_vX_40          EQU     0x1380
SYS_ParityFill_vX_44            EQU     0x13a0
SYS_ConvertVTableX_66           EQU     0x1fc0
SYS_DrawSpriteH_vX_140          EQU     0x2000
SYS_ScrollVTableY_vX_38         EQU     0x20c0
SYS_RestoreSprite_vX_126        EQU     0x2100
SYS_ScrollRectVTableY_vX_44     EQU     0x21a0
SYS_LoadSerialIn_vX_58          EQU     0x3060

SYS_LoaderNextByteIn_32         EQU     0x1a6d
SYS_LoaderProcessInput_64       EQU     0x1a82
SYS_LoaderPayloadCopy_34        EQU     0x1ac0
SYS_Unpack_56                   EQU     0x1ad4

giga_Black                      EQU     0x00
giga_DarkRed                    EQU     0x01
giga_Red                        EQU     0x02
giga_LightRed                   EQU     0x03
giga_DarkGreen                  EQU     0x04
giga_Green                      EQU     0x08
giga_LightGreen                 EQU     0x0c
giga_DarkBlue                   EQU     0x10
giga_Blue                       EQU     0x20
giga_LightBlue                  EQU     0x30
giga_White                      EQU     0x3f

giga_CursorX                    EQU     2
giga_CursorY                    EQU     0