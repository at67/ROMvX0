#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cmath>
#include <vector>
#include <stack>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <iterator>
#include <algorithm>
#include <cstdarg>
#include <cctype>

#include "memory.h"
#include "cpu.h"
#include "expression.h"
#include "assembler.h"
#include "audio.h"
#include "loader.h"

#ifndef STAND_ALONE
#include "editor.h"
#include "Debugger.h"
#endif


#define BRANCH_ADJUSTMENT 2
#define MAX_DASM_LINES    30


namespace Assembler
{
    enum ParseType {PreProcessPass=0, MnemonicPass, CodePass, NumParseTypes};
    enum EvaluateResult {Failed=-1, NotFound, Reserved, Duplicate, Skipped, Success};
    enum AddressMode {D_AC=0b00000000, X_AC=0b00000100, YD_AC=0b00001000, YX_AC=0b00001100, D_X=0b00010000, D_Y=0b00010100, D_OUT=0b00011000, YXpp_OUT=0b00011100};
    enum BusMode {D=0b00000000, RAM=0b00000001, AC=0b00000010, IN=0b00000011};
    enum ReservedWords {CallTable=0, StartAddress, SingleStepWatch, DisableUpload, CpuUsageAddressA, CpuUsageAddressB, INCLUDE, MACRO, ENDM, GPRINTF, NumReservedWords};


    struct Label
    {
        uint16_t _address;
        std::string _name;
    };

    struct Equate
    {
        bool _isCustomAddress;
        uint32_t _operand;
        std::string _name;
    };

    struct Instruction
    {
        bool _isRomAddress;
        bool _isCustomAddress;
        ByteSize _byteSize;
        uint8_t _opcode;
        uint8_t _operand0;
        uint8_t _operand1;
        uint8_t _operand2;
        uint8_t _operand3;
        uint16_t _address;
        OpcodeType _opcodeType;
    };

    struct CallTableEntry
    {
        uint8_t _operand;
        uint16_t _address;
    };

    struct Macro
    {
        bool _complete = false;
        bool _fromInclude = false;
        int _fileStartLine;
        std::string _name;
        std::string _filename;
        std::vector<std::string> _params;
        std::vector<std::string> _lines;
    };


    int _lineNumber;

    uint8_t _vSpMin = 0x00;

    uint16_t _byteCount = 0;
    uint16_t _callTablePtr = 0x0000;
    uint16_t _startAddress = DEFAULT_EXEC_ADDRESS;
    uint16_t _currentAddress = _startAddress;
    uint16_t _currDasmByteCount = 1, _prevDasmByteCount = 1;
    uint16_t _currDasmPageByteCount = 0, _prevDasmPageByteCount = 0;

    std::string _includePath = ".";

    std::vector<Label> _labels;
    std::vector<Equate> _equates;
    std::vector<ByteCode> _byteCode;
    std::vector<Instruction> _instructions;
    std::vector<CallTableEntry> _callTableEntries;
    std::vector<std::string> _reservedWords;
    std::vector<DasmCode> _disassembledCode;

    std::map<std::string, InstructionType> _asmOpcodes;
    std::map<uint16_t, InstructionDasm> _vcpuOpcodes;
    std::map<uint16_t, InstructionDasm> _vbraOpcodes;
    std::map<uint8_t, InstructionDasm> _nativeOpcodes;
    std::map<uint16_t, Gprintf> _gprintfs;
    std::map<std::string, Define> _defines;
    std::stack<std::string> _currentDefine;


    uint8_t getvSpMin(void) {return _vSpMin;}
    uint16_t getStartAddress(void) {return _startAddress;}
    int getPrevDasmByteCount(void) {return _prevDasmByteCount;}
    int getCurrDasmByteCount(void) {return _currDasmByteCount;}
    int getPrevDasmPageByteCount(void) {return _prevDasmPageByteCount;}
    int getCurrDasmPageByteCount(void) {return _currDasmPageByteCount;}
    int getDisassembledCodeSize(void) {return int(_disassembledCode.size());}
    const std::string& getIncludePath(void) {return _includePath;}
    DasmCode* getDisassembledCode(int index) {return &_disassembledCode[index % _disassembledCode.size()];}

    void setvSpMin(uint8_t vSpMin) {_vSpMin = vSpMin;}
    void setIncludePath(const std::string& includePath) {_includePath = includePath;}


    int getAsmOpcodeSize(const std::string& opcodeStr)
    {
        if(opcodeStr[0] == ';') return 0;

        if(_asmOpcodes.find(opcodeStr) != _asmOpcodes.end())
        {
            return _asmOpcodes[opcodeStr]._byteSize;
        }

        return 0;
    }

    int getAsmOpcodeSizeText(const std::string& textStr)
    {
        for(auto it=_asmOpcodes.begin(); it!=_asmOpcodes.end(); ++it)
        {
            if(textStr.find(it->first) != std::string::npos  &&  _asmOpcodes.find(it->first) != _asmOpcodes.end())
            {
                return _asmOpcodes[it->first]._byteSize;
            }
        }

        return 0;
    }

    VACType getAsmOpcodeVACType(const std::string& opcodeStr)
    {
        if(opcodeStr[0] == ';') return NoVAC;

        if(_asmOpcodes.find(opcodeStr) != _asmOpcodes.end())
        {
            return _asmOpcodes[opcodeStr]._vAcType;
        }

        return NoVAC;
    }

    void initialiseOpcodes(void)
    {
        // Gigatron vCPU instructions
        _asmOpcodes["LDWI"  ] = {0x11, 0x00, ThreeBytes, vCpu, OutVAC  };
        _asmOpcodes["DEC"   ] = {0x14, 0x00, TwoBytes,   vCpu, NoVAC   };
        _asmOpcodes["MOVQB" ] = {0x16, 0x00, ThreeBytes, vCpu, NoVAC   };
        _asmOpcodes["LSRB"  ] = {0x18, 0x00, TwoBytes,   vCpu, NoVAC   };
        _asmOpcodes["LD"    ] = {0x1A, 0x00, TwoBytes,   vCpu, OutVAC  };
        _asmOpcodes["ADDBI" ] = {0x1C, 0x00, FourBytes,  vCpu, NoVAC   };
        _asmOpcodes["CMPHS" ] = {0x1F, 0x00, TwoBytes,   vCpu, InOutVAC};
        _asmOpcodes["LDW"   ] = {0x21, 0x00, TwoBytes,   vCpu, OutVAC  };
        _asmOpcodes["PEEKV+"] = {0x23, 0x00, TwoBytes,   vCpu, OutVAC  };
        _asmOpcodes["POKEI" ] = {0x25, 0x00, TwoBytes,   vCpu, InVAC   };
        _asmOpcodes["LSLV"  ] = {0x27, 0x00, TwoBytes,   vCpu, NoVAC   };
        _asmOpcodes["ADDVB" ] = {0x29, 0x00, FourBytes,  vCpu, NoVAC   };
        _asmOpcodes["STW"   ] = {0x2B, 0x00, TwoBytes,   vCpu, InVAC   };
        _asmOpcodes["CNVXY" ] = {0x2D, 0x00, FourBytes,  vCpu, NoVAC   };
        _asmOpcodes["MOVWA" ] = {0x32, 0x00, ThreeBytes, vCpu, OutVAC  };
        _asmOpcodes["SUBBI" ] = {0x38, 0x00, FourBytes,  vCpu, NoVAC   };
        _asmOpcodes["DEEKV" ] = {0x3B, 0x00, TwoBytes,   vCpu, OutVAC  };
        _asmOpcodes["ARRVW" ] = {0x3D, 0x00, FourBytes,  vCpu, OutVAC  };
        _asmOpcodes["LDARRW"] = {0x3F, 0x00, FourBytes,  vCpu, OutVAC  };
        _asmOpcodes["ADDVI" ] = {0x42, 0x00, FourBytes,  vCpu, OutVAC  };
        _asmOpcodes["SUBVI" ] = {0x44, 0x00, FourBytes,  vCpu, OutVAC  };
        _asmOpcodes["DEEKV+"] = {0x46, 0x00, TwoBytes,   vCpu, OutVAC  };
        _asmOpcodes["SUBVB" ] = {0x48, 0x00, FourBytes,  vCpu, NoVAC   };
        _asmOpcodes["DJGE"  ] = {0x4A, 0x00, FourBytes,  vCpu, NoVAC   };
        _asmOpcodes["MOVQW" ] = {0x4D, 0x00, ThreeBytes, vCpu, NoVAC   };
        _asmOpcodes["STWM"  ] = {0x4F, 0x00, ThreeBytes, vCpu, InVAC   };
        _asmOpcodes["STARRW"] = {0x51, 0x00, FourBytes,  vCpu, InVAC   };
        _asmOpcodes["LDARRB"] = {0x53, 0x00, FourBytes,  vCpu, OutVAC  };
        _asmOpcodes["STARRB"] = {0x55, 0x00, FourBytes,  vCpu, InVAC   };
        _asmOpcodes["STARRI"] = {0x57, 0x00, FiveBytes,  vCpu, NoVAC   };
        _asmOpcodes["LDI"   ] = {0x59, 0x00, TwoBytes,   vCpu, OutVAC  };
        _asmOpcodes["PEEKV" ] = {0x5B, 0x00, TwoBytes,   vCpu, OutVAC  };
        _asmOpcodes["ST"    ] = {0x5E, 0x00, TwoBytes,   vCpu, InVAC   };
        _asmOpcodes["DOKEV+"] = {0x60, 0x00, TwoBytes,   vCpu, InVAC   };
        _asmOpcodes["POP"   ] = {0x63, 0x00, OneByte,    vCpu, NoVAC   };
        _asmOpcodes["MOVB"  ] = {0x65, 0x00, ThreeBytes, vCpu, NoVAC   };
        _asmOpcodes["PEEKA" ] = {0x67, 0x00, TwoBytes,   vCpu, InVAC   };
        _asmOpcodes["POKEA" ] = {0x69, 0x00, TwoBytes,   vCpu, InVAC   };
        _asmOpcodes["TEQ"   ] = {0x6B, 0x00, TwoBytes,   vCpu, InVAC   };
        _asmOpcodes["TNE"   ] = {0x6D, 0x00, TwoBytes,   vCpu, InVAC   };
        _asmOpcodes["DEEKA" ] = {0x6F, 0x00, TwoBytes,   vCpu, InVAC   };
        _asmOpcodes["LDWM"  ] = {0x72, 0x00, ThreeBytes, vCpu, OutVAC  };
        _asmOpcodes["PUSH"  ] = {0x75, 0x00, OneByte,    vCpu, NoVAC   };
        _asmOpcodes["DOKEI" ] = {0x77, 0x00, ThreeBytes, vCpu, InVAC   };
        _asmOpcodes["ARRW"  ] = {0x79, 0x00, ThreeBytes, vCpu, InOutVAC};
        _asmOpcodes["POKEA+"] = {0x7B, 0x00, TwoBytes,   vCpu, InOutVAC};
        _asmOpcodes["DOKEA" ] = {0x7D, 0x00, TwoBytes,   vCpu, InVAC   };
        _asmOpcodes["LUP"   ] = {0x7F, 0x00, TwoBytes,   vCpu, InOutVAC};
        _asmOpcodes["ANDI"  ] = {0x82, 0x00, TwoBytes,   vCpu, InOutVAC};
        _asmOpcodes["CALLI" ] = {0x85, 0x00, ThreeBytes, vCpu, NoVAC   };
        _asmOpcodes["ORI"   ] = {0x88, 0x00, TwoBytes,   vCpu, InOutVAC};
        _asmOpcodes["PEEKA+"] = {0x8A, 0x00, TwoBytes,   vCpu, InOutVAC};
        _asmOpcodes["XORI"  ] = {0x8C, 0x00, TwoBytes,   vCpu, InOutVAC};
        _asmOpcodes["DBGE"  ] = {0x8E, 0x00, ThreeBytes, vCpu, NoVAC   };
        _asmOpcodes["BRA"   ] = {0x90, 0x00, TwoBytes,   vCpu, NoVAC   };
        _asmOpcodes["INC"   ] = {0x93, 0x00, TwoBytes,   vCpu, NoVAC   };
        _asmOpcodes["INCWA" ] = {0x95, 0x00, TwoBytes,   vCpu, OutVAC  };
        _asmOpcodes["CMPHU" ] = {0x97, 0x00, TwoBytes,   vCpu, InOutVAC};
        _asmOpcodes["ADDW"  ] = {0x99, 0x00, TwoBytes,   vCpu, InOutVAC};
        _asmOpcodes["LDNI"  ] = {0x9C, 0x00, TwoBytes,   vCpu, OutVAC  };
        _asmOpcodes["DBNE"  ] = {0x9E, 0x00, ThreeBytes, vCpu, NoVAC   };
        _asmOpcodes["DEEKR" ] = {0xA0, 0x00, TwoBytes,   vCpu, InOutVAC};
        _asmOpcodes["PACKAW"] = {0xA2, 0x00, ThreeBytes, vCpu, OutVAC  };
        _asmOpcodes["DJNE"  ] = {0xA4, 0x00, FourBytes,  vCpu, NoVAC   };
        _asmOpcodes["CMPI"  ] = {0xA7, 0x00, ThreeBytes, vCpu, OutVAC  };
        _asmOpcodes["ADDVW" ] = {0xA9, 0x00, FourBytes,  vCpu, OutVAC  };
        _asmOpcodes["SUBVW" ] = {0xAB, 0x00, FourBytes,  vCpu, OutVAC  };
        _asmOpcodes["PEEK"  ] = {0xAD, 0x00, OneByte,    vCpu, InOutVAC};
        _asmOpcodes["SYS"   ] = {0xB4, 0x00, TwoBytes,   vCpu, NoVAC   };
        _asmOpcodes["SUBW"  ] = {0xB8, 0x00, TwoBytes,   vCpu, InOutVAC};
        _asmOpcodes["JEQ"   ] = {0xBB, 0x00, ThreeBytes, vCpu, InVAC   };
        _asmOpcodes["JNE"   ] = {0xBD, 0x00, ThreeBytes, vCpu, InVAC   };
        _asmOpcodes["JLT"   ] = {0xBF, 0x00, ThreeBytes, vCpu, InVAC   };
        _asmOpcodes["JGT"   ] = {0xC1, 0x00, ThreeBytes, vCpu, InVAC   };
        _asmOpcodes["JLE"   ] = {0xC3, 0x00, ThreeBytes, vCpu, InVAC   };
        _asmOpcodes["JGE"   ] = {0xC5, 0x00, ThreeBytes, vCpu, InVAC   };
        _asmOpcodes["DEF"   ] = {0xCD, 0x00, TwoBytes,   vCpu, OutVAC  };
        _asmOpcodes["CALL"  ] = {0xCF, 0x00, TwoBytes,   vCpu, NoVAC   };
        _asmOpcodes["POKEV+"] = {0xD1, 0x00, TwoBytes,   vCpu, InVAC   };
        _asmOpcodes["LSRV"  ] = {0xD3, 0x00, TwoBytes,   vCpu, NoVAC   };
        _asmOpcodes["TGE"   ] = {0xD5, 0x00, TwoBytes,   vCpu, InVAC   };
        _asmOpcodes["TLT"   ] = {0xD7, 0x00, TwoBytes,   vCpu, InVAC   };
        _asmOpcodes["TGT"   ] = {0xD9, 0x00, TwoBytes,   vCpu, InVAC   };
        _asmOpcodes["TLE"   ] = {0xDB, 0x00, TwoBytes,   vCpu, InVAC   };
        _asmOpcodes["DECWA" ] = {0xDD, 0x00, TwoBytes,   vCpu, OutVAC  };
        _asmOpcodes["ALLOC" ] = {0xDF, 0x00, TwoBytes,   vCpu, NoVAC   };
        _asmOpcodes["PACKVW"] = {0xE1, 0x00, FourBytes,  vCpu, NoVAC   };
        _asmOpcodes["ADDI"  ] = {0xE3, 0x00, TwoBytes,   vCpu, InOutVAC};
        _asmOpcodes["SUBI"  ] = {0xE6, 0x00, TwoBytes,   vCpu, InOutVAC};
        _asmOpcodes["LSLW"  ] = {0xE9, 0x00, OneByte,    vCpu, InOutVAC};
        _asmOpcodes["STLW"  ] = {0xEC, 0x00, TwoBytes,   vCpu, InVAC   };
        _asmOpcodes["LDLW"  ] = {0xEE, 0x00, TwoBytes,   vCpu, OutVAC  };
        _asmOpcodes["POKE"  ] = {0xF0, 0x00, TwoBytes,   vCpu, InVAC   };
        _asmOpcodes["DOKE"  ] = {0xF3, 0x00, TwoBytes,   vCpu, InVAC   };
        _asmOpcodes["DEEK"  ] = {0xF6, 0x00, OneByte,    vCpu, InOutVAC};
        _asmOpcodes["ANDW"  ] = {0xF8, 0x00, TwoBytes,   vCpu, InOutVAC};
        _asmOpcodes["ORW"   ] = {0xFA, 0x00, TwoBytes,   vCpu, InOutVAC};
        _asmOpcodes["XORW"  ] = {0xFC, 0x00, TwoBytes,   vCpu, InOutVAC};
        _asmOpcodes["RET"   ] = {0xFF, 0x00, OneByte,    vCpu, NoVAC   };

        // Psuedo vCPU instructions
        _asmOpcodes["HALT"] = {0xB4, 0x80, TwoBytes, vCpu, InVAC};

        // PREFX1 vCPU instructions
        _asmOpcodes["NOTE"  ] = {0xB1, 0x11, TwoBytes, vCpu, InOutVAC};
        _asmOpcodes["MIDI"  ] = {0xB1, 0x14, TwoBytes, vCpu, InOutVAC};
        _asmOpcodes["XLA"   ] = {0xB1, 0x17, TwoBytes, vCpu, InOutVAC};
        _asmOpcodes["RANDW" ] = {0xB1, 0x2F, TwoBytes, vCpu, OutVAC  };
        _asmOpcodes["LDPX"  ] = {0xB1, 0x31, TwoBytes, vCpu, InOutVAC};
        _asmOpcodes["ABSW"  ] = {0xB1, 0x33, TwoBytes, vCpu, InOutVAC};
        _asmOpcodes["SGNW"  ] = {0xB1, 0x36, TwoBytes, vCpu, InOutVAC};
        _asmOpcodes["MULB3" ] = {0xB1, 0x39, TwoBytes, vCpu, InOutVAC};
        _asmOpcodes["MULB5" ] = {0xB1, 0x3b, TwoBytes, vCpu, InOutVAC};
        _asmOpcodes["MULB6" ] = {0xB1, 0x3d, TwoBytes, vCpu, InOutVAC};
        _asmOpcodes["MULB7" ] = {0xB1, 0x3f, TwoBytes, vCpu, InOutVAC};
        _asmOpcodes["MULB8" ] = {0xB1, 0x41, TwoBytes, vCpu, InOutVAC};
        _asmOpcodes["MULB9" ] = {0xB1, 0x43, TwoBytes, vCpu, InOutVAC};
        _asmOpcodes["MULB10"] = {0xB1, 0x45, TwoBytes, vCpu, InOutVAC};
        _asmOpcodes["WAITVB"] = {0xB1, 0x47, TwoBytes, vCpu, NoVAC   };

        // PREFX2 vCPU instructions
        _asmOpcodes["LSLN"  ] = {0x2F, 0x11, ThreeBytes, vCpu, InOutVAC};
        _asmOpcodes["SEXT"  ] = {0x2F, 0x13, ThreeBytes, vCpu, InOutVAC};
        _asmOpcodes["NOTW"  ] = {0x2F, 0x15, ThreeBytes, vCpu, NoVAC   };
        _asmOpcodes["NEGW"  ] = {0x2F, 0x17, ThreeBytes, vCpu, NoVAC   };
        _asmOpcodes["ANDBA" ] = {0x2F, 0x19, ThreeBytes, vCpu, InOutVAC};
        _asmOpcodes["ORBA"  ] = {0x2F, 0x1C, ThreeBytes, vCpu, InOutVAC};
        _asmOpcodes["XORBA" ] = {0x2F, 0x1F, ThreeBytes, vCpu, InOutVAC};
        _asmOpcodes["FREQM" ] = {0x2F, 0x22, ThreeBytes, vCpu, InVAC   };
        _asmOpcodes["FREQA" ] = {0x2F, 0x24, ThreeBytes, vCpu, InVAC   };
        _asmOpcodes["FREQI" ] = {0x2F, 0x27, ThreeBytes, vCpu, InVAC   };
        _asmOpcodes["VOLM"  ] = {0x2F, 0x29, ThreeBytes, vCpu, InVAC   };
        _asmOpcodes["VOLA"  ] = {0x2F, 0x2C, ThreeBytes, vCpu, InVAC   };
        _asmOpcodes["MODA"  ] = {0x2F, 0x2F, ThreeBytes, vCpu, InVAC   };
        _asmOpcodes["MODI"  ] = {0x2F, 0x32, ThreeBytes, vCpu, InVAC   };
        _asmOpcodes["SMPCPY"] = {0x2F, 0x34, ThreeBytes, vCpu, InOutVAC};
        _asmOpcodes["LEEKA" ] = {0x2F, 0x3D, ThreeBytes, vCpu, InVAC   };
        _asmOpcodes["LOKEA" ] = {0x2F, 0x3F, ThreeBytes, vCpu, InVAC   };
        _asmOpcodes["FEEKA" ] = {0x2F, 0x41, ThreeBytes, vCpu, InVAC   };
        _asmOpcodes["FOKEA" ] = {0x2F, 0x43, ThreeBytes, vCpu, InVAC   };
        _asmOpcodes["MEEKA" ] = {0x2F, 0x45, ThreeBytes, vCpu, InVAC   };
        _asmOpcodes["MOKEA" ] = {0x2F, 0x47, ThreeBytes, vCpu, InVAC   };
        _asmOpcodes["LSRVL" ] = {0x2F, 0x49, ThreeBytes, vCpu, NoVAC   };
        _asmOpcodes["LSLVL" ] = {0x2F, 0x4C, ThreeBytes, vCpu, NoVAC   };
        _asmOpcodes["INCL"  ] = {0x2F, 0x4F, ThreeBytes, vCpu, NoVAC   };
        _asmOpcodes["DECL"  ] = {0x2F, 0x52, ThreeBytes, vCpu, NoVAC   };
        _asmOpcodes["STPX"  ] = {0x2F, 0x54, ThreeBytes, vCpu, InVAC   };
        _asmOpcodes["PRN4X6"] = {0x2F, 0x57, ThreeBytes, vCpu, InVAC   };
        _asmOpcodes["VTBL"  ] = {0x2F, 0x59, ThreeBytes, vCpu, InVAC   };
        _asmOpcodes["OSCZ"  ] = {0x2F, 0x5c, ThreeBytes, vCpu, NoVAC   };
        _asmOpcodes["LSL8"  ] = {0x2F, 0x5E, ThreeBytes, vCpu, OutVAC  };
        _asmOpcodes["ADDBA" ] = {0x2F, 0x60, ThreeBytes, vCpu, InOutVAC};
        _asmOpcodes["SUBBA" ] = {0x2F, 0x62, ThreeBytes, vCpu, InOutVAC};
        _asmOpcodes["NOTB"  ] = {0x2F, 0x64, ThreeBytes, vCpu, NoVAC   };
        _asmOpcodes["ABSVW" ] = {0x2F, 0x67, ThreeBytes, vCpu, NoVAC   };
        _asmOpcodes["INCW"  ] = {0x2F, 0x6a, ThreeBytes, vCpu, NoVAC   };
        _asmOpcodes["DECW"  ] = {0x2F, 0x6c, ThreeBytes, vCpu, NoVAC   };

        // PREFX3 vCPU instructions
        _asmOpcodes["STB2"  ] = {0xC7, 0x11, FourBytes, vCpu, InVAC   };
        _asmOpcodes["STW2"  ] = {0xC7, 0x14, FourBytes, vCpu, InVAC   };
        _asmOpcodes["XCHGB" ] = {0xC7, 0x17, FourBytes, vCpu, NoVAC   };
        _asmOpcodes["MOVW"  ] = {0xC7, 0x19, FourBytes, vCpu, NoVAC   };
        _asmOpcodes["ADDWI" ] = {0xC7, 0x1B, FourBytes, vCpu, InOutVAC};
        _asmOpcodes["SUBWI" ] = {0xC7, 0x1D, FourBytes, vCpu, InOutVAC};
        _asmOpcodes["ANDWI" ] = {0xC7, 0x1F, FourBytes, vCpu, InOutVAC};
        _asmOpcodes["ORWI"  ] = {0xC7, 0x21, FourBytes, vCpu, InOutVAC};
        _asmOpcodes["XORWI" ] = {0xC7, 0x23, FourBytes, vCpu, InOutVAC};
        _asmOpcodes["FNT6X8"] = {0xC7, 0x25, FourBytes, vCpu, OutVAC  };
        _asmOpcodes["FNT4X6"] = {0xC7, 0x28, FourBytes, vCpu, OutVAC  };
        _asmOpcodes["CONDII"] = {0xC7, 0x2A, FourBytes, vCpu, InOutVAC};
        _asmOpcodes["CONDBB"] = {0xC7, 0x2C, FourBytes, vCpu, InOutVAC};
        _asmOpcodes["CONDIB"] = {0xC7, 0x2F, FourBytes, vCpu, InOutVAC};
        _asmOpcodes["CONDBI"] = {0xC7, 0x32, FourBytes, vCpu, InOutVAC};
        _asmOpcodes["XCHGW" ] = {0xC7, 0x34, FourBytes, vCpu, NoVAC   };
        _asmOpcodes["OSCPX" ] = {0xC7, 0x37, FourBytes, vCpu, InVAC   };
        _asmOpcodes["SWAPB" ] = {0xC7, 0x39, FourBytes, vCpu, NoVAC   };
        _asmOpcodes["SWAPW" ] = {0xC7, 0x3C, FourBytes, vCpu, NoVAC   };
        _asmOpcodes["NEEKA" ] = {0xC7, 0x3F, FourBytes, vCpu, InVAC   };
        _asmOpcodes["NOKEA" ] = {0xC7, 0x42, FourBytes, vCpu, InVAC   };
        _asmOpcodes["ADDVL" ] = {0xC7, 0x45, FourBytes, vCpu, NoVAC   };
        _asmOpcodes["SUBVL" ] = {0xC7, 0x48, FourBytes, vCpu, NoVAC   };
        _asmOpcodes["ANDVL" ] = {0xC7, 0x4B, FourBytes, vCpu, NoVAC   };
        _asmOpcodes["ORVL"  ] = {0xC7, 0x4E, FourBytes, vCpu, NoVAC   };
        _asmOpcodes["XORVL" ] = {0xC7, 0x51, FourBytes, vCpu, NoVAC   };
        _asmOpcodes["JEQL"  ] = {0xC7, 0x54, FourBytes, vCpu, InVAC   };
        _asmOpcodes["JNEL"  ] = {0xC7, 0x57, FourBytes, vCpu, InVAC   };
        _asmOpcodes["JLTL"  ] = {0xC7, 0x5A, FourBytes, vCpu, InVAC   };
        _asmOpcodes["JGTL"  ] = {0xC7, 0x5D, FourBytes, vCpu, InVAC   };
        _asmOpcodes["JLEL"  ] = {0xC7, 0x60, FourBytes, vCpu, InVAC   };
        _asmOpcodes["JGEL"  ] = {0xC7, 0x63, FourBytes, vCpu, InVAC   };
        _asmOpcodes["ANDBI" ] = {0xC7, 0x66, FourBytes, vCpu, NoVAC   };
        _asmOpcodes["ORBI"  ] = {0xC7, 0x69, FourBytes, vCpu, NoVAC   };
        _asmOpcodes["XORBI" ] = {0xC7, 0x6C, FourBytes, vCpu, NoVAC   };
        _asmOpcodes["ANDBK" ] = {0xC7, 0x6F, FourBytes, vCpu, OutVAC  };
        _asmOpcodes["ORBK"  ] = {0xC7, 0x72, FourBytes, vCpu, OutVAC  };
        _asmOpcodes["XORBK" ] = {0xC7, 0x75, FourBytes, vCpu, OutVAC  };
        _asmOpcodes["JMPI"  ] = {0xC7, 0x78, FourBytes, vCpu, NoVAC   };
        _asmOpcodes["SUBIW" ] = {0xC7, 0x7B, FourBytes, vCpu, InOutVAC};
        _asmOpcodes["VADDBW"] = {0xC7, 0x7D, FourBytes, vCpu, NoVAC   };
        _asmOpcodes["VSUBBW"] = {0xC7, 0x80, FourBytes, vCpu, NoVAC   };
        _asmOpcodes["VADDBL"] = {0xC7, 0x83, FourBytes, vCpu, NoVAC   };
        _asmOpcodes["VSUBBL"] = {0xC7, 0x86, FourBytes, vCpu, NoVAC   };
        _asmOpcodes["CMPII" ] = {0xC7, 0x89, FourBytes, vCpu, InOutVAC};

        // Gigatron vCPU branch instructions
        _asmOpcodes["BEQ"] = {0x35, 0x3F, ThreeBytes, vCpu, InVAC};
        _asmOpcodes["BGT"] = {0x35, 0x4D, ThreeBytes, vCpu, InVAC};
        _asmOpcodes["BLT"] = {0x35, 0x50, ThreeBytes, vCpu, InVAC};
        _asmOpcodes["BGE"] = {0x35, 0x53, ThreeBytes, vCpu, InVAC};
        _asmOpcodes["BLE"] = {0x35, 0x56, ThreeBytes, vCpu, InVAC};
        _asmOpcodes["BNE"] = {0x35, 0x72, ThreeBytes, vCpu, InVAC};

        // Reserved assembler opcodes
        _asmOpcodes["DB" ] = {0x00, 0x00, TwoBytes,   ReservedDB };
        _asmOpcodes["DW" ] = {0x00, 0x00, ThreeBytes, ReservedDW };
        _asmOpcodes["DBR"] = {0x00, 0x00, TwoBytes,   ReservedDBR};
        _asmOpcodes["DWR"] = {0x00, 0x00, ThreeBytes, ReservedDWR};

        // Gigatron native instructions
        _asmOpcodes[".LD"  ] = {0x00, 0x00, TwoBytes, Native};
        _asmOpcodes[".NOP" ] = {0x02, 0x00, TwoBytes, Native};
        _asmOpcodes[".ANDA"] = {0x20, 0x00, TwoBytes, Native};
        _asmOpcodes[".ORA" ] = {0x40, 0x00, TwoBytes, Native};
        _asmOpcodes[".XORA"] = {0x60, 0x00, TwoBytes, Native};
        _asmOpcodes[".ADDA"] = {0x80, 0x00, TwoBytes, Native};
        _asmOpcodes[".SUBA"] = {0xA0, 0x00, TwoBytes, Native};
        _asmOpcodes[".ST"  ] = {0xC0, 0x00, TwoBytes, Native};
        _asmOpcodes[".JMP" ] = {0xE0, 0x00, TwoBytes, Native};
        _asmOpcodes[".BGT" ] = {0xE4, 0x00, TwoBytes, Native};
        _asmOpcodes[".BLT" ] = {0xE8, 0x00, TwoBytes, Native};
        _asmOpcodes[".BNE" ] = {0xEC, 0x00, TwoBytes, Native};
        _asmOpcodes[".BEQ" ] = {0xF0, 0x00, TwoBytes, Native};
        _asmOpcodes[".BGE" ] = {0xF4, 0x00, TwoBytes, Native};
        _asmOpcodes[".BLE" ] = {0xF8, 0x00, TwoBytes, Native};
        _asmOpcodes[".BRA" ] = {0xFC, 0x00, TwoBytes, Native};

        // Gigatron vCPU instructions
        _vcpuOpcodes[0x11] = {0x11, 0x00, ThreeBytes, vCpu, "LDWI"  , OutVAC  };   
        _vcpuOpcodes[0x14] = {0x14, 0x00, TwoBytes,   vCpu, "DEC"   , NoVAC   };
        _vcpuOpcodes[0x16] = {0x16, 0x00, ThreeBytes, vCpu, "MOVQB" , NoVAC   };
        _vcpuOpcodes[0x18] = {0x18, 0x00, TwoBytes,   vCpu, "LSRB"  , NoVAC   };
        _vcpuOpcodes[0x1A] = {0x1A, 0x00, TwoBytes,   vCpu, "LD"    , OutVAC  };
        _vcpuOpcodes[0x1C] = {0x1C, 0x00, FourBytes,  vCpu, "ADDBI" , NoVAC   };
        _vcpuOpcodes[0x1F] = {0x1F, 0x00, TwoBytes,   vCpu, "CMPHS" , InOutVAC};
        _vcpuOpcodes[0x21] = {0x21, 0x00, TwoBytes,   vCpu, "LDW"   , OutVAC  };
        _vcpuOpcodes[0x23] = {0x23, 0x00, TwoBytes,   vCpu, "PEEKV+", OutVAC  };
        _vcpuOpcodes[0x25] = {0x25, 0x00, TwoBytes,   vCpu, "POKEI" , InVAC   };
        _vcpuOpcodes[0x27] = {0x27, 0x00, TwoBytes,   vCpu, "LSLV"  , NoVAC   };
        _vcpuOpcodes[0x29] = {0x29, 0x00, FourBytes,  vCpu, "ADDVB" , NoVAC   };
        _vcpuOpcodes[0x2B] = {0x2B, 0x00, TwoBytes,   vCpu, "STW"   , InVAC   };
        _vcpuOpcodes[0x2D] = {0x2D, 0x00, FourBytes,  vCpu, "CNVXY" , NoVAC   };
        _vcpuOpcodes[0x32] = {0x32, 0x00, ThreeBytes, vCpu, "MOVWA" , OutVAC  };
        _vcpuOpcodes[0x38] = {0x38, 0x00, FourBytes,  vCpu, "SUBBI" , NoVAC   };
        _vcpuOpcodes[0x3B] = {0x3B, 0x00, TwoBytes,   vCpu, "DEEKV" , OutVAC  };
        _vcpuOpcodes[0x3D] = {0x3D, 0x00, FourBytes,  vCpu, "ARRVW" , OutVAC  };
        _vcpuOpcodes[0x3F] = {0x3F, 0x00, FourBytes,  vCpu, "LDARRW", OutVAC  };
        _vcpuOpcodes[0x42] = {0x42, 0x00, FourBytes,  vCpu, "ADDVI" , OutVAC  };
        _vcpuOpcodes[0x44] = {0x44, 0x00, FourBytes,  vCpu, "SUBVI" , OutVAC  };
        _vcpuOpcodes[0x46] = {0x46, 0x00, TwoBytes,   vCpu, "DEEKV+", OutVAC  };
        _vcpuOpcodes[0x48] = {0x48, 0x00, FourBytes,  vCpu, "SUBVB" , NoVAC   };
        _vcpuOpcodes[0x4A] = {0x4A, 0x00, FourBytes,  vCpu, "DJGE"  , NoVAC   };
        _vcpuOpcodes[0x4D] = {0x4D, 0x00, ThreeBytes, vCpu, "MOVQW" , NoVAC   };
        _vcpuOpcodes[0x4F] = {0x4F, 0x00, ThreeBytes, vCpu, "STWM"  , InVAC   };
        _vcpuOpcodes[0x51] = {0x51, 0x00, FourBytes,  vCpu, "STARRW", InVAC   };
        _vcpuOpcodes[0x53] = {0x53, 0x00, FourBytes,  vCpu, "LDARRB", OutVAC  };
        _vcpuOpcodes[0x55] = {0x55, 0x00, FourBytes,  vCpu, "STARRB", InVAC   };
        _vcpuOpcodes[0x57] = {0x57, 0x00, FiveBytes,  vCpu, "STARRI", NoVAC   };
        _vcpuOpcodes[0x59] = {0x59, 0x00, TwoBytes,   vCpu, "LDI"   , OutVAC  };
        _vcpuOpcodes[0x5B] = {0x5B, 0x00, TwoBytes,   vCpu, "PEEKV" , OutVAC  };
        _vcpuOpcodes[0x5E] = {0x5E, 0x00, TwoBytes,   vCpu, "ST"    , InVAC   };
        _vcpuOpcodes[0x60] = {0x60, 0x00, TwoBytes,   vCpu, "DOKEV+", InVAC   };
        _vcpuOpcodes[0x63] = {0x63, 0x00, OneByte,    vCpu, "POP"   , NoVAC   };
        _vcpuOpcodes[0x65] = {0x65, 0x00, ThreeBytes, vCpu, "MOVB"  , NoVAC   };
        _vcpuOpcodes[0x67] = {0x67, 0x00, TwoBytes,   vCpu, "PEEKA" , InVAC   };
        _vcpuOpcodes[0x69] = {0x69, 0x00, TwoBytes,   vCpu, "POKEA" , InVAC   };
        _vcpuOpcodes[0x6B] = {0x6B, 0x00, TwoBytes,   vCpu, "TEQ"   , InVAC   };
        _vcpuOpcodes[0x6D] = {0x6D, 0x00, TwoBytes,   vCpu, "TNE"   , InVAC   };
        _vcpuOpcodes[0x6F] = {0x6F, 0x00, TwoBytes,   vCpu, "DEEKA" , InVAC   };
        _vcpuOpcodes[0x72] = {0x72, 0x00, ThreeBytes, vCpu, "LDWM"  , OutVAC  };
        _vcpuOpcodes[0x75] = {0x75, 0x00, OneByte,    vCpu, "PUSH"  , NoVAC   };
        _vcpuOpcodes[0x77] = {0x77, 0x00, ThreeBytes, vCpu, "DOKEI" , InVAC   };
        _vcpuOpcodes[0x79] = {0x79, 0x00, ThreeBytes, vCpu, "ARRW"  , InOutVAC};
        _vcpuOpcodes[0x7B] = {0x7B, 0x00, TwoBytes,   vCpu, "POKEA+", InOutVAC};
        _vcpuOpcodes[0x7D] = {0x7D, 0x00, TwoBytes,   vCpu, "DOKEA" , InVAC   };
        _vcpuOpcodes[0x7F] = {0x7F, 0x00, TwoBytes,   vCpu, "LUP"   , InOutVAC};
        _vcpuOpcodes[0x82] = {0x82, 0x00, TwoBytes,   vCpu, "ANDI"  , InOutVAC};
        _vcpuOpcodes[0x85] = {0x85, 0x00, ThreeBytes, vCpu, "CALLI" , NoVAC   };
        _vcpuOpcodes[0x88] = {0x88, 0x00, TwoBytes,   vCpu, "ORI"   , InOutVAC};
        _vcpuOpcodes[0x8A] = {0x8A, 0x00, TwoBytes,   vCpu, "PEEKA+", InOutVAC};
        _vcpuOpcodes[0x8C] = {0x8C, 0x00, TwoBytes,   vCpu, "XORI"  , InOutVAC};
        _vcpuOpcodes[0x8E] = {0x8E, 0x00, ThreeBytes, vCpu, "DBGE"  , NoVAC   };
        _vcpuOpcodes[0x90] = {0x90, 0x00, TwoBytes,   vCpu, "BRA"   , NoVAC   };
        _vcpuOpcodes[0x93] = {0x93, 0x00, TwoBytes,   vCpu, "INC"   , NoVAC   };
        _vcpuOpcodes[0x95] = {0x95, 0x00, TwoBytes,   vCpu, "INCWA" , OutVAC  };
        _vcpuOpcodes[0x97] = {0x97, 0x00, TwoBytes,   vCpu, "CMPHU" , InOutVAC};
        _vcpuOpcodes[0x99] = {0x99, 0x00, TwoBytes,   vCpu, "ADDW"  , InOutVAC};
        _vcpuOpcodes[0x9C] = {0x9C, 0x00, TwoBytes,   vCpu, "LDNI"  , OutVAC  };
        _vcpuOpcodes[0x9E] = {0x9E, 0x00, ThreeBytes, vCpu, "DBNE"  , NoVAC   };
        _vcpuOpcodes[0xA0] = {0xA0, 0x00, TwoBytes,   vCpu, "DEEKR" , InOutVAC};
        _vcpuOpcodes[0xA2] = {0xA2, 0x00, ThreeBytes, vCpu, "PACKAW", OutVAC  };
        _vcpuOpcodes[0xA4] = {0xA4, 0x00, FourBytes,  vCpu, "DJNE"  , NoVAC   };
        _vcpuOpcodes[0xA7] = {0xA7, 0x00, ThreeBytes, vCpu, "CMPI"  , OutVAC  };
        _vcpuOpcodes[0xA9] = {0xA9, 0x00, FourBytes,  vCpu, "ADDVW" , OutVAC  };
        _vcpuOpcodes[0xAB] = {0xAB, 0x00, FourBytes,  vCpu, "SUBVW" , OutVAC  };
        _vcpuOpcodes[0xAD] = {0xAD, 0x00, OneByte,    vCpu, "PEEK"  , InOutVAC};
        _vcpuOpcodes[0xB4] = {0xB4, 0x00, TwoBytes,   vCpu, "SYS"   , NoVAC   };
        _vcpuOpcodes[0xB8] = {0xB8, 0x00, TwoBytes,   vCpu, "SUBW"  , InOutVAC};
        _vcpuOpcodes[0xBB] = {0xBB, 0x00, ThreeBytes, vCpu, "JEQ"   , InVAC   };
        _vcpuOpcodes[0xBD] = {0xBD, 0x00, ThreeBytes, vCpu, "JNE"   , InVAC   };
        _vcpuOpcodes[0xBF] = {0xBF, 0x00, ThreeBytes, vCpu, "JLT"   , InVAC   };
        _vcpuOpcodes[0xC1] = {0xC1, 0x00, ThreeBytes, vCpu, "JGT"   , InVAC   };
        _vcpuOpcodes[0xC3] = {0xC3, 0x00, ThreeBytes, vCpu, "JLE"   , InVAC   };
        _vcpuOpcodes[0xC5] = {0xC5, 0x00, ThreeBytes, vCpu, "JGE"   , InVAC   };
        _vcpuOpcodes[0xCD] = {0xCD, 0x00, TwoBytes,   vCpu, "DEF"   , OutVAC  };
        _vcpuOpcodes[0xCF] = {0xCF, 0x00, TwoBytes,   vCpu, "CALL"  , NoVAC   };
        _vcpuOpcodes[0xD1] = {0xD1, 0x00, TwoBytes,   vCpu, "POKEV+", InVAC   };
        _vcpuOpcodes[0xD3] = {0xD3, 0x00, TwoBytes,   vCpu, "LSRV"  , NoVAC   };
        _vcpuOpcodes[0xD5] = {0xD5, 0x00, TwoBytes,   vCpu, "TGE"   , InVAC   };
        _vcpuOpcodes[0xD7] = {0xD7, 0x00, TwoBytes,   vCpu, "TLT"   , InVAC   };
        _vcpuOpcodes[0xD9] = {0xD9, 0x00, TwoBytes,   vCpu, "TGT"   , InVAC   };
        _vcpuOpcodes[0xDB] = {0xDB, 0x00, TwoBytes,   vCpu, "TLE"   , InVAC   };
        _vcpuOpcodes[0xDD] = {0xDD, 0x00, TwoBytes,   vCpu, "DECWA" , OutVAC  };
        _vcpuOpcodes[0xDF] = {0xDF, 0x00, TwoBytes,   vCpu, "ALLOC" , NoVAC   }; 
        _vcpuOpcodes[0xE1] = {0xE1, 0x00, FourBytes,  vCpu, "PACKVW", NoVAC   };
        _vcpuOpcodes[0xE3] = {0xE3, 0x00, TwoBytes,   vCpu, "ADDI"  , InOutVAC};
        _vcpuOpcodes[0xE6] = {0xE6, 0x00, TwoBytes,   vCpu, "SUBI"  , InOutVAC};
        _vcpuOpcodes[0xE9] = {0xE9, 0x00, OneByte,    vCpu, "LSLW"  , InOutVAC};
        _vcpuOpcodes[0xEC] = {0xEC, 0x00, TwoBytes,   vCpu, "STLW"  , InVAC   };
        _vcpuOpcodes[0xEE] = {0xEE, 0x00, TwoBytes,   vCpu, "LDLW"  , OutVAC  };
        _vcpuOpcodes[0xF0] = {0xF0, 0x00, TwoBytes,   vCpu, "POKE"  , InVAC   };
        _vcpuOpcodes[0xF3] = {0xF3, 0x00, TwoBytes,   vCpu, "DOKE"  , InVAC   };
        _vcpuOpcodes[0xF6] = {0xF6, 0x00, OneByte,    vCpu, "DEEK"  , InOutVAC};
        _vcpuOpcodes[0xF8] = {0xF8, 0x00, TwoBytes,   vCpu, "ANDW"  , InOutVAC};
        _vcpuOpcodes[0xFA] = {0xFA, 0x00, TwoBytes,   vCpu, "ORW"   , InOutVAC};
        _vcpuOpcodes[0xFC] = {0xFC, 0x00, TwoBytes,   vCpu, "XORW"  , InOutVAC};
        _vcpuOpcodes[0xFF] = {0xFF, 0x00, OneByte,    vCpu, "RET"   , NoVAC   };

        // vCPU branch instructions, (this works because condition code is still unique compared to opcodes)
        _vbraOpcodes[0x3F] = {OPCODE_V_BCC, 0x3F, ThreeBytes, vCpu, "BEQ", InVAC};
        _vbraOpcodes[0x4D] = {OPCODE_V_BCC, 0x4D, ThreeBytes, vCpu, "BGT", InVAC};
        _vbraOpcodes[0x50] = {OPCODE_V_BCC, 0x50, ThreeBytes, vCpu, "BLT", InVAC};
        _vbraOpcodes[0x53] = {OPCODE_V_BCC, 0x53, ThreeBytes, vCpu, "BGE", InVAC};
        _vbraOpcodes[0x56] = {OPCODE_V_BCC, 0x56, ThreeBytes, vCpu, "BLE", InVAC};
        _vbraOpcodes[0x72] = {OPCODE_V_BCC, 0x72, ThreeBytes, vCpu, "BNE", InVAC};

        // vCPU PREFX1 instructions
        _vcpuOpcodes[0xB111] = {0xB1, 0x11, TwoBytes, vCpu, "NOTE"  , InOutVAC};
        _vcpuOpcodes[0xB114] = {0xB1, 0x14, TwoBytes, vCpu, "MIDI"  , InOutVAC};
        _vcpuOpcodes[0xB117] = {0xB1, 0x17, TwoBytes, vCpu, "XLA"   , InOutVAC};
        _vcpuOpcodes[0xB12F] = {0xB1, 0x2F, TwoBytes, vCpu, "RANDW" , OutVAC  };
        _vcpuOpcodes[0xB131] = {0xB1, 0x31, TwoBytes, vCpu, "LDPX"  , InOutVAC};
        _vcpuOpcodes[0xB133] = {0xB1, 0x33, TwoBytes, vCpu, "ABSW"  , InOutVAC};
        _vcpuOpcodes[0xB136] = {0xB1, 0x36, TwoBytes, vCpu, "SGNW"  , InOutVAC};
        _vcpuOpcodes[0xB139] = {0xB1, 0x39, TwoBytes, vCpu, "MULB3" , InOutVAC};
        _vcpuOpcodes[0xB13b] = {0xB1, 0x3b, TwoBytes, vCpu, "MULB5" , InOutVAC};
        _vcpuOpcodes[0xB13d] = {0xB1, 0x3d, TwoBytes, vCpu, "MULB6" , InOutVAC};
        _vcpuOpcodes[0xB13f] = {0xB1, 0x3f, TwoBytes, vCpu, "MULB7" , InOutVAC};
        _vcpuOpcodes[0xB141] = {0xB1, 0x41, TwoBytes, vCpu, "MULB8" , InOutVAC};
        _vcpuOpcodes[0xB143] = {0xB1, 0x43, TwoBytes, vCpu, "MULB9" , InOutVAC};
        _vcpuOpcodes[0xB145] = {0xB1, 0x45, TwoBytes, vCpu, "MULB10", InOutVAC};
        _vcpuOpcodes[0xB147] = {0xB1, 0x47, TwoBytes, vCpu, "WAITVB", NoVAC   };

        // vCPU PREFX2 instructions
        _vcpuOpcodes[0x2F11] = {0x2F, 0x11, ThreeBytes, vCpu, "LSLN"  , InOutVAC};
        _vcpuOpcodes[0x2F13] = {0x2F, 0x13, ThreeBytes, vCpu, "SEXT"  , InOutVAC};
        _vcpuOpcodes[0x2F15] = {0x2F, 0x15, ThreeBytes, vCpu, "NOTW"  , NoVAC   };
        _vcpuOpcodes[0x2F17] = {0x2F, 0x17, ThreeBytes, vCpu, "NEGW"  , NoVAC   };
        _vcpuOpcodes[0x2F19] = {0x2F, 0x19, ThreeBytes, vCpu, "ANDBA" , InOutVAC};
        _vcpuOpcodes[0x2F1C] = {0x2F, 0x1C, ThreeBytes, vCpu, "ORBA"  , InOutVAC};
        _vcpuOpcodes[0x2F1F] = {0x2F, 0x1F, ThreeBytes, vCpu, "XORBA" , InOutVAC};
        _vcpuOpcodes[0x2F22] = {0x2F, 0x22, ThreeBytes, vCpu, "FREQM" , InVAC   };
        _vcpuOpcodes[0x2F24] = {0x2F, 0x24, ThreeBytes, vCpu, "FREQA" , InVAC   };
        _vcpuOpcodes[0x2F27] = {0x2F, 0x27, ThreeBytes, vCpu, "FREQI" , InVAC   };
        _vcpuOpcodes[0x2F29] = {0x2F, 0x29, ThreeBytes, vCpu, "VOLM"  , InVAC   };
        _vcpuOpcodes[0x2F2C] = {0x2F, 0x2C, ThreeBytes, vCpu, "VOLA"  , InVAC   };
        _vcpuOpcodes[0x2F2F] = {0x2F, 0x2F, ThreeBytes, vCpu, "MODA"  , InVAC   };
        _vcpuOpcodes[0x2F32] = {0x2F, 0x32, ThreeBytes, vCpu, "MODI"  , InVAC   };
        _vcpuOpcodes[0x2F34] = {0x2F, 0x34, ThreeBytes, vCpu, "SMPCPY", InOutVAC};
        _vcpuOpcodes[0x2F3D] = {0x2F, 0x3D, ThreeBytes, vCpu, "LEEKA" , InVAC   };
        _vcpuOpcodes[0x2F3F] = {0x2F, 0x3F, ThreeBytes, vCpu, "LOKEA" , InVAC   };
        _vcpuOpcodes[0x2F41] = {0x2F, 0x41, ThreeBytes, vCpu, "FEEKA" , InVAC   };
        _vcpuOpcodes[0x2F43] = {0x2F, 0x43, ThreeBytes, vCpu, "FOKEA" , InVAC   };
        _vcpuOpcodes[0x2F45] = {0x2F, 0x45, ThreeBytes, vCpu, "MEEKA" , InVAC   };
        _vcpuOpcodes[0x2F47] = {0x2F, 0x47, ThreeBytes, vCpu, "MOKEA" , InVAC   };
        _vcpuOpcodes[0x2F49] = {0x2F, 0x49, ThreeBytes, vCpu, "LSRVL" , NoVAC   };
        _vcpuOpcodes[0x2F4C] = {0x2F, 0x4C, ThreeBytes, vCpu, "LSLVL" , NoVAC   };
        _vcpuOpcodes[0x2F4F] = {0x2F, 0x4F, ThreeBytes, vCpu, "INCL"  , NoVAC   }; 
        _vcpuOpcodes[0x2F52] = {0x2F, 0x52, ThreeBytes, vCpu, "DECL"  , NoVAC   };
        _vcpuOpcodes[0x2F54] = {0x2F, 0x54, ThreeBytes, vCpu, "STPX"  , InVAC   };
        _vcpuOpcodes[0x2F57] = {0x2F, 0x57, ThreeBytes, vCpu, "PRN4X6", InVAC   };
        _vcpuOpcodes[0x2F59] = {0x2F, 0x59, ThreeBytes, vCpu, "VTBL"  , InVAC   };
        _vcpuOpcodes[0x2F5C] = {0x2F, 0x5C, ThreeBytes, vCpu, "OSCZ"  , NoVAC   };
        _vcpuOpcodes[0x2F5E] = {0x2F, 0x5E, ThreeBytes, vCpu, "LSL8"  , OutVAC  };
        _vcpuOpcodes[0x2F60] = {0x2F, 0x60, ThreeBytes, vCpu, "ADDBA" , InOutVAC};
        _vcpuOpcodes[0x2F62] = {0x2F, 0x62, ThreeBytes, vCpu, "SUBBA" , InOutVAC};
        _vcpuOpcodes[0x2F64] = {0x2F, 0x64, ThreeBytes, vCpu, "NOTB"  , NoVAC   };
        _vcpuOpcodes[0x2F67] = {0x2F, 0x67, ThreeBytes, vCpu, "ABSVW" , NoVAC   };
        _vcpuOpcodes[0x2F6a] = {0x2F, 0x6a, ThreeBytes, vCpu, "INCW"  , NoVAC   };
        _vcpuOpcodes[0x2F6c] = {0x2F, 0x6c, ThreeBytes, vCpu, "DECW"  , NoVAC   };

        // vCPU PREFX3 instructions
        _vcpuOpcodes[0xC711] = {0xC7, 0x11, FourBytes, vCpu, "STB2"  , InVAC   };
        _vcpuOpcodes[0xC714] = {0xC7, 0x14, FourBytes, vCpu, "STW2"  , InVAC   };
        _vcpuOpcodes[0xC717] = {0xC7, 0x17, FourBytes, vCpu, "XCHGB" , NoVAC   };
        _vcpuOpcodes[0xC719] = {0xC7, 0x19, FourBytes, vCpu, "MOVW"  , NoVAC   };
        _vcpuOpcodes[0xC71B] = {0xC7, 0x1B, FourBytes, vCpu, "ADDWI" , InOutVAC};
        _vcpuOpcodes[0xC71D] = {0xC7, 0x1D, FourBytes, vCpu, "SUBWI" , InOutVAC};
        _vcpuOpcodes[0xC71F] = {0xC7, 0x1F, FourBytes, vCpu, "ANDWI" , InOutVAC};
        _vcpuOpcodes[0xC721] = {0xC7, 0x21, FourBytes, vCpu, "ORWI"  , InOutVAC};
        _vcpuOpcodes[0xC723] = {0xC7, 0x23, FourBytes, vCpu, "XORWI" , InOutVAC};
        _vcpuOpcodes[0xC725] = {0xC7, 0x25, FourBytes, vCpu, "FNT6X8", OutVAC  };
        _vcpuOpcodes[0xC728] = {0xC7, 0x28, FourBytes, vCpu, "FNT4X6", OutVAC  };
        _vcpuOpcodes[0xC72A] = {0xC7, 0x2A, FourBytes, vCpu, "CONDII", InOutVAC};
        _vcpuOpcodes[0xC72C] = {0xC7, 0x2C, FourBytes, vCpu, "CONDBB", InOutVAC};
        _vcpuOpcodes[0xC72F] = {0xC7, 0x2F, FourBytes, vCpu, "CONDIB", InOutVAC};
        _vcpuOpcodes[0xC732] = {0xC7, 0x32, FourBytes, vCpu, "CONDBI", InOutVAC};
        _vcpuOpcodes[0xC734] = {0xC7, 0x34, FourBytes, vCpu, "XCHGW" , NoVAC   };
        _vcpuOpcodes[0xC737] = {0xC7, 0x37, FourBytes, vCpu, "OSCPX" , InVAC   };
        _vcpuOpcodes[0xC739] = {0xC7, 0x39, FourBytes, vCpu, "SWAPB" , NoVAC   };
        _vcpuOpcodes[0xC73C] = {0xC7, 0x3C, FourBytes, vCpu, "SWAPW" , NoVAC   };
        _vcpuOpcodes[0xC73F] = {0xC7, 0x3F, FourBytes, vCpu, "NEEKA" , InVAC   };
        _vcpuOpcodes[0xC742] = {0xC7, 0x42, FourBytes, vCpu, "NOKEA" , InVAC   };
        _vcpuOpcodes[0xC745] = {0xC7, 0x45, FourBytes, vCpu, "ADDVL" , NoVAC   };
        _vcpuOpcodes[0xC748] = {0xC7, 0x48, FourBytes, vCpu, "SUBVL" , NoVAC   };
        _vcpuOpcodes[0xC74B] = {0xC7, 0x4B, FourBytes, vCpu, "ANDVL" , NoVAC   };
        _vcpuOpcodes[0xC74E] = {0xC7, 0x4E, FourBytes, vCpu, "ORVL"  , NoVAC   };
        _vcpuOpcodes[0xC751] = {0xC7, 0x51, FourBytes, vCpu, "XORVL" , NoVAC   };
        _vcpuOpcodes[0xC754] = {0xC7, 0x54, FourBytes, vCpu, "JEQL"  , InVAC   };
        _vcpuOpcodes[0xC757] = {0xC7, 0x57, FourBytes, vCpu, "JNEL"  , InVAC   };
        _vcpuOpcodes[0xC75A] = {0xC7, 0x5A, FourBytes, vCpu, "JLTL"  , InVAC   };
        _vcpuOpcodes[0xC75D] = {0xC7, 0x5D, FourBytes, vCpu, "JGTL"  , InVAC   };
        _vcpuOpcodes[0xC760] = {0xC7, 0x60, FourBytes, vCpu, "JLEL"  , InVAC   };
        _vcpuOpcodes[0xC763] = {0xC7, 0x63, FourBytes, vCpu, "JGEL"  , InVAC   };
        _vcpuOpcodes[0xC766] = {0xC7, 0x66, FourBytes, vCpu, "ANDBI" , NoVAC   };
        _vcpuOpcodes[0xC769] = {0xC7, 0x69, FourBytes, vCpu, "ORBI"  , NoVAC   };
        _vcpuOpcodes[0xC76C] = {0xC7, 0x6C, FourBytes, vCpu, "XORBI" , NoVAC   };
        _vcpuOpcodes[0xC76F] = {0xC7, 0x6F, FourBytes, vCpu, "ANDBK" , OutVAC  };
        _vcpuOpcodes[0xC772] = {0xC7, 0x72, FourBytes, vCpu, "ORBK"  , OutVAC  };
        _vcpuOpcodes[0xC775] = {0xC7, 0x75, FourBytes, vCpu, "XORBK" , OutVAC  };
        _vcpuOpcodes[0xC778] = {0xC7, 0x78, FourBytes, vCpu, "JMPI"  , NoVAC   };
        _vcpuOpcodes[0xC77B] = {0xC7, 0x7B, FourBytes, vCpu, "SUBIW" , InOutVAC};
        _vcpuOpcodes[0xC77D] = {0xC7, 0x7D, FourBytes, vCpu, "VADDBW", NoVAC   };
        _vcpuOpcodes[0xC780] = {0xC7, 0x80, FourBytes, vCpu, "VSUBBW", NoVAC   };
        _vcpuOpcodes[0xC783] = {0xC7, 0x83, FourBytes, vCpu, "VADDBL", NoVAC   };
        _vcpuOpcodes[0xC786] = {0xC7, 0x86, FourBytes, vCpu, "VSUBBL", NoVAC   };
        _vcpuOpcodes[0xC789] = {0xC7, 0x89, FourBytes, vCpu, "CMPII" , InOutVAC};

        // Native instructions
        _nativeOpcodes[0x00] = {0x00, 0x00, TwoBytes, Native, "LD"  };
        _nativeOpcodes[0x02] = {0x02, 0x00, TwoBytes, Native, "NOP" };
        _nativeOpcodes[0x20] = {0x20, 0x00, TwoBytes, Native, "ANDA"};
        _nativeOpcodes[0x40] = {0x40, 0x00, TwoBytes, Native, "ORA" };
        _nativeOpcodes[0x60] = {0x60, 0x00, TwoBytes, Native, "XORA"};
        _nativeOpcodes[0x80] = {0x80, 0x00, TwoBytes, Native, "ADDA"};
        _nativeOpcodes[0xA0] = {0xA0, 0x00, TwoBytes, Native, "SUBA"};
        _nativeOpcodes[0xC0] = {0xC0, 0x00, TwoBytes, Native, "ST"  };
        _nativeOpcodes[0xE0] = {0xE0, 0x00, TwoBytes, Native, "JMP" };
        _nativeOpcodes[0xE4] = {0xE4, 0x00, TwoBytes, Native, "BGT" };
        _nativeOpcodes[0xE8] = {0xE8, 0x00, TwoBytes, Native, "BLT" };
        _nativeOpcodes[0xEC] = {0xEC, 0x00, TwoBytes, Native, "BNE" };
        _nativeOpcodes[0xF0] = {0xF0, 0x00, TwoBytes, Native, "BEQ" };
        _nativeOpcodes[0xF4] = {0xF4, 0x00, TwoBytes, Native, "BGE" };
        _nativeOpcodes[0xF8] = {0xF8, 0x00, TwoBytes, Native, "BLE" };
        _nativeOpcodes[0xFC] = {0xFC, 0x00, TwoBytes, Native, "BRA" };
    }

