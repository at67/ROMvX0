swpSrcAddr          EQU     register0
swpDstAddr          EQU     register1
swpTmpData          EQU     register2
swapCount           EQU     register3
swpTmpAddr          EQU     register8
cpySrcAddr          EQU     register0
cpyDstAddr          EQU     register1
cpyCount            EQU     register2
cpyLoaderLut        EQU     register3
regsWork            EQU     giga_sysArg4                        ; use SYS arg registers to stop conflicts with time slicing/vblanks
regsAddr            EQU     giga_sysArg6                        ; use SYS arg registers to stop conflicts with time slicing/vblanks


%SUB                swapByte
                    ; swap a byte from one memory location to another
swapByte            SWAPB   swpSrcAddr, swpDstAddr
                    RET
%ENDS

%SUB                swapWord
                    ; swap a word from one memory location to another
swapWord            SWAPW   swpSrcAddr, swpDstAddr
                    RET
%ENDS

%SUB                swapBytes
                    ; swap 1..65535 bytes from one memory location to another, (this can be bad on a 32K RAM system)
swapBytes           PUSH
                    
swapB_loop          SWAPB   swpSrcAddr, swpDstAddr
                    INCW    swpSrcAddr
                    INCW    swpDstAddr
                    DJNE    swapCount, swapB_loop               ; don't use DBNE
                    POP
                    RET
%ENDS

%SUB                swapWords
                    ; swap 1..65535 words from one memory location to another, (this can be bad on a 32K/64K RAM system)
swapWords           PUSH
                    
swapW_loop          SWAPW   swpSrcAddr, swpDstAddr              ; CALLI   swapWord
                    ADDVI   swpSrcAddr, 2
                    ADDVI   swpDstAddr, 2
                    DJNE    swapCount, swapW_loop               ; don't use DBNE
                    POP
                    RET
%ENDS

%SUB                loadRegs8_15
                    ; hard coded to load register8 to register15
loadRegs8_15        LDWI    regsWorkArea
                    MEEKA   register8
                    ADDI    8
                    MEEKA   register12
                    RET
%ENDS

%SUB                saveRegs8_15
                    ; hard coded to save register8 to register15
saveRegs8_15        LDWI    regsWorkArea
                    MOKEA   register8
                    ADDI    8
                    MOKEA   register12
                    RET
%ENDS

%SUB                getArrayByte
                    ; get 8bit value from array
getArrayByte        LDW     memAddr
                    ADDW    memIndex0
                    PEEK
                    RET
%ENDS

%SUB                setArrayByte
                    ; set 8bit value from array
setArrayByte        LDW     memAddr
                    ADDW    memIndex0
                    POKEA   memValue
                    RET
%ENDS

%SUB                getArrayInt16
                    ; get 16bit value from array
getArrayInt16       LDW     memAddr
                    DEEKR   memIndex0
                    RET
%ENDS

%SUB                setArrayInt16
                    ; set 16bit value from array
setArrayInt16       LDW     memAddr
                    ADDW    memIndex0
                    ADDW    memIndex0
                    DOKEA   memValue
                    RET
%ENDS

%SUB                getArrayInt16Low
                    ; get low byte from 16bit array value
getArrayInt16Low    LDW     memAddr
                    ADDW    memIndex0
                    ADDW    memIndex0
                    PEEK
                    RET
%ENDS

%SUB                setArrayInt16Low
                    ; set low byte from 16bit array value
setArrayInt16Low    LDW     memAddr
                    ADDW    memIndex0
                    ADDW    memIndex0
                    POKEA   memValue
                    RET
%ENDS

%SUB                getArrayInt16High
                    ; get High byte from 16bit array value
getArrayInt16High   LDW     memAddr
                    ADDW    memIndex0
                    ADDW    memIndex0
                    ADDI    1
                    PEEK
                    RET
%ENDS

%SUB                setArrayInt16High
                    ; set High byte from 16bit array value
