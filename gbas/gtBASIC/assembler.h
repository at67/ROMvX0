#ifndef ASSEMBLER_H
#define ASSEMBLER_H


#include <stdint.h>
#include <string>
#include <vector>


#define OPCODE_N_LD  0x00
#define OPCODE_N_AND 0x20
#define OPCODE_N_OR  0x40
#define OPCODE_N_XOR 0x60
#define OPCODE_N_ADD 0x80
#define OPCODE_N_SUB 0xA0
#define OPCODE_N_ST  0xC0
#define OPCODE_N_J   0xE0

#define EA_0D_AC    0x00
#define EA_0X_AC    0x04
#define EA_YD_AC    0x08
#define EA_YX_AC    0x0C
#define EA_0D_X     0x10
#define EA_0D_Y     0x14
#define EA_0D_OUT   0x18
#define EA_YX_OUTIX 0x1C

#define BUS_D   0x00
#define BUS_RAM 0x01
#define BUS_AC  0x02
#define BUS_IN  0x03

#define BRA_CC_FAR    0x00
#define BRA_CC_GT     0x04
#define BRA_CC_LT     0x08
#define BRA_CC_NE     0x0C
#define BRA_CC_EQ     0x10
#define BRA_CC_GE     0x14
#define BRA_CC_LE     0x18
#define BRA_CC_ALWAYS 0x1C

#define DEFAULT_EXEC_ADDRESS 0x0200
#define DEFAULT_CALL_TABLE   0x0000

#define USER_ROMv1_ADDRESS 0x0B00 // pictures in ROM v1

#define OPCODE_V_MOVQB  0x16
#define OPCODE_V_LOKEQI 0x1C
#define OPCODE_V_ADDBI  0x2D
#define OPCODE_V_PREFX2 0x2F
#define OPCODE_V_DBNE   0x32
#define OPCODE_V_BCC    0x35
#define OPCODE_V_DOKEI  0x37
#define OPCODE_V_LOKEI  0x3D
#define OPCODE_V_ADDVI  0x42
#define OPCODE_V_SUBVI  0x44
#define OPCODE_V_DJGE   0x4A
#define OPCODE_V_MOVQW  0x5B
#define OPCODE_V_MOVB   0x65
#define OPCODE_V_DBGE   0x8E
#define OPCODE_V_BRA    0x90
#define OPCODE_V_SPARE1 0x95
#define OPCODE_V_ANDBK  0x9E
#define OPCODE_V_ORBK   0xA0
#define OPCODE_V_XORBK  0xA2
#define OPCODE_V_DJNE   0xA4
#define OPCODE_V_CMPI   0xA7
#define OPCODE_V_ADDVW  0xA9
#define OPCODE_V_SUBVW  0xAB
#define OPCODE_V_PREFX1 0xB1
#define OPCODE_V_JEQ    0xBB
#define OPCODE_V_JNE    0xBD
#define OPCODE_V_JLT    0xBF
#define OPCODE_V_JGT    0xC1
#define OPCODE_V_JLE    0xC3
#define OPCODE_V_JGE    0xC5
#define OPCODE_V_PREFX3 0xC7
#define OPCODE_V_CALL   0xCF
#define OPCODE_V_SPARE2 0xDD
#define OPCODE_V_SUBBI  0xE1

// PREFX1 instructions
#define OPCODE_V_NOTE   0x11
#define OPCODE_V_MIDI   0x14
#define OPCODE_V_XLA    0x17

// PREFX2 instructions
#define OPCODE_V_LSLN   0x11
#define OPCODE_V_SEXT   0x13
#define OPCODE_V_NOTW   0x15
#define OPCODE_V_NEGW   0x17
#define OPCODE_V_ANDBA  0x19
#define OPCODE_V_ORBA   0x1C
#define OPCODE_V_XORBA  0x1F
#define OPCODE_V_FREQM  0x22
#define OPCODE_V_FREQA  0x24
#define OPCODE_V_FREQZI 0x27
#define OPCODE_V_VOLM   0x2A
#define OPCODE_V_VOLA   0x2C
#define OPCODE_V_MODA   0x2F
#define OPCODE_V_MODZI  0x32
#define OPCODE_V_SMPCPY 0x34
#define OPCODE_V_CMPWS  0x37
#define OPCODE_V_CMPWU  0x39
#define OPCODE_V_LEEKA  0x3B
#define OPCODE_V_LOKEA  0x3D
#define OPCODE_V_FEEKA  0x3F
#define OPCODE_V_FOKEA  0x42
#define OPCODE_V_MEEKA  0x44
#define OPCODE_V_MOKEA  0x46
#define OPCODE_V_LSRVL  0x48
#define OPCODE_V_LSLVL  0x4B
#define OPCODE_V_INCL   0x4E
#define OPCODE_V_DECL   0x51