    void initialise(void)
    {
        _reservedWords.push_back("_CALLTABLE_");
        _reservedWords.push_back("_BREAKPOINT_");
        _reservedWords.push_back("_STARTADDRESS_");
        _reservedWords.push_back("_SINGLESTEPWATCH_");
        _reservedWords.push_back("_DISABLEUPLOAD_");
        _reservedWords.push_back("_CPUUSAGEADDRESSA_");
        _reservedWords.push_back("_CPUUSAGEADDRESSB_");
        _reservedWords.push_back("%INCLUDE");
        _reservedWords.push_back("%DEFINE");
        _reservedWords.push_back("%IF");
        _reservedWords.push_back("%ELSE");
        _reservedWords.push_back("%ELSEIF");
        _reservedWords.push_back("%ENDIF");
        _reservedWords.push_back("%MACRO");
        _reservedWords.push_back("%ENDM");
        _reservedWords.push_back("%SUB");
        _reservedWords.push_back("%ENDS");
        _reservedWords.push_back("GPRINTF");

        initialiseOpcodes();
    }

#ifndef STAND_ALONE
    void getDasmCurrAndPrevByteSize(uint16_t address, ByteSize byteSize)
    {
        // Save current and previous instruction lengths
        if(_disassembledCode.size() == 0)
        {
            _currDasmByteCount = uint16_t(byteSize);

            // Attempt to get bytesize of previous instruction
            uint16_t addr = address - 1;
            for(uint16_t size=OneByte; size<MaxByteSize; size++)
            {
                uint8_t inst = Cpu::getRAM(addr);

                // Branch
                if(inst == OPCODE_V_BCC)
                {
                    inst = Cpu::getRAM(addr + 1);
                    if(_vbraOpcodes.find(inst) != _vbraOpcodes.end()  &&  _vbraOpcodes[inst]._opcodeType == vCpu  &&  _vbraOpcodes[inst]._byteSize == size)
                    {
                        _prevDasmByteCount = size;
                        break;
                    }
                }
                // Normal
                else if(_vcpuOpcodes.find(inst) != _vcpuOpcodes.end()  &&  _vcpuOpcodes[inst]._opcodeType == vCpu  &&  _vcpuOpcodes[inst]._byteSize == size)
                {
                    _prevDasmByteCount = size;
                    break;
                }

                addr--;
            }
        }
    }

    void getDasmCurrAndPrevPageByteSize(int pageSize)
    {
        // Current page size
        _currDasmPageByteCount = 0;
        for(int i=0; i<pageSize; i++) _currDasmPageByteCount += _disassembledCode[i]._byteSize;

        // Get bytesize of previous page worth of instructions
        _prevDasmPageByteCount = 0;
        uint16_t address = _disassembledCode[0]._address;
        for(int i=0; i<pageSize; i++)
        {
            // Attempt to get bytesize of previous instruction
            bool foundInstruction = false;
            uint16_t addr = address - 1;
            for(uint16_t size=OneByte; size<MaxByteSize; size++)
            {
                uint8_t inst = Cpu::getRAM(addr);

                // Branch
                if(inst == OPCODE_V_BCC)
                {
                    inst = Cpu::getRAM(addr + 1);
                    if(_vbraOpcodes.find(inst) != _vbraOpcodes.end()  &&  _vbraOpcodes[inst]._opcodeType == vCpu  &&  _vbraOpcodes[inst]._byteSize == size)
                    {
                        foundInstruction = true;
                        _prevDasmPageByteCount += size;
                        address = address - size;
                        break;
                    }
                }
                // Normal
                else if(_vcpuOpcodes.find(inst) != _vcpuOpcodes.end()  &&  _vcpuOpcodes[inst]._opcodeType == vCpu  &&  _vcpuOpcodes[inst]._byteSize == size)
                {
                    foundInstruction = true;
                    _prevDasmPageByteCount += size;
                    address = address - size;
                    break;
                }

                addr--;
            }

            if(!foundInstruction)
            {
                _prevDasmPageByteCount++;
                address--;
            }
        }
    }

    std::string removeBrackets(const char* str)
    {
        std::string string = str;
        Expression::stripChars(string, "[]");
        return string;
    }

    // Adapted from disassemble() in Core\asm.py
    bool getNativeMnemonic(uint8_t opcode, uint8_t operand, char* mnemonic)
    {
        uint8_t inst, addr, bus;

        // Special case NOP
        if(opcode == 0x02  &&  operand == 0x00)
        {
            strcpy(mnemonic, _nativeOpcodes[opcode]._mnemonic.c_str());
            return true;
        }

        inst = opcode & 0xE0;
        addr = opcode & 0x1C;
        bus  = opcode & 0x03;

        bool store = (inst == 0xC0);
        bool jump = (inst == 0xE0);

        if(_nativeOpcodes.find(inst) == _nativeOpcodes.end()) return false;

        // Opcode mnemonic, jump = 0xE0 + (condition codes)
        char instStr[8];
        (!jump) ? strcpy(instStr, _nativeOpcodes[inst]._mnemonic.c_str()) : strcpy(instStr, _nativeOpcodes[0xE0 + addr]._mnemonic.c_str());

        // Effective address string
        char addrStr[12];
        char regStr[4];
        if(!jump)
        {
            switch(addr)
            {
                case EA_0D_AC:    sprintf(addrStr, "[$%02x]",   operand); sprintf(regStr, "AC");  break;
                case EA_0X_AC:    sprintf(addrStr, "[X]");                sprintf(regStr, "AC");  break;
                case EA_YD_AC:    sprintf(addrStr, "[Y,$%02x]", operand); sprintf(regStr, "AC");  break;
                case EA_YX_AC:    sprintf(addrStr, "[Y,X]");              sprintf(regStr, "AC");  break;
                case EA_0D_X:     sprintf(addrStr, "[$%02x]",   operand); sprintf(regStr, "X");   break;
                case EA_0D_Y:     sprintf(addrStr, "[$%02x]",   operand); sprintf(regStr, "Y");   break;
                case EA_0D_OUT:   sprintf(addrStr, "[$%02x]",   operand); sprintf(regStr, "OUT"); break;
                case EA_YX_OUTIX: sprintf(addrStr, "[Y,X++]");            sprintf(regStr, "OUT"); break;

                default: break;
            }
        }
        else
        {
            sprintf(addrStr, "[$%02x]", operand);
        }

        // Bus string
        char busStr[8];
        switch(bus)
        {
            case BUS_D:   sprintf(busStr, "$%02x", operand);                       break;
            case BUS_RAM: (!store) ? strcpy(busStr, addrStr) : strcpy(busStr, ""); break;
            case BUS_AC:  strcpy(busStr, "AC");                                    break;
            case BUS_IN:  strcpy(busStr, "IN");                                    break;

            default: break;
        }
        
        // Compose opcode string
        if(!jump)
        {
            if(store)
            {
                char storeStr[32];
                (bus == BUS_AC) ? sprintf(storeStr, "%-4s %s", instStr, addrStr) : sprintf(storeStr, "%-4s %s,%s", instStr, busStr, addrStr);
                if(bus == BUS_RAM) sprintf(storeStr, "CTRL %s", removeBrackets(addrStr).c_str());
                if(addr == EA_0D_X  ||  addr == EA_0D_Y) sprintf(mnemonic, "%s,%s", storeStr, regStr);
                else strcpy(mnemonic, storeStr);
            }
            else
            {
                // if reg == AC
                (addr <= EA_YX_AC) ? sprintf(mnemonic, "%-4s %s", instStr, busStr) : sprintf(mnemonic, "%-4s %s,%s", instStr, busStr, regStr);
            }
        }
        // Compose jump string
        else
        {
            char jumpStr[32];
            switch(addr)
            {
                case BRA_CC_FAR: sprintf(jumpStr, "%-4s Y,", instStr); break;
                default:         sprintf(jumpStr, "%-4s ",   instStr); break;
            }

            sprintf(mnemonic, "%-4s%s", jumpStr, busStr);
        }

        return true;
    }

