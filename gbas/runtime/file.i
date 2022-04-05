; payload protocol: <isLast>, <length>, <payload 0...62>
; filename packet: <type>, <length>, <name>, <0> : <type> = 1:file or 2:dir

; do *NOT* use register4 to register7 during time slicing
fileBufAddr         EQU     register0
fileCmdAddr         EQU     register1
fileCmdSize         EQU     register2


getRomType:
    'defaults to ROMv1
    NextByteIn_32 = &h02E9
    romType = get("ROM_TYPE")
    if romType     &=  &h20 'ROMv2
        NextByteIn_32 = &h0BB1
    elseif romType &=  &h28 'ROMv3
        NextByteIn_32 = &h0BDF
    elseif romType &&= &h38 'ROMv4
        NextByteIn_32 = &h8A31
    elseif romType &&= &h40 'ROMv5a
        NextByteIn_32 = &h5907
    elseif romType &&= &hF0 'SDCARD
        NextByteIn_32 = &h1300
    elseif romType &&= &hF8 'DEVROM
        NextByteIn_32 = &h5907
    endif
return

%SUB                fileSendCmd
fileSendCmd         LDWI    SYS_SendSerial1_v3_80
                    STW     giga_sysFn
                    LDW     fileCmdAddr
                    STW     giga_sysArg0
                    LDI     1
                    STW     giga_sysArg2
                    LD      fileCmdSize
                    LSLW
                    LSLW
                    LSLW
                    STW     giga_sysArg3
                    SYS     80                                  ; return result in vAC
                    RET
%ENDS

; simplified version of Loader, removes checksum, destination address, copying of
; buffer to destination address and visuals
%SUB                fileLoadPacket
fileLoadPacket      LDW     _NextByteIn_32
                    STW     giga_sysFn

fileLoadP_idle      LDW     fileBufAddr
                    STW     giga_sysArg0            'buffer
            
                    LDI     207
                    ST      giga_sysArg3            'scanline 207
                    SYS     32                      'isLast, (0 or 1)
                    
                    LDW     fileBufAddr
                    PEEK
                    XORI    255
                    BEQ     fileLoadP_idle          'loop while idle
            
                    LDI     219
                    ST      giga_sysArg3            'scanline 219
                    SYS     32                      'length, (6 bits : 0..62)
            
                    LDI     235
                    ST      giga_sysArg3            'scanline 235
                    SYS     32                      'payload 0
                    
                    LDI     251
                    ST      giga_sysArg3            'scanline 251
                    SYS     32                      'payload 1
                    
                    LDI     2
                    ST      giga_sysArg3            'scanline 2
                    
fileLoadP_data      SYS     32                      'payload 2-61
                    LD      giga_sysArg3
                    ADDI    4
                    ST      giga_sysArg3
                    XORI    242                     'scanline end is 238
                    BNE     fileLoadP_data
                    LDI     185
                    ST      giga_sysArg3            'scanline 185
                    SYS     32                      'payload 62
                    RET
%ENDS