setArrayInt16High   LDW     memAddr
                    ADDW    memIndex0
                    ADDW    memIndex0
                    ADDI    1
                    POKEA   memValue
                    RET
%ENDS

%SUB                convert8Arr2d
convert8Arr2d       DEEKR   memIndex0
                    ADDW    memIndex1
                    RET
%ENDS

%SUB                convert8Arr3d
convert8Arr3d       DEEKR   memIndex0
                    DEEKR   memIndex1
                    ADDW    memIndex2
                    RET
%ENDS

%SUB                convert16Arr2d
convert16Arr2d      DEEKR   memIndex0
                    ADDW    memIndex1
                    ADDW    memIndex1
                    RET
%ENDS

%SUB                convert16Arr3d
convert16Arr3d      DEEKR   memIndex0
                    DEEKR   memIndex1
                    ADDW    memIndex2
                    ADDW    memIndex2
                    RET
%ENDS

%SUB                readIntVar
readIntVar          LDWI    _dataIndex_
                    STW     memAddr
                    DEEK
                    STW     memIndex0
                    ADDI    1
                    DOKE    memAddr
                    LDARRW  memIndex0, _data_
                    RET
%ENDS

%SUB                readStrVar
readStrVar          PUSH
                    CALLI   readIntVar
                    STW     strSrcAddr
                    LDW     strDstAddr
                    CALLI   stringCopy
                    POP
                    RET
%ENDS

                    ; giga_sysArg0 = src, giga_sysArg2 = dst
                    ; requires sysArgs0 = src.lo-1 and sysArgs2 = dst.lo-1
%SUB                copyBytes
copyBytes           MOVQW   giga_sysFn, SYS_MemCopyByte_vX_40
                    LDWI    0x0101                              ; src step = 1, dst step = 1
                    STW     giga_sysArg4
                    LD      cpyCount
                    SYS     40
                    RET
%ENDS

                    ; giga_sysArg0 = src, giga_sysArg2 = dst
%SUB                copyWords
copyWords           MOVQW   giga_sysFn, SYS_MemCopyWord_vX_48
                    LD      cpyCount
                    SYS     48
                    RET
%ENDS

                    ; giga_sysArg0 = src, giga_sysArg2 = dst
%SUB                copyDWords
copyDWords          MOVQW   giga_sysFn, SYS_MemCopyDWord_vX_58
                    LD      cpyCount
                    SYS     58
                    RET
%ENDS
        
%SUB                copyBytesFar
copyBytesFar        PEEKV+  cpySrcAddr
                    POKEV+  cpyDstAddr
                    DJNE    cpyCount, copyBytesFar
                    RET
%ENDS

%SUB                copyWordsFar
copyWordsFar        DEEKV+  cpySrcAddr
                    DOKEV+  cpyDstAddr
                    DJNE    cpyCount, copyWordsFar
                    RET
%ENDS

%SUB                copyDWordsFar
copyDWordsFar       DEEKV+  cpySrcAddr
                    DOKEV+  cpyDstAddr
                    DEEKV+  cpySrcAddr
                    DOKEV+  cpyDstAddr
                    DJNE    cpyCount, copyDWordsFar
                    RET
%ENDS

%SUB                copyLoaderImages
copyLoaderImages    PUSH
                    MOVQW   giga_sysFn, SYS_MemCopyByte_vX_40
                    LDWI    _loader_image_chunksLut
                    STW     cpyLoaderLut
                    
copyLI_loop         DEEKV+  cpyLoaderLut
                    BEQ     copyLI_exit
                    STW     giga_sysArg0
                    DEEKV+  cpyLoaderLut
                    STW     giga_sysArg2
                    LDWI    0x0101                              ; src step = 1, dst step = 1
                    STW     giga_sysArg4
                    PEEKV+  cpyLoaderLut
                    SYS     40
                    BRA     copyLI_loop

copyLI_exit         POP
                    RET
%ENDS