    bool getVCpuMnemonic(uint16_t address, uint8_t opcode, uint8_t operand0, uint8_t operand1, uint8_t operand2, uint8_t operand3, ByteSize& byteSize, char* mnemonic)
    {
        // PREFX opcodes
        if(opcode == OPCODE_V_PREFX1  ||  opcode == OPCODE_V_PREFX2  ||  opcode == OPCODE_V_PREFX3)
        {
            uint16_t opc = opcode*256 + operand0;
            if(_vcpuOpcodes.find(opc) == _vcpuOpcodes.end())
            {
                sprintf(mnemonic, "%04x #%02x", address, opcode);
                return false;
            }

            byteSize = _vcpuOpcodes[opc]._byteSize;

            if((opcode == OPCODE_V_PREFX3)  &&  (operand0 == OPCODE_V_XCHGB   ||  operand0 == OPCODE_V_MOVW    ||  operand0 == OPCODE_V_CMPII   ||  operand0 == OPCODE_V_CONDII  ||
                                                 operand0 == OPCODE_V_CONDBB  ||  operand0 == OPCODE_V_CONDIB  ||  operand0 == OPCODE_V_CONDBI  ||  operand0 == OPCODE_V_XCHGW   ||
                                                 operand0 == OPCODE_V_OSCPX   ||  operand0 == OPCODE_V_SWAPB   ||  operand0 == OPCODE_V_SWAPW   ||  operand0 == OPCODE_V_NEEKA   ||
                                                 operand0 == OPCODE_V_NOKEA   ||  operand0 == OPCODE_V_ADDVL   ||  operand0 == OPCODE_V_SUBVL   ||  operand0 == OPCODE_V_ANDVL   ||
                                                 operand0 == OPCODE_V_ORVL    ||  operand0 == OPCODE_V_XORVL   ||  operand0 == OPCODE_V_ANDBI   ||  operand0 == OPCODE_V_ORBI    ||
                                                 operand0 == OPCODE_V_XORBI   ||  operand0 == OPCODE_V_ANDBK   ||  operand0 == OPCODE_V_ORBK    ||  operand0 == OPCODE_V_XORBK   ||
                                                 operand0 == OPCODE_V_VADDBW  ||  operand0 == OPCODE_V_VSUBBW  ||  operand0 == OPCODE_V_VADDBL  ||  operand0 == OPCODE_V_VSUBBL))
            {
                sprintf(mnemonic, "%04x %-6s $%02x %02x", address, _vcpuOpcodes[opc]._mnemonic.c_str(), operand1, operand2);
            }
            else if((opcode == OPCODE_V_PREFX3)  &&  (operand0 == OPCODE_V_FNT6X8  ||  operand0 == OPCODE_V_FNT4X6))
            {
                sprintf(mnemonic, "%04x %-6s $%02x $%02x", address, _vcpuOpcodes[opc]._mnemonic.c_str(), operand2, operand1);
            }
            // JEQL JNEL JLTL JGTL JLEL JGEL opcodes
            else if((opcode == OPCODE_V_PREFX3)  &&  (operand0 == OPCODE_V_JEQL  ||  operand0 == OPCODE_V_JNEL  ||  operand0 == OPCODE_V_JLTL  ||  operand0 == OPCODE_V_JGTL  ||  operand0 == OPCODE_V_JLEL  ||  operand0 == OPCODE_V_JGEL))
            {
                sprintf(mnemonic, "%04x %-6s $%02x%02x", address, _vcpuOpcodes[opc]._mnemonic.c_str(), operand1, uint8_t(operand2 + BRANCH_ADJUSTMENT));
            }
            // Valid opcode 
            else
            {
                switch(byteSize)
                {
                    case TwoBytes:   sprintf(mnemonic, "%04x %-6s", address, _vcpuOpcodes[opc]._mnemonic.c_str());                               break;
                    case ThreeBytes: sprintf(mnemonic, "%04x %-6s $%02x", address, _vcpuOpcodes[opc]._mnemonic.c_str(), operand1);               break;
                    case FourBytes:  sprintf(mnemonic, "%04x %-6s $%02x%02x", address, _vcpuOpcodes[opc]._mnemonic.c_str(), operand2, operand1); break;

                    default: break;
                }
            }

            return true;
        }

        // Invalid opcode or invalid address space
        if((opcode != OPCODE_V_BCC  &&  _vcpuOpcodes.find(opcode) == _vcpuOpcodes.end())  ||
            (address >= GIGA_CH0_WAV_A  &&  address <= GIGA_CH0_OSC_H) ||  (address >= GIGA_CH1_WAV_A  &&  address <= GIGA_CH1_OSC_H)  ||
            (address >= GIGA_CH2_WAV_A  &&  address <= GIGA_CH2_OSC_H) ||  (address >= GIGA_CH3_WAV_A  &&  address <= GIGA_CH3_OSC_H))
        {
            sprintf(mnemonic, "%04x #%02x", address, opcode);
            return false;
        }

        // Branch opcodes
        uint8_t branchAdjustment = (opcode == OPCODE_V_BRA) ? BRANCH_ADJUSTMENT : 0;
        if(opcode == OPCODE_V_BCC)
        {
            opcode = operand0;
            if(_vbraOpcodes.find(opcode) == _vbraOpcodes.end())
            {
                sprintf(mnemonic, "%04x #%02x", address, opcode);
                return false;
            }
            byteSize = _vbraOpcodes[opcode]._byteSize;
            sprintf(mnemonic, "%04x %-6s $%02x", address, _vbraOpcodes[opcode]._mnemonic.c_str(), uint8_t(operand1 + BRANCH_ADJUSTMENT));
            return true;
        }
        // Normal opcodes
        else
        {
            byteSize = _vcpuOpcodes[opcode]._byteSize;
        }

        // Page3 MOVQB MOVB MOVQW MOVWA CMPI opcodes
        if(opcode == OPCODE_V_MOVQB  ||  opcode == OPCODE_V_MOVB   ||  opcode == OPCODE_V_MOVQW  ||  opcode == OPCODE_V_MOVWA  ||  opcode == OPCODE_V_CMPI   ||  opcode == OPCODE_V_PACKAW)
        {
            sprintf(mnemonic, "%04x %-6s $%02x $%02x", address, _vcpuOpcodes[opcode]._mnemonic.c_str(), operand1, operand0);
        }
        // JEQ JNE JLT JGT JLE JGE opcodes
        else if(opcode == OPCODE_V_JEQ  ||  opcode == OPCODE_V_JNE  ||  opcode == OPCODE_V_JLT  ||  opcode == OPCODE_V_JGT  ||  opcode == OPCODE_V_JLE  ||  opcode == OPCODE_V_JGE)
        {
            sprintf(mnemonic, "%04x %-6s $%02x%02x", address, _vcpuOpcodes[opcode]._mnemonic.c_str(), operand1, uint8_t(operand0 + BRANCH_ADJUSTMENT));
        }
        // DJNE DJGE opcodes
        else if(opcode == OPCODE_V_DJNE  ||  opcode == OPCODE_V_DJGE)
        {
            sprintf(mnemonic, "%04x %-6s $%02x $%02x%02x", address, _vcpuOpcodes[opcode]._mnemonic.c_str(), operand0, operand2, uint8_t(operand1 + BRANCH_ADJUSTMENT));
        }
        // DBNE DBGE opcodes
        else if(opcode == OPCODE_V_DBNE  ||  opcode == OPCODE_V_DBGE)
        {
            sprintf(mnemonic, "%04x %-6s $%02x $%02x", address, _vcpuOpcodes[opcode]._mnemonic.c_str(), operand1, uint8_t(operand0 + BRANCH_ADJUSTMENT));
        }
        // DOKEI ARRW opcodes
        else if(opcode == OPCODE_V_DOKEI  ||  opcode == OPCODE_V_ARRW)
        {
            sprintf(mnemonic, "%04x %-6s $%02x%02x", address, _vcpuOpcodes[opcode]._mnemonic.c_str(), operand0, operand1);
        }
        // LDWM STWM opcodes
        else if(opcode == OPCODE_V_LDWM  ||  opcode == OPCODE_V_STWM)
        {
            sprintf(mnemonic, "%04x %-6s $%02x%02x", address, _vcpuOpcodes[opcode]._mnemonic.c_str(), operand1, operand0);
        }
        // ARRVW LDARRB STARRB LDARRW STARRW opcodes
        else if(opcode == OPCODE_V_ARRVW  ||  opcode == OPCODE_V_LDARRB  ||  opcode == OPCODE_V_STARRB  ||  opcode == OPCODE_V_LDARRW  ||  opcode == OPCODE_V_STARRW)
        {
            sprintf(mnemonic, "%04x %-6s $%02x $%02x%02x", address, _vcpuOpcodes[opcode]._mnemonic.c_str(), operand0, operand2, operand1);
        }
        // ADDVW SUBWV ADDVI SUBVI ADDVB SUBVB PACKVW CNVXY opcodes
        else if(opcode == OPCODE_V_ADDVW  ||  opcode == OPCODE_V_SUBVW  ||  opcode == OPCODE_V_ADDVI   ||  opcode == OPCODE_V_SUBVI  ||
                opcode == OPCODE_V_ADDVB  ||  opcode == OPCODE_V_SUBVB  ||  opcode == OPCODE_V_PACKVW  ||  opcode == OPCODE_V_CNVXY)
        {
            sprintf(mnemonic, "%04x %-6s $%02x $%02x $%02x", address, _vcpuOpcodes[opcode]._mnemonic.c_str(), operand2, operand1, operand0);
        }
        // ADDBI
        else if(opcode == OPCODE_V_ADDBI)
        {
            sprintf(mnemonic, "%04x %-6s $%02x $%02x $%02x", address, _vcpuOpcodes[opcode]._mnemonic.c_str(), operand0, operand1, operand2);
        }
        // SUBBI
        else if(opcode == OPCODE_V_SUBBI)
        {
            sprintf(mnemonic, "%04x %-6s $%02x $%02x $%02x", address, _vcpuOpcodes[opcode]._mnemonic.c_str(), operand2, operand1, operand0);
        }
        // STARRI opcode
        else if(opcode == OPCODE_V_STARRI)
        {
            sprintf(mnemonic, "%04x %-6s $%02x $%02x%02x $%02x", address, _vcpuOpcodes[opcode]._mnemonic.c_str(), operand0, operand2, operand1, operand3);
        }
        // HALT opcode
        else if(opcode == OPCODE_V_HALT  &&  operand0 == OPERAND_V_HALT)
        {
            sprintf(mnemonic, "%04x %-6s", address, "HALT");
        }
        // Valid opcode
        else
        {
            switch(byteSize)
            {
                case OneByte:    sprintf(mnemonic, "%04x %-6s", address, _vcpuOpcodes[opcode]._mnemonic.c_str());                                             break;
                case TwoBytes:   sprintf(mnemonic, "%04x %-6s $%02x", address, _vcpuOpcodes[opcode]._mnemonic.c_str(), uint8_t(operand0 + branchAdjustment)); break;
                case ThreeBytes: sprintf(mnemonic, "%04x %-6s $%02x%02x", address, _vcpuOpcodes[opcode]._mnemonic.c_str(), operand1, operand0);               break;

                default: return false;
            }
        }

        return true;
    }

    int disassemble(uint16_t address)
    {
        _disassembledCode.clear();

        _currDasmByteCount = 1;
        _prevDasmByteCount = 1;

        while(_disassembledCode.size() < MAX_DASM_LINES)
        {
            char dasmText[32];
            DasmCode dasmCode;
            ByteSize byteSize = OneByte;
            uint8_t opcode = 0, operand0 = 0, operand1 = 0, operand2 = 0, operand3 = 0;

            switch(Editor::getCpuMode())
            {
                // Native opcode
                case Editor::Native:
                {
                    opcode = Cpu::getROM(address, 0);
                    operand0 = Cpu::getROM(address, 1);
                    operand1 = 0;

                    char mnemonic[32];
                    if(!getNativeMnemonic(opcode, operand0, mnemonic))
                    {
                        sprintf(dasmText, "%04x  $%02x $%02x", address, opcode, operand0);
                        dasmCode._address = address;
                        address++;
                        break;
                    }

                    sprintf(dasmText, "%04x  %s", address, mnemonic);
                    dasmCode._address = address;
                    address++;
                }
                break;

                // vCPU opcode
                case Editor::vCpu:
                {
                    char mnemonic[32];
                    opcode = Cpu::getRAM(address);
                    operand0 = Cpu::getRAM(address + 1);
                    operand1 = Cpu::getRAM(address + 2);
                    operand2 = Cpu::getRAM(address + 3);
                    operand3 = Cpu::getRAM(address + 4);
                    dasmCode._address = address;

                    // PREFIX ARG1 OPCODE ARG0
                    if(opcode == OPCODE_V_PREFX3)
                    {
                        if(getVCpuMnemonic(address, opcode, operand1, operand2, operand0, operand3, byteSize, mnemonic))
                        {
                            // Save current and previous opcode sizes to allow scrolling
                            getDasmCurrAndPrevByteSize(dasmCode._address, byteSize);
                            address = uint16_t(address + byteSize);
                        }
                        else
                        {
                            address++;
                        }
                    }
                    // PREFIX ARG0 OPCODE
                    else if(opcode == OPCODE_V_PREFX2)
                    {
                        if(getVCpuMnemonic(address, opcode, operand1, operand0, operand2, operand3, byteSize, mnemonic))
                        {
                            // Save current and previous opcode sizes to allow scrolling
                            getDasmCurrAndPrevByteSize(dasmCode._address, byteSize);
                            address = uint16_t(address + byteSize);
                        }
                        else
                        {
                            address++;
                        }
                    }
                    else
                    {
                        if(getVCpuMnemonic(address, opcode, operand0, operand1, operand2, operand3, byteSize, mnemonic))
                        {
                            // Save current and previous opcode sizes to allow scrolling
                            getDasmCurrAndPrevByteSize(dasmCode._address, byteSize);
                            address = uint16_t(address + byteSize);
                        }
                        else
                        {
                            address++;
                        }
                    }

                    address &= (Memory::getSizeRAM() - 1);
                    strcpy(dasmText, mnemonic);
                }
                break;
            
                default: break;
            }

            std::string dasmCodeText = std::string(dasmText);
            dasmCode._opcode = opcode;
            dasmCode._byteSize = uint8_t(byteSize);
            dasmCode._operand0 = operand0;
            dasmCode._operand1 = operand1;
            dasmCode._operand2 = operand2;
            switch(Editor::getCpuMode())
            {
                case Editor::vCpu:   dasmCode._text = Expression::strToUpper(dasmCodeText); break;
                case Editor::Native: dasmCode._text = Expression::strToLower(dasmCodeText); break;

                default: break;
            }

            _disassembledCode.push_back(dasmCode);
        }

        // Save current and previous page instruction sizes to allow page scrolling
        switch(Editor::getCpuMode())
        {
            case Editor::vCpu: getDasmCurrAndPrevPageByteSize(MAX_DASM_LINES); break;

            case Editor::Native:
            {
                _currDasmPageByteCount = MAX_DASM_LINES;
                _prevDasmPageByteCount = MAX_DASM_LINES;
            }
            break;

            default: break;
        }

        return int(_disassembledCode.size());
    }
#endif

    // Returns true when finished
    bool getNextAssembledByte(ByteCode& byteCode, bool debug)
    {
        static bool isUserCode = false;

        if(_byteCount >= _byteCode.size())
        {
            _byteCount = 0;
            if(debug  &&  isUserCode) Cpu::reportError(Cpu::NoError, stderr, "\n");
            return true;
        }

        static uint16_t address = 0x0000;
        static uint16_t customAddress = 0x0000;

        // Get next byte
        if(_byteCount == 0) address = _startAddress;
        byteCode = _byteCode[_byteCount++];

        // New section
        if(byteCode._isCustomAddress)
        {
            address = byteCode._address;
            customAddress = byteCode._address;
        }

        // User code is RAM code or ROM code in user ROM space
        isUserCode = !byteCode._isRomAddress  ||  (byteCode._isRomAddress  &&  customAddress >= USER_ROMv1_ADDRESS);

        // Seperate sections
        if(debug  &&  byteCode._isCustomAddress  &&  isUserCode) Cpu::reportError(Cpu::NoError, stderr, "\n");

        // 16bit for ROM, 8bit for RAM
        if(debug  &&  isUserCode)
        {
            if(byteCode._isRomAddress)
            {
                if((address & 0x0001) == 0x0000)
                {
                    Cpu::reportError(Cpu::NoError, stderr, "Assembler::getNextAssembledByte() : ROM : %04X  %02X", customAddress + (LO_BYTE(address)>>1), byteCode._data);
                }
                else
                {
                    Cpu::reportError(Cpu::NoError, stderr, "%02X\n", byteCode._data);
                }
            }
            else
            {
                Cpu::reportError(Cpu::NoError, stderr, "Assembler::getNextAssembledByte() : RAM : %04X  %02X\n", address, byteCode._data);
            }
        }

        address++;

        return false;
    }    

    bool parseDefineOffset(const std::string& token, uint16_t& offset, size_t& lbra)
    {
        size_t rbra;
        if(Expression::findMatchingBrackets(token, 0, lbra, rbra))
        {
            Expression::Numeric value;
            if(Expression::parse(token.substr(lbra + 1, rbra - (lbra + 1)), _lineNumber, value))
            {
                offset = uint16_t(std::lround(value._value));
                return true;
            }
        }

        return false;
    }

    InstructionType getOpcode(const std::string& opcodeStr, uint16_t& offset)
    {
        offset = 0;
        std::string opcode = opcodeStr;
        Expression::strToUpper(opcode); 
        if(_asmOpcodes.find(opcode) != _asmOpcodes.end())
        {
            return _asmOpcodes[opcode];
        }

        size_t lbra;
        if(parseDefineOffset(opcode, offset, lbra))
        {
            if(_asmOpcodes.find(opcode.substr(0, lbra)) != _asmOpcodes.end())
            {
                return _asmOpcodes[opcode.substr(0, lbra)];
            }
        }

        return {0x00, 0x00, BadByteSize, vCpu};
    }

    int preProcessExpression(const std::vector<std::string>& tokens, int tokenIndex, std::string& input, bool stripWhiteSpace, bool isNative=false)
    {
        int commaTokenIndex = -1;

        input.clear();

        // Pre-process
        for(int j=tokenIndex; j<int(tokens.size()); j++)
        {
            // Strip comments
            if(tokens[j].find_first_of(";#") != std::string::npos) break;

            // Comma seperated for instructions with compound equates, (e.g. DBNE, etc)
            if(!isNative  &&  tokens[j].find(',') != std::string::npos)
            {
                commaTokenIndex = j;
                std::string equate = tokens[j];
                Expression::stripChars(equate, ",");
                input += equate;
                break;
            }

            // Concatenate
            input += tokens[j];
        }

        // Strip white space
        if(stripWhiteSpace) Expression::stripWhitespace(input);

        return commaTokenIndex;
    }

    size_t findSymbol(const std::string& input, const std::string& symbol, size_t pos=0)
    {
        const size_t len = input.length();
        if(pos >= len) return std::string::npos;

        const std::string separators = "&|^+-*/().,!?;#'[]<> \t\n\r";
        const std::vector<std::string> operators = {"**", ">>", "<<", "==", "!=", "<=", ">="};

        for(;;)
        {
            size_t sep = input.find_first_of(separators, pos);
            bool eos = (sep == std::string::npos);
            if(eos)
            {
                for(int i=0; i<int(operators.size()); i++)
                {
                    sep = input.find(operators[i]);
                    eos = (sep == std::string::npos);
                    if(eos) break;
                }
            }

            size_t end = eos ? len : sep;
            if(input.substr(pos, end-pos) == symbol)
            {
                break;
            }
            else if(eos)
            {
                pos = std::string::npos;
                break;
            }

            pos = sep + 1;
        }

        return pos;
    }

    bool applyEquatesToExpression(std::string& expression, const std::vector<Equate>& equates)
    {
        bool modified = false;
        for(int i=0; i<int(equates.size()); i++)
        {
            for(;;)
            {
                size_t pos = findSymbol(expression, equates[i]._name);
                if(pos == std::string::npos) break;  // not found
                modified = true;
                expression.replace(pos, equates[i]._name.size(), std::to_string(equates[i]._operand));
            }
        }

        return modified;
    }

    bool applyLabelsToExpression(std::string& expression, const std::vector<Label>& labels, bool nativeCode)
    {
        bool modified = false;
        for(int i=0; i<int(labels.size()); i++)
        {
            for(;;)
            {
                size_t pos = findSymbol(expression, labels[i]._name);
                if (pos == std::string::npos) break;  // not found
                modified = true;
                uint16_t address = (nativeCode) ? labels[i]._address >>1 : labels[i]._address;
                expression.replace(pos, labels[i]._name.size(), std::to_string(address));
            }
        }

        return modified;
    }

    bool evaluateExpression(std::string input, bool nativeCode, int32_t& result)
    { 
        // Replace equates
        applyEquatesToExpression(input, _equates);

        // Replace labels
        applyLabelsToExpression(input, _labels, nativeCode);

        // Strip white space
        input.erase(remove_if(input.begin(), input.end(), isspace), input.end());

        // Parse expression and return with a result
        Expression::Numeric numeric;
        bool valid = Expression::parse(input, _lineNumber, numeric);
        result = int32_t(std::lround(numeric._value));
        return valid;
    }

    bool searchEquate(const std::string& token, Equate& equate)
    {
        bool success = false;
        for(int i=0; i<int(_equates.size()); i++)
        {
            if(_equates[i]._name == token)
            {
                equate = _equates[i];
                success = true;
                break;
            }
        }

        return success;
    }

    bool evaluateEquateOperand(const std::string& token, Equate& equate)
    {
        // Expression equates
        Expression::ExpressionType expressionType = Expression::isExpression(token);
        if(expressionType == Expression::IsInvalid) return false;
        if(expressionType == Expression::HasOperators)
        {
            int32_t value;
            if(!evaluateExpression(token, false, value)) return false;
            equate._operand = value;
            return true;
        }

        // Check for existing equate
        return searchEquate(token, equate);
    }

    bool evaluateEquateOperand(const std::vector<std::string>& tokens, int tokenIndex, Equate& equate, int& commaTokenIndex, bool compoundInstruction)
    {
        commaTokenIndex = -1;

        if(tokenIndex >= int(tokens.size())) return false;

        // Expression equates
        std::string token;
        if(compoundInstruction)
        {
            token = tokens[tokenIndex];
        }
        else
        {
            commaTokenIndex = preProcessExpression(tokens, tokenIndex, token, false);
        }

        return evaluateEquateOperand(token, equate);
    }

    EvaluateResult evaluateEquates(const std::vector<std::string>& tokens, ParseType parse)
    {
        std::string token0 = Expression::strUpper(tokens[0]);
        std::string token1 = Expression::strUpper(tokens[1]);
        if(token1 == "EQU")
        {
            if(parse == MnemonicPass)
            {
                int commaTokenIndex = -1;
                Equate equate = {false, 0x0000, tokens[0]};
                if(!Expression::stringToU32(tokens[2], equate._operand))
                {
                    if(!evaluateEquateOperand(tokens, 2, equate, commaTokenIndex, false)) return NotFound;
                }

                // Reserved word, (equate), _callTable_
                if(token0 == "_CALLTABLE_")
                {
                    _callTablePtr = uint16_t(equate._operand);
                }
                // Reserved word, (equate), _startAddress_
                else if(token0 == "_STARTADDRESS_")
                {
                    _startAddress = uint16_t(equate._operand);
                    _currentAddress = _startAddress;
                }
#ifndef STAND_ALONE
                // Disable upload of the current assembler module
                else if(token0 == "_DISABLEUPLOAD_")
                {
                    Loader::disableUploads(equate._operand != 0);
                }
                // Reserved word, (equate), _singleStepWatch_
                else if(token0 == "_SINGLESTEPWATCH_")
                {
                    Debugger::setWatchAddress(uint16_t(equate._operand));
                }
                // Start address of vCPU exclusion zone
                else if(token0 == "_CPUUSAGEADDRESSA_")
                {
                    Editor::setCpuUsageAddressA(uint16_t(equate._operand));
                }
                // End address of vCPU exclusion zone
                else if(token0 == "_CPUUSAGEADDRESSB_")
                {
                    Editor::setCpuUsageAddressB(uint16_t(equate._operand));
                }
#endif
                // Standard equates
                else
                {
                    // Check for duplicate
                    equate._name = tokens[0];
                    if(searchEquate(tokens[0], equate)) return Duplicate;

                    _equates.push_back(equate);
                }
            }
            else if(parse == CodePass)
            {
            }

            return Success;
        }

        return Failed;
    }

    bool searchLabel(const std::string& token, Label& label)
    {
        bool success = false;
        for(int i=0; i<int(_labels.size()); i++)
        {
            if(token == _labels[i]._name)
            {
                success = true;
                label = _labels[i];
                break;
            }
        }

        return success;
    }

    bool evaluateLabelOperand(const std::string& token, Label& label)
    {
        // Expression labels
        Expression::ExpressionType expressionType = Expression::isExpression(token);
        if(expressionType == Expression::IsInvalid) return false;
        if(expressionType == Expression::HasOperators)
        {
            int32_t value;
            if(!evaluateExpression(token, false, value)) return false;
            label._address = uint16_t(value);
            return true;
        }

        // Check for existing label
        return searchLabel(token, label);
    }

    bool evaluateLabelOperand(const std::vector<std::string>& tokens, int tokenIndex, Label& label, bool compoundInstruction)
    {
        if(tokenIndex >= int(tokens.size())) return false;

        // Expression labels
        std::string token;
        if(compoundInstruction)
        {
            token = tokens[tokenIndex];
        }
        else
        {
            preProcessExpression(tokens, tokenIndex, token, false);
        }

        return evaluateLabelOperand(token, label);
    }

    EvaluateResult EvaluateLabels(const std::vector<std::string>& tokens, ParseType parse, int tokenIndex)
    {
        if(parse == MnemonicPass) 
        {
            // Check reserved words
            for(int i=0; i<int(_reservedWords.size()); i++)
            {
                if(tokens[tokenIndex] == _reservedWords[i]) return Reserved;
            }
            
            Label label;
            if(searchLabel(tokens[tokenIndex], label)) return Duplicate;

            // Check equates for a custom start address
            for(int i=0; i<int(_equates.size()); i++)
            {
                if(_equates[i]._name == tokens[tokenIndex])
                {
                    _equates[i]._isCustomAddress = true;
                    _currentAddress = uint16_t(_equates[i]._operand);
                    break;
                }
            }

            // Normal labels
            label = {_currentAddress, tokens[tokenIndex]};
            _labels.push_back(label);
        }
        else if(parse == CodePass)
        {
        }

        return Success;
    }

    bool handleDefineByte(std::vector<std::string>& tokens, int tokenIndex, const Instruction& instruction, bool createInstruction, uint16_t defineOffset, int& dbSize)
    {
        bool success = false;

        // Handle case where first operand is a string
        size_t quote1 = tokens[tokenIndex].find_first_of("'");
        size_t quote2 = tokens[tokenIndex].find_last_of("'");
        bool quotes = (quote1 != std::string::npos  &&  quote2 != std::string::npos  &&  (quote2 - quote1 > 1));
        if(quotes)
        {
            // Remove escape sequence
            std::string token = tokens[tokenIndex].substr(quote1+1, quote2 - (quote1+1));
            Expression::stripChars(token, "\\");
            if(createInstruction)
            {
                for(int j=1; j<int(token.size()); j++) // First instruction was created by callee
                {
                    Instruction inst = {instruction._isRomAddress, false, OneByte, uint8_t(token[j]), 0x00, 0x00, 0x00, 0x00, 0x0000, instruction._opcodeType};
                    _instructions.push_back(inst);
                }
            }
            dbSize += int(token.size()) - 1; // First instruction was created by callee
            success = true;
        }
       
        for(int i=tokenIndex+1; i<int(tokens.size()); i++)
        {
            // Handle all other variations of strings
            quote1 = tokens[i].find_first_of("'");
            quote2 = tokens[i].find_last_of("'");
            quotes = (quote1 != std::string::npos  &&  quote2 != std::string::npos);
            if(quotes)
            {
                // Remove escape sequence
                std::string token = tokens[i].substr(quote1+1, quote2 - (quote1+1));
                Expression::stripChars(token, "\\");
                if(createInstruction)
                {
                    for(int j=0; j<int(token.size()); j++)
                    {
                        Instruction inst = {instruction._isRomAddress, false, OneByte, uint8_t(token[j]), 0x00, 0x00, 0x00, 0x00, 0x0000, instruction._opcodeType};
                        _instructions.push_back(inst);
                    }
                }
                dbSize += int(token.size());
                success = true;
            }
            // Non string tokens
            else
            {
                // Strip comments
                if(tokens[i].find_first_of(";#") != std::string::npos) break;

                uint8_t operand;
                success = Expression::stringToU8(tokens[i], operand);
                if(!success)
                {
                    // Search equates
                    Equate equate;
                    Label label;
                    if((success = evaluateEquateOperand(tokens[i], equate)) == true)
                    {
                        operand = uint8_t(equate._operand);
                    }
                    // Search labels
                    else if((success = evaluateLabelOperand(tokens[i], label)) == true)
                    {
                        operand = uint8_t(label._address);
                    }
                    else
                    {
                        // Normal expression
                        if(Expression::isExpression(tokens[i]) == Expression::HasOperators)
                        {
                            Expression::Numeric value;
                            if(Expression::parse(tokens[i], _lineNumber, value))
                            {
                                operand = uint8_t(std::lround(value._value));
                                success = true;
                            }
                        }
                        else
                        {
                            break;
                        }
                    }
                }

                if(createInstruction)
                {
                    bool hasDefineOffset = (defineOffset != 0);
                    uint16_t address = (hasDefineOffset) ? _currentAddress : 0x0000;
                    Instruction inst = {instruction._isRomAddress, hasDefineOffset, OneByte, operand, 0x00, 0x00, 0x00, 0x00, address, instruction._opcodeType};
                    _currentAddress += defineOffset;
                    _instructions.push_back(inst);
                }
                dbSize++;
            }
        }

        return success;
    }


    bool handleDefineWord(const std::vector<std::string>& tokens, int tokenIndex, const Instruction& instruction, bool createInstruction, uint16_t defineOffset, int& dwSize)
    {
        bool success = false;

        for(int i=tokenIndex+1; i<int(tokens.size()); i++)
        {
            // Strip comments
            if(tokens[i].find_first_of(";#") != std::string::npos)
            {
                success = true;
                break;
            }

            uint16_t operand;
            success = Expression::stringToU16(tokens[i], operand);
            if(!success)
            {
                // Search equates
                Equate equate;
                Label label;
                if((success = evaluateEquateOperand(tokens[i], equate)) == true)
                {
                    operand = uint16_t(equate._operand);
                }
                // Search labels
                else if((success = evaluateLabelOperand(tokens[i], label)) == true)
                {
                    operand = label._address;
                }
                else
                {
                    // Normal expression
                    if(Expression::isExpression(tokens[i]) == Expression::HasOperators)
                    {
                        Expression::Numeric value;
                        if(Expression::parse(tokens[i], _lineNumber, value))
                        {
                            operand = int16_t(std::lround(value._value));
                            success = true;
                        }
                    }
                    else
                    {
                        break;
                    }
                }
            }

            if(createInstruction)
            {
                bool hasDefineOffset = (defineOffset != 0);
                uint16_t address = (hasDefineOffset) ? _currentAddress : 0x0000;
                Instruction inst = {instruction._isRomAddress, hasDefineOffset, TwoBytes, uint8_t(LO_BYTE(operand)), uint8_t(HI_BYTE(operand)), 0x00, 0x00, 0x00, address, instruction._opcodeType};
                _currentAddress += defineOffset * 2;
                _instructions.push_back(inst);
            }
            dwSize += 2;
        }

        return success;
    }

    bool handleNativeOperand(const std::string& token, uint8_t& operand)
    {
        Expression::ExpressionType expressionType = Expression::isExpression(token);
        if(expressionType == Expression::IsInvalid) return false;
        if(expressionType == Expression::HasOperators)
        {
            // Parse expression and return with a result
            int32_t value;
            if(!evaluateExpression(token, true, value)) return false;
            operand = uint8_t(value);
            return true;
        }

        Label label;
        if(searchLabel(token, label))
        {
            operand = uint8_t(LO_BYTE(label._address) >>1);
            return true;
        }

        Equate equate;
        if(searchEquate(token, equate))
        {
            operand = uint8_t(equate._operand);
            return true;
        }

        return Expression::stringToU8(token, operand);
    }

    bool handleNativeInstruction(const std::vector<std::string>& tokens, int tokenIndex, uint8_t& opcode, uint8_t& operand)
    {
        std::string input, token;

        preProcessExpression(tokens, tokenIndex, input, true, true);
        size_t openBracket = input.find_first_of("[");
        size_t closeBracket = input.find_first_of("]");
        bool noBrackets = (openBracket == std::string::npos  &&  closeBracket == std::string::npos);
        bool validBrackets = (openBracket != std::string::npos  &&  closeBracket != std::string::npos  &&  closeBracket > openBracket);

        size_t comma1 = input.find_first_of(",");
        size_t comma2 = input.find_first_of(",", comma1+1);
        bool noCommas = (comma1 == std::string::npos  &&  comma2 == std::string::npos);
        bool oneComma = (comma1 != std::string::npos  &&  comma2 == std::string::npos);
        bool twoCommas = (comma1 != std::string::npos  &&  comma2 != std::string::npos);

        operand = 0x00;

        // NOP
        if(opcode == 0x02) return true;

        // Accumulator
        if(input == "AC"  ||  input == "ac")
        {
            opcode |= BusMode::AC;
            return true;
        }

        // Jump
        if(opcode == 0xE0)
        {
            // y,[D]
            if(validBrackets  &&  oneComma  &&  comma1 < openBracket)
            {
                opcode |= BusMode::RAM;
                token = input.substr(openBracket+1, closeBracket - (openBracket+1));
                return handleNativeOperand(token, operand);
            }

            // y,D
            if(noBrackets  &&  oneComma)
            {
                token = input.substr(comma1+1, input.size() - (comma1+1));
                return handleNativeOperand(token, operand);
            }
        
            return false;                    
        }

        // Branch
        if(opcode >= 0xE4)
        {
            token = input;
            if(validBrackets) {opcode |= BusMode::RAM; token = input.substr(openBracket+1, closeBracket - (openBracket+1));}
            if(Expression::stringToU8(token, operand)) return true;
            return handleNativeOperand(token, operand);
        }

        // IN or IN,[D]
        if(input.find("IN") != std::string::npos  ||  input.find("in") != std::string::npos)
        {
            opcode |= BusMode::IN;

            // IN,[D]
            if(validBrackets &&  oneComma  &&  comma1 < openBracket)
            {
                token = input.substr(openBracket+1, closeBracket - (openBracket+1));
                return handleNativeOperand(token, operand);
            }
            
            // IN
            return true;
        }

        // D
        if(noBrackets && noCommas) return handleNativeOperand(input, operand);

        // Read or write
        (opcode != 0xC0) ? opcode |= BusMode::RAM : opcode |= BusMode::AC;

        // [D] or [X]
        if(validBrackets  &&  noCommas)
        {
            token = input.substr(openBracket+1, closeBracket - (openBracket+1));
            if(token == "X"  ||  token == "x") {opcode |= AddressMode::X_AC; return true;}
            return handleNativeOperand(token, operand);
        }

        // AC,X or AC,Y or AC,OUT or D,X or D,Y or D,OUT or [D],X or [D],Y or [D],OUT or D,[D] or D,[X] or D,[Y] or [Y,D] or [Y,X] or [Y,X++]
        if(oneComma)
        {
            token = input.substr(comma1+1, input.size() - (comma1+1));
            if(token == "X"    ||  token == "x")   opcode |= AddressMode::D_X;
            if(token == "Y"    ||  token == "y")   opcode |= AddressMode::D_Y;
            if(token == "OUT"  ||  token == "out") opcode |= AddressMode::D_OUT;

            token = input.substr(0, comma1);

            // AC,X or AC,Y or AC,OUT
            if(token == "AC"  ||  token == "ac") {opcode &= 0xFC; opcode |= BusMode::AC; return true;}

            // D,X or D,Y or D,OUT
            if(noBrackets)
            {
                opcode &= 0xFC; return handleNativeOperand(token, operand);
            }

            // [D],X or [D],Y or [D],OUT or D,[D] or D,[X] or D,[Y] or [Y,D] or [Y,X] or [Y,X++]
            if(validBrackets)
            {
                if(comma1 > closeBracket) token = input.substr(openBracket+1, closeBracket - (openBracket+1));
                else if(comma1 < openBracket) {opcode &= 0xFC; token = input.substr(0, comma1);}
                else if(comma1 > openBracket  &&  comma1 < closeBracket)
                {
                    token = input.substr(openBracket+1, comma1 - (openBracket+1));
                    if(token != "Y"  &&  token != "y") return false;

                    token = input.substr(comma1+1, closeBracket - (comma1+1));
                    if(token == "X"    ||  token == "x")   {opcode |= AddressMode::YX_AC;    return true;}
                    if(token == "X++"  ||  token == "x++") {opcode |= AddressMode::YXpp_OUT; return true;}

                    opcode |= AddressMode::YD_AC;                
                }
                return handleNativeOperand(token, operand);
            }

            return false;
        }

        // D,[Y,X] or D,[Y,X++]
        if(validBrackets  &&  twoCommas  &&  comma1 < openBracket  &&  comma2 > openBracket  &&  comma2 < closeBracket)
        {
            token = input.substr(0, comma1);
            if(!handleNativeOperand(token, operand)) return false;

            token = input.substr(openBracket+1, comma2 - (openBracket+1));
            if(token != "Y"  &&  token != "y") return false;
            opcode &= 0xFC; // reset bus bits to D

            token = input.substr(comma2+1, closeBracket - (comma2+1));
            if(token == "X"    ||  token == "x")   {opcode |= YX_AC;    return true;}
            if(token == "X++"  ||  token == "x++") {opcode |= YXpp_OUT; return true;}
                
            return false;
        }

        // [Y,X++],out
        if(validBrackets  &&  twoCommas  &&  comma1 > openBracket  &&  comma2 > closeBracket)
        {
            token = input.substr(openBracket+1, comma1 - (openBracket+1));
            if(token != "Y"  &&  token != "y") return false;

            token = input.substr(comma1+1, closeBracket - (comma1+1));
            if(token == "X"    ||  token == "x")   {opcode |= YX_AC;    return true;}
            if(token == "X++"  ||  token == "x++") {opcode |= YXpp_OUT; return true;}
                
            return false;
        }

        return false;
    }