// PREFX3 instructions
#define OPCODE_V_STB2   0x11
#define OPCODE_V_STW2   0x14
#define OPCODE_V_XCHGB  0x17
#define OPCODE_V_MOVW   0x19
#define OPCODE_V_ADDWI  0x1B
#define OPCODE_V_SUBWI  0x1D
#define OPCODE_V_ANDWI  0x1F
#define OPCODE_V_ORWI   0x21
#define OPCODE_V_XORWI  0x23
#define OPCODE_V_LDPX   0x25
#define OPCODE_V_STPX   0x28
#define OPCODE_V_CONDI  0x2B
#define OPCODE_V_CONDB  0x2D
#define OPCODE_V_CONDIB 0x30
#define OPCODE_V_CONDBI 0x33
#define OPCODE_V_XCHGW  0x35
#define OPCODE_V_OSCPX  0x38
#define OPCODE_V_SWAPB  0x3A
#define OPCODE_V_SWAPW  0x3D
#define OPCODE_V_NEEKA  0x40
#define OPCODE_V_NOKEA  0x43
#define OPCODE_V_ADDVL  0x46
#define OPCODE_V_SUBVL  0x49
#define OPCODE_V_ANDVL  0x4C
#define OPCODE_V_ORVL   0x4F
#define OPCODE_V_XORVL  0x52
#define OPCODE_V_JEQL   0x55
#define OPCODE_V_JNEL   0x58
#define OPCODE_V_JLTL   0x5B
#define OPCODE_V_JGTL   0x5E
#define OPCODE_V_JLEL   0x61
#define OPCODE_V_JGEL   0x64
#define OPCODE_V_ANDBI  0x67
#define OPCODE_V_ORBI   0x6A
#define OPCODE_V_XORBI  0x6D

#define OPCODE_V_HALT   0xB4
#define OPERAND_V_HALT  0x80

#define GIGA_V_AC 0x18

#define MAX_INST_LENGTH   24  // this includes vPC, ($XXXX), instruction, (XXXXX), and 2 byte operands, ($XX) and ($YY), e.g. "$0200 ADDBI $30 $31"


namespace Assembler
{
    enum ByteSize {BadByteSize=-1, OneByte=1, TwoBytes=2, ThreeBytes=3, FourBytes=4, FiveBytes=5, MaxByteSize};
    enum OpcodeType {ReservedDB=0, ReservedDW, ReservedDBR, ReservedDWR, vCpu, Native};

    struct ByteCode
    {
        bool _isRomAddress;
        bool _isCustomAddress;
        uint8_t _data;
        uint16_t _address;
    };

    struct DasmCode
    {
        uint8_t _opcode;
        uint8_t _byteSize;
        uint8_t _operand0;
        uint8_t _operand1;
        uint8_t _operand2;
        uint16_t _address;
        std::string _text;
    };

    struct InstructionType
    {
        uint8_t _opcode0;
        uint8_t _opcode1;
        ByteSize _byteSize;
        OpcodeType _opcodeType;
    };

    struct InstructionDasm
    {
        uint8_t _opcode;
        uint8_t _branch;
        ByteSize _byteSize;
        OpcodeType _opcodeType;
        std::string _mnemonic;
    };

    struct LineToken
    {
        bool _fromInclude = false;
        int _includeLineNumber;
        std::string _text;
        std::string _includeName;
    };

    struct Gprintf
    {
        enum Format {Chr, Byt, Int, Bit, Oct, Hex, Str};
        struct Var
        {
            int _indirection = 0;
            Format _format;
            int _width;
            uint16_t _data;
            std::string _text;
            bool _isUpper = false;
        };

        uint16_t _address;
        int _lineNumber;
        std::string _lineToken;
        std::string _format;
        std::vector<Var> _vars;
        std::vector<std::string> _subs;
    };

    struct Define
    {
        bool _enabled = false;
        bool _toggle = false;
        int32_t _value = 0;
        std::string _name;
    };


    uint8_t getvSpMin(void);
    uint16_t getStartAddress(void);
    int getCurrDasmByteCount(void);
    int getPrevDasmByteCount(void);
    int getPrevDasmPageByteCount(void);
    int getCurrDasmPageByteCount(void);
    int getDisassembledCodeSize(void);
    DasmCode* getDisassembledCode(int index);
    const std::string& getIncludePath(void);

    void setvSpMin(uint8_t vSpMin);
    void setIncludePath(const std::string& includePath);

    int getAsmOpcodeSize(const std::string& opcodeStr);
    int getAsmOpcodeSizeText(const std::string& textStr);
    int getAsmOpcodeSizeFile(const std::string& filename);

    void clearDefines(void);
    bool createDefine(const std::string& filename, const std::vector<std::string>& tokens, int adjustedLineIndex);
    bool handleIfDefine(const std::string& filename, const std::vector<std::string>& tokens, int adjustedLineIndex);
    bool handleEndIfDefine(const std::string& filename, const std::vector<std::string>& tokens, int adjustedLineIndex);
    bool handleElseDefine(const std::string& filename, const std::vector<std::string>& tokens, int adjustedLineIndex);
    bool isCurrentDefineDisabled(void);
    int16_t getRuntimeVersion(void);

    void initialise(void);
    void clearAssembler(bool dontClearGprintfs=false);
    bool getNextAssembledByte(ByteCode& byteCode, bool debug=false);

    bool assemble(const std::string& filename, uint16_t startAddress=DEFAULT_EXEC_ADDRESS, bool dontClearGprintfs=false);
    int disassemble(uint16_t address);

#ifndef STAND_ALONE
    bool addGprintf(const Gprintf& gprintf, uint16_t address);
    bool parseGprintfFormat(const std::string& fmtstr, const std::vector<std::string>& variables, std::vector<Gprintf::Var>& vars, std::vector<std::string>& subs);
    bool getGprintfString(const Gprintf& gprintf, std::string& gstring);
    void handleGprintfs(void);

    bool getNativeMnemonic(uint8_t opcode, uint8_t operand, char* mnemonic);
    bool getVCpuMnemonic(uint16_t address, uint8_t opcode, uint8_t operand0, uint8_t operand1, uint8_t operand2, uint8_t operand3, ByteSize& byteSize, char* mnemonic);
#endif
}

#endif