    void packByteCode(Instruction& instruction, ByteCode& byteCode)
    {
        switch(instruction._byteSize)
        {
            case OneByte:
            {
                byteCode._isRomAddress = instruction._isRomAddress;
                byteCode._isCustomAddress = instruction._isCustomAddress;
                byteCode._data = instruction._opcode;
                byteCode._address = instruction._address;
                _byteCode.push_back(byteCode);
            }
            break;

            case TwoBytes:
            {
                byteCode._isRomAddress = instruction._isRomAddress;
                byteCode._isCustomAddress = instruction._isCustomAddress;
                byteCode._data = instruction._opcode;
                byteCode._address = instruction._address;
                _byteCode.push_back(byteCode);

                byteCode._isRomAddress = instruction._isRomAddress;
                byteCode._isCustomAddress = false;
                byteCode._data = instruction._operand0;
                byteCode._address = 0x0000;
                _byteCode.push_back(byteCode);
            }
            break;

            case ThreeBytes:
            {
                byteCode._isRomAddress = instruction._isRomAddress;
                byteCode._isCustomAddress = instruction._isCustomAddress;
                byteCode._data = instruction._opcode;
                byteCode._address = instruction._address;
                _byteCode.push_back(byteCode);

                byteCode._isRomAddress = instruction._isRomAddress;
                byteCode._isCustomAddress = false;
                byteCode._data = instruction._operand0;
                byteCode._address = 0x0000;
                _byteCode.push_back(byteCode);

                byteCode._isRomAddress = instruction._isRomAddress;
                byteCode._isCustomAddress = false;
                byteCode._data = instruction._operand1;
                byteCode._address = 0x0000;
                _byteCode.push_back(byteCode);
            }
            break;

            case FourBytes:
            {
                byteCode._isRomAddress = instruction._isRomAddress;
                byteCode._isCustomAddress = instruction._isCustomAddress;
                byteCode._data = instruction._opcode;
                byteCode._address = instruction._address;
                _byteCode.push_back(byteCode);

                byteCode._isRomAddress = instruction._isRomAddress;
                byteCode._isCustomAddress = false;
                byteCode._data = instruction._operand0;
                byteCode._address = 0x0000;
                _byteCode.push_back(byteCode);

                byteCode._isRomAddress = instruction._isRomAddress;
                byteCode._isCustomAddress = false;
                byteCode._data = instruction._operand1;
                byteCode._address = 0x0000;
                _byteCode.push_back(byteCode);

                byteCode._isRomAddress = instruction._isRomAddress;
                byteCode._isCustomAddress = false;
                byteCode._data = instruction._operand2;
                byteCode._address = 0x0000;
                _byteCode.push_back(byteCode);
            }
            break;

            case FiveBytes:
            {
                byteCode._isRomAddress = instruction._isRomAddress;
                byteCode._isCustomAddress = instruction._isCustomAddress;
                byteCode._data = instruction._opcode;
                byteCode._address = instruction._address;
                _byteCode.push_back(byteCode);

                byteCode._isRomAddress = instruction._isRomAddress;
                byteCode._isCustomAddress = false;
                byteCode._data = instruction._operand0;
                byteCode._address = 0x0000;
                _byteCode.push_back(byteCode);

                byteCode._isRomAddress = instruction._isRomAddress;
                byteCode._isCustomAddress = false;
                byteCode._data = instruction._operand1;
                byteCode._address = 0x0000;
                _byteCode.push_back(byteCode);

                byteCode._isRomAddress = instruction._isRomAddress;
                byteCode._isCustomAddress = false;
                byteCode._data = instruction._operand2;
                byteCode._address = 0x0000;
                _byteCode.push_back(byteCode);

                byteCode._isRomAddress = instruction._isRomAddress;
                byteCode._isCustomAddress = false;
                byteCode._data = instruction._operand3;
                byteCode._address = 0x0000;
                _byteCode.push_back(byteCode);
            }
            break;

            default: break;
        }
    }

    void packByteCodeBuffer(void)
    {
        // Pack instructions
        ByteCode byteCode;
        uint16_t segmentOffset = 0x0000;
        uint16_t segmentAddress = 0x0000;
        for(int i=0; i<int(_instructions.size()); i++)
        {
            // Segment RAM instructions into 256 byte pages for .gt1 file format
            if(!_instructions[i]._isRomAddress)
            {
                // Save start of segment
                if(_instructions[i]._isCustomAddress)
                {
                    segmentOffset = 0x0000;
                    segmentAddress = _instructions[i]._address;
                }

                // Force a new segment, (this could fail if an instruction straddles a page boundary, but
                // the page boundary crossing detection logic will stop the assembler before we get here)
                if(!_instructions[i]._isCustomAddress  &&  (HI_BYTE(segmentAddress + segmentOffset) != HI_BYTE(segmentAddress)))
                {
                    segmentAddress += segmentOffset;
                    segmentOffset = 0x0000;

                    _instructions[i]._isCustomAddress = true;
                    _instructions[i]._address = segmentAddress;
                }

                segmentOffset += uint16_t(_instructions[i]._byteSize);
            }

            packByteCode(_instructions[i], byteCode);
        }

        // Append call table
        if(_callTablePtr  &&  _callTableEntries.size())
        {
            // _callTable grows downwards, pointer is 2 bytes below the bottom of the table by the time we get here
            for(int i=int(_callTableEntries.size())-1; i>=0; i--)
            {
                byteCode._isRomAddress = false;
                byteCode._isCustomAddress = true;  // calltable entries can be non-sequential because of 0x80, (ONE_CONST_ADDRESS)
                byteCode._data = LO_BYTE(_callTableEntries[i]._address);
                byteCode._address = LO_BYTE(_callTableEntries[i]._operand);
                _byteCode.push_back(byteCode);

                byteCode._isRomAddress = false;
                byteCode._isCustomAddress = false;
                byteCode._data = HI_BYTE(_callTableEntries[i]._address);
                byteCode._address = LO_BYTE(_callTableEntries[i]._operand + 1);
                _byteCode.push_back(byteCode);
            }
        }
    }

    bool checkInvalidAddress(ParseType parse, uint16_t currentAddress, uint16_t instructionSize, const Instruction& instruction, const LineToken& lineToken, const std::string& filename, int lineNumber)
    {
        // Check for audio channel stomping
        if(parse == CodePass  &&  !instruction._isRomAddress)
        {
            uint16_t start = currentAddress;
            uint16_t end = currentAddress + instructionSize - 1;
            if((start >= GIGA_CH0_WAV_A  &&  start <= GIGA_CH0_OSC_H)  ||  (end >= GIGA_CH0_WAV_A  &&  end <= GIGA_CH0_OSC_H)  ||
               (start >= GIGA_CH1_WAV_A  &&  start <= GIGA_CH1_OSC_H)  ||  (end >= GIGA_CH1_WAV_A  &&  end <= GIGA_CH1_OSC_H)  ||
               (start >= GIGA_CH2_WAV_A  &&  start <= GIGA_CH2_OSC_H)  ||  (end >= GIGA_CH2_WAV_A  &&  end <= GIGA_CH2_OSC_H)  ||
               (start >= GIGA_CH3_WAV_A  &&  start <= GIGA_CH3_OSC_H)  ||  (end >= GIGA_CH3_WAV_A  &&  end <= GIGA_CH3_OSC_H))
            {
                Cpu::reportError(Cpu::WarnError, stderr, "Assembler::checkInvalidAddress() : Warning, audio channel boundary compromised, (if you've disabled the audio channels, then ignore this warning) ");
                Cpu::reportError(Cpu::WarnError, stderr, "Assembler::checkInvalidAddress() : '%s:%d' : 0x%04X <-> 0x%04X\nAssembler::checkInvalidAddress() : '%s'\nAssembler::checkInvalidAddress()\n",
                                 filename.c_str(), lineNumber+1, start, end, lineToken._text.c_str());
            }
        }

        // Check for page boundary crossings
        if(parse == CodePass  &&  (instruction._opcodeType == vCpu || instruction._opcodeType == Native))
        {
            static uint16_t customAddress = 0x0000;
            if(instruction._isCustomAddress) customAddress = instruction._address;

            uint16_t oldAddress = (instruction._isRomAddress) ? customAddress + (LO_BYTE(currentAddress)>>1) : currentAddress;
            currentAddress += instructionSize - 1;
            uint16_t newAddress = (instruction._isRomAddress) ? customAddress + (LO_BYTE(currentAddress)>>1) : currentAddress;
            if((oldAddress >>8) != (newAddress >>8))
            {
                Cpu::reportError(Cpu::AsmError, stderr, "Assembler::checkInvalidAddress() : '%s:%d' : page boundary compromised : %04X : %04X : '%s'\n", filename.c_str(), lineNumber+1, oldAddress,
                                                                                                                                                         newAddress, lineToken._text.c_str());
                return false;
            }
        }

        return true;
    }


    bool handleInclude(const std::vector<std::string>& tokens, const std::string& lineToken, int lineIndex, std::vector<LineToken>& includeLineTokens)
    {
        // Check include syntax
        if(tokens.size() != 2)
        {
            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::handleInclude() : '%s:%d' : bad %%include statement\n", lineToken.c_str(), lineIndex);
            return false;
        }

        std::string filepath = _includePath + "/" + tokens[1];
        std::replace(filepath.begin(), filepath.end(), '\\', '/');
        std::ifstream infile(filepath);
        if(!infile.is_open())
        {
            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::handleInclude() : failed to open file '%s'\n", filepath.c_str());
            return false;
        }

        // Collect lines from include file
        int lineNumber = lineIndex;
        while(!infile.eof())
        {
            LineToken includeLineToken = {true, lineNumber++ - lineIndex, "", filepath};
            std::getline(infile, includeLineToken._text);
            includeLineTokens.push_back(includeLineToken);

            if(!infile.good() && !infile.eof())
            {
                Cpu::reportError(Cpu::AsmError, stderr, "Assembler::handleInclude() : '%s:%d' : bad lineToken '%s'\n", filepath.c_str(), lineNumber - lineIndex, includeLineToken._text.c_str());
                return false;
            }
        }

        return true;
    }

    bool handleMacros(const std::vector<Macro>& macros, std::vector<LineToken>& lineTokens)
    {
        // Incomplete macros
        for(int i=0; i<int(macros.size()); i++)
        {
            if(!macros[i]._complete)
            {
                Cpu::reportError(Cpu::AsmError, stderr, "Assembler::handleMacros() : '%s:%d' : bad macro, missing 'ENDM'\n", macros[i]._filename.c_str(), macros[i]._fileStartLine);
                return false;
            }
        }

        // Delete original macros
        auto filter = [](LineToken& lineToken)
        {
            static bool foundMacro = false;
            if(lineToken._text.find("%MACRO") != std::string::npos)
            {
                foundMacro = true;
                return true;
            }
            if(foundMacro)
            {
                if(lineToken._text.find("%ENDM") != std::string::npos) foundMacro = false;
                return true;
            }

            return false;
        };
        lineTokens.erase(std::remove_if(lineTokens.begin(), lineTokens.end(), filter), lineTokens.end());

        // Find and expand macro
        int macroInstanceId = 0;
        for(int m=0; m<int(macros.size()); m++)
        {
            bool macroMissing = true;
            bool macroMissingParams = true;
            Macro macro = macros[m];

            for(auto itLine=lineTokens.begin(); itLine!=lineTokens.end();)
            {
                // Lines containing only white space are skipped
                LineToken lineToken = *itLine;
                size_t nonWhiteSpace = lineToken._text.find_first_not_of("  \n\r\f\t\v");
                if(nonWhiteSpace == std::string::npos)
                {
                    ++itLine;
                    continue;
                }

                // Tokenise current line
                std::vector<std::string> tokens = Expression::tokeniseLine(lineToken._text);

                // Find macro
                bool macroSuccess = false;
                for(int t=0; t<int(tokens.size()); t++)
                {
                    if(tokens[t] == macro._name)
                    {
                        macroMissing = false;
                        if(tokens.size() - t > macro._params.size())
                        {
                            macroMissingParams = false;
                            std::vector<std::string> labels;
                            std::vector<LineToken> macroLines;

                            // Create substitute lines
                            for(int ml=0; ml<int(macro._lines.size()); ml++)
                            {
                                // Tokenise macro line
                                std::vector<std::string> mtokens =  Expression::tokeniseLine(macro._lines[ml]);

                                // Save labels
                                nonWhiteSpace = macro._lines[ml].find_first_not_of("  \n\r\f\t\v");
                                if(nonWhiteSpace == 0) labels.push_back(mtokens[0]);

                                // Replace parameters
                                for(int mt=0; mt<int(mtokens.size()); mt++)
                                {
                                    for(int p=0; p<int(macro._params.size()); p++)
                                    {
                                        //if(mtokens[mt] == macro._params[p]) mtokens[mt] = tokens[t + 1 + p];
                                        size_t param = mtokens[mt].find(macro._params[p]);
                                        if(param != std::string::npos)
                                        {
                                            mtokens[mt].erase(param, macro._params[p].size());
                                            mtokens[mt].insert(param, tokens[t + 1 + p]);
                                        }
                                    }
                                }

                                // New macro line using any existing label
                                LineToken macroLine = {false, 0, "", ""};
                                macroLine._text = (t > 0  &&  ml == 0) ? tokens[0] : "";

                                // Append to macro line
                                for(int mt=0; mt<int(mtokens.size()); mt++)
                                {
                                    // Don't prefix macro labels with a space
                                    if(nonWhiteSpace != 0  ||  mt != 0) macroLine._text += " ";

                                    macroLine._text += mtokens[mt];
                                }

                                macroLines.push_back(macroLine);
                            }

                            // Insert substitute lines
                            for(int ml=0; ml<int(macro._lines.size()); ml++)
                            {
                                // Delete macro caller
                                if(ml == 0) itLine = lineTokens.erase(itLine);

                                // Each instance of a macro's labels are made unique
                                for(int i=0; i<int(labels.size()); i++)
                                {
                                    size_t labelFoundPos = macroLines[ml]._text.find(labels[i]);
                                    if(labelFoundPos != std::string::npos) macroLines[ml]._text.insert(labelFoundPos + labels[i].size(), std::to_string(macroInstanceId));
                                }

                                // Insert macro lines
                                itLine = lineTokens.insert(itLine, macroLines[ml]);
                                ++itLine;
                            }

                            macroInstanceId++;
                            macroSuccess = true;
                        }
                    }
                }

                if(!macroSuccess) ++itLine;
            }

            if(macroMissing)
            {
                //Cpu::reportError(Cpu::WarnError, stderr, "Assembler::handleMacros() : '%s:%d' : warning, macro '%s' is never called\n", macro._filename.c_str(), macro._fileStartLine, macro._name.c_str());
                continue;
            }

            if(macroMissingParams)
            {
                Cpu::reportError(Cpu::AsmError, stderr, "Assembler::handleMacros() : '%s:%d' : missing macro parameters for '%s'\n", macro._filename.c_str(), macro._fileStartLine, macro._name.c_str());
                return false;
            }
        }

        return true;
    }

    bool handleMacroStart(const std::string& filename, const LineToken& lineToken, const std::vector<std::string>& tokens, Macro& macro, int adjustedLineIndex)
    {
        int lineNumber = (lineToken._fromInclude) ? lineToken._includeLineNumber + 1 : adjustedLineIndex + 1;
        std::string macroFileName = (lineToken._fromInclude) ? lineToken._includeName : filename;

        // Check macro syntax
        if(tokens.size() < 2)
        {
            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::handleMacroStart() : '%s:%d' : bad macro, missing name\n", macroFileName.c_str(), lineNumber);
            return false;
        }                    

        macro._name = tokens[1];
        macro._fromInclude = lineToken._fromInclude;
        macro._fileStartLine = lineNumber;
        macro._filename = macroFileName;

        // Save params
        for(int i=2; i<int(tokens.size()); i++) macro._params.push_back(tokens[i]);

        return true;
    }

    bool handleMacroEnd(std::vector<Macro>& macros, Macro& macro)
    {
        // Check for duplicates
        for(int i=0; i<int(macros.size()); i++)
        {
            if(macro._name == macros[i]._name)
            {
                Cpu::reportError(Cpu::AsmError, stderr, "Assembler::handleMacroEnd() : '%s:%d' : bad macro, duplicate name '%s'\n", macro._filename.c_str(), macro._fileStartLine, macro._name.c_str());
                return false;
            }
        }
        macro._complete = true;
        macros.push_back(macro);

        macro._name = "";
        macro._lines.clear();
        macro._params.clear();
        macro._complete = false;

        return true;
    }

    void clearDefines(void)
    {
        _defines.clear();
        while(!_currentDefine.empty()) _currentDefine.pop();
    }

    bool createDefine(const std::string& filename, const std::vector<std::string>& tokens, int adjustedLineIndex)
    {
        if(tokens.size() < 2  ||  tokens.size() > 3)
        {
            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::createDefine() : '%s:%d' : %%define requires one or two params\n", filename.c_str(), adjustedLineIndex);
            return false;
        }

        // Define name
        std::string defineName = tokens[1];
        if(_defines.find(defineName) != _defines.end())
        {
            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::createDefine() : '%s:%d' : found duplicate define '%s'\n", filename.c_str(), adjustedLineIndex, defineName.c_str());
            return false;
        }

        // Define value
        int32_t defineValue = 0;
        if(tokens.size() == 3)
        {
            if(!evaluateExpression(tokens[2], false, defineValue))
            {
                Cpu::reportError(Cpu::AsmError, stderr, "Assembler::createDefine() : '%s:%d' : found invalid define value '%s'\n", filename.c_str(), adjustedLineIndex, tokens[2].c_str());
                return false;
            }
        }

        _defines[defineName] = {true, false, defineValue, defineName};

        return true;
    }

    bool handleIfDefine(const std::string& filename, const std::vector<std::string>& tokens, int adjustedLineIndex)
    {
        if(tokens.size() != 2)
        {
            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::handleIfDefine() : '%s:%d' : %%if requires one param\n", filename.c_str(), adjustedLineIndex);
            return false;
        }

        std::string define = tokens[1];
        if(_defines.find(define) == _defines.end())
        {
            _defines[define] = {false, false, 0, define};
        }

        // Push current define to stack
        _currentDefine.push(define);

        return true;
    }

    bool handleEndIfDefine(const std::string& filename, const std::vector<std::string>& tokens, int adjustedLineIndex)
    {
        if(tokens.size() != 1)
        {
            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::handleEndIfDefine() : '%s:%d' : %%endif requires no params\n", filename.c_str(), adjustedLineIndex);
            return false;
        }

        if(_currentDefine.empty())
        {
            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::handleEndIfDefine() : '%s:%d' : syntax error, no valid define\n", filename.c_str(), adjustedLineIndex);
            return false;
        }

        // If enabled was toggled by an else then reset it
        std::string define = _currentDefine.top();
        if(_defines[define]._toggle == true)
        {
            _defines[define]._enabled = !_defines[define]._enabled;
            _defines[define]._toggle = false;
        }

        // Pop current define from stack
        _currentDefine.pop();

        return true;
    }

    bool handleElseDefine(const std::string& filename, const std::vector<std::string>& tokens, int adjustedLineIndex)
    {
        if(tokens.size() != 1)
        {
            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::handleElseDefine() : '%s:%d' : %%else requires no params\n", filename.c_str(), adjustedLineIndex);
            return false;
        }

        if(_currentDefine.empty())
        {
            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::handleElseDefine() : '%s:%d' : syntax error, no valid define\n", filename.c_str(), adjustedLineIndex);
            return false;
        }

        // Toggle current define
        std::string define = _currentDefine.top();
        _defines[define]._enabled = !_defines[define]._enabled;
        _defines[define]._toggle = true;

        return true;
    }

    bool isCurrentDefineDisabled(void)
    {
        // Check top of define stack
        if(!_currentDefine.empty())
        {
            std::string define = _currentDefine.top();
            if(_defines.find(define) != _defines.end()  &&  !_defines[define]._enabled)
            {
                return true;
            }
        }

        return false;
    }

    int16_t getRuntimeVersion(void)
    {
        int16_t runtimeVersion = 0;
        if(_defines.find("RUNTIME_VERSION") != _defines.end())
        {
            runtimeVersion = int16_t(_defines["RUNTIME_VERSION"]._value);
        }

        return runtimeVersion;
    }

    bool preProcess(const std::string& filename, std::vector<LineToken>& lineTokens, bool doMacros)
    {
        Macro macro;
        std::vector<Macro> macros;
        bool buildingMacro = false;

        int adjustedLineIndex = 0;
        for(auto itLine=lineTokens.begin(); itLine != lineTokens.end();)
        {
            // Lines containing only white space are skipped
            LineToken lineToken = *itLine;
            size_t nonWhiteSpace = lineToken._text.find_first_not_of("  \n\r\f\t\v");
            if(nonWhiteSpace == std::string::npos)
            {
                ++itLine;
                ++adjustedLineIndex;
                continue;
            }

            bool eraseLine = false;
            int lineIndex = int(itLine - lineTokens.begin()) + 1;

            // Strip comments
            size_t commentPos = lineToken._text.find_first_of(";#");
            if(commentPos != std::string::npos)
            {
                lineToken._text = lineToken._text.substr(0, commentPos);
            }

            // Tokenise current line
            std::vector<std::string> tokens = Expression::tokeniseLine(lineToken._text);

            // Valid pre-processor commands
            if(tokens.size() > 0)
            {
                Expression::strToUpper(tokens[0]);

                // Remove subroutine header and footer
                if(tokens[0] == "%SUB"  ||  tokens[0] == "%ENDS")
                {
                    itLine = lineTokens.erase(itLine);
                    continue;
                }

                // Include
                if(tokens[0] == "%INCLUDE")
                {
                    std::vector<LineToken> includeLineTokens;
                    if(!handleInclude(tokens, lineToken._text, lineIndex, includeLineTokens)) return false;

                    // Recursively include everything in order
                    if(!preProcess(filename, includeLineTokens, false))
                    {
                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::preProcess() : '%s:%d' : bad include file : '%s'\n", filename.c_str(), adjustedLineIndex, tokens[1].c_str());
                        return false;
                    }

                    // Remove original include line and replace with include text
                    itLine = lineTokens.erase(itLine);
                    itLine = lineTokens.insert(itLine, includeLineTokens.begin(), includeLineTokens.end());
                    ++adjustedLineIndex -= int(includeLineTokens.end() - includeLineTokens.begin());
                    eraseLine = true;
                }
                // Include path
                else if(tokens[0] == "%INCLUDEPATH"  &&  tokens.size() > 1)
                {
                    if(Expression::isStringValid(tokens[1]))
                    {
                        // Strip quotes
                        std::string includePath = tokens[1];
                        includePath.erase(0, 1);
                        includePath.erase(includePath.size()-1, 1);

                        // Prepend current file path to relative paths
                        if(includePath.find(":") == std::string::npos  &&  includePath[0] != '/')
                        {
                            std::string filepath = Loader::getFilePath();
                            size_t slash = filepath.find_last_of("\\/");
                            filepath = (slash != std::string::npos) ? filepath.substr(0, slash) : ".";
                            includePath = filepath + "/" + includePath;
                        }

                        _includePath = includePath;
                        itLine = lineTokens.erase(itLine);
                        eraseLine = true;
                    }
                }
                // Define
                else if(tokens[0] == "%DEFINE")
                {
                    if(!createDefine(filename, tokens, 3)) return false;

                    itLine = lineTokens.erase(itLine);
                    eraseLine = true;
                }
                // If
                else if(tokens[0] == "%IF")
                {
                    if(!handleIfDefine(filename, tokens, adjustedLineIndex)) return false;

                    itLine = lineTokens.erase(itLine);
                    eraseLine = true;
                }
                // EndIf
                else if(tokens[0] == "%ENDIF")
                {
                    if(!handleEndIfDefine(filename, tokens, adjustedLineIndex)) return false;

                    itLine = lineTokens.erase(itLine);
                    eraseLine = true;
                }
                // Else
                else if(tokens[0] == "%ELSE")
                {
                    if(!handleElseDefine(filename, tokens, adjustedLineIndex)) return false;

                    itLine = lineTokens.erase(itLine);
                    eraseLine = true;
                }
                // Check if there is a current define and delete code if define is disabled
                else if(!_currentDefine.empty())
                {
                    std::string define = _currentDefine.top();
                    if(_defines.find(define) == _defines.end())
                    {
                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::preProcess() : '%s:%d' : define '%s' does not exist\n", filename.c_str(), adjustedLineIndex, define.c_str());
                        return false;
                    }

                    if(!_defines[define]._enabled)
                    {                    
                        itLine = lineTokens.erase(itLine);
                        eraseLine = true;
                    }
                }
                // Build macro
                else if(doMacros)
                {
                    if(tokens[0] == "%MACRO"  &&  !buildingMacro)
                    {
                        if(!handleMacroStart(filename, lineToken, tokens, macro, adjustedLineIndex)) return false;

                        buildingMacro = true;
                    }
                    else if(buildingMacro  &&  tokens[0] == "%ENDM")
                    {
                        if(!handleMacroEnd(macros, macro)) return false;
                        buildingMacro = false;
                    }
                    else if(buildingMacro)
                    {
                        macro._lines.push_back(lineToken._text);
                    }
                }
            }

            if(!eraseLine)
            {
                ++itLine;
                ++adjustedLineIndex;
            }
        }

        // Check matching %if %endif
        if(!_currentDefine.empty())
        {
            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::preProcess() : '%s' : missing '%%endif'\n", filename.c_str());
            return false;
        }

        // Handle complete macros
        if(doMacros  &&  !handleMacros(macros, lineTokens)) return false;

        return true;
    }

    int getAsmOpcodeSize(const std::string& filename, std::vector<LineToken>& lineTokens)
    {
        // Pre-processor
        if(!preProcess(filename, lineTokens, true)) return -1;

        // Tokenise lines
        int vasmSize = 0;
        for(int i=0; i<int(lineTokens.size()); i++)
        {
            std::vector<std::string> tokens = Expression::tokeniseLine(lineTokens[i]._text);
            for(int j=0; j<int(tokens.size()); j++)
            {
                std::string token = Expression::strToUpper(tokens[j]);
                vasmSize += getAsmOpcodeSize(token);
            }
        }

        return vasmSize;
    }

    int getAsmOpcodeSizeFile(const std::string& filename)
    {
        std::ifstream infile(filename);
        if(!infile.is_open())
        {
            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::getAsmOpcodeSizeFile() : failed to open file : '%s'\n", filename.c_str());
            return -1;
        }

        // Get file
        int numLines = 0;
        LineToken lineToken;
        std::vector<LineToken> lineTokens;
        while(!infile.eof())
        {
            std::getline(infile, lineToken._text);
            lineTokens.push_back(lineToken);

            if(!infile.good() && !infile.eof())
            {
                Cpu::reportError(Cpu::AsmError, stderr, "Assembler::getAsmOpcodeSizeFile() : '%s:%d' : bad lineToken '%s'\n", filename.c_str(), numLines+1, lineToken._text.c_str());
                return -1;
            }

            numLines++;
        }
        
        return getAsmOpcodeSize(filename, lineTokens);
    }

#ifndef STAND_ALONE
    bool handleBreakPoints(ParseType parse, const std::string& lineToken, int lineNumber)
    {
        UNREFERENCED_PARAM(lineNumber);

        std::string input = lineToken;
        Expression::strToUpper(input);

        if(input.find("_BREAKPOINT_") != std::string::npos)
        {
            if(parse == MnemonicPass) Debugger::getVpcBreakPoints().push_back(_currentAddress);
            return true;
        }

        return false;
    }

    bool addGprintf(const Gprintf& gprintf, uint16_t address)
    {
        if(_gprintfs.find(address) != _gprintfs.end()) return false;

        _gprintfs[address] = gprintf;
        return true;
    }

    bool parseGprintfFormat(const std::string& fmtstr, const std::vector<std::string>& variables, std::vector<Gprintf::Var>& vars, std::vector<std::string>& subs)
    {
        const char* fmt = fmtstr.c_str();
        std::string sub;
        char chr;

        int width = 0, index = 0;
        bool foundToken = false;

        while((chr = *fmt++) != 0)
        {
            if(index + 1 > int(variables.size())) return false;

            if(chr == '%'  ||  foundToken)
            {
                foundToken = true;
                Gprintf::Format format = Gprintf::Int;
                sub += chr;
                bool isUpper = std::isupper(chr);

                bool foundFormat = true;
                switch(std::toupper(chr))
                {
                    case '0':
                    {
                        // Maximum field width of 16 digits
                        width = strtol(fmt, nullptr, 10) % 17;
                        foundFormat = false;
                    }
                    break;

                    case 'C': format = Gprintf::Chr; break;
                    case 'B': format = Gprintf::Byt; break;
                    case 'D': format = Gprintf::Int; break;
                    case 'I': format = Gprintf::Bit; break;
                    case 'Q':
                    case 'O': format = Gprintf::Oct; break;
                    case 'X': format = Gprintf::Hex; break;
                    case 'S': format = Gprintf::Str; break;

                    default: foundFormat = false;    break;
                }

                if(foundFormat)
                {
                    foundToken = false;
                    Gprintf::Var var = {0, format, width, 0x0000, variables[index++], isUpper};
                    vars.push_back(var);
                    subs.push_back(sub);
                    sub.clear();
                    width = 0;
                }
            }
        }

        return true;
    }

    bool handleGprintf(ParseType parse, const std::string& lineToken, int lineNumber)
    {
        std::string input = lineToken;
        Expression::strToUpper(input);

        // Handle non commented GPRINTF
        size_t gprintfPos = input.find("GPRINTF");
        if(gprintfPos != std::string::npos  &&  input.find_last_of(';', gprintfPos) == std::string::npos)
        {
            size_t lbra, rbra;
            if(Expression::findMatchingBrackets(lineToken, 0, lbra, rbra))
            {
                size_t quote1 = lineToken.find_first_of("\"", lbra+1);
                size_t quote2 = lineToken.find_last_of("\"", rbra-1);
                bool quotes = (quote1 != std::string::npos  &&  quote2 != std::string::npos  &&  (quote2 - quote1 > 0));
                if(quotes)
                {
                    if(parse == MnemonicPass)
                    {
                        std::string formatText = lineToken.substr(quote1+1, quote2 - (quote1+1));
                        std::string variableText = lineToken.substr(quote2+1, rbra - (quote2+1));

                        std::vector<Gprintf::Var> vars;
                        std::vector<std::string> subs;
                        std::vector<std::string> variables = Expression::tokenise(variableText, ',');
                        parseGprintfFormat(formatText, variables, vars, subs);

                        Gprintf gprintf = {_currentAddress, lineNumber, lineToken, formatText, vars, subs};
                        if(_gprintfs.find(_currentAddress) != _gprintfs.end())
                        {
                            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::handleGprintf() : '%s:%d' : gprintf at address 0x%04x already exists\n", lineToken.c_str(), lineNumber, _currentAddress);
                            return false;
                        }

                        _gprintfs[_currentAddress] = gprintf;
                    }

                    return true;
                }
            }

            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::handleGprintf() : '%s:%d' : bad gprintf format\n", lineToken.c_str(), lineNumber);
            return false;
        }

        return false;
    }

    bool parseGprintfVar(const std::string& param, uint16_t& data)
    {
        bool success = Expression::stringToU16(param, data);
        if(!success)
        {
            std::vector<std::string> tokens;
            tokens.push_back(param);

            // Search equates
            Equate equate;
            Label label;
            int commaTokenIndex = -1;
            if((success = evaluateEquateOperand(tokens, 0, equate, commaTokenIndex, false)) == true)
            {
                data = uint16_t(equate._operand);
            }
            // Search labels
            else if((success = evaluateLabelOperand(tokens, 0, label, false)) == true)
            {
                data = label._address;
            }
            // Normal expression
            else
            {
                if(Expression::isExpression(param) == Expression::HasOperators)
                {
                    Expression::Numeric value;
                    if(Expression::parse(param, _lineNumber, value))
                    {
                        data = int16_t(std::lround(value._value));
                        success = true;
                    }
                }
            }
        }

        return success;
    }

    bool parseGprintfs(void)
    {
        for(auto it=_gprintfs.begin(); it!=_gprintfs.end(); it++ )
        {
            Gprintf& gprintf = it->second;

            for(int j=0; j<int(gprintf._vars.size()); j++)
            {
                uint16_t addr = 0x0000;
                std::string token = gprintf._vars[j]._text;
        
                // Strip white space
                token.erase(remove_if(token.begin(), token.end(), isspace), token.end());
                gprintf._vars[j]._text = token;

                // Check for indirection
                size_t asterisk = token.find_first_of("*");
                if(asterisk != std::string::npos)
                {
                    // Don't overwrites gtBASIC vars
                    if(gprintf._vars[j]._indirection == 0) gprintf._vars[j]._indirection = 1;
                    token = token.substr(asterisk + 1);
                }

                if(!parseGprintfVar(token, addr))
                {
                    Cpu::reportError(Cpu::AsmError, stderr, "Assembler::parseGprintfs() : '%s:%d' : error in gprintf(), missing label or equate '%s'\n", gprintf._lineToken.c_str(), gprintf._lineNumber, token.c_str());
                    _gprintfs.erase(it);
                    return false;
                }

                gprintf._vars[j]._data = addr;
            }
        }

        return true;
    }

    bool getGprintfString(const Gprintf& gprintf, std::string& gstring)
    {
        gstring = gprintf._format;
   
        size_t subIndex = 0;
        for(int i=0; i<int(gprintf._vars.size()); i++)
        {
            char token[256];

            // Use indirection if required
            uint16_t data = gprintf._vars[i]._data;
            if(gprintf._vars[i]._indirection)
            {
                data = (gprintf._vars[i]._format == Gprintf::Byt) ? Cpu::getRAM(gprintf._vars[i]._data) : Cpu::getRAM16(gprintf._vars[i]._data);
            }

            // Maximum field width of 16 digits
            uint8_t width = gprintf._vars[i]._width % 17;
            std::string fieldWidth = "%";
            if(width) fieldWidth = std::string("%0" + std::to_string(width));

            // Case
            bool isUpper = gprintf._vars[i]._isUpper;

            switch(gprintf._vars[i]._format)
            {
                case Gprintf::Chr: fieldWidth += (isUpper) ? "C" : "c"; sprintf(token, fieldWidth.c_str(), data); break;
                case Gprintf::Byt:
                case Gprintf::Int: fieldWidth += (isUpper) ? "D" : "d"; sprintf(token, fieldWidth.c_str(), data); break;
                case Gprintf::Oct: fieldWidth += (isUpper) ? "O" : "o"; sprintf(token, fieldWidth.c_str(), data); break;
                case Gprintf::Hex: fieldWidth += (isUpper) ? "X" : "x"; sprintf(token, fieldWidth.c_str(), data); break;

                // Strings are always indirect and assume that length is first byte
                case Gprintf::Str:
                {
                    // gtBASIC strings are dereferenced by 2 levels of indirection
                    uint16_t addr = (gprintf._vars[i]._indirection == 2) ? Cpu::getRAM16(gprintf._vars[i]._data) : gprintf._vars[i]._data;

                    // Maximum length of 254, first byte is length, last byte is '0' trailing delimiter
                    uint8_t length = Cpu::getRAM(addr) % 254;
                    for(uint16_t j=0; j<length; j++) token[j] = Cpu::getRAM(addr + j + 1);
                    token[length] = 0;
                }
                break;

                case Gprintf::Bit:
                {
                    for(int j=width-1; j>=0; j--)
                    {
                        token[width-1 - j] = '0' + ((data >> j) & 1);
                    }
                    token[width] = 0;
                }
                break;

                default: return false;
            }

            // Replace substrings
            subIndex = gstring.find(gprintf._subs[i], subIndex);
            if(subIndex != std::string::npos)
            {
                gstring.erase(subIndex, gprintf._subs[i].size());
                gstring.insert(subIndex, token);
            }
        }

        return true;
    }

    void handleGprintfs(void)
    {
        if(_gprintfs.size() == 0) return;

        if(_gprintfs.find(Cpu::getVPC()) != _gprintfs.end())
        {
            std::string gstring;
            const Gprintf& gprintf = _gprintfs[Cpu::getVPC()];

            getGprintfString(gprintf, gstring);
            Cpu::reportError(Cpu::NoError, stderr, "gprintf() : 0x%04X : %s\n", gprintf._address, gstring.c_str());
        }
    }
#endif

    uint8_t sysHelper(const std::string& opcodeStr, uint16_t operand, const std::string& filename, int lineNumber)
    {
        std::string opcode = opcodeStr;
        Expression::strToUpper(opcode); 
        
        if(opcode != "SYS") return uint8_t(operand);

        if((operand & 0x0001) || operand < 28 || operand > 284)
        {
            Cpu::reportError(Cpu::WarnError, stderr, "Assembler::sysHelper() : '%s:%d' : SYS operand '%d' must be an even constant in [28, 284]\n", filename.c_str(), lineNumber, operand);
            return uint8_t(operand);
        }

        return uint8_t((270 - operand / 2) & 0x00FF);
    }


    void clearAssembler(bool dontClearGprintfs)
    {
        _labels.clear();
        _equates.clear();
        _byteCode.clear();
        _instructions.clear();
        _callTableEntries.clear();

        if(!dontClearGprintfs) _gprintfs.clear();

        clearDefines();        

        _vSpMin = 0x00;
        _callTablePtr = 0x0000;

        Cpu::clearCriticalError();

        Expression::setExprFunc(Expression::expression);

#ifndef STAND_ALONE
        Debugger::getVpcBreakPoints().clear();
#endif
    }

    bool validateOperand(const std::vector<std::string>& tokens, int tokenIndex, bool compoundInstruction, uint16_t& operand)
    {
        Label label;
        Equate equate;
        int commaTokenIndex = -1;

        // Valid equate
        if(evaluateEquateOperand(tokens, tokenIndex, equate, commaTokenIndex, compoundInstruction))
        {
            operand = uint16_t(equate._operand);
            return true;
        }

        // Valid label
        if(evaluateLabelOperand(tokens, tokenIndex, label, compoundInstruction))
        {
            operand = label._address;
            return true;
        }

        // Valid expression
        if(Expression::isExpression(tokens[tokenIndex]) == Expression::HasOperators)
        {
            Expression::Numeric value;
            std::string input;
            preProcessExpression(tokens, tokenIndex, input, true);
            if(!Expression::parse((char*)input.c_str(), _lineNumber, value)) return false;
            operand = int16_t(std::lround(value._value));
            return true;
        }

        // Valid constant
        if(Expression::stringToU16(tokens[tokenIndex], operand)) return true;

        operand = 0x0000;

        return false;
    }

    bool assemble(const std::string& filename, uint16_t startAddress, bool dontClearGprintfs)
    {
        std::ifstream infile(filename);
        if(!infile.is_open())
        {
            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : failed to open file '%s'\n", filename.c_str());
            return false;
        }

        clearAssembler(dontClearGprintfs);

        _startAddress = startAddress;
        _currentAddress = _startAddress;

#ifndef STAND_ALONE
        Loader::disableUploads(false);
#endif

        // Get file
        int numLines = 0;
        LineToken lineToken;
        std::vector<LineToken> lineTokens;
        while(!infile.eof())
        {
            std::getline(infile, lineToken._text);
            lineTokens.push_back(lineToken);

            if(!infile.good() && !infile.eof())
            {
                Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : bad lineToken '%s'\n", filename.c_str(), numLines+1, lineToken._text.c_str());
                return false;
            }

            numLines++;
        }

        // Pre-processor
        if(!preProcess(filename, lineTokens, true)) return false;

        Cpu::reportError(Cpu::NoError, stderr, "\n*******************************************************\n");
        Cpu::reportError(Cpu::NoError, stderr, "* Assembling file : '%s'\n", filename.c_str());
        Cpu::reportError(Cpu::NoError, stderr, "*******************************************************\n");

        numLines = int(lineTokens.size());

        // The mnemonic pass we evaluate all the equates and labels, the code pass is for the opcodes and operands
        for(int parse=MnemonicPass; parse<NumParseTypes; parse++)
        {
            for(_lineNumber=0; _lineNumber<numLines; _lineNumber++)
            {
                lineToken = lineTokens[_lineNumber];

                // Lines containing only white space are skipped
                size_t nonWhiteSpace = lineToken._text.find_first_not_of("  \n\r\f\t\v");
                if(nonWhiteSpace == std::string::npos) continue;

                int tokenIndex = 0;

                // Tokenise current line
                std::vector<std::string> tokens = Expression::tokeniseLine(lineToken._text);
                if(tokens.size() == 0)
                {
                    Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : unknown error '%s'\n", filename.c_str(), _lineNumber+1, lineToken._text.c_str());
                    return false;
                }

                // Comments
                if(tokens.size() > 0  &&  tokens[0].find_first_of(";#") != std::string::npos) continue;

#ifndef STAND_ALONE
                // Gprintf lines are skipped
                if(handleGprintf(ParseType(parse), lineToken._text, _lineNumber+1)) continue;

                // _breakPoint_ lines are skipped
                if(handleBreakPoints(ParseType(parse), lineToken._text, _lineNumber+1)) continue;
#endif

                // Starting address, labels and equates
                if(nonWhiteSpace == 0)
                {
                    if(tokens.size() >= 2)
                    {
                        EvaluateResult result = evaluateEquates(tokens, (ParseType)parse);
                        if(result == NotFound)
                        {
                            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : missing equate '%s'\n", filename.c_str(), _lineNumber+1, lineToken._text.c_str());
                            return false;
                        }
                        else if(result == Duplicate)
                        {
                            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : duplicate equate '%s'\n", filename.c_str(), _lineNumber+1, lineToken._text.c_str());
                            return false;
                        }
                        // Skip equate lines
                        else if(result == Success) 
                        {
                            continue;
                        }
                            
                        // Labels
                        result = EvaluateLabels(tokens, (ParseType)parse, tokenIndex);
                        if(result == Reserved)
                        {
                            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : can't use a reserved word in label '%s'\n", filename.c_str(), _lineNumber+1, tokens[tokenIndex].c_str());
                            return false;
                        }
                        else if(result == Duplicate)
                        {
                            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : duplicate label '%s'\n", filename.c_str(), _lineNumber+1, lineToken._text.c_str());
                            return false;
                        }
                    }

                    // On to the next token even if we failed with this one
                    if(tokens.size() > 1) tokenIndex++;
                }

                // Opcode
                bool operandValid = false;
                uint16_t defineOffset;
                std::string opcodeStr = tokens[tokenIndex++];
                InstructionType instructionType = getOpcode(opcodeStr, defineOffset);
                uint8_t opcode0 = instructionType._opcode0;
                uint8_t opcode1 = instructionType._opcode1;
                int outputSize = instructionType._byteSize;
                OpcodeType opcodeType = instructionType._opcodeType;
                Instruction instruction = {false, false, ByteSize(outputSize), opcode0, 0x00, 0x00, 0x00, 0x00, _currentAddress, opcodeType};

                if(outputSize == BadByteSize)
                {
                    std::string opcStr = Expression::strUpper(opcodeStr);
                    if(opcStr.find("GPRINTF") != std::string::npos) continue; // skip gprintf's in non emulation
                    Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : bad Opcode '%s' in '%s'\n", filename.c_str(), _lineNumber+1, opcodeStr.c_str(), lineToken._text.c_str());
                    return false;
                }

                // Compound instructions that require a Mnemonic pass
                bool compoundInstruction = false;
                if(opcodeType == ReservedDB  ||  opcodeType == ReservedDBR)
                {
                    compoundInstruction = true;
                    if(parse == MnemonicPass)
                    {
                        outputSize = OneByte; // first instruction has already been parsed
                        if(tokenIndex + 1 < int(tokens.size()))
                        {
                            if(!handleDefineByte(tokens, tokenIndex, instruction, false, defineOffset, outputSize))
                            {
                                Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : bad DB data '%s'\n", filename.c_str(), _lineNumber+1, lineToken._text.c_str());
                                return false;
                            }
                        }
                    }
                }
                else if(opcodeType == ReservedDW  ||  opcodeType == ReservedDWR)
                {
                    compoundInstruction = true;
                    if(parse == MnemonicPass)
                    {
                        outputSize = TwoBytes; // first instruction has already been parsed
                        if(tokenIndex + 1 < int(tokens.size()))
                        {
                            if(!handleDefineWord(tokens, tokenIndex, instruction, false, defineOffset, outputSize))
                            {
                                Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : bad DW data '%s'\n", filename.c_str(), _lineNumber+1, lineToken._text.c_str());
                                return false;
                            }
                        }
                    }
                }
                
                if(parse == CodePass)
                {
                    // Native NOP
                    if(opcodeType == Native  &&  opcode0 == 0x02)
                    {
                        operandValid = true;
                    }
                    else if(opcodeType == vCpu  &&  opcode0 == OPCODE_V_HALT  &&  opcode1 == OPERAND_V_HALT)
                    {
                        // Handle in switch(outputSize)
                    }
                    // Missing operand, (for non PREFX1 instructions)
                    else if(opcode0 != OPCODE_V_PREFX1  &&  (outputSize == TwoBytes  ||  outputSize == ThreeBytes  ||  outputSize == FourBytes  ||  outputSize == FiveBytes)  &&  int(tokens.size()) <= tokenIndex)
                    {
                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : missing operand/s '%s'\n", filename.c_str(), _lineNumber+1, lineToken._text.c_str());
                        return false;
                    }

                    // First instruction inherits start address
                    if(_instructions.size() == 0)
                    {
                        instruction._address = _startAddress;
                        instruction._isCustomAddress = true;
                        _currentAddress = _startAddress;
                    }

                    // Custom address
                    for(int i=0; i<int(_equates.size()); i++)
                    {
                        if(_equates[i]._name == tokens[0]  &&  _equates[i]._isCustomAddress)
                        {
                            instruction._address = uint16_t(_equates[i]._operand);
                            instruction._isCustomAddress = true;
                            _currentAddress = uint16_t(_equates[i]._operand);
                        }
                    }

                    // Operand
                    switch(outputSize)
                    {
                        case OneByte:
                        {
                            _instructions.push_back(instruction);
                            if(!checkInvalidAddress(ParseType(parse), _currentAddress, uint16_t(outputSize), instruction, lineToken, filename, _lineNumber)) return false;
                        }
                        break;

                        case TwoBytes:
                        {
                            uint8_t operand = 0x00;

                            // HALT
                            if(opcodeType == vCpu  &&  opcode0 == OPCODE_V_HALT  &&  opcode1 == OPERAND_V_HALT)
                            {
                                operand = opcode1;
                                operandValid = true;
                            }
                            // BRA
                            else if(opcodeType == vCpu  &&  opcode0 == OPCODE_V_BRA)
                            {
                                // Search for branch label
                                Label label;
                                if(evaluateLabelOperand(tokens, tokenIndex, label, false))
                                {
                                    operandValid = true;
                                    operand = uint8_t(label._address) - BRANCH_ADJUSTMENT;
                                }
                                // Allow branches to raw hex values, lets hope the user knows what he is doing
                                else if(Expression::stringToU8(tokens[tokenIndex], operand))
                                {
                                    operandValid = true;
                                    operand -= BRANCH_ADJUSTMENT;
                                }
                                else
                                {
                                    Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : label '%s' missing\n", filename.c_str(), _lineNumber+1, tokens[tokenIndex].c_str());
                                    return false;
                                }
                            }
                            // CALL+
                            else if(opcodeType == vCpu  &&  opcode0 == OPCODE_V_CALL)
                            {
                                // Search for call label
                                if(_callTablePtr  &&  operand != GIGA_V_AC)
                                {
                                    Label label;
                                    if(evaluateLabelOperand(tokens, tokenIndex, label, false))
                                    {
                                        // Search for address
                                        bool newLabel = true;
                                        uint16_t address = uint16_t(label._address);
                                        for(int i=0; i<int(_callTableEntries.size()); i++)
                                        {
                                            if(_callTableEntries[i]._address == address)
                                            {
                                                operandValid = true;
                                                operand = _callTableEntries[i]._operand;
                                                newLabel = false;
                                                break;
                                            }
                                        }

                                        // Found a new call address label, put it's address into the call table and point the call instruction to the call table
                                        if(newLabel)
                                        {
                                            operandValid = true;
                                            operand = uint8_t(LO_BYTE(_callTablePtr));
                                            CallTableEntry entry = {operand, address};
                                            _callTableEntries.push_back(entry);
                                            _callTablePtr -= 0x0002;

                                            // Avoid ONE_CONST_ADDRESS
                                            if(_callTablePtr == ONE_CONST_ADDRESS)
                                            {
                                                Cpu::reportError(Cpu::WarnError, stderr, "Assembler::assemble() : warning, (safe to ignore), Calltable : 0x%02x : collided with : 0x%02x : on line %d\n", _callTablePtr, 
                                                                                                                                                                                                          ONE_CONST_ADDRESS,
                                                                                                                                                                                                          _lineNumber+1);
                                                _callTablePtr -= 0x0002;
                                            }
                                            else if(_callTablePtr+1 == ONE_CONST_ADDRESS)
                                            {
                                                Cpu::reportError(Cpu::WarnError, stderr, "Assembler::assemble() : warning, (safe to ignore), Calltable : 0x%02x : collided with : 0x%02x : on line %d\n", _callTablePtr+1,
                                                                                                                                                                                                          ONE_CONST_ADDRESS,
                                                                                                                                                                                                          _lineNumber+1);
                                                _callTablePtr -= 0x0001;
                                            }
                                        }
                                    }
                                }
                                // CALL that doesn't use the call table, ('CALL GIGA_V_AC', usually to save zero page memory at the expense of code size and code speed).
                                else
                                {
                                    Equate equate;
                                    int commaTokenIndex = -1;
                                    if((operandValid = evaluateEquateOperand(tokens, tokenIndex, equate, commaTokenIndex, false)) == true)
                                    {
                                        operand = uint8_t(equate._operand);
                                    }
                                    else 
                                    {
                                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : label '%s' missing\n", filename.c_str(), _lineNumber+1, tokens[tokenIndex].c_str());
                                        return false;
                                    }
                                }
                            }
                            // PREFX1 instructions
                            else if(opcodeType == vCpu  &&  opcode0 == OPCODE_V_PREFX1)
                            {
                                operand = opcode1;
                                operandValid = true;
                            }

                            // All other non native 2 byte instructions
                            if(opcodeType != Native  &&  !operandValid)
                            {
                                Label label;
                                Equate equate;
                                int commaTokenIndex = -1;

                                // String
                                size_t quote1 = tokens[tokenIndex].find_first_of("'");
                                size_t quote2 = tokens[tokenIndex].find_last_of("'");
                                bool quotes = (quote1 != std::string::npos  &&  quote2 != std::string::npos  &&  (quote2 - quote1 > 1));
                                if(quotes)
                                {
                                    operand = sysHelper(opcodeStr, uint16_t(tokens[tokenIndex][quote1+1]), filename, _lineNumber+1);
                                }
                                // Search equates
                                else if((operandValid = evaluateEquateOperand(tokens, tokenIndex, equate, commaTokenIndex, compoundInstruction)) == true)
                                {
                                    operand = sysHelper(opcodeStr, uint16_t(equate._operand), filename, _lineNumber+1);
                                }
                                // Search labels
                                else if((operandValid = evaluateLabelOperand(tokens, tokenIndex, label, compoundInstruction)) == true)
                                {
                                    operand = sysHelper(opcodeStr, label._address, filename, _lineNumber+1);
                                }
                                else if(Expression::isExpression(tokens[tokenIndex]) == Expression::HasOperators)
                                {
                                    Expression::Numeric value;
                                    std::string input;
                                    preProcessExpression(tokens, tokenIndex, input, true);
                                    if(!Expression::parse(input, _lineNumber, value)) return false;
                                    operand = sysHelper(opcodeStr, uint16_t(std::lround(value._value)), filename, _lineNumber+1);
                                    operandValid = true;
                                }
                                else
                                {
                                    uint16_t operandU16 = 0;
                                    operandValid = Expression::stringToU16(tokens[tokenIndex], operandU16);
                                    if(!operandValid)
                                    {
                                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : invalid label/Equate '%s'\n", filename.c_str(), _lineNumber+1, tokens[tokenIndex].c_str());
                                        return false;
                                    }

                                    operand = sysHelper(opcodeStr, operandU16, filename, _lineNumber+1);
                                }
                            }

                            // Native instructions
                            if(opcodeType == Native)
                            {
                                if(!operandValid)
                                {
                                    if(!handleNativeInstruction(tokens, tokenIndex, opcode0, operand))
                                    {
                                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : native instruction '%s' is malformed\n", filename.c_str(), _lineNumber+1, lineToken._text.c_str());
                                        return false;
                                    }
                                }

                                instruction._isRomAddress = true;
                                instruction._opcode = opcode0;
                                instruction._operand0 = uint8_t(LO_BYTE(operand));
                                _instructions.push_back(instruction);
                                if(!checkInvalidAddress(ParseType(parse), _currentAddress, uint16_t(outputSize), instruction, lineToken, filename, _lineNumber)) return false;

#ifndef STAND_ALONE
                                uint16_t add = instruction._address>>1;
                                uint8_t opc = Cpu::getROM(add, 0);
                                uint8_t ope = Cpu::getROM(add, 1);
                                if(instruction._opcode != opc  ||  instruction._operand0 != ope)
                                {
                                    Cpu::reportError(Cpu::WarnError, stderr, "Assembler::assemble() : '%s:%d' : warning, ROM Native instruction mismatch : 0x%04X : ASM=0x%02X%02X : ROM=0x%02X%02X\n",
                                                                             filename.c_str(), _lineNumber+1, add, instruction._opcode, instruction._operand0, opc, ope);

                                    // Fix mismatched instruction?
                                    //instruction._opcode = opc;
                                    //instruction._operand0 = ope;
                                    //_instructions.back() = instruction;
                                }
#endif
                            }
                            // Reserved assembler opcode DB, (define byte)
                            else if(opcodeType == ReservedDB  ||  opcodeType == ReservedDBR)
                            {
                                // Push first operand
                                outputSize = OneByte;
                                instruction._isRomAddress = (opcodeType == ReservedDBR) ? true : false;
                                instruction._byteSize = ByteSize(outputSize);
                                instruction._opcode = uint8_t(LO_BYTE(operand));
                                if(defineOffset != 0)
                                {
                                    instruction._address = _currentAddress;
                                    instruction._isCustomAddress = true;
                                    _currentAddress += defineOffset;
                                }
                                _instructions.push_back(instruction);
    
                                // Push any remaining operands
                                if(tokenIndex + 1 < int(tokens.size()))
                                {
                                    if(!handleDefineByte(tokens, tokenIndex, instruction, true, defineOffset, outputSize))
                                    {
                                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : bad DB data '%s'\n", filename.c_str(), _lineNumber+1, lineToken._text.c_str());
                                        return false;
                                    }
                                }

                                if(!checkInvalidAddress(ParseType(parse), _currentAddress, uint16_t(outputSize), instruction, lineToken, filename, _lineNumber)) return false;
                            }
                            // Normal instructions
                            else
                            {
                                instruction._operand0 = operand;
                                _instructions.push_back(instruction);
                                if(!checkInvalidAddress(ParseType(parse), _currentAddress, uint16_t(outputSize), instruction, lineToken, filename, _lineNumber)) return false;
                            }
                        }
                        break;
                            
                        case ThreeBytes:
                        {
                            // BCC
                            if(opcodeType == vCpu  &&  opcode0 == OPCODE_V_BCC)
                            {
                                // Label/equate operand
                                uint16_t operand = 0x0000;
                                if(!validateOperand(tokens, tokenIndex, compoundInstruction, operand))
                                {
                                    Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'BCC' invalid label/equate '%s'\n", filename.c_str(), _lineNumber+1, tokens[tokenIndex].c_str());
                                    return false;
                                }

                                instruction._operand0 = opcode1;
                                instruction._operand1 = uint8_t(operand) - BRANCH_ADJUSTMENT;
                                _instructions.push_back(instruction);
                                if(!checkInvalidAddress(ParseType(parse), _currentAddress, uint16_t(outputSize), instruction, lineToken, filename, _lineNumber)) return false;
                            }
                            // PREFX2 instructions
                            else if(opcodeType == vCpu  &&  opcode0 == OPCODE_V_PREFX2)
                            {
                                uint8_t operand = 0x00;
                                Label label;
                                Equate equate;
                                int commaTokenIndex = -1;

                                // Search equates
                                if((operandValid = evaluateEquateOperand(tokens, tokenIndex, equate, commaTokenIndex, compoundInstruction)) == true)
                                {
                                    operand = uint8_t(equate._operand);
                                }
                                // Search labels
                                else if((operandValid = evaluateLabelOperand(tokens, tokenIndex, label, compoundInstruction)) == true)
                                {
                                    operand = uint8_t(label._address);
                                }
                                else if(Expression::isExpression(tokens[tokenIndex]) == Expression::HasOperators)
                                {
                                    Expression::Numeric value;
                                    std::string input;
                                    preProcessExpression(tokens, tokenIndex, input, true);
                                    if(!Expression::parse((char*)input.c_str(), _lineNumber, value)) return false;
                                    operand = uint8_t(std::lround(value._value));
                                    operandValid = true;
                                }
                                else
                                {
                                    operandValid = Expression::stringToU8(tokens[tokenIndex], operand);
                                    if(!operandValid)
                                    {
                                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : invalid label/equate '%s'\n", filename.c_str(), _lineNumber+1, tokens[tokenIndex].c_str());
                                        return false;
                                    }
                                }

                                // PREFIX ARG0 OPCODE
                                instruction._operand0 = operand;
                                instruction._operand1 = opcode1;
                                _instructions.push_back(instruction);
                                if(!checkInvalidAddress(ParseType(parse), _currentAddress, uint16_t(outputSize), instruction, lineToken, filename, _lineNumber)) return false;
                            }
                            // Special case MOVQB as it has multiple parseable tokens
                            else if(opcodeType == vCpu  &&  opcode0 == OPCODE_V_MOVQB)
                            {
                                if(tokens.size() < 3)
                                {
                                    Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'MOVQB <zero page var>, <constant 0..255>' expects 2 parameters '%s'\n", filename.c_str(), _lineNumber+1,
                                                                                                                                                                                        tokens[tokenIndex].c_str());
                                    return false;
                                }

                                Equate equate;
                                uint8_t equates[2] = {0x00};
                                int commaTokenIndex = -1;
                                for(int i=0; i<2; i++)
                                {
                                    int tokIdx = (commaTokenIndex >= 0) ? commaTokenIndex + 1 : tokenIndex + i;
                                    if(tokIdx >= int(tokens.size()))
                                    {
                                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'MOVQB' syntax error in '%s'\n", filename.c_str(), _lineNumber+1, lineToken._text.c_str());
                                        return false;
                                    }

                                    // Search equates
                                    if((operandValid = evaluateEquateOperand(tokens, tokIdx, equate, commaTokenIndex, compoundInstruction)) == true)
                                    {
                                        equates[i] = uint8_t(equate._operand);
                                    }
                                    // Can be a literal byte
                                    else
                                    {
                                        operandValid = Expression::stringToU8(tokens[tokIdx], equates[i]);
                                    }
                                    if(!operandValid)
                                    {
                                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'MOVQB' invalid equate/literal '%s'\n", filename.c_str(), _lineNumber+1, tokens[tokIdx].c_str());
                                        return false;
                                    }
                                }

                                instruction._operand0 = equates[1]; // native code expects the operands swapped for MOVQB
                                instruction._operand1 = equates[0];
                                _instructions.push_back(instruction);
                                if(!checkInvalidAddress(ParseType(parse), _currentAddress, uint16_t(outputSize), instruction, lineToken, filename, _lineNumber)) return false;
                            }
                            // Special case MOVB as it has multiple parseable tokens
                            else if(opcodeType == vCpu  &&  opcode0 == OPCODE_V_MOVB)
                            {
                                if(tokens.size() < 3)
                                {
                                    Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'MOVB <src zero page var>, <dst zero page var>' expects 2 parameters '%s'\n", filename.c_str(), _lineNumber+1,
                                                                                                                                                                                             tokens[tokenIndex].c_str());
                                    return false;
                                }

                                Equate equate;
                                uint8_t equates[2] = {0x00};
                                int commaTokenIndex = -1;
                                for(int i=0; i<2; i++)
                                {
                                    int tokIdx = (commaTokenIndex >= 0) ? commaTokenIndex + 1 : tokenIndex + i;
                                    if(tokIdx >= int(tokens.size()))
                                    {
                                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'MOVB' syntax error in '%s'\n", filename.c_str(), _lineNumber+1, lineToken._text.c_str());
                                        return false;
                                    }

                                    // Search equates
                                    if((operandValid = evaluateEquateOperand(tokens, tokIdx, equate, commaTokenIndex, compoundInstruction)) == true)
                                    {
                                        equates[i] = uint8_t(equate._operand);
                                    }
                                    // Can be a literal byte
                                    else
                                    {
                                        operandValid = Expression::stringToU8(tokens[tokIdx], equates[i]);
                                    }
                                    if(!operandValid)
                                    {
                                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'MOVB' invalid equate/literal '%s'\n", filename.c_str(), _lineNumber+1, tokens[tokIdx].c_str());
                                        return false;
                                    }
                                }

                                instruction._operand0 = equates[1]; // native code expects the operands swapped for MOVB
                                instruction._operand1 = equates[0];
                                _instructions.push_back(instruction);
                                if(!checkInvalidAddress(ParseType(parse), _currentAddress, uint16_t(outputSize), instruction, lineToken, filename, _lineNumber)) return false;
                            }
                            // Special case MOVQW as it has multiple parseable tokens
                            else if(opcodeType == vCpu  &&  opcode0 == OPCODE_V_MOVQW)
                            {
                                if(tokens.size() < 3)
                                {
                                    Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'MOVQW <zero page var>, <constant 0..255>' expects 2 parameters '%s'\n", filename.c_str(), _lineNumber+1,
                                                                                                                                                                                        tokens[tokenIndex].c_str());
                                    return false;
                                }

                                Equate equate;
                                uint8_t equates[2] = {0x00};
                                int commaTokenIndex = -1;
                                for(int i=0; i<2; i++)
                                {
                                    int tokIdx = (commaTokenIndex >= 0) ? commaTokenIndex + 1 : tokenIndex + i;
                                    if(tokIdx >= int(tokens.size()))
                                    {
                                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'MOVQW' syntax error in '%s'\n", filename.c_str(), _lineNumber+1, lineToken._text.c_str());
                                        return false;
                                    }

                                    // Search equates
                                    if((operandValid = evaluateEquateOperand(tokens, tokIdx, equate, commaTokenIndex, compoundInstruction)) == true)
                                    {
                                        equates[i] = uint8_t(equate._operand);
                                    }
                                    // Can be a literal byte
                                    else
                                    {
                                        operandValid = Expression::stringToU8(tokens[tokIdx], equates[i]);
                                    }
                                    if(!operandValid)
                                    {
                                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'MOVQW' invalid equate/literal '%s'\n", filename.c_str(), _lineNumber+1, tokens[tokIdx].c_str());
                                        return false;
                                    }
                                }

                                instruction._operand0 = equates[1]; // native code expects the operands swapped for MOVQW
                                instruction._operand1 = equates[0];
                                _instructions.push_back(instruction);
                                if(!checkInvalidAddress(ParseType(parse), _currentAddress, uint16_t(outputSize), instruction, lineToken, filename, _lineNumber)) return false;
                            }
                            // Special case MOVWA as it has multiple parseable tokens
                            else if(opcodeType == vCpu  &&  opcode0 == OPCODE_V_MOVWA)
                            {
                                if(tokens.size() < 3)
                                {
                                    Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'MOVWA <src zero page var>, <dst zero page var>' expects 2 parameters '%s'\n", filename.c_str(), _lineNumber+1,
                                                                                                                                                                                              tokens[tokenIndex].c_str());
                                    return false;
                                }

                                Equate equate;
                                uint8_t equates[2] = {0x00};
                                int commaTokenIndex = -1;
                                for(int i=0; i<2; i++)
                                {
                                    int tokIdx = (commaTokenIndex >= 0) ? commaTokenIndex + 1 : tokenIndex + i;
                                    if(tokIdx >= int(tokens.size()))
                                    {
                                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'MOVWA' syntax error in '%s'\n", filename.c_str(), _lineNumber+1, lineToken._text.c_str());
                                        return false;
                                    }

                                    // Search equates
                                    if((operandValid = evaluateEquateOperand(tokens, tokIdx, equate, commaTokenIndex, compoundInstruction)) == true)
                                    {
                                        equates[i] = uint8_t(equate._operand);
                                    }
                                    // Can be a literal byte
                                    else
                                    {
                                        operandValid = Expression::stringToU8(tokens[tokIdx], equates[i]);
                                    }
                                    if(!operandValid)
                                    {
                                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'MOVWA' invalid equate/literal '%s'\n", filename.c_str(), _lineNumber+1, tokens[tokIdx].c_str());
                                        return false;
                                    }
                                }

                                instruction._operand0 = equates[1]; // native code expects the operands swapped for MOVWA
                                instruction._operand1 = equates[0];
                                _instructions.push_back(instruction);
                                if(!checkInvalidAddress(ParseType(parse), _currentAddress, uint16_t(outputSize), instruction, lineToken, filename, _lineNumber)) return false;
                            }
                            // Special case DBNE as it has multiple parseable tokens
                            else if(opcodeType == vCpu  &&  opcode0 == OPCODE_V_DBNE)
                            {
                                if(tokens.size() < 3)
                                {
                                    Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'DBNE <zero page var>, <label>' expects 2 parameters '%s'\n", filename.c_str(), _lineNumber+1,
                                                                                                                                                                             tokens[tokenIndex].c_str());
                                    return false;
                                }

                                Label label;
                                Equate equate;
                                int commaTokenIndex = -1;
                                uint8_t zpreg = 0x00;

                                // Search equates
                                if((operandValid = evaluateEquateOperand(tokens, tokenIndex, equate, commaTokenIndex, compoundInstruction)) == true)
                                {
                                    zpreg = uint8_t(equate._operand);
                                }
                                // Can be a literal byte
                                else
                                {
                                    operandValid = Expression::stringToU8(tokens[tokenIndex], zpreg);
                                }
                                if(!operandValid)
                                {
                                    Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'DBNE' invalid zero page var '%s'\n", filename.c_str(), _lineNumber+1, tokens[tokenIndex].c_str());
                                    return false;
                                }

                                int tokIdx = (commaTokenIndex >= 0) ? commaTokenIndex + 1 : tokenIndex + 1;
                                if(tokIdx >= int(tokens.size()))
                                {
                                    Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'DBNE' syntax error in '%s'\n", filename.c_str(), _lineNumber+1, lineToken._text.c_str());
                                    return false;
                                }

                                // Label/equate operand
                                uint16_t operand = 0x0000;
                                if(!validateOperand(tokens, tokIdx, compoundInstruction, operand))
                                {
                                    Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'DBNE' invalid label/equate '%s'\n", filename.c_str(), _lineNumber+1, tokens[tokenIndex].c_str());
                                    return false;
                                }

                                instruction._operand0 = uint8_t(operand) - BRANCH_ADJUSTMENT; // native code expects the operands swapped for DBNE
                                instruction._operand1 = zpreg;
                                _instructions.push_back(instruction);
                                if(!checkInvalidAddress(ParseType(parse), _currentAddress, uint16_t(outputSize), instruction, lineToken, filename, _lineNumber)) return false;
                            }
                            // Special case DBGE as it has multiple parseable tokens
                            else if(opcodeType == vCpu  &&  opcode0 == OPCODE_V_DBGE)
                            {
                                if(tokens.size() < 3)
                                {
                                    Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'DBGE <zero page var>, <label>' expects 2 parameters '%s'\n", filename.c_str(), _lineNumber+1,
                                                                                                                                                                             tokens[tokenIndex].c_str());
                                    return false;
                                }

                                Label label;
                                Equate equate;
                                int commaTokenIndex = -1;
                                uint8_t zpreg = 0x00;

                                // Search equates
                                if((operandValid = evaluateEquateOperand(tokens, tokenIndex, equate, commaTokenIndex, compoundInstruction)) == true)
                                {
                                    zpreg = uint8_t(equate._operand);
                                }
                                // Can be a literal byte
                                else
                                {
                                    operandValid = Expression::stringToU8(tokens[tokenIndex], zpreg);
                                }
                                if(!operandValid)
                                {
                                    Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'DBGE' invalid zero page var '%s'\n", filename.c_str(), _lineNumber+1, tokens[tokenIndex].c_str());
                                    return false;
                                }

                                int tokIdx = (commaTokenIndex >= 0) ? commaTokenIndex + 1 : tokenIndex + 1;
                                if(tokIdx >= int(tokens.size()))
                                {
                                    Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'DBGE' syntax error in '%s'\n", filename.c_str(), _lineNumber+1, lineToken._text.c_str());
                                    return false;
                                }

                                // Label/equate operand
                                uint16_t operand = 0x0000;
                                if(!validateOperand(tokens, tokIdx, compoundInstruction, operand))
                                {
                                    Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'DBGE' invalid label/equate '%s'\n", filename.c_str(), _lineNumber+1, tokens[tokenIndex].c_str());
                                    return false;
                                }

                                instruction._operand0 = uint8_t(operand) - BRANCH_ADJUSTMENT; // native code expects the operands swapped for DBGE
                                instruction._operand1 = zpreg;
                                _instructions.push_back(instruction);
                                if(!checkInvalidAddress(ParseType(parse), _currentAddress, uint16_t(outputSize), instruction, lineToken, filename, _lineNumber)) return false;
                            }
                            // Special case CMPI as it has multiple parseable tokens
                            else if(opcodeType == vCpu  &&  opcode0 == OPCODE_V_CMPI)
                            {
                                if(tokens.size() < 3)
                                {
                                    Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'CMPI <zero page var>, <constant 0..255>' expects 2 parameters '%s'\n", filename.c_str(), _lineNumber+1,
                                                                                                                                                                                       tokens[tokenIndex].c_str());
                                    return false;
                                }

                                Equate equate;
                                uint8_t equates[2] = {0x00};
                                int commaTokenIndex = -1;
                                for(int i=0; i<2; i++)
                                {
                                    int tokIdx = (commaTokenIndex >= 0) ? commaTokenIndex + 1 : tokenIndex + i;
                                    if(tokIdx >= int(tokens.size()))
                                    {
                                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'CMPI' syntax error in '%s'\n", filename.c_str(), _lineNumber+1, lineToken._text.c_str());
                                        return false;
                                    }

                                    // Search equates
                                    if((operandValid = evaluateEquateOperand(tokens, tokIdx, equate, commaTokenIndex, compoundInstruction)) == true)
                                    {
                                        equates[i] = uint8_t(equate._operand);
                                    }
                                    // Can be a literal byte
                                    else
                                    {
                                        operandValid = Expression::stringToU8(tokens[tokIdx], equates[i]);
                                    }
                                    if(!operandValid)
                                    {
                                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'CMPI' invalid equate/literal '%s'\n", filename.c_str(), _lineNumber+1, tokens[tokIdx].c_str());
                                        return false;
                                    }
                                }

                                instruction._operand0 = equates[1]; // native code expects the operands swapped for CMPI
                                instruction._operand1 = equates[0];
                                _instructions.push_back(instruction);
                                if(!checkInvalidAddress(ParseType(parse), _currentAddress, uint16_t(outputSize), instruction, lineToken, filename, _lineNumber)) return false;
                            }
                            // Special case PACKAW as it has multiple parseable tokens
                            else if(opcodeType == vCpu  &&  opcode0 == OPCODE_V_PACKAW)
                            {
                                if(tokens.size() < 3)
                                {
                                    Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'PACKAW <src0 zero page var>, <src1 zero page var>' expects 2 parameters '%s'\n", filename.c_str(), _lineNumber+1,
                                                                                                                                                                                                 tokens[tokenIndex].c_str());
                                    return false;
                                }

                                Equate equate;
                                uint8_t equates[2] = {0x00};
                                int commaTokenIndex = -1;
                                for(int i=0; i<2; i++)
                                {
                                    int tokIdx = (commaTokenIndex >= 0) ? commaTokenIndex + 1 : tokenIndex + i;
                                    if(tokIdx >= int(tokens.size()))
                                    {
                                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'PACKAW' syntax error in '%s'\n", filename.c_str(), _lineNumber+1, lineToken._text.c_str());
                                        return false;
                                    }

                                    // Search equates
                                    if((operandValid = evaluateEquateOperand(tokens, tokIdx, equate, commaTokenIndex, compoundInstruction)) == true)
                                    {
                                        equates[i] = uint8_t(equate._operand);
                                    }
                                    // Can be a literal byte
                                    else
                                    {
                                        operandValid = Expression::stringToU8(tokens[tokIdx], equates[i]);
                                    }
                                    if(!operandValid)
                                    {
                                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'PACKAW' invalid equate/literal '%s'\n", filename.c_str(), _lineNumber+1, tokens[tokIdx].c_str());
                                        return false;
                                    }
                                }

                                instruction._operand0 = equates[1]; // native code expects the operands swapped for PACKAW
                                instruction._operand1 = equates[0];
                                _instructions.push_back(instruction);
                                if(!checkInvalidAddress(ParseType(parse), _currentAddress, uint16_t(outputSize), instruction, lineToken, filename, _lineNumber)) return false;
                            }
                            // Special case JCC
                            else if(opcodeType == vCpu  &&  (opcode0 == OPCODE_V_JEQ  ||  opcode0 == OPCODE_V_JNE  ||  opcode0 == OPCODE_V_JLT  ||  opcode0 == OPCODE_V_JGT  ||  opcode0 == OPCODE_V_JLE  ||  opcode0 == OPCODE_V_JGE))
                            {
                                if(tokens.size() < 2)
                                {
                                    Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'JCC <label>' expects 1 parameter '%s'\n", filename.c_str(), _lineNumber+1, tokens[tokenIndex].c_str());
                                    return false;
                                }

                                // Label/equate operand
                                uint16_t operand = 0x0000;
                                if(!validateOperand(tokens, tokenIndex, compoundInstruction, operand))
                                {
                                    Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'JCC' invalid label/equate '%s'\n", filename.c_str(), _lineNumber+1, tokens[tokenIndex].c_str());
                                    return false;
                                }

                                instruction._operand0 = uint8_t(LO_BYTE(operand)) - BRANCH_ADJUSTMENT;
                                instruction._operand1 = uint8_t(HI_BYTE(operand));
                                _instructions.push_back(instruction);
                                if(!checkInvalidAddress(ParseType(parse), _currentAddress, uint16_t(outputSize), instruction, lineToken, filename, _lineNumber)) return false;
                            }
                            // All other 3 byte instructions
                            else
                            {
                                uint16_t operand = 0x0000;
                                Label label;
                                Equate equate;
                                int commaTokenIndex = -1;

                                // Search equates
                                if((operandValid = evaluateEquateOperand(tokens, tokenIndex, equate, commaTokenIndex, compoundInstruction)) == true)
                                {
                                    operand = uint16_t(equate._operand);
                                }
                                // Search labels
                                else if((operandValid = evaluateLabelOperand(tokens, tokenIndex, label, compoundInstruction)) == true)
                                {
                                    operand = label._address;
                                }
                                else if(Expression::isExpression(tokens[tokenIndex]) == Expression::HasOperators)
                                {
                                    Expression::Numeric value;
                                    std::string input;
                                    preProcessExpression(tokens, tokenIndex, input, true);
                                    if(!Expression::parse((char*)input.c_str(), _lineNumber, value)) return false;
                                    operand = int16_t(std::lround(value._value));
                                    operandValid = true;
                                }
                                else
                                {
                                    operandValid = Expression::stringToU16(tokens[tokenIndex], operand);
                                    if(!operandValid)
                                    {
                                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : invalid label/equate '%s'\n", filename.c_str(), _lineNumber+1, tokens[tokenIndex].c_str());
                                        return false;
                                    }
                                }
                                
                                // Reserved assembler opcode DW, (define word)
                                if(opcodeType == ReservedDW  ||  opcodeType == ReservedDWR)
                                {
                                    // Push first operand
                                    outputSize = TwoBytes;
                                    instruction._isRomAddress = (opcodeType == ReservedDWR) ? true : false;
                                    instruction._byteSize = ByteSize(outputSize);
                                    instruction._opcode   = uint8_t(LO_BYTE(operand));
                                    instruction._operand0 = uint8_t(HI_BYTE(operand));
                                    if(defineOffset != 0)
                                    {
                                        instruction._address = _currentAddress;
                                        instruction._isCustomAddress = true;
                                        _currentAddress += defineOffset * 2;
                                    }
                                    _instructions.push_back(instruction);

                                    // Push any remaining operands
                                    if(tokenIndex + 1 < int(tokens.size())) handleDefineWord(tokens, tokenIndex, instruction, true, defineOffset, outputSize);
                                    if(!checkInvalidAddress(ParseType(parse), _currentAddress, uint16_t(outputSize), instruction, lineToken, filename, _lineNumber)) return false;
                                }
                                // Normal instructions
                                else
                                {
                                    instruction._operand0 = uint8_t(LO_BYTE(operand));
                                    instruction._operand1 = uint8_t(HI_BYTE(operand));

                                    // DOKEI has it's operands swapped
                                    if(opcodeType == vCpu  &&  opcode0 == OPCODE_V_DOKEI)
                                    {
                                        std::swap(instruction._operand0, instruction._operand1);
                                    }

                                    _instructions.push_back(instruction);
                                    if(!checkInvalidAddress(ParseType(parse), _currentAddress, uint16_t(instruction._byteSize), instruction, lineToken, filename, _lineNumber)) return false;
                                }
                            }
                        }
                        break;

                        case FourBytes:
                        {
                            // PREFX3 instructions
                            if(opcodeType == vCpu  &&  opcode0 == OPCODE_V_PREFX3)
                            {
                                uint16_t operand = 0x0000;
                                Label label;
                                Equate equate;
                                int commaTokenIndex = -1;

                                // Special case XCHGB as it has multiple parseable tokens
                                if(opcode1 == OPCODE_V_XCHGB)
                                {
                                    if(tokens.size() < 3)
                                    {
                                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'XCHGB <zero page var>, <zero page var>' expects 2 parameters '%s'\n", filename.c_str(), _lineNumber+1,
                                                                                                                                                                                          tokens[tokenIndex].c_str());
                                        return false;
                                    }

                                    uint8_t equates[2] = {0x00};
                                    for(int i=0; i<2; i++)
                                    {
                                        int tokIdx = (commaTokenIndex >= 0) ? commaTokenIndex + 1 : tokenIndex + i;
                                        if(tokIdx >= int(tokens.size()))
                                        {
                                            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'XCHGB' syntax error in '%s'\n", filename.c_str(), _lineNumber+1, lineToken._text.c_str());
                                            return false;
                                        }

                                        // Search equates
                                        if((operandValid = evaluateEquateOperand(tokens, tokIdx, equate, commaTokenIndex, compoundInstruction)) == true)
                                        {
                                            equates[i] = uint8_t(equate._operand);
                                        }
                                        // Can be a literal byte
                                        else
                                        {
                                            operandValid = Expression::stringToU8(tokens[tokIdx], equates[i]);
                                        }
                                        if(!operandValid)
                                        {
                                            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'XCHGB' invalid equate/literal '%s'\n", filename.c_str(), _lineNumber+1, tokens[tokIdx].c_str());
                                            return false;
                                        }
                                    }

                                    // PREFIX ARG1 OPCODE ARG0
                                    instruction._operand0 = equates[1];
                                    instruction._operand1 = opcode1;
                                    instruction._operand2 = equates[0];
                                    _instructions.push_back(instruction);
                                    if(!checkInvalidAddress(ParseType(parse), _currentAddress, uint16_t(outputSize), instruction, lineToken, filename, _lineNumber)) return false;
                                }
                                // Special case MOVW as it has multiple parseable tokens
                                else if(opcode1 == OPCODE_V_MOVW)
                                {
                                    if(tokens.size() < 3)
                                    {
                                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'MOVW <src zero page var>, <dst zero page var>' expects 2 parameters '%s'\n", filename.c_str(), _lineNumber+1,
                                                                                                                                                                                                 tokens[tokenIndex].c_str());
                                        return false;
                                    }

                                    uint8_t equates[2] = {0x00};
                                    for(int i=0; i<2; i++)
                                    {
                                        int tokIdx = (commaTokenIndex >= 0) ? commaTokenIndex + 1 : tokenIndex + i;
                                        if(tokIdx >= int(tokens.size()))
                                        {
                                            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'MOVW' syntax error in '%s'\n", filename.c_str(), _lineNumber+1, lineToken._text.c_str());
                                            return false;
                                        }

                                        // Search equates
                                        if((operandValid = evaluateEquateOperand(tokens, tokIdx, equate, commaTokenIndex, compoundInstruction)) == true)
                                        {
                                            equates[i] = uint8_t(equate._operand);
                                        }
                                        // Can be a literal byte
                                        else
                                        {
                                            operandValid = Expression::stringToU8(tokens[tokIdx], equates[i]);
                                        }
                                        if(!operandValid)
                                        {
                                            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'MOVW' invalid equate/literal '%s'\n", filename.c_str(), _lineNumber+1, tokens[tokIdx].c_str());
                                            return false;
                                        }
                                    }

                                    // PREFIX ARG1 OPCODE ARG0
                                    instruction._operand0 = equates[1];
                                    instruction._operand1 = opcode1;
                                    instruction._operand2 = equates[0];
                                    _instructions.push_back(instruction);
                                    if(!checkInvalidAddress(ParseType(parse), _currentAddress, uint16_t(outputSize), instruction, lineToken, filename, _lineNumber)) return false;
                                }
                                // Special case FNT6X8 as it has multiple parseable tokens
                                else if(opcode1 == OPCODE_V_FNT6X8)
                                {
                                    if(tokens.size() < 3)
                                    {
                                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'FNT6X8 <textFont var>, <textChr var>' expects 2 parameters '%s'\n", filename.c_str(), _lineNumber+1,
                                                                                                                                                                                        tokens[tokenIndex].c_str());
                                        return false;
                                    }

                                    uint8_t equates[2] = {0x00};
                                    for(int i=0; i<2; i++)
                                    {
                                        int tokIdx = (commaTokenIndex >= 0) ? commaTokenIndex + 1 : tokenIndex + i;
                                        if(tokIdx >= int(tokens.size()))
                                        {
                                            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'FNT6X8' syntax error in '%s'\n", filename.c_str(), _lineNumber+1, lineToken._text.c_str());
                                            return false;
                                        }

                                        // Search equates
                                        if((operandValid = evaluateEquateOperand(tokens, tokIdx, equate, commaTokenIndex, compoundInstruction)) == true)
                                        {
                                            equates[i] = uint8_t(equate._operand);
                                        }
                                        // Can be a literal byte
                                        else
                                        {
                                            operandValid = Expression::stringToU8(tokens[tokIdx], equates[i]);
                                        }
                                        if(!operandValid)
                                        {
                                            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'FNT6X8' invalid equate/literal '%s'\n", filename.c_str(), _lineNumber+1, tokens[tokIdx].c_str());
                                            return false;
                                        }
                                    }

                                    // PREFIX ARG0 OPCODE ARG1
                                    instruction._operand0 = equates[0];
                                    instruction._operand1 = opcode1;
                                    instruction._operand2 = equates[1];
                                    _instructions.push_back(instruction);
                                    if(!checkInvalidAddress(ParseType(parse), _currentAddress, uint16_t(outputSize), instruction, lineToken, filename, _lineNumber)) return false;
                                }
                                // Special case FNT4X6 as it has multiple parseable tokens
                                else if(opcode1 == OPCODE_V_FNT4X6)
                                {
                                    if(tokens.size() < 3)
                                    {
                                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'FNT4X6 <textFont var>, <textChr var>' expects 2 parameters '%s'\n", filename.c_str(), _lineNumber+1,
                                                                                                                                                                                        tokens[tokenIndex].c_str());
                                        return false;
                                    }

                                    uint8_t equates[2] = {0x00};
                                    for(int i=0; i<2; i++)
                                    {
                                        int tokIdx = (commaTokenIndex >= 0) ? commaTokenIndex + 1 : tokenIndex + i;
                                        if(tokIdx >= int(tokens.size()))
                                        {
                                            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'FNT4X6' syntax error in '%s'\n", filename.c_str(), _lineNumber+1, lineToken._text.c_str());
                                            return false;
                                        }

                                        // Search equates
                                        if((operandValid = evaluateEquateOperand(tokens, tokIdx, equate, commaTokenIndex, compoundInstruction)) == true)
                                        {
                                            equates[i] = uint8_t(equate._operand);
                                        }
                                        // Can be a literal byte
                                        else
                                        {
                                            operandValid = Expression::stringToU8(tokens[tokIdx], equates[i]);
                                        }
                                        if(!operandValid)
                                        {
                                            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'FNT4X6' invalid equate/literal '%s'\n", filename.c_str(), _lineNumber+1, tokens[tokIdx].c_str());
                                            return false;
                                        }
                                    }

                                    // PREFIX ARG0 OPCODE ARG1
                                    instruction._operand0 = equates[0];
                                    instruction._operand1 = opcode1;
                                    instruction._operand2 = equates[1];
                                    _instructions.push_back(instruction);
                                    if(!checkInvalidAddress(ParseType(parse), _currentAddress, uint16_t(outputSize), instruction, lineToken, filename, _lineNumber)) return false;
                                }
                                // Special case CONDII as it has multiple parseable tokens
                                else if(opcode1 == OPCODE_V_CONDII)
                                {
                                    if(tokens.size() < 3)
                                    {
                                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'CONDII <immediate 0>, <immediate 1>' expects 2 parameters '%s'\n", filename.c_str(), _lineNumber+1,
                                                                                                                                                                                       tokens[tokenIndex].c_str());
                                        return false;
                                    }

                                    uint8_t equates[2] = {0x00};
                                    for(int i=0; i<2; i++)
                                    {
                                        int tokIdx = (commaTokenIndex >= 0) ? commaTokenIndex + 1 : tokenIndex + i;
                                        if(tokIdx >= int(tokens.size()))
                                        {
                                            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'CONDII' syntax error in '%s'\n", filename.c_str(), _lineNumber+1, lineToken._text.c_str());
                                            return false;
                                        }

                                        // Search equates
                                        if((operandValid = evaluateEquateOperand(tokens, tokIdx, equate, commaTokenIndex, compoundInstruction)) == true)
                                        {
                                            equates[i] = uint8_t(equate._operand);
                                        }
                                        // Can be a literal byte
                                        else
                                        {
                                            operandValid = Expression::stringToU8(tokens[tokIdx], equates[i]);
                                        }
                                        if(!operandValid)
                                        {
                                            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'CONDII' invalid equate/literal '%s'\n", filename.c_str(), _lineNumber+1, tokens[tokIdx].c_str());
                                            return false;
                                        }
                                    }

                                    // PREFIX ARG1 OPCODE ARG0
                                    instruction._operand0 = equates[1];
                                    instruction._operand1 = opcode1;
                                    instruction._operand2 = equates[0];
                                    _instructions.push_back(instruction);
                                    if(!checkInvalidAddress(ParseType(parse), _currentAddress, uint16_t(outputSize), instruction, lineToken, filename, _lineNumber)) return false;
                                }
                                // Special case CONDBB as it has multiple parseable tokens
                                else if(opcode1 == OPCODE_V_CONDBB)
                                {
                                    if(tokens.size() < 3)
                                    {
                                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'CONDBB <src zero page var>, <dst zero page var>' expects 2 parameters '%s'\n", filename.c_str(), _lineNumber+1,
                                                                                                                                                                                                   tokens[tokenIndex].c_str());
                                        return false;
                                    }

                                    uint8_t equates[2] = {0x00};
                                    for(int i=0; i<2; i++)
                                    {
                                        int tokIdx = (commaTokenIndex >= 0) ? commaTokenIndex + 1 : tokenIndex + i;
                                        if(tokIdx >= int(tokens.size()))
                                        {
                                            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'CONDBB' syntax error in '%s'\n", filename.c_str(), _lineNumber+1, lineToken._text.c_str());
                                            return false;
                                        }

                                        // Search equates
                                        if((operandValid = evaluateEquateOperand(tokens, tokIdx, equate, commaTokenIndex, compoundInstruction)) == true)
                                        {
                                            equates[i] = uint8_t(equate._operand);
                                        }
                                        // Can be a literal byte
                                        else
                                        {
                                            operandValid = Expression::stringToU8(tokens[tokIdx], equates[i]);
                                        }
                                        if(!operandValid)
                                        {
                                            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'CONDBB' invalid equate/literal '%s'\n", filename.c_str(), _lineNumber+1, tokens[tokIdx].c_str());
                                            return false;
                                        }
                                    }

                                    // PREFIX ARG1 OPCODE ARG0
                                    instruction._operand0 = equates[1];
                                    instruction._operand1 = opcode1;
                                    instruction._operand2 = equates[0];
                                    _instructions.push_back(instruction);
                                    if(!checkInvalidAddress(ParseType(parse), _currentAddress, uint16_t(outputSize), instruction, lineToken, filename, _lineNumber)) return false;
                                }
                                // Special case CONDIB as it has multiple parseable tokens
                                else if(opcode1 == OPCODE_V_CONDIB)
                                {
                                    if(tokens.size() < 3)
                                    {
                                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'CONDIB <constant 0..255>, <zero page var>' expects 2 parameters '%s'\n", filename.c_str(), _lineNumber+1,
                                                                                                                                                                                             tokens[tokenIndex].c_str());
                                        return false;
                                    }

                                    uint8_t equates[2] = {0x00};
                                    for(int i=0; i<2; i++)
                                    {
                                        int tokIdx = (commaTokenIndex >= 0) ? commaTokenIndex + 1 : tokenIndex + i;
                                        if(tokIdx >= int(tokens.size()))
                                        {
                                            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'CONDIB' syntax error in '%s'\n", filename.c_str(), _lineNumber+1, lineToken._text.c_str());
                                            return false;
                                        }

                                        // Search equates
                                        if((operandValid = evaluateEquateOperand(tokens, tokIdx, equate, commaTokenIndex, compoundInstruction)) == true)
                                        {
                                            equates[i] = uint8_t(equate._operand);
                                        }
                                        // Can be a literal byte
                                        else
                                        {
                                            operandValid = Expression::stringToU8(tokens[tokIdx], equates[i]);
                                        }
                                        if(!operandValid)
                                        {
                                            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'CONDIB' invalid equate/literal '%s'\n", filename.c_str(), _lineNumber+1, tokens[tokIdx].c_str());
                                            return false;
                                        }
                                    }

                                    // PREFIX ARG1 OPCODE ARG0
                                    instruction._operand0 = equates[1];
                                    instruction._operand1 = opcode1;
                                    instruction._operand2 = equates[0];
                                    _instructions.push_back(instruction);
                                    if(!checkInvalidAddress(ParseType(parse), _currentAddress, uint16_t(outputSize), instruction, lineToken, filename, _lineNumber)) return false;
                                }
                                // Special case CONDBI as it has multiple parseable tokens
                                else if(opcode1 == OPCODE_V_CONDBI)
                                {
                                    if(tokens.size() < 3)
                                    {
                                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'CONDBI <zero page var>, <constant 0..255>' expects 2 parameters '%s'\n", filename.c_str(), _lineNumber+1,
                                                                                                                                                                                             tokens[tokenIndex].c_str());
                                        return false;
                                    }

                                    uint8_t equates[2] = {0x00};
                                    for(int i=0; i<2; i++)
                                    {
                                        int tokIdx = (commaTokenIndex >= 0) ? commaTokenIndex + 1 : tokenIndex + i;
                                        if(tokIdx >= int(tokens.size()))
                                        {
                                            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'CONDBI' syntax error in '%s'\n", filename.c_str(), _lineNumber+1, lineToken._text.c_str());
                                            return false;
                                        }

                                        // Search equates
                                        if((operandValid = evaluateEquateOperand(tokens, tokIdx, equate, commaTokenIndex, compoundInstruction)) == true)
                                        {
                                            equates[i] = uint8_t(equate._operand);
                                        }
                                        // Can be a literal byte
                                        else
                                        {
                                            operandValid = Expression::stringToU8(tokens[tokIdx], equates[i]);
                                        }
                                        if(!operandValid)
                                        {
                                            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'CONDBI' invalid equate/literal '%s'\n", filename.c_str(), _lineNumber+1, tokens[tokIdx].c_str());
                                            return false;
                                        }
                                    }

                                    // PREFIX ARG1 OPCODE ARG0
                                    instruction._operand0 = equates[1];
                                    instruction._operand1 = opcode1;
                                    instruction._operand2 = equates[0];
                                    _instructions.push_back(instruction);
                                    if(!checkInvalidAddress(ParseType(parse), _currentAddress, uint16_t(outputSize), instruction, lineToken, filename, _lineNumber)) return false;
                                }
                                // Special case ANDBI as it has multiple parseable tokens
                                else if(opcode1 == OPCODE_V_ANDBI)
                                {
                                    if(tokens.size() < 3)
                                    {
                                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'ANDBI <zero page var>, <constant 0..255>' expects 2 parameters '%s'\n", filename.c_str(), _lineNumber+1,
                                                                                                                                                                                            tokens[tokenIndex].c_str());
                                        return false;
                                    }

                                    uint8_t equates[2] = {0x00};
                                    for(int i=0; i<2; i++)
                                    {
                                        int tokIdx = (commaTokenIndex >= 0) ? commaTokenIndex + 1 : tokenIndex + i;
                                        if(tokIdx >= int(tokens.size()))
                                        {
                                            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'ANDBI' syntax error in '%s'\n", filename.c_str(), _lineNumber+1, lineToken._text.c_str());
                                            return false;
                                        }

                                        // Search equates
                                        if((operandValid = evaluateEquateOperand(tokens, tokIdx, equate, commaTokenIndex, compoundInstruction)) == true)
                                        {
                                            equates[i] = uint8_t(equate._operand);
                                        }
                                        // Can be a literal byte
                                        else
                                        {
                                            operandValid = Expression::stringToU8(tokens[tokIdx], equates[i]);
                                        }
                                        if(!operandValid)
                                        {
                                            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'ANDBI' invalid equate/literal '%s'\n", filename.c_str(), _lineNumber+1, tokens[tokIdx].c_str());
                                            return false;
                                        }
                                    }

                                    // PREFIX ARG1 OPCODE ARG0
                                    instruction._operand0 = equates[1];
                                    instruction._operand1 = opcode1;
                                    instruction._operand2 = equates[0];
                                    _instructions.push_back(instruction);
                                    if(!checkInvalidAddress(ParseType(parse), _currentAddress, uint16_t(outputSize), instruction, lineToken, filename, _lineNumber)) return false;
                                }
                                // Special case ORBI as it has multiple parseable tokens
                                else if(opcode1 == OPCODE_V_ORBI)
                                {
                                    if(tokens.size() < 3)
                                    {
                                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'ORBI <zero page var>, <constant 0..255>' expects 2 parameters '%s'\n", filename.c_str(), _lineNumber+1,
                                                                                                                                                                                           tokens[tokenIndex].c_str());
                                        return false;
                                    }

                                    uint8_t equates[2] = {0x00};
                                    for(int i=0; i<2; i++)
                                    {
                                        int tokIdx = (commaTokenIndex >= 0) ? commaTokenIndex + 1 : tokenIndex + i;
                                        if(tokIdx >= int(tokens.size()))
                                        {
                                            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'ORBI' syntax error in '%s'\n", filename.c_str(), _lineNumber+1, lineToken._text.c_str());
                                            return false;
                                        }

                                        // Search equates
                                        if((operandValid = evaluateEquateOperand(tokens, tokIdx, equate, commaTokenIndex, compoundInstruction)) == true)
                                        {
                                            equates[i] = uint8_t(equate._operand);
                                        }
                                        // Can be a literal byte
                                        else
                                        {
                                            operandValid = Expression::stringToU8(tokens[tokIdx], equates[i]);
                                        }
                                        if(!operandValid)
                                        {
                                            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'ORBI' invalid equate/literal '%s'\n", filename.c_str(), _lineNumber+1, tokens[tokIdx].c_str());
                                            return false;
                                        }
                                    }

                                    // PREFIX ARG1 OPCODE ARG0
                                    instruction._operand0 = equates[1];
                                    instruction._operand1 = opcode1;
                                    instruction._operand2 = equates[0];
                                    _instructions.push_back(instruction);
                                    if(!checkInvalidAddress(ParseType(parse), _currentAddress, uint16_t(outputSize), instruction, lineToken, filename, _lineNumber)) return false;
                                }
                                // Special case XORBI as it has multiple parseable tokens
                                else if(opcode1 == OPCODE_V_XORBI)
                                {
                                    if(tokens.size() < 3)
                                    {
                                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'XORBI <zero page var>, <constant 0..255>' expects 2 parameters '%s'\n", filename.c_str(), _lineNumber+1,
                                                                                                                                                                                            tokens[tokenIndex].c_str());
                                        return false;
                                    }

                                    uint8_t equates[2] = {0x00};
                                    for(int i=0; i<2; i++)
                                    {
                                        int tokIdx = (commaTokenIndex >= 0) ? commaTokenIndex + 1 : tokenIndex + i;
                                        if(tokIdx >= int(tokens.size()))
                                        {
                                            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'XORBI' syntax error in '%s'\n", filename.c_str(), _lineNumber+1, lineToken._text.c_str());
                                            return false;
                                        }

                                        // Search equates
                                        if((operandValid = evaluateEquateOperand(tokens, tokIdx, equate, commaTokenIndex, compoundInstruction)) == true)
                                        {
                                            equates[i] = uint8_t(equate._operand);
                                        }
                                        // Can be a literal byte
                                        else
                                        {
                                            operandValid = Expression::stringToU8(tokens[tokIdx], equates[i]);
                                        }
                                        if(!operandValid)
                                        {
                                            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'XORBI' invalid equate/literal '%s'\n", filename.c_str(), _lineNumber+1, tokens[tokIdx].c_str());
                                            return false;
                                        }
                                    }

                                    // PREFIX ARG1 OPCODE ARG0
                                    instruction._operand0 = equates[1];
                                    instruction._operand1 = opcode1;
                                    instruction._operand2 = equates[0];
                                    _instructions.push_back(instruction);
                                    if(!checkInvalidAddress(ParseType(parse), _currentAddress, uint16_t(outputSize), instruction, lineToken, filename, _lineNumber)) return false;
                                }
                                // Special case ANDBK as it has multiple parseable tokens
                                else if(opcode1 == OPCODE_V_ANDBK)
                                {
                                    if(tokens.size() < 3)
                                    {
                                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'ANDBK <zero page var>, <constant 0..255>' expects 2 parameters '%s'\n", filename.c_str(), _lineNumber+1,
                                                                                                                                                                                            tokens[tokenIndex].c_str());
                                        return false;
                                    }

                                    uint8_t equates[2] = {0x00};
                                    for(int i=0; i<2; i++)
                                    {
                                        int tokIdx = (commaTokenIndex >= 0) ? commaTokenIndex + 1 : tokenIndex + i;
                                        if(tokIdx >= int(tokens.size()))
                                        {
                                            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'ANDBK' syntax error in '%s'\n", filename.c_str(), _lineNumber+1, lineToken._text.c_str());
                                            return false;
                                        }

                                        // Search equates
                                        if((operandValid = evaluateEquateOperand(tokens, tokIdx, equate, commaTokenIndex, compoundInstruction)) == true)
                                        {
                                            equates[i] = uint8_t(equate._operand);
                                        }
                                        // Can be a literal byte
                                        else
                                        {
                                            operandValid = Expression::stringToU8(tokens[tokIdx], equates[i]);
                                        }
                                        if(!operandValid)
                                        {
                                            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'ANDBK' invalid equate/literal '%s'\n", filename.c_str(), _lineNumber+1, tokens[tokIdx].c_str());
                                            return false;
                                        }
                                    }

                                    // PREFIX ARG1 OPCODE ARG0
                                    instruction._operand0 = equates[1];
                                    instruction._operand1 = opcode1;
                                    instruction._operand2 = equates[0];
                                    _instructions.push_back(instruction);
                                    if(!checkInvalidAddress(ParseType(parse), _currentAddress, uint16_t(outputSize), instruction, lineToken, filename, _lineNumber)) return false;
                                }
                                // Special case ORBK as it has multiple parseable tokens
                                else if(opcode1 == OPCODE_V_ORBK)
                                {
                                    if(tokens.size() < 3)
                                    {
                                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'ORBK <zero page var>, <constant 0..255>' expects 2 parameters '%s'\n", filename.c_str(), _lineNumber+1,
                                                                                                                                                                                           tokens[tokenIndex].c_str());
                                        return false;
                                    }

                                    uint8_t equates[2] = {0x00};
                                    for(int i=0; i<2; i++)
                                    {
                                        int tokIdx = (commaTokenIndex >= 0) ? commaTokenIndex + 1 : tokenIndex + i;
                                        if(tokIdx >= int(tokens.size()))
                                        {
                                            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'ORBK' syntax error in '%s'\n", filename.c_str(), _lineNumber+1, lineToken._text.c_str());
                                            return false;
                                        }

                                        // Search equates
                                        if((operandValid = evaluateEquateOperand(tokens, tokIdx, equate, commaTokenIndex, compoundInstruction)) == true)
                                        {
                                            equates[i] = uint8_t(equate._operand);
                                        }
                                        // Can be a literal byte
                                        else
                                        {
                                            operandValid = Expression::stringToU8(tokens[tokIdx], equates[i]);
                                        }
                                        if(!operandValid)
                                        {
                                            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'ORBK' invalid equate/literal '%s'\n", filename.c_str(), _lineNumber+1, tokens[tokIdx].c_str());
                                            return false;
                                        }
                                    }

                                    // PREFIX ARG1 OPCODE ARG0
                                    instruction._operand0 = equates[1];
                                    instruction._operand1 = opcode1;
                                    instruction._operand2 = equates[0];
                                    _instructions.push_back(instruction);
                                    if(!checkInvalidAddress(ParseType(parse), _currentAddress, uint16_t(outputSize), instruction, lineToken, filename, _lineNumber)) return false;
                                }
                                // Special case XORBK as it has multiple parseable tokens
                                else if(opcode1 == OPCODE_V_XORBK)
                                {
                                    if(tokens.size() < 3)
                                    {
                                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'XORBK <zero page var>, <constant 0..255>' expects 2 parameters '%s'\n", filename.c_str(), _lineNumber+1,
                                                                                                                                                                                            tokens[tokenIndex].c_str());
                                        return false;
                                    }

                                    uint8_t equates[2] = {0x00};
                                    for(int i=0; i<2; i++)
                                    {
                                        int tokIdx = (commaTokenIndex >= 0) ? commaTokenIndex + 1 : tokenIndex + i;
                                        if(tokIdx >= int(tokens.size()))
                                        {
                                            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'XORBK' syntax error in '%s'\n", filename.c_str(), _lineNumber+1, lineToken._text.c_str());
                                            return false;
                                        }

                                        // Search equates
                                        if((operandValid = evaluateEquateOperand(tokens, tokIdx, equate, commaTokenIndex, compoundInstruction)) == true)
                                        {
                                            equates[i] = uint8_t(equate._operand);
                                        }
                                        // Can be a literal byte
                                        else
                                        {
                                            operandValid = Expression::stringToU8(tokens[tokIdx], equates[i]);
                                        }
                                        if(!operandValid)
                                        {
                                            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'XORBK' invalid equate/literal '%s'\n", filename.c_str(), _lineNumber+1, tokens[tokIdx].c_str());
                                            return false;
                                        }
                                    }

                                    // PREFIX ARG1 OPCODE ARG0
                                    instruction._operand0 = equates[1];
                                    instruction._operand1 = opcode1;
                                    instruction._operand2 = equates[0];
                                    _instructions.push_back(instruction);
                                    if(!checkInvalidAddress(ParseType(parse), _currentAddress, uint16_t(outputSize), instruction, lineToken, filename, _lineNumber)) return false;
                                }
                                // Special case XCHGW as it has multiple parseable tokens
                                else if(opcode1 == OPCODE_V_XCHGW)
                                {
                                    if(tokens.size() < 3)
                                    {
                                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'XCHGW <zero page var>, <zero page var>' expects 2 parameters '%s'\n", filename.c_str(), _lineNumber+1,
                                                                                                                                                                                          tokens[tokenIndex].c_str());
                                        return false;
                                    }

                                    uint8_t equates[2] = {0x00};
                                    for(int i=0; i<2; i++)
                                    {
                                        int tokIdx = (commaTokenIndex >= 0) ? commaTokenIndex + 1 : tokenIndex + i;
                                        if(tokIdx >= int(tokens.size()))
                                        {
                                            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'XCHGW' syntax error in '%s'\n", filename.c_str(), _lineNumber+1, lineToken._text.c_str());
                                            return false;
                                        }

                                        // Search equates
                                        if((operandValid = evaluateEquateOperand(tokens, tokIdx, equate, commaTokenIndex, compoundInstruction)) == true)
                                        {
                                            equates[i] = uint8_t(equate._operand);
                                        }
                                        // Can be a literal byte
                                        else
                                        {
                                            operandValid = Expression::stringToU8(tokens[tokIdx], equates[i]);
                                        }
                                        if(!operandValid)
                                        {
                                            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'XCHGW' invalid equate/literal '%s'\n", filename.c_str(), _lineNumber+1, tokens[tokIdx].c_str());
                                            return false;
                                        }
                                    }

                                    // PREFIX ARG1 OPCODE ARG0
                                    instruction._operand0 = equates[1];
                                    instruction._operand1 = opcode1;
                                    instruction._operand2 = equates[0];
                                    _instructions.push_back(instruction);
                                    if(!checkInvalidAddress(ParseType(parse), _currentAddress, uint16_t(outputSize), instruction, lineToken, filename, _lineNumber)) return false;
                                }
                                // Special case OSCPX as it has multiple parseable tokens
                                else if(opcode1 == OPCODE_V_OSCPX)
                                {
                                    if(tokens.size() < 3)
                                    {
                                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'OSCPX <sample addr var>, <index var>' expects 2 parameters '%s'\n", filename.c_str(), _lineNumber+1,
                                                                                                                                                                                        tokens[tokenIndex].c_str());
                                        return false;
                                    }

                                    uint8_t equates[2] = {0x00};
                                    for(int i=0; i<2; i++)
                                    {
                                        int tokIdx = (commaTokenIndex >= 0) ? commaTokenIndex + 1 : tokenIndex + i;
                                        if(tokIdx >= int(tokens.size()))
                                        {
                                            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'OSCPX' syntax error in '%s'\n", filename.c_str(), _lineNumber+1, lineToken._text.c_str());
                                            return false;
                                        }

                                        // Search equates
                                        if((operandValid = evaluateEquateOperand(tokens, tokIdx, equate, commaTokenIndex, compoundInstruction)) == true)
                                        {
                                            equates[i] = uint8_t(equate._operand);
                                        }
                                        // Can be a literal byte
                                        else
                                        {
                                            operandValid = Expression::stringToU8(tokens[tokIdx], equates[i]);
                                        }
                                        if(!operandValid)
                                        {
                                            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'OSCPX' invalid equate/literal '%s'\n", filename.c_str(), _lineNumber+1, tokens[tokIdx].c_str());
                                            return false;
                                        }
                                    }

                                    // PREFIX ARG1 OPCODE ARG0
                                    instruction._operand0 = equates[1];
                                    instruction._operand1 = opcode1;
                                    instruction._operand2 = equates[0];
                                    _instructions.push_back(instruction);
                                    if(!checkInvalidAddress(ParseType(parse), _currentAddress, uint16_t(outputSize), instruction, lineToken, filename, _lineNumber)) return false;
                                }
                                // Special case SWAPB as it has multiple parseable tokens
                                else if(opcode1 == OPCODE_V_SWAPB)
                                {
                                    if(tokens.size() < 3)
                                    {
                                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'SWAPB <addr var0.lo>, <addr var1.lo>' expects 2 parameters '%s'\n", filename.c_str(), _lineNumber+1,
                                                                                                                                                                                        tokens[tokenIndex].c_str());
                                        return false;
                                    }

                                    uint8_t equates[2] = {0x00};
                                    for(int i=0; i<2; i++)
                                    {
                                        int tokIdx = (commaTokenIndex >= 0) ? commaTokenIndex + 1 : tokenIndex + i;
                                        if(tokIdx >= int(tokens.size()))
                                        {
                                            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'SWAPB' syntax error in '%s'\n", filename.c_str(), _lineNumber+1, lineToken._text.c_str());
                                            return false;
                                        }

                                        // Search equates
                                        if((operandValid = evaluateEquateOperand(tokens, tokIdx, equate, commaTokenIndex, compoundInstruction)) == true)
                                        {
                                            equates[i] = uint8_t(equate._operand);
                                        }
                                        // Can be a literal byte
                                        else
                                        {
                                            operandValid = Expression::stringToU8(tokens[tokIdx], equates[i]);
                                        }
                                        if(!operandValid)
                                        {
                                            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'SWAPB' invalid equate/literal '%s'\n", filename.c_str(), _lineNumber+1, tokens[tokIdx].c_str());
                                            return false;
                                        }
                                    }

                                    // PREFIX ARG1 OPCODE ARG0
                                    instruction._operand0 = equates[1];
                                    instruction._operand1 = opcode1;
                                    instruction._operand2 = equates[0];
                                    _instructions.push_back(instruction);
                                    if(!checkInvalidAddress(ParseType(parse), _currentAddress, uint16_t(outputSize), instruction, lineToken, filename, _lineNumber)) return false;
                                }
                                // Special case SWAPW as it has multiple parseable tokens
                                else if(opcode1 == OPCODE_V_SWAPW)
                                {
                                    if(tokens.size() < 3)
                                    {
                                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'SWAPW <addr var0>, <addr var1>' expects 2 parameters '%s'\n", filename.c_str(), _lineNumber+1,
                                                                                                                                                                                  tokens[tokenIndex].c_str());
                                        return false;
                                    }

                                    uint8_t equates[2] = {0x00};
                                    for(int i=0; i<2; i++)
                                    {
                                        int tokIdx = (commaTokenIndex >= 0) ? commaTokenIndex + 1 : tokenIndex + i;
                                        if(tokIdx >= int(tokens.size()))
                                        {
                                            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'SWAPW' syntax error in '%s'\n", filename.c_str(), _lineNumber+1, lineToken._text.c_str());
                                            return false;
                                        }

                                        // Search equates
                                        if((operandValid = evaluateEquateOperand(tokens, tokIdx, equate, commaTokenIndex, compoundInstruction)) == true)
                                        {
                                            equates[i] = uint8_t(equate._operand);
                                        }
                                        // Can be a literal byte
                                        else
                                        {
                                            operandValid = Expression::stringToU8(tokens[tokIdx], equates[i]);
                                        }
                                        if(!operandValid)
                                        {
                                            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'SWAPW' invalid equate/literal '%s'\n", filename.c_str(), _lineNumber+1, tokens[tokIdx].c_str());
                                            return false;
                                        }
                                    }

                                    // PREFIX ARG1 OPCODE ARG0
                                    instruction._operand0 = equates[1];
                                    instruction._operand1 = opcode1;
                                    instruction._operand2 = equates[0];
                                    _instructions.push_back(instruction);
                                    if(!checkInvalidAddress(ParseType(parse), _currentAddress, uint16_t(outputSize), instruction, lineToken, filename, _lineNumber)) return false;
                                }
                                // Special case VADDBW as it has multiple parseable tokens
                                else if(opcode1 == OPCODE_V_VADDBW)
                                {
                                    if(tokens.size() < 3)
                                    {
                                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'VADDBW <src zero page var>, <dst zero page var>' expects 2 parameters '%s'\n", filename.c_str(), _lineNumber+1,
                                                                                                                                                                                                   tokens[tokenIndex].c_str());
                                        return false;
                                    }

                                    uint8_t equates[2] = {0x00};
                                    for(int i=0; i<2; i++)
                                    {
                                        int tokIdx = (commaTokenIndex >= 0) ? commaTokenIndex + 1 : tokenIndex + i;
                                        if(tokIdx >= int(tokens.size()))
                                        {
                                            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'VADDBW' syntax error in '%s'\n", filename.c_str(), _lineNumber+1, lineToken._text.c_str());
                                            return false;
                                        }

                                        // Search equates
                                        if((operandValid = evaluateEquateOperand(tokens, tokIdx, equate, commaTokenIndex, compoundInstruction)) == true)
                                        {
                                            equates[i] = uint8_t(equate._operand);
                                        }
                                        // Can be a literal byte
                                        else
                                        {
                                            operandValid = Expression::stringToU8(tokens[tokIdx], equates[i]);
                                        }
                                        if(!operandValid)
                                        {
                                            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'VADDBW' invalid equate/literal '%s'\n", filename.c_str(), _lineNumber+1, tokens[tokIdx].c_str());
                                            return false;
                                        }
                                    }

                                    // PREFIX ARG1 OPCODE ARG0
                                    instruction._operand0 = equates[1];
                                    instruction._operand1 = opcode1;
                                    instruction._operand2 = equates[0];
                                    _instructions.push_back(instruction);
                                    if(!checkInvalidAddress(ParseType(parse), _currentAddress, uint16_t(outputSize), instruction, lineToken, filename, _lineNumber)) return false;
                                }
                                // Special case VSUBBW as it has multiple parseable tokens
                                else if(opcode1 == OPCODE_V_VSUBBW)
                                {
                                    if(tokens.size() < 3)
                                    {
                                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'VSUBBW <src zero page var>, <dst zero page var>' expects 2 parameters '%s'\n", filename.c_str(), _lineNumber+1,
                                                                                                                                                                                                   tokens[tokenIndex].c_str());
                                        return false;
                                    }

                                    uint8_t equates[2] = {0x00};
                                    for(int i=0; i<2; i++)
                                    {
                                        int tokIdx = (commaTokenIndex >= 0) ? commaTokenIndex + 1 : tokenIndex + i;
                                        if(tokIdx >= int(tokens.size()))
                                        {
                                            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'VSUBBW' syntax error in '%s'\n", filename.c_str(), _lineNumber+1, lineToken._text.c_str());
                                            return false;
                                        }

                                        // Search equates
                                        if((operandValid = evaluateEquateOperand(tokens, tokIdx, equate, commaTokenIndex, compoundInstruction)) == true)
                                        {
                                            equates[i] = uint8_t(equate._operand);
                                        }
                                        // Can be a literal byte
                                        else
                                        {
                                            operandValid = Expression::stringToU8(tokens[tokIdx], equates[i]);
                                        }
                                        if(!operandValid)
                                        {
                                            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'VSUBBW' invalid equate/literal '%s'\n", filename.c_str(), _lineNumber+1, tokens[tokIdx].c_str());
                                            return false;
                                        }
                                    }

                                    // PREFIX ARG1 OPCODE ARG0
                                    instruction._operand0 = equates[1];
                                    instruction._operand1 = opcode1;
                                    instruction._operand2 = equates[0];
                                    _instructions.push_back(instruction);
                                    if(!checkInvalidAddress(ParseType(parse), _currentAddress, uint16_t(outputSize), instruction, lineToken, filename, _lineNumber)) return false;
                                }
                                // Special case VADDBL as it has multiple parseable tokens
                                else if(opcode1 == OPCODE_V_VADDBL)
                                {
                                    if(tokens.size() < 3)
                                    {
                                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'VADDBL <src zero page var>, <dst zero page var>' expects 2 parameters '%s'\n", filename.c_str(), _lineNumber+1,
                                                                                                                                                                                                   tokens[tokenIndex].c_str());
                                        return false;
                                    }

                                    uint8_t equates[2] = {0x00};
                                    for(int i=0; i<2; i++)
                                    {
                                        int tokIdx = (commaTokenIndex >= 0) ? commaTokenIndex + 1 : tokenIndex + i;
                                        if(tokIdx >= int(tokens.size()))
                                        {
                                            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'VADDBL' syntax error in '%s'\n", filename.c_str(), _lineNumber+1, lineToken._text.c_str());
                                            return false;
                                        }

                                        // Search equates
                                        if((operandValid = evaluateEquateOperand(tokens, tokIdx, equate, commaTokenIndex, compoundInstruction)) == true)
                                        {
                                            equates[i] = uint8_t(equate._operand);
                                        }
                                        // Can be a literal byte
                                        else
                                        {
                                            operandValid = Expression::stringToU8(tokens[tokIdx], equates[i]);
                                        }
                                        if(!operandValid)
                                        {
                                            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'VADDBL' invalid equate/literal '%s'\n", filename.c_str(), _lineNumber+1, tokens[tokIdx].c_str());
                                            return false;
                                        }
                                    }

                                    // PREFIX ARG1 OPCODE ARG0
                                    instruction._operand0 = equates[1];
                                    instruction._operand1 = opcode1;
                                    instruction._operand2 = equates[0];
                                    _instructions.push_back(instruction);
                                    if(!checkInvalidAddress(ParseType(parse), _currentAddress, uint16_t(outputSize), instruction, lineToken, filename, _lineNumber)) return false;
                                }
                                // Special case VSUBBL as it has multiple parseable tokens
                                else if(opcode1 == OPCODE_V_VSUBBL)
                                {
                                    if(tokens.size() < 3)
                                    {
                                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'VSUBBL <src zero page var>, <dst zero page var>' expects 2 parameters '%s'\n", filename.c_str(), _lineNumber+1,
                                                                                                                                                                                                   tokens[tokenIndex].c_str());
                                        return false;
                                    }

                                    uint8_t equates[2] = {0x00};
                                    for(int i=0; i<2; i++)
                                    {
                                        int tokIdx = (commaTokenIndex >= 0) ? commaTokenIndex + 1 : tokenIndex + i;
                                        if(tokIdx >= int(tokens.size()))
                                        {
                                            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'VSUBBL' syntax error in '%s'\n", filename.c_str(), _lineNumber+1, lineToken._text.c_str());
                                            return false;
                                        }

                                        // Search equates
                                        if((operandValid = evaluateEquateOperand(tokens, tokIdx, equate, commaTokenIndex, compoundInstruction)) == true)
                                        {
                                            equates[i] = uint8_t(equate._operand);
                                        }
                                        // Can be a literal byte
                                        else
                                        {
                                            operandValid = Expression::stringToU8(tokens[tokIdx], equates[i]);
                                        }
                                        if(!operandValid)
                                        {
                                            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'VSUBBL' invalid equate/literal '%s'\n", filename.c_str(), _lineNumber+1, tokens[tokIdx].c_str());
                                            return false;
                                        }
                                    }

                                    // PREFIX ARG1 OPCODE ARG0
                                    instruction._operand0 = equates[1];
                                    instruction._operand1 = opcode1;
                                    instruction._operand2 = equates[0];
                                    _instructions.push_back(instruction);
                                    if(!checkInvalidAddress(ParseType(parse), _currentAddress, uint16_t(outputSize), instruction, lineToken, filename, _lineNumber)) return false;
                                }
                                // Special case CMPII as it has multiple parseable tokens
                                else if(opcode1 == OPCODE_V_CMPII)
                                {
                                    if(tokens.size() < 3)
                                    {
                                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'CMPII <immediate 0>, <immediate 1>' expects 2 parameters '%s'\n", filename.c_str(), _lineNumber+1,
                                                                                                                                                                                      tokens[tokenIndex].c_str());
                                        return false;
                                    }

                                    uint8_t equates[2] = {0x00};
                                    for(int i=0; i<2; i++)
                                    {
                                        int tokIdx = (commaTokenIndex >= 0) ? commaTokenIndex + 1 : tokenIndex + i;
                                        if(tokIdx >= int(tokens.size()))
                                        {
                                            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'CMPII' syntax error in '%s'\n", filename.c_str(), _lineNumber+1, lineToken._text.c_str());
                                            return false;
                                        }

                                        // Search equates
                                        if((operandValid = evaluateEquateOperand(tokens, tokIdx, equate, commaTokenIndex, compoundInstruction)) == true)
                                        {
                                            equates[i] = uint8_t(equate._operand);
                                        }
                                        // Can be a literal byte
                                        else
                                        {
                                            operandValid = Expression::stringToU8(tokens[tokIdx], equates[i]);
                                        }
                                        if(!operandValid)
                                        {
                                            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'CMPII' invalid equate/literal '%s'\n", filename.c_str(), _lineNumber+1, tokens[tokIdx].c_str());
                                            return false;
                                        }
                                    }

                                    // PREFIX ARG1 OPCODE ARG0
                                    instruction._operand0 = equates[1];
                                    instruction._operand1 = opcode1;
                                    instruction._operand2 = equates[0];
                                    _instructions.push_back(instruction);
                                    if(!checkInvalidAddress(ParseType(parse), _currentAddress, uint16_t(outputSize), instruction, lineToken, filename, _lineNumber)) return false;
                                }
                                // Special case NEEKA as it has multiple parseable tokens
                                else if(opcode1 == OPCODE_V_NEEKA)
                                {
                                    if(tokens.size() < 3)
                                    {
                                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'NEEKA <src zero page var>, <imm num bytes>' expects 2 parameters '%s'\n", filename.c_str(), _lineNumber+1,
                                                                                                                                                                                              tokens[tokenIndex].c_str());
                                        return false;
                                    }

                                    uint8_t equates[2] = {0x00};
                                    for(int i=0; i<2; i++)
                                    {
                                        int tokIdx = (commaTokenIndex >= 0) ? commaTokenIndex + 1 : tokenIndex + i;
                                        if(tokIdx >= int(tokens.size()))
                                        {
                                            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'NEEKA' syntax error in '%s'\n", filename.c_str(), _lineNumber+1, lineToken._text.c_str());
                                            return false;
                                        }

                                        // Search equates
                                        if((operandValid = evaluateEquateOperand(tokens, tokIdx, equate, commaTokenIndex, compoundInstruction)) == true)
                                        {
                                            equates[i] = uint8_t(equate._operand);
                                        }
                                        // Can be a literal byte
                                        else
                                        {
                                            operandValid = Expression::stringToU8(tokens[tokIdx], equates[i]);
                                        }
                                        if(!operandValid)
                                        {
                                            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'NEEKA' invalid equate/literal '%s'\n", filename.c_str(), _lineNumber+1, tokens[tokIdx].c_str());
                                            return false;
                                        }
                                    }

                                    // PREFIX ARG1 OPCODE ARG0
                                    instruction._operand0 = equates[1];
                                    instruction._operand1 = opcode1;
                                    instruction._operand2 = equates[0];
                                    _instructions.push_back(instruction);
                                    if(!checkInvalidAddress(ParseType(parse), _currentAddress, uint16_t(outputSize), instruction, lineToken, filename, _lineNumber)) return false;
                                }
                                // Special case NOKEA as it has multiple parseable tokens
                                else if(opcode1 == OPCODE_V_NOKEA)
                                {
                                    if(tokens.size() < 3)
                                    {
                                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'NOKEA <dst zero page var>, <imm num bytes>' expects 2 parameters '%s'\n", filename.c_str(), _lineNumber+1,
                                                                                                                                                                                              tokens[tokenIndex].c_str());
                                        return false;
                                    }

                                    uint8_t equates[2] = {0x00};
                                    for(int i=0; i<2; i++)
                                    {
                                        int tokIdx = (commaTokenIndex >= 0) ? commaTokenIndex + 1 : tokenIndex + i;
                                        if(tokIdx >= int(tokens.size()))
                                        {
                                            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'NOKEA' syntax error in '%s'\n", filename.c_str(), _lineNumber+1, lineToken._text.c_str());
                                            return false;
                                        }

                                        // Search equates
                                        if((operandValid = evaluateEquateOperand(tokens, tokIdx, equate, commaTokenIndex, compoundInstruction)) == true)
                                        {
                                            equates[i] = uint8_t(equate._operand);
                                        }
                                        // Can be a literal byte
                                        else
                                        {
                                            operandValid = Expression::stringToU8(tokens[tokIdx], equates[i]);
                                        }
                                        if(!operandValid)
                                        {
                                            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'NOKEA' invalid equate/literal '%s'\n", filename.c_str(), _lineNumber+1, tokens[tokIdx].c_str());
                                            return false;
                                        }
                                    }

                                    // PREFIX ARG1 OPCODE ARG0
                                    instruction._operand0 = equates[1];
                                    instruction._operand1 = opcode1;
                                    instruction._operand2 = equates[0];
                                    _instructions.push_back(instruction);
                                    if(!checkInvalidAddress(ParseType(parse), _currentAddress, uint16_t(outputSize), instruction, lineToken, filename, _lineNumber)) return false;
                                }
                                // Special case ADDVL as it has multiple parseable tokens
                                else if(opcode1 == OPCODE_V_ADDVL)
                                {
                                    if(tokens.size() < 3)
                                    {
                                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'ADDVL <dst zero page var>, <src zero page var>' expects 2 parameters '%s'\n", filename.c_str(), _lineNumber+1,
                                                                                                                                                                                                  tokens[tokenIndex].c_str());
                                        return false;
                                    }

                                    uint8_t equates[2] = {0x00};
                                    for(int i=0; i<2; i++)
                                    {
                                        int tokIdx = (commaTokenIndex >= 0) ? commaTokenIndex + 1 : tokenIndex + i;
                                        if(tokIdx >= int(tokens.size()))
                                        {
                                            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'ADDVL' syntax error in '%s'\n", filename.c_str(), _lineNumber+1, lineToken._text.c_str());
                                            return false;
                                        }

                                        // Search equates
                                        if((operandValid = evaluateEquateOperand(tokens, tokIdx, equate, commaTokenIndex, compoundInstruction)) == true)
                                        {
                                            equates[i] = uint8_t(equate._operand);
                                        }
                                        // Can be a literal byte
                                        else
                                        {
                                            operandValid = Expression::stringToU8(tokens[tokIdx], equates[i]);
                                        }
                                        if(!operandValid)
                                        {
                                            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'ADDVL' invalid equate/literal '%s'\n", filename.c_str(), _lineNumber+1, tokens[tokIdx].c_str());
                                            return false;
                                        }
                                    }

                                    // PREFIX ARG1 OPCODE ARG0
                                    instruction._operand0 = equates[1];
                                    instruction._operand1 = opcode1;
                                    instruction._operand2 = equates[0];
                                    _instructions.push_back(instruction);
                                    if(!checkInvalidAddress(ParseType(parse), _currentAddress, uint16_t(outputSize), instruction, lineToken, filename, _lineNumber)) return false;
                                }
                                // Special case SUBVL as it has multiple parseable tokens
                                else if(opcode1 == OPCODE_V_SUBVL)
                                {
                                    if(tokens.size() < 3)
                                    {
                                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'SUBVL <dst zero page var>, <src zero page var>' expects 2 parameters '%s'\n", filename.c_str(), _lineNumber+1,
                                                                                                                                                                                                  tokens[tokenIndex].c_str());
                                        return false;
                                    }

                                    uint8_t equates[2] = {0x00};
                                    for(int i=0; i<2; i++)
                                    {
                                        int tokIdx = (commaTokenIndex >= 0) ? commaTokenIndex + 1 : tokenIndex + i;
                                        if(tokIdx >= int(tokens.size()))
                                        {
                                            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'SUBVL' syntax error in '%s'\n", filename.c_str(), _lineNumber+1, lineToken._text.c_str());
                                            return false;
                                        }

                                        // Search equates
                                        if((operandValid = evaluateEquateOperand(tokens, tokIdx, equate, commaTokenIndex, compoundInstruction)) == true)
                                        {
                                            equates[i] = uint8_t(equate._operand);
                                        }
                                        // Can be a literal byte
                                        else
                                        {
                                            operandValid = Expression::stringToU8(tokens[tokIdx], equates[i]);
                                        }
                                        if(!operandValid)
                                        {
                                            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'SUBVL' invalid equate/literal '%s'\n", filename.c_str(), _lineNumber+1, tokens[tokIdx].c_str());
                                            return false;
                                        }
                                    }

                                    // PREFIX ARG1 OPCODE ARG0
                                    instruction._operand0 = equates[1];
                                    instruction._operand1 = opcode1;
                                    instruction._operand2 = equates[0];
                                    _instructions.push_back(instruction);
                                    if(!checkInvalidAddress(ParseType(parse), _currentAddress, uint16_t(outputSize), instruction, lineToken, filename, _lineNumber)) return false;
                                }
                                // Special case ANDVL as it has multiple parseable tokens
                                else if(opcode1 == OPCODE_V_ANDVL)
                                {
                                    if(tokens.size() < 3)
                                    {
                                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'ANDVL <dst zero page var>, <src zero page var>' expects 2 parameters '%s'\n", filename.c_str(), _lineNumber+1,
                                                                                                                                                                                                  tokens[tokenIndex].c_str());
                                        return false;
                                    }

                                    uint8_t equates[2] = {0x00};
                                    for(int i=0; i<2; i++)
                                    {
                                        int tokIdx = (commaTokenIndex >= 0) ? commaTokenIndex + 1 : tokenIndex + i;
                                        if(tokIdx >= int(tokens.size()))
                                        {
                                            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'ANDVL' syntax error in '%s'\n", filename.c_str(), _lineNumber+1, lineToken._text.c_str());
                                            return false;
                                        }

                                        // Search equates
                                        if((operandValid = evaluateEquateOperand(tokens, tokIdx, equate, commaTokenIndex, compoundInstruction)) == true)
                                        {
                                            equates[i] = uint8_t(equate._operand);
                                        }
                                        // Can be a literal byte
                                        else
                                        {
                                            operandValid = Expression::stringToU8(tokens[tokIdx], equates[i]);
                                        }
                                        if(!operandValid)
                                        {
                                            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'ANDVL' invalid equate/literal '%s'\n", filename.c_str(), _lineNumber+1, tokens[tokIdx].c_str());
                                            return false;
                                        }
                                    }

                                    // PREFIX ARG1 OPCODE ARG0
                                    instruction._operand0 = equates[1];
                                    instruction._operand1 = opcode1;
                                    instruction._operand2 = equates[0];
                                    _instructions.push_back(instruction);
                                    if(!checkInvalidAddress(ParseType(parse), _currentAddress, uint16_t(outputSize), instruction, lineToken, filename, _lineNumber)) return false;
                                }
                                // Special case ORVL as it has multiple parseable tokens
                                else if(opcode1 == OPCODE_V_ORVL)
                                {
                                    if(tokens.size() < 3)
                                    {
                                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'ORVL <dst zero page var>, <src zero page var>' expects 2 parameters '%s'\n", filename.c_str(), _lineNumber+1,
                                                                                                                                                                                                 tokens[tokenIndex].c_str());
                                        return false;
                                    }

                                    uint8_t equates[2] = {0x00};
                                    for(int i=0; i<2; i++)
                                    {
                                        int tokIdx = (commaTokenIndex >= 0) ? commaTokenIndex + 1 : tokenIndex + i;
                                        if(tokIdx >= int(tokens.size()))
                                        {
                                            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'ORVL' syntax error in '%s'\n", filename.c_str(), _lineNumber+1, lineToken._text.c_str());
                                            return false;
                                        }

                                        // Search equates
                                        if((operandValid = evaluateEquateOperand(tokens, tokIdx, equate, commaTokenIndex, compoundInstruction)) == true)
                                        {
                                            equates[i] = uint8_t(equate._operand);
                                        }
                                        // Can be a literal byte
                                        else
                                        {
                                            operandValid = Expression::stringToU8(tokens[tokIdx], equates[i]);
                                        }
                                        if(!operandValid)
                                        {
                                            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'ORVL' invalid equate/literal '%s'\n", filename.c_str(), _lineNumber+1, tokens[tokIdx].c_str());
                                            return false;
                                        }
                                    }

                                    // PREFIX ARG1 OPCODE ARG0
                                    instruction._operand0 = equates[1];
                                    instruction._operand1 = opcode1;
                                    instruction._operand2 = equates[0];
                                    _instructions.push_back(instruction);
                                    if(!checkInvalidAddress(ParseType(parse), _currentAddress, uint16_t(outputSize), instruction, lineToken, filename, _lineNumber)) return false;
                                }
                                // Special case XORVL as it has multiple parseable tokens
                                else if(opcode1 == OPCODE_V_XORVL)
                                {
                                    if(tokens.size() < 3)
                                    {
                                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'XORVL <dst zero page var>, <src zero page var>' expects 2 parameters '%s'\n", filename.c_str(), _lineNumber+1,
                                                                                                                                                                                                  tokens[tokenIndex].c_str());
                                        return false;
                                    }

                                    uint8_t equates[2] = {0x00};
                                    for(int i=0; i<2; i++)
                                    {
                                        int tokIdx = (commaTokenIndex >= 0) ? commaTokenIndex + 1 : tokenIndex + i;
                                        if(tokIdx >= int(tokens.size()))
                                        {
                                            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'XORVL' syntax error in '%s'\n", filename.c_str(), _lineNumber+1, lineToken._text.c_str());
                                            return false;
                                        }

                                        // Search equates
                                        if((operandValid = evaluateEquateOperand(tokens, tokIdx, equate, commaTokenIndex, compoundInstruction)) == true)
                                        {
                                            equates[i] = uint8_t(equate._operand);
                                        }
                                        // Can be a literal byte
                                        else
                                        {
                                            operandValid = Expression::stringToU8(tokens[tokIdx], equates[i]);
                                        }
                                        if(!operandValid)
                                        {
                                            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'XORVL' invalid equate/literal '%s'\n", filename.c_str(), _lineNumber+1, tokens[tokIdx].c_str());
                                            return false;
                                        }
                                    }

                                    // PREFIX ARG1 OPCODE ARG0
                                    instruction._operand0 = equates[1];
                                    instruction._operand1 = opcode1;
                                    instruction._operand2 = equates[0];
                                    _instructions.push_back(instruction);
                                    if(!checkInvalidAddress(ParseType(parse), _currentAddress, uint16_t(outputSize), instruction, lineToken, filename, _lineNumber)) return false;
                                }
                                // Special case JCCL
                                else if(opcode1 == OPCODE_V_JEQL  ||  opcode1 == OPCODE_V_JNEL  ||  opcode1 == OPCODE_V_JLTL  ||  opcode1 == OPCODE_V_JGTL  ||  opcode1 == OPCODE_V_JLEL  ||  opcode1 == OPCODE_V_JGEL)
                                {
                                    if(tokens.size() < 2)
                                    {
                                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'JCCL <label>' expects 1 parameter '%s'\n", filename.c_str(), _lineNumber+1, tokens[tokenIndex].c_str());
                                        return false;
                                    }

                                    // Label/equate operand
                                    if(!validateOperand(tokens, tokenIndex, compoundInstruction, operand))
                                    {
                                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'JCCL' invalid label/equate '%s'\n", filename.c_str(), _lineNumber+1, tokens[tokenIndex].c_str());
                                        return false;
                                    }

                                    // PREFIX ARG1 OPCODE ARG0
                                    instruction._operand0 = uint8_t(LO_BYTE(operand)) - BRANCH_ADJUSTMENT;
                                    instruction._operand1 = opcode1;
                                    instruction._operand2 = uint8_t(HI_BYTE(operand));
                                    _instructions.push_back(instruction);
                                    if(!checkInvalidAddress(ParseType(parse), _currentAddress, uint16_t(outputSize), instruction, lineToken, filename, _lineNumber)) return false;
                                }
                                // Default PREFX3 instruction
                                else
                                {
                                    // Search equates
                                    if((operandValid = evaluateEquateOperand(tokens, tokenIndex, equate, commaTokenIndex, compoundInstruction)) == true)
                                    {
                                        operand = uint16_t(equate._operand);
                                    }
                                    // Search labels
                                    else if((operandValid = evaluateLabelOperand(tokens, tokenIndex, label, compoundInstruction)) == true)
                                    {
                                        operand = label._address;
                                    }
                                    else if(Expression::isExpression(tokens[tokenIndex]) == Expression::HasOperators)
                                    {
                                        Expression::Numeric value;
                                        std::string input;
                                        preProcessExpression(tokens, tokenIndex, input, true);
                                        if(!Expression::parse((char*)input.c_str(), _lineNumber, value)) return false;
                                        operand = int16_t(std::lround(value._value));
                                        operandValid = true;
                                    }
                                    else
                                    {
                                        operandValid = Expression::stringToU16(tokens[tokenIndex], operand);
                                        if(!operandValid)
                                        {
                                            Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : invalid label/equate '%s'\n", filename.c_str(), _lineNumber+1, tokens[tokenIndex].c_str());
                                            return false;
                                        }
                                    }

                                    // PREFIX ARG1 OPCODE ARG0
                                    instruction._operand0 = uint8_t(HI_BYTE(operand));
                                    instruction._operand1 = opcode1;
                                    instruction._operand2 = uint8_t(LO_BYTE(operand));
                                    _instructions.push_back(instruction);
                                    if(!checkInvalidAddress(ParseType(parse), _currentAddress, uint16_t(outputSize), instruction, lineToken, filename, _lineNumber)) return false;
                                }
                            }
                            // Special case DJNE as it has multiple parseable tokens
                            else if(opcodeType == vCpu  &&  opcode0 == OPCODE_V_DJNE)
                            {
                                if(tokens.size() < 3)
                                {
                                    Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'DJNE <count zero page var>, <label>' expects 2 parameters '%s'\n", filename.c_str(), _lineNumber+1,
                                                                                                                                                                                   tokens[tokenIndex].c_str());
                                    return false;
                                }

                                Equate equate;
                                uint8_t counter = {0x00};
                                int commaTokenIndex = -1;

                                // Search equates
                                if((operandValid = evaluateEquateOperand(tokens, tokenIndex, equate, commaTokenIndex, compoundInstruction)) == true)
                                {
                                    counter = uint8_t(equate._operand);
                                }
                                // Can be a literal byte
                                else
                                {
                                    operandValid = Expression::stringToU8(tokens[tokenIndex], counter);
                                }
                                if(!operandValid  ||  commaTokenIndex == -1)
                                {
                                    Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'DJNE' invalid equate/literal or missing comma '%s'\n", filename.c_str(), _lineNumber+1,
                                                                                                                                                                       tokens[tokenIndex].c_str());
                                    return false;
                                }

                                // Label/equate operand
                                uint16_t operand = 0x0000;
                                if(!validateOperand(tokens, commaTokenIndex + 1, compoundInstruction, operand))
                                {
                                    Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'DJNE' invalid label/equate '%s'\n", filename.c_str(), _lineNumber+1, tokens[tokenIndex].c_str());
                                    return false;
                                }

                                instruction._operand0 = counter;
                                instruction._operand1 = uint8_t(LO_BYTE(operand)) - BRANCH_ADJUSTMENT;
                                instruction._operand2 = uint8_t(HI_BYTE(operand));
                                _instructions.push_back(instruction);
                                if(!checkInvalidAddress(ParseType(parse), _currentAddress, uint16_t(outputSize), instruction, lineToken, filename, _lineNumber)) return false;
                            }
                            // Special case DJGE as it has multiple parseable tokens
                            else if(opcodeType == vCpu  &&  opcode0 == OPCODE_V_DJGE)
                            {
                                if(tokens.size() < 3)
                                {
                                    Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'DJGE <count zero page var>, <label>' expects 2 parameters '%s'\n", filename.c_str(), _lineNumber+1,
                                                                                                                                                                                   tokens[tokenIndex].c_str());
                                    return false;
                                }

                                Equate equate;
                                uint8_t counter = {0x00};
                                int commaTokenIndex = -1;

                                // Search equates
                                if((operandValid = evaluateEquateOperand(tokens, tokenIndex, equate, commaTokenIndex, compoundInstruction)) == true)
                                {
                                    counter = uint8_t(equate._operand);
                                }
                                // Can be a literal byte
                                else
                                {
                                    operandValid = Expression::stringToU8(tokens[tokenIndex], counter);
                                }
                                if(!operandValid  ||  commaTokenIndex == -1)
                                {
                                    Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'DJGE' invalid equate/literal or missing comma '%s'\n", filename.c_str(), _lineNumber+1,
                                                                                                                                                                       tokens[tokenIndex].c_str());
                                    return false;
                                }

                                // Label/equate operand
                                uint16_t operand = 0x0000;
                                if(!validateOperand(tokens, commaTokenIndex + 1, compoundInstruction, operand))
                                {
                                    Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'DJGE' invalid label/equate '%s'\n", filename.c_str(), _lineNumber+1, tokens[tokenIndex].c_str());
                                    return false;
                                }

                                instruction._operand0 = counter;
                                instruction._operand1 = uint8_t(LO_BYTE(operand)) - BRANCH_ADJUSTMENT;
                                instruction._operand2 = uint8_t(HI_BYTE(operand));
                                _instructions.push_back(instruction);
                                if(!checkInvalidAddress(ParseType(parse), _currentAddress, uint16_t(outputSize), instruction, lineToken, filename, _lineNumber)) return false;
                            }
                            // Special case ARRVW as it has multiple parseable tokens
                            else if(opcodeType == vCpu  &&  opcode0 == OPCODE_V_ARRVW)
                            {
                                if(tokens.size() < 3)
                                {
                                    Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'ARRVW <index var>, <16bit imm array addr >' expects 2 parameters '%s'\n", filename.c_str(), _lineNumber+1,
                                                                                                                                                                                          tokens[tokenIndex].c_str());
                                    return false;
                                }

                                Equate equate;
                                uint8_t index = {0x00};
                                int commaTokenIndex = -1;

                                // Search equates
                                if((operandValid = evaluateEquateOperand(tokens, tokenIndex, equate, commaTokenIndex, compoundInstruction)) == true)
                                {
                                    index = uint8_t(equate._operand);
                                }
                                // Can be a literal byte
                                else
                                {
                                    operandValid = Expression::stringToU8(tokens[tokenIndex], index);
                                }
                                if(!operandValid  ||  commaTokenIndex == -1)
                                {
                                    Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'ARRVW' invalid equate/literal or missing comma '%s'\n", filename.c_str(), _lineNumber+1,
                                                                                                                                                                        tokens[tokenIndex].c_str());
                                    return false;
                                }

                                // Label/equate operand
                                uint16_t operand = 0x0000;
                                if(!validateOperand(tokens, commaTokenIndex + 1, compoundInstruction, operand))
                                {
                                    Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'ARRVW' invalid label/equate '%s'\n", filename.c_str(), _lineNumber+1, tokens[tokenIndex].c_str());
                                    return false;
                                }

                                instruction._operand0 = index;
                                instruction._operand1 = uint8_t(LO_BYTE(operand));
                                instruction._operand2 = uint8_t(HI_BYTE(operand));
                                _instructions.push_back(instruction);
                                if(!checkInvalidAddress(ParseType(parse), _currentAddress, uint16_t(outputSize), instruction, lineToken, filename, _lineNumber)) return false;
                            }
                            // Special case LDARRB as it has multiple parseable tokens
                            else if(opcodeType == vCpu  &&  opcode0 == OPCODE_V_LDARRB)
                            {
                                if(tokens.size() < 3)
                                {
                                    Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'LDARRB <index var>, <16bit imm array addr >' expects 2 parameters '%s'\n", filename.c_str(), _lineNumber+1,
                                                                                                                                                                                           tokens[tokenIndex].c_str());
                                    return false;
                                }

                                Equate equate;
                                uint8_t index = {0x00};
                                int commaTokenIndex = -1;

                                // Search equates
                                if((operandValid = evaluateEquateOperand(tokens, tokenIndex, equate, commaTokenIndex, compoundInstruction)) == true)
                                {
                                    index = uint8_t(equate._operand);
                                }
                                // Can be a literal byte
                                else
                                {
                                    operandValid = Expression::stringToU8(tokens[tokenIndex], index);
                                }
                                if(!operandValid  ||  commaTokenIndex == -1)
                                {
                                    Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'LDARRB' invalid equate/literal or missing comma '%s'\n", filename.c_str(), _lineNumber+1,
                                                                                                                                                                        tokens[tokenIndex].c_str());
                                    return false;
                                }

                                // Label/equate operand
                                uint16_t operand = 0x0000;
                                if(!validateOperand(tokens, commaTokenIndex + 1, compoundInstruction, operand))
                                {
                                    Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'LDARRB' invalid label/equate '%s'\n", filename.c_str(), _lineNumber+1, tokens[tokenIndex].c_str());
                                    return false;
                                }

                                instruction._operand0 = index;
                                instruction._operand1 = uint8_t(LO_BYTE(operand));
                                instruction._operand2 = uint8_t(HI_BYTE(operand));
                                _instructions.push_back(instruction);
                                if(!checkInvalidAddress(ParseType(parse), _currentAddress, uint16_t(outputSize), instruction, lineToken, filename, _lineNumber)) return false;
                            }
                            // Special case STARRB as it has multiple parseable tokens
                            else if(opcodeType == vCpu  &&  opcode0 == OPCODE_V_STARRB)
                            {
                                if(tokens.size() < 3)
                                {
                                    Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'STARRB <index var>, <16bit imm array addr >' expects 2 parameters '%s'\n", filename.c_str(), _lineNumber+1,
                                                                                                                                                                                           tokens[tokenIndex].c_str());
                                    return false;
                                }

                                Equate equate;
                                uint8_t index = {0x00};
                                int commaTokenIndex = -1;

                                // Search equates
                                if((operandValid = evaluateEquateOperand(tokens, tokenIndex, equate, commaTokenIndex, compoundInstruction)) == true)
                                {
                                    index = uint8_t(equate._operand);
                                }
                                // Can be a literal byte
                                else
                                {
                                    operandValid = Expression::stringToU8(tokens[tokenIndex], index);
                                }
                                if(!operandValid  ||  commaTokenIndex == -1)
                                {
                                    Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'STARRB' invalid equate/literal or missing comma '%s'\n", filename.c_str(), _lineNumber+1,
                                                                                                                                                                        tokens[tokenIndex].c_str());
                                    return false;
                                }

                                // Label/equate operand
                                uint16_t operand = 0x0000;
                                if(!validateOperand(tokens, commaTokenIndex + 1, compoundInstruction, operand))
                                {
                                    Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'STARRB' invalid label/equate '%s'\n", filename.c_str(), _lineNumber+1, tokens[tokenIndex].c_str());
                                    return false;
                                }

                                instruction._operand0 = index;
                                instruction._operand1 = uint8_t(LO_BYTE(operand));
                                instruction._operand2 = uint8_t(HI_BYTE(operand));
                                _instructions.push_back(instruction);
                                if(!checkInvalidAddress(ParseType(parse), _currentAddress, uint16_t(outputSize), instruction, lineToken, filename, _lineNumber)) return false;
                            }
                            // Special case LDARRW as it has multiple parseable tokens
                            else if(opcodeType == vCpu  &&  opcode0 == OPCODE_V_LDARRW)
                            {
                                if(tokens.size() < 3)
                                {
                                    Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'LDARRW <var>, <16bit imm addr >' expects 2 parameters '%s'\n", filename.c_str(), _lineNumber+1,
                                                                                                                                                                               tokens[tokenIndex].c_str());
                                    return false;
                                }

                                Equate equate;
                                uint8_t index = {0x00};
                                int commaTokenIndex = -1;

                                // Search equates
                                if((operandValid = evaluateEquateOperand(tokens, tokenIndex, equate, commaTokenIndex, compoundInstruction)) == true)
                                {
                                    index = uint8_t(equate._operand);
                                }
                                // Can be a literal byte
                                else
                                {
                                    operandValid = Expression::stringToU8(tokens[tokenIndex], index);
                                }
                                if(!operandValid  ||  commaTokenIndex == -1)
                                {
                                    Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'LDARRW' invalid equate/literal or missing comma '%s'\n", filename.c_str(), _lineNumber+1,
                                                                                                                                                                         tokens[tokenIndex].c_str());
                                    return false;
                                }

                                // Label/equate operand
                                uint16_t operand = 0x0000;
                                if(!validateOperand(tokens, commaTokenIndex + 1, compoundInstruction, operand))
                                {
                                    Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'LDARRW' invalid label/equate '%s'\n", filename.c_str(), _lineNumber+1, tokens[tokenIndex].c_str());
                                    return false;
                                }

                                instruction._operand0 = index;
                                instruction._operand1 = uint8_t(LO_BYTE(operand));
                                instruction._operand2 = uint8_t(HI_BYTE(operand));
                                _instructions.push_back(instruction);
                                if(!checkInvalidAddress(ParseType(parse), _currentAddress, uint16_t(outputSize), instruction, lineToken, filename, _lineNumber)) return false;
                            }
                            // Special case STARRW as it has multiple parseable tokens
                            else if(opcodeType == vCpu  &&  opcode0 == OPCODE_V_STARRW)
                            {
                                if(tokens.size() < 3)
                                {
                                    Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'STARRW <var>, <16bit imm addr >' expects 2 parameters '%s'\n", filename.c_str(), _lineNumber+1,
                                                                                                                                                                               tokens[tokenIndex].c_str());
                                    return false;
                                }

                                Equate equate;
                                uint8_t index = {0x00};
                                int commaTokenIndex = -1;

                                // Search equates
                                if((operandValid = evaluateEquateOperand(tokens, tokenIndex, equate, commaTokenIndex, compoundInstruction)) == true)
                                {
                                    index = uint8_t(equate._operand);
                                }
                                // Can be a literal byte
                                else
                                {
                                    operandValid = Expression::stringToU8(tokens[tokenIndex], index);
                                }
                                if(!operandValid  ||  commaTokenIndex == -1)
                                {
                                    Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'STARRW' invalid equate/literal or missing comma '%s'\n", filename.c_str(), _lineNumber+1,
                                                                                                                                                                         tokens[tokenIndex].c_str());
                                    return false;
                                }

                                // Label/equate operand
                                uint16_t operand = 0x0000;
                                if(!validateOperand(tokens, commaTokenIndex + 1, compoundInstruction, operand))
                                {
                                    Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'STARRW' invalid label/equate '%s'\n", filename.c_str(), _lineNumber+1, tokens[tokenIndex].c_str());
                                    return false;
                                }

                                instruction._operand0 = index;
                                instruction._operand1 = uint8_t(LO_BYTE(operand));
                                instruction._operand2 = uint8_t(HI_BYTE(operand));
                                _instructions.push_back(instruction);
                                if(!checkInvalidAddress(ParseType(parse), _currentAddress, uint16_t(outputSize), instruction, lineToken, filename, _lineNumber)) return false;
                            }
                            // Special case ADDVW as it has multiple parseable tokens
                            else if(opcodeType == vCpu  &&  opcode0 == OPCODE_V_ADDVW)
                            {
                                if(tokens.size() < 4)
                                {
                                    Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'ADDVW <src0 var>, <src1 var>, <dst var>' expects 4 parameters '%s'\n", filename.c_str(), _lineNumber+1,
                                                                                                                                                                                       tokens[tokenIndex].c_str());
                                    return false;
                                }

                                Equate equate;
                                uint8_t equates[3] = {0x00};
                                int commaTokenIndex = -1;
                                for(int i=0; i<3; i++)
                                {
                                    int tokIdx = (commaTokenIndex >= 0) ? commaTokenIndex + 1 : tokenIndex + i;
                                    if(tokIdx >= int(tokens.size()))
                                    {
                                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'ADDVW' syntax error in '%s'\n", filename.c_str(), _lineNumber+1, lineToken._text.c_str());
                                        return false;
                                    }

                                    // Search equates
                                    if((operandValid = evaluateEquateOperand(tokens, tokIdx, equate, commaTokenIndex, compoundInstruction)) == true)
                                    {
                                        equates[i] = uint8_t(equate._operand);
                                    }
                                    // Can be a literal byte
                                    else
                                    {
                                        operandValid = Expression::stringToU8(tokens[tokIdx], equates[i]);
                                    }
                                    if(!operandValid)
                                    {
                                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'ADDVW' invalid equate/literal '%s'\n", filename.c_str(), _lineNumber+1, tokens[tokIdx].c_str());
                                        return false;
                                    }
                                }

                                instruction._operand0 = equates[2]; // native code expects the operands swapped for ADDVW
                                instruction._operand1 = equates[1];
                                instruction._operand2 = equates[0];
                                _instructions.push_back(instruction);
                                if(!checkInvalidAddress(ParseType(parse), _currentAddress, uint16_t(outputSize), instruction, lineToken, filename, _lineNumber)) return false;
                            }
                            // Special case SUBVW as it has multiple parseable tokens
                            else if(opcodeType == vCpu  &&  opcode0 == OPCODE_V_SUBVW)
                            {
                                if(tokens.size() < 4)
                                {
                                    Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'SUBVW <src0 var>, <src1 var>, <dst var>' expects 4 parameters '%s'\n", filename.c_str(), _lineNumber+1,
                                                                                                                                                                                       tokens[tokenIndex].c_str());
                                    return false;
                                }

                                Equate equate;
                                uint8_t equates[3] = {0x00};
                                int commaTokenIndex = -1;
                                for(int i=0; i<3; i++)
                                {
                                    int tokIdx = (commaTokenIndex >= 0) ? commaTokenIndex + 1 : tokenIndex + i;
                                    if(tokIdx >= int(tokens.size()))
                                    {
                                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'SUBVW' syntax error in '%s'\n", filename.c_str(), _lineNumber+1, lineToken._text.c_str());
                                        return false;
                                    }

                                    // Search equates
                                    if((operandValid = evaluateEquateOperand(tokens, tokIdx, equate, commaTokenIndex, compoundInstruction)) == true)
                                    {
                                        equates[i] = uint8_t(equate._operand);
                                    }
                                    // Can be a literal byte
                                    else
                                    {
                                        operandValid = Expression::stringToU8(tokens[tokIdx], equates[i]);
                                    }
                                    if(!operandValid)
                                    {
                                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'SUBVW' invalid equate/literal '%s'\n", filename.c_str(), _lineNumber+1, tokens[tokIdx].c_str());
                                        return false;
                                    }
                                }

                                instruction._operand0 = equates[2]; // native code expects the operands swapped for SUBVW
                                instruction._operand1 = equates[1];
                                instruction._operand2 = equates[0];
                                _instructions.push_back(instruction);
                                if(!checkInvalidAddress(ParseType(parse), _currentAddress, uint16_t(outputSize), instruction, lineToken, filename, _lineNumber)) return false;
                            }
                            // Special case ADDVI as it has multiple parseable tokens
                            else if(opcodeType == vCpu  &&  opcode0 == OPCODE_V_ADDVI)
                            {
                                if(tokens.size() < 4)
                                {
                                    Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'ADDVI <src var>, <dst var>, <imm>' expects 4 parameters '%s'\n", filename.c_str(), _lineNumber+1,
                                                                                                                                                                                 tokens[tokenIndex].c_str());
                                    return false;
                                }

                                Equate equate;
                                uint8_t equates[3] = {0x00};
                                int commaTokenIndex = -1;
                                for(int i=0; i<3; i++)
                                {
                                    int tokIdx = (commaTokenIndex >= 0) ? commaTokenIndex + 1 : tokenIndex + i;
                                    if(tokIdx >= int(tokens.size()))
                                    {
                                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'ADDVI' syntax error in '%s'\n", filename.c_str(), _lineNumber+1, lineToken._text.c_str());
                                        return false;
                                    }

                                    // Search equates
                                    if((operandValid = evaluateEquateOperand(tokens, tokIdx, equate, commaTokenIndex, compoundInstruction)) == true)
                                    {
                                        equates[i] = uint8_t(equate._operand);
                                    }
                                    // Can be a literal byte
                                    else
                                    {
                                        operandValid = Expression::stringToU8(tokens[tokIdx], equates[i]);
                                    }
                                    if(!operandValid)
                                    {
                                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'ADDVI' invalid equate/literal '%s'\n", filename.c_str(), _lineNumber+1, tokens[tokIdx].c_str());
                                        return false;
                                    }
                                }

                                instruction._operand0 = equates[2]; // native code expects the operands swapped for ADDVI
                                instruction._operand1 = equates[1];
                                instruction._operand2 = equates[0];
                                _instructions.push_back(instruction);
                                if(!checkInvalidAddress(ParseType(parse), _currentAddress, uint16_t(outputSize), instruction, lineToken, filename, _lineNumber)) return false;
                            }
                            // Special case SUBVI as it has multiple parseable tokens
                            else if(opcodeType == vCpu  &&  opcode0 == OPCODE_V_SUBVI)
                            {
                                if(tokens.size() < 4)
                                {
                                    Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'SUBVI <src var>, <dst var>, <imm>' expects 4 parameters '%s'\n", filename.c_str(), _lineNumber+1,
                                                                                                                                                                                 tokens[tokenIndex].c_str());
                                    return false;
                                }

                                Equate equate;
                                uint8_t equates[3] = {0x00};
                                int commaTokenIndex = -1;
                                for(int i=0; i<3; i++)
                                {
                                    int tokIdx = (commaTokenIndex >= 0) ? commaTokenIndex + 1 : tokenIndex + i;
                                    if(tokIdx >= int(tokens.size()))
                                    {
                                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'SUBVI' syntax error in '%s'\n", filename.c_str(), _lineNumber+1, lineToken._text.c_str());
                                        return false;
                                    }

                                    // Search equates
                                    if((operandValid = evaluateEquateOperand(tokens, tokIdx, equate, commaTokenIndex, compoundInstruction)) == true)
                                    {
                                        equates[i] = uint8_t(equate._operand);
                                    }
                                    // Can be a literal byte
                                    else
                                    {
                                        operandValid = Expression::stringToU8(tokens[tokIdx], equates[i]);
                                    }
                                    if(!operandValid)
                                    {
                                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'SUBVI' invalid equate/literal '%s'\n", filename.c_str(), _lineNumber+1, tokens[tokIdx].c_str());
                                        return false;
                                    }
                                }

                                instruction._operand0 = equates[2]; // native code expects the operands swapped for SUBVI
                                instruction._operand1 = equates[1];
                                instruction._operand2 = equates[0];
                                _instructions.push_back(instruction);
                                if(!checkInvalidAddress(ParseType(parse), _currentAddress, uint16_t(outputSize), instruction, lineToken, filename, _lineNumber)) return false;
                            }
                            // Special case ADDVB as it has multiple parseable tokens
                            else if(opcodeType == vCpu  &&  opcode0 == OPCODE_V_ADDVB)
                            {
                                if(tokens.size() < 4)
                                {
                                    Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'ADDVB <src0 var>, <src1 var>, <dst var>' expects 4 parameters '%s'\n", filename.c_str(), _lineNumber+1,
                                                                                                                                                                                       tokens[tokenIndex].c_str());
                                    return false;
                                }

                                Equate equate;
                                uint8_t equates[3] = {0x00};
                                int commaTokenIndex = -1;
                                for(int i=0; i<3; i++)
                                {
                                    int tokIdx = (commaTokenIndex >= 0) ? commaTokenIndex + 1 : tokenIndex + i;
                                    if(tokIdx >= int(tokens.size()))
                                    {
                                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'ADDVB' syntax error in '%s'\n", filename.c_str(), _lineNumber+1, lineToken._text.c_str());
                                        return false;
                                    }

                                    // Search equates
                                    if((operandValid = evaluateEquateOperand(tokens, tokIdx, equate, commaTokenIndex, compoundInstruction)) == true)
                                    {
                                        equates[i] = uint8_t(equate._operand);
                                    }
                                    // Can be a literal byte
                                    else
                                    {
                                        operandValid = Expression::stringToU8(tokens[tokIdx], equates[i]);
                                    }
                                    if(!operandValid)
                                    {
                                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'ADDVB' invalid equate/literal '%s'\n", filename.c_str(), _lineNumber+1, tokens[tokIdx].c_str());
                                        return false;
                                    }
                                }

                                instruction._operand0 = equates[2]; // native code expects the operands swapped for ADDVB
                                instruction._operand1 = equates[1];
                                instruction._operand2 = equates[0];
                                _instructions.push_back(instruction);
                                if(!checkInvalidAddress(ParseType(parse), _currentAddress, uint16_t(outputSize), instruction, lineToken, filename, _lineNumber)) return false;
                            }
                            // Special case SUBVB as it has multiple parseable tokens
                            else if(opcodeType == vCpu  &&  opcode0 == OPCODE_V_SUBVB)
                            {
                                if(tokens.size() < 4)
                                {
                                    Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'SUBVB <src0 var>, <src1 var>, <dst var>' expects 4 parameters '%s'\n", filename.c_str(), _lineNumber+1,
                                                                                                                                                                                       tokens[tokenIndex].c_str());
                                    return false;
                                }

                                Equate equate;
                                uint8_t equates[3] = {0x00};
                                int commaTokenIndex = -1;
                                for(int i=0; i<3; i++)
                                {
                                    int tokIdx = (commaTokenIndex >= 0) ? commaTokenIndex + 1 : tokenIndex + i;
                                    if(tokIdx >= int(tokens.size()))
                                    {
                                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'SUBVB' syntax error in '%s'\n", filename.c_str(), _lineNumber+1, lineToken._text.c_str());
                                        return false;
                                    }

                                    // Search equates
                                    if((operandValid = evaluateEquateOperand(tokens, tokIdx, equate, commaTokenIndex, compoundInstruction)) == true)
                                    {
                                        equates[i] = uint8_t(equate._operand);
                                    }
                                    // Can be a literal byte
                                    else
                                    {
                                        operandValid = Expression::stringToU8(tokens[tokIdx], equates[i]);
                                    }
                                    if(!operandValid)
                                    {
                                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'SUBVB' invalid equate/literal '%s'\n", filename.c_str(), _lineNumber+1, tokens[tokIdx].c_str());
                                        return false;
                                    }
                                }

                                instruction._operand0 = equates[2]; // native code expects the operands swapped for SUBVB
                                instruction._operand1 = equates[1];
                                instruction._operand2 = equates[0];
                                _instructions.push_back(instruction);
                                if(!checkInvalidAddress(ParseType(parse), _currentAddress, uint16_t(outputSize), instruction, lineToken, filename, _lineNumber)) return false;
                            }
                            // Special case ADDBI as it has multiple parseable tokens
                            else if(opcodeType == vCpu  &&  opcode0 == OPCODE_V_ADDBI)
                            {
                                if(tokens.size() < 4)
                                {
                                    Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'ADDBI <src var>, <dst var>, <imm>' expects 4 parameters '%s'\n", filename.c_str(), _lineNumber+1,
                                                                                                                                                                                 tokens[tokenIndex].c_str());
                                    return false;
                                }

                                Equate equate;
                                uint8_t equates[3] = {0x00};
                                int commaTokenIndex = -1;
                                for(int i=0; i<3; i++)
                                {
                                    int tokIdx = (commaTokenIndex >= 0) ? commaTokenIndex + 1 : tokenIndex + i;
                                    if(tokIdx >= int(tokens.size()))
                                    {
                                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'ADDBI' syntax error in '%s'\n", filename.c_str(), _lineNumber+1, lineToken._text.c_str());
                                        return false;
                                    }

                                    // Search equates
                                    if((operandValid = evaluateEquateOperand(tokens, tokIdx, equate, commaTokenIndex, compoundInstruction)) == true)
                                    {
                                        equates[i] = uint8_t(equate._operand);
                                    }
                                    // Can be a literal byte
                                    else
                                    {
                                        operandValid = Expression::stringToU8(tokens[tokIdx], equates[i]);
                                    }
                                    if(!operandValid)
                                    {
                                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'ADDBI' invalid equate/literal '%s'\n", filename.c_str(), _lineNumber+1, tokens[tokIdx].c_str());
                                        return false;
                                    }
                                }

                                instruction._operand0 = equates[0];
                                instruction._operand1 = equates[1];
                                instruction._operand2 = equates[2];
                                _instructions.push_back(instruction);
                                if(!checkInvalidAddress(ParseType(parse), _currentAddress, uint16_t(outputSize), instruction, lineToken, filename, _lineNumber)) return false;
                            }
                            // Special case SUBBI as it has multiple parseable tokens
                            else if(opcodeType == vCpu  &&  opcode0 == OPCODE_V_SUBBI)
                            {
                                if(tokens.size() < 4)
                                {
                                    Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'SUBBI <src var>, <dst var>, <imm>' expects 4 parameters '%s'\n", filename.c_str(), _lineNumber+1,
                                                                                                                                                                                 tokens[tokenIndex].c_str());
                                    return false;
                                }

                                Equate equate;
                                uint8_t equates[3] = {0x00};
                                int commaTokenIndex = -1;
                                for(int i=0; i<3; i++)
                                {
                                    int tokIdx = (commaTokenIndex >= 0) ? commaTokenIndex + 1 : tokenIndex + i;
                                    if(tokIdx >= int(tokens.size()))
                                    {
                                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'SUBBI' syntax error in '%s'\n", filename.c_str(), _lineNumber+1, lineToken._text.c_str());
                                        return false;
                                    }

                                    // Search equates
                                    if((operandValid = evaluateEquateOperand(tokens, tokIdx, equate, commaTokenIndex, compoundInstruction)) == true)
                                    {
                                        equates[i] = uint8_t(equate._operand);
                                    }
                                    // Can be a literal byte
                                    else
                                    {
                                        operandValid = Expression::stringToU8(tokens[tokIdx], equates[i]);
                                    }
                                    if(!operandValid)
                                    {
                                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'SUBBI' invalid equate/literal '%s'\n", filename.c_str(), _lineNumber+1, tokens[tokIdx].c_str());
                                        return false;
                                    }
                                }

                                instruction._operand0 = equates[2];
                                instruction._operand1 = equates[1];
                                instruction._operand2 = equates[0];
                                _instructions.push_back(instruction);
                                if(!checkInvalidAddress(ParseType(parse), _currentAddress, uint16_t(outputSize), instruction, lineToken, filename, _lineNumber)) return false;
                            }
                            // Special case PACKVW as it has multiple parseable tokens
                            else if(opcodeType == vCpu  &&  opcode0 == OPCODE_V_PACKVW)
                            {
                                if(tokens.size() < 4)
                                {
                                    Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'PACKVW <src0 var>, <src1 var>, <dst var>' expects 4 parameters '%s'\n", filename.c_str(), _lineNumber+1,
                                                                                                                                                                                        tokens[tokenIndex].c_str());
                                    return false;
                                }

                                Equate equate;
                                uint8_t equates[3] = {0x00};
                                int commaTokenIndex = -1;
                                for(int i=0; i<3; i++)
                                {
                                    int tokIdx = (commaTokenIndex >= 0) ? commaTokenIndex + 1 : tokenIndex + i;
                                    if(tokIdx >= int(tokens.size()))
                                    {
                                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'PACKVW' syntax error in '%s'\n", filename.c_str(), _lineNumber+1, lineToken._text.c_str());
                                        return false;
                                    }

                                    // Search equates
                                    if((operandValid = evaluateEquateOperand(tokens, tokIdx, equate, commaTokenIndex, compoundInstruction)) == true)
                                    {
                                        equates[i] = uint8_t(equate._operand);
                                    }
                                    // Can be a literal byte
                                    else
                                    {
                                        operandValid = Expression::stringToU8(tokens[tokIdx], equates[i]);
                                    }
                                    if(!operandValid)
                                    {
                                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'PACKVW' invalid equate/literal '%s'\n", filename.c_str(), _lineNumber+1, tokens[tokIdx].c_str());
                                        return false;
                                    }
                                }

                                instruction._operand0 = equates[2]; // native code expects the operands swapped for PACKVW
                                instruction._operand1 = equates[1];
                                instruction._operand2 = equates[0];
                                _instructions.push_back(instruction);
                                if(!checkInvalidAddress(ParseType(parse), _currentAddress, uint16_t(outputSize), instruction, lineToken, filename, _lineNumber)) return false;
                            }
                            // Special case CNVXY as it has multiple parseable tokens
                            else if(opcodeType == vCpu  &&  opcode0 == OPCODE_V_CNVXY)
                            {
                                if(tokens.size() < 4)
                                {
                                    Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'CNVXY <src0 var>, <src1 var>, <dst var>' expects 4 parameters '%s'\n", filename.c_str(), _lineNumber+1,
                                                                                                                                                                                        tokens[tokenIndex].c_str());
                                    return false;
                                }

                                Equate equate;
                                uint8_t equates[3] = {0x00};
                                int commaTokenIndex = -1;
                                for(int i=0; i<3; i++)
                                {
                                    int tokIdx = (commaTokenIndex >= 0) ? commaTokenIndex + 1 : tokenIndex + i;
                                    if(tokIdx >= int(tokens.size()))
                                    {
                                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'CNVXY' syntax error in '%s'\n", filename.c_str(), _lineNumber+1, lineToken._text.c_str());
                                        return false;
                                    }

                                    // Search equates
                                    if((operandValid = evaluateEquateOperand(tokens, tokIdx, equate, commaTokenIndex, compoundInstruction)) == true)
                                    {
                                        equates[i] = uint8_t(equate._operand);
                                    }
                                    // Can be a literal byte
                                    else
                                    {
                                        operandValid = Expression::stringToU8(tokens[tokIdx], equates[i]);
                                    }
                                    if(!operandValid)
                                    {
                                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'CNVXY' invalid equate/literal '%s'\n", filename.c_str(), _lineNumber+1, tokens[tokIdx].c_str());
                                        return false;
                                    }
                                }

                                instruction._operand0 = equates[2]; // native code expects the operands swapped for CNVXY
                                instruction._operand1 = equates[1];
                                instruction._operand2 = equates[0];
                                _instructions.push_back(instruction);
                                if(!checkInvalidAddress(ParseType(parse), _currentAddress, uint16_t(outputSize), instruction, lineToken, filename, _lineNumber)) return false;
                            }
                            else
                            {
                                Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : unknown 4 byte instruction '%s'\n", filename.c_str(), _lineNumber+1, tokens[tokenIndex].c_str());
                                return false;
                            }
                        }
                        break;

                        case FiveBytes:
                        {
                            // Special case STARRI as it has multiple parseable tokens
                            if(opcodeType == vCpu  &&  opcode0 == OPCODE_V_STARRI)
                            {
                                if(tokens.size() < 4)
                                {
                                    Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'STARRI <index var>, <16bit imm array addr>, <8 bit imm>' expects 3 parameters '%s'\n", filename.c_str(), _lineNumber+1,
                                                                                                                                                                                                       tokens[tokenIndex].c_str());
                                    return false;
                                }

                                Equate equate;
                                int commaTokenIndex = -1;

                                // Search equates for index var
                                uint8_t index = 0x00;
                                if((operandValid = evaluateEquateOperand(tokens, tokenIndex, equate, commaTokenIndex, compoundInstruction)) == true)
                                {
                                    index = uint8_t(equate._operand);
                                }
                                // Can be a literal byte
                                else
                                {
                                    operandValid = Expression::stringToU8(tokens[tokenIndex], index);
                                }
                                if(!operandValid  ||  commaTokenIndex == -1)
                                {
                                    Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'STARRI' invalid equate/literal or missing comma '%s'\n", filename.c_str(), _lineNumber+1,
                                                                                                                                                                         tokens[tokenIndex].c_str());
                                    return false;
                                }

                                // Label/equate operand
                                uint16_t address = 0x0000;
                                if(!validateOperand(tokens, commaTokenIndex + 1, compoundInstruction, address))
                                {
                                    Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'STARRI' invalid label/equate '%s'\n", filename.c_str(), _lineNumber+1, tokens[tokenIndex].c_str());
                                    return false;
                                }

                                // Search equates for 8bit immediate value
                                uint16_t immediate = 0x00;
                                if((operandValid = validateOperand(tokens, commaTokenIndex + 2, compoundInstruction, immediate)) == false)
                                {
                                    // Can be a literal byte
                                    operandValid = Expression::stringToU16(tokens[commaTokenIndex + 2], immediate);
                                }
                                if(!operandValid  ||  commaTokenIndex == -1)
                                {
                                    Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : 'STARRI' invalid equate/literal or missing comma '%s'\n", filename.c_str(), _lineNumber+1,
                                                                                                                                                                         tokens[tokenIndex].c_str());
                                    return false;
                                }

                                instruction._operand0 = index;
                                instruction._operand1 = uint8_t(LO_BYTE(address));
                                instruction._operand2 = uint8_t(HI_BYTE(address));
                                instruction._operand3 = uint8_t(LO_BYTE(immediate));
                                _instructions.push_back(instruction);
                                if(!checkInvalidAddress(ParseType(parse), _currentAddress, uint16_t(outputSize), instruction, lineToken, filename, _lineNumber)) return false;
                            }
                            else
                            {
                                uint32_t operand = 0x0000;
                                Label label;
                                Equate equate;
                                int commaTokenIndex = -1;

                                // Search equates
                                if((operandValid = evaluateEquateOperand(tokens, tokenIndex, equate, commaTokenIndex, compoundInstruction)) == true)
                                {
                                    operand = uint32_t(equate._operand);
                                }
                                // Search labels
                                else if((operandValid = evaluateLabelOperand(tokens, tokenIndex, label, compoundInstruction)) == true)
                                {
                                    operand = label._address;
                                }
                                else if(Expression::isExpression(tokens[tokenIndex]) == Expression::HasOperators)
                                {
                                    Expression::Numeric value;
                                    std::string input;
                                    preProcessExpression(tokens, tokenIndex, input, true);
                                    if(!Expression::parse((char*)input.c_str(), _lineNumber, value)) return false;
                                    operand = uint32_t(std::lround(value._value));
                                    operandValid = true;
                                }
                                else
                                {
                                    operandValid = Expression::stringToU32(tokens[tokenIndex], operand);
                                    if(!operandValid)
                                    {
                                        Cpu::reportError(Cpu::AsmError, stderr, "Assembler::assemble() : '%s:%d' : invalid label/equate '%s'\n", filename.c_str(), _lineNumber+1, tokens[tokenIndex].c_str());
                                        return false;
                                    }
                                }

                                instruction._operand0 = uint8_t(BYTE_0(operand));
                                instruction._operand1 = uint8_t(BYTE_1(operand));
                                instruction._operand2 = uint8_t(BYTE_2(operand));
                                instruction._operand3 = uint8_t(BYTE_3(operand));

                                // 5BYTE has it's operands swapped
                                //if(opcodeType == vCpu  &&  opcode0 == OPCODE_V_5BYTE)
                                //{
                                //    std::swap(instruction._operand0, instruction._operand3);
                                //    std::swap(instruction._operand1, instruction._operand2);
                                //}

                                _instructions.push_back(instruction);
                                if(!checkInvalidAddress(ParseType(parse), _currentAddress, uint16_t(instruction._byteSize), instruction, lineToken, filename, _lineNumber)) return false;

                                break;
                            }
                        }
                        break;

                        default: break;
                    }
                }

                _currentAddress += uint16_t(outputSize);
            }              
        }

        // Pack byte code buffer from instruction buffer
        packByteCodeBuffer();

#ifndef STAND_ALONE
        // Parse gprintf labels, equates and expressions
        if(!parseGprintfs()) return false;
#endif

        return true;
    }
}
