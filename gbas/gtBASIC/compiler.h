#ifndef COMPILER_H
#define COMPILER_H

#include <vector>
#include <stack>
#include <map>
#include <memory>
#include <algorithm>

#include "expression.h"


#define LABEL_TRUNC_SIZE   34 // The smaller you make this, the more your BASIC label names will be truncated in the resultant .vasm code
#define OPCODE_TRUNC_SIZE  34 // The smaller you make this, the more your VASM opcode/macro names will be truncated in the resultant .vasm code
#define USER_STR_SIZE      94
#define NUM_STR_WORK_AREAS 2
#define LOOP_VARS_SIZE     4
#define MAX_NESTED_LOOPS   4
#define MAX_ARRAY_DIMS     3

#define BLIT_CHUNK_SIZE         6
#define BLIT_STRIPE_CHUNKS_LO  15  // 15 fits into a 96 byte page
#define BLIT_STRIPE_CHUNKS_HI  40  // 40 fits into a 256 byte page
#define MAX_NUM_BLITS_LO      128  // maximum number of BLIT's for 32K RAM
#define MAX_NUM_BLITS_HI      1024 // maximum number of BLIT's for 64K RAM
#define MAX_NUM_PATTERNS      128  // maximum number of PATTERNS

#define SPRITE_WIDTH        9
#define MAX_SPRITE_HEIGHT 120

// 16 bytes, (0x00F0 <-> 0x00FF), reserved for vCPU stack, allows for 8 nested calls. The amount of nested GOSUBS/CALLS you can use is dependant on how
// much of the stack is being used by nested system calls. *NOTE* there is NO call table for user code for this compiler
// do NOT change the order of these defines, code that relies on their order will break! e.g. Compiler::createIntVar()
#define USER_VAR_START   0x0030  // 80 bytes, (0x0030 <-> 0x007F), reserved for BASIC user variables
#define SER_RAW_PREV     0x0081  // 1 byte,   (0x0081 <-> 0x0081), previous version of serialRaw, used by the BASIC runtime for input edge detection
#define INT_VAR_START    0x0082  // 46 bytes, (0x0082 <-> 0x00AF), internal register variables, used by the BASIC runtime
#define CONVERT_CC_OPS   0x00B0  // 12 bytes, (0x00B0 <-> 0x00BB), critical relational operator routines that can't straddle page boundaries
#define CONVERT_ARRAY    0x00BC  // 8 bytes,  (0x00BC <-> 0x00C3), critical array accessing routines
#define LOCAL_VAR_START  0x00C4  // 16 bytes, (0x00C4 <-> 0x00D3), reserved for function/procedure params and local vars
#define REAL_TIME_PROC   0x00D4  // 2 bytes,  (0x00D4 <-> 0x00D5), critical time sliced routine that usually handles TIME/MIDI, etc
#define FRAME_COUNT_PREV 0x00D6  // 2 bytes,  (0x00D6 <-> 0x00D7), robust vertical blank
#define LOOP_VAR_START   0x00D8  // 16 bytes, (0x00D8 <-> 0x00E7), reserved for FOR loops with vars, maximum of 4 nested FOR loops
#define TEMP_VAR_START   0x00E8  // 8 bytes,  (0x00E8 <-> 0x00EF), reserved for temporary expression variables
#define TEMP_VAR_SIZE    8
#define USER_CODE_START  0x0200
#define RUN_TIME_START   0x7FFF
#define USER_VAR_END     0x007F
#define REG_WORK_SIZE    16
#define GPRINT_VAR_ADDRS 16

// Misc flag bits
#define MISC_WAIT_VBLANK_BIT   0x01
#define MISC_ENABLE_SCROLL_BIT 0x02
#define MISC_ON_BOTTOM_ROW_BIT 0x04
#define MISC_DISABLE_CLIP_BIT  0x08

// Misc flag masks
#define MISC_WAIT_VBLANK_MSK   0xFE
#define MISC_ENABLE_SCROLL_MSK 0xFD
#define MISC_ON_BOTTOM_ROW_MSK 0xFB
#define MISC_DISABLE_CLIP_MSK  0xF7

// Loader.gcl prohibited addresses
#define LOADER_SCANLINE0_START 0x5900
#define LOADER_SCANLINE0_END   0x599F
#define LOADER_SCANLINE1_START 0x5A00
#define LOADER_SCANLINE1_END   0x5A9F
#define LOADER_SCANLINE2_START 0x5B00
#define LOADER_SCANLINE2_END   0x5B9F

#define SYS_INIT_FUNC_LEN 5

#define MODULE_MAIN "Main"


namespace Compiler
{
    enum VarType {ConstInt16, ConstStr, VarInt8, VarInt16, VarInt32, VarStr, VarStr2, VarFloat16, VarFloat32, Var1Arr8, Var2Arr8, Var3Arr8, Var1Arr16, Var2Arr16, Var3Arr16};
    enum IntSize {Int8=1, Int16=2, Int32=4};
    enum ConstStrType {StrChar, StrHex, StrLeft, StrRight, StrMid, StrLower, StrUpper};
    enum IfElseEndType {IfBlock, ElseIfBlock, ElseBlock, EndIfBlock};
    enum OperandType {OperandInvalid, OperandVar, OperandTemp, OperandConst};
    enum StatementResult {StatementError, StatementSuccess, StatementExpression, SingleStatementParsed, MultiStatementParsed, StringStatementParsed, RedoStatementParse};
    enum CodeOptimiseType {CodeSpeed, CodeSize};
    enum BlitFlipType {NoFlip, FlipX, FlipY, FlipXY};
    enum SpriteCmd {SpriteMove, SpritePattern, SpriteShow};
    enum SpritesCmd {SpritesInit, SpritesDraw, SpritesRestore, SpritesShow};
    enum Vec8Cmd {Vec8Add, Vec8Sub, Vec8Mul, Vec8Div, Vec8Chk};
    enum DataType {DataInteger, DataString};
    enum ForNextType {AutoTo, UpTo, DownTo};
    enum TypeVarType {Byte, Word, String, ArrayB, Array2B, Array3B, ArrayW, Array2W, Array3W, ArrayS};
    enum LabelModifierType {LabelModNone, LabelModError, LabelModNoPush};

    struct Constant
    {
        uint8_t _size;
        int16_t _data;
        uint16_t _address;
        std::string _text;
        std::string _name;
        std::string _internalName;
        VarType _varType = ConstInt16;
        int16_t* _lazyUpdate = nullptr;
    };

    struct IntegerVar
    {
        int16_t _data;
        int16_t _init;
        uint16_t _address;
        std::string _name;
        std::string _output;
        int _codeLineIndex = -1;
        VarType _varType = VarInt16;
        int _intSize = Int16;
        std::vector<uint16_t> _arrSizes;
        std::vector<int16_t> _arrInits;
        std::vector<std::vector<uint16_t>> _arrAddrs;
        std::vector<uint16_t> _arrLut;
        bool _arrInit = true;
        bool _withinPage = true;
    };

    struct StringVar
    {
        uint8_t _size;
        uint8_t _maxSize;
        uint16_t _address;
        std::string _text;
        std::string _name;
        std::string _output;
        VarType _varType = VarStr;
        int _codeLineIndex = -1;
        bool _constant = true;
        std::vector<std::string> _arrInits;
        std::vector<uint16_t> _arrAddrs;
        bool _arrInit = true;
    };

    struct InternalLabel
    {
        uint16_t _address = 0x0000;
        std::string _name = "";
    };

    struct Label
    {
        uint16_t _address;
        std::string _name;
        std::string _output;
        int _codeLineIndex = -1;
        bool _numeric = false;
        bool _pageJump = false;
        bool _gosub = false;
        bool _noPush = false;
    };

    struct VasmLine
    {
        uint16_t _address;
        std::string _opcode;
        std::string _operand;
        std::string _code;
        std::string _internalLabel;
        bool _pageJump = false;
        bool _lazyErase = false;
        int _vasmSize = 0;
    };

    struct MacroNameEntry
    {
        std::string _name;
        int _indexEnd;
        int _byteSize;
    };

    struct MacroIndexEntry
    {
        int _indexStart;
        int _indexEnd;
        int _byteSize;
    };

    struct OnGotoGosubLut
    {
        uint16_t _address;
        std::string _name;
        std::vector<int> _lut;
    };

    struct StrConcatLut
    {
        uint16_t _address;
        std::vector<uint16_t> _lut;
    };

    struct InputLut
    {
        uint16_t _address;
        uint16_t _varsAddr;
        uint16_t _strsAddr;
        uint16_t _typesAddr;

        std::vector<uint16_t> _varsLut;
        std::vector<uint16_t> _strsLut;
        std::vector<uint16_t> _typesLut;
    };

    struct Input
    {
        bool _parse = true;
        std::string _text;
    };

    struct CodeLine
    {
        std::string _text;
        std::string _code;
        std::vector<std::string> _tokens;
        std::vector<size_t> _offsets;
        std::vector<VasmLine> _vasm;
        std::string _expression;
        OnGotoGosubLut _onGotoGosubLut;
        StrConcatLut _strConcatLut;
        InputLut _inputLut;
        int _vasmSize = 0;
        int _labelIndex = -1;
        int  _varIndex = -1;
        VarType _varType = VarInt16;
        Expression::Int16Byte _int16Byte = Expression::Int16Both;
        bool _containsVars = false;
        bool _pushEmitted = false;
        bool _dontParse = false;
        bool _optimiserEnabled = true;
        std::string _moduleName = MODULE_MAIN;
    };

    struct ModuleLine
    {
        int _index;
        std::string _name = MODULE_MAIN;
    };

    struct InternalSub
    {
        uint16_t _address;
        uint16_t _size;
        std::string _name;
        std::string _includeName;
        bool _inUse = false;
        bool _loaded = false;
    };

    struct ForNextData
    {
        int _varIndex;
        std::string _labelName;
        int16_t _loopStart;
        int16_t _loopEnd;
        int16_t _loopStep;
        uint16_t _varEnd;
        uint16_t _varStep;
        ForNextType _type = AutoTo;
        bool _farJump = true;
        bool _optimise = true;
        bool _romvx = false;
        int _codeLineIndex;
    };

    struct EndIfData
    {
        int _jmpIndex;
        int _codeLineIndex;
        Expression::CCType _ccType;
    };
    struct ElseIfData
    {
        int _jmpIndex;
        std::string _labelName;
        int _codeLineIndex;
        IfElseEndType _ifElseEndType;
        Expression::CCType _ccType;
        std::stack<EndIfData> _endIfData;
    };

    struct WhileWendData
    {
        int _jmpIndex;
        std::string _labelName;
        int _codeLineIndex;
        Expression::CCType _ccType;
    };

    struct RepeatUntilData
    {
        std::string _labelName;
        int _codeLineIndex;
    };

    struct TypeVar
    {
        uint16_t _address;
        TypeVarType _type;
    };
    struct TypeData
    {
        uint16_t _address;
        std::map<std::string, TypeVar> _vars;
    };

    struct CallData
    {
        int _numParams = 0;
        int _codeLineIndex;
        std::string _name;
    };

    struct ProcData
    {
        int _numParams = 0;
        int _numLocals = 0;
        int _codeLineIndex;
        std::string _name;
        std::map<std::string, std::string> _localVarNameMap;
    };

    struct DefDataByte
    {
        uint16_t _address;
        uint16_t _offset;
        std::vector<uint8_t> _data;
    };
    struct DefDataWord
    {
        uint16_t _address;
        uint16_t _offset;
        std::vector<int16_t> _data;
    };

    struct DefDataImage
    {
        uint16_t _address;
        uint16_t _width, _height, _stride;
        std::vector<uint8_t> _data;
    };

    // Image data that conflicts with loader must be handled separately
    const int DefDataLoaderImageChunkLutEntrySize = 5;
    struct DefDataLoaderImageChunk
    {
        struct LutEntry
        {
            uint16_t _srcAddr, _dstAddr;
            uint8_t _length;
        } _lutEntry;

        std::vector<uint8_t> _data;
    };

    struct DefDataMidi
    {
        int _id;
        bool _volume = false;
        uint8_t _loops = 0;
        std::vector<uint8_t> _data;
        std::vector<uint16_t> _segmentSizes;
        std::vector<uint16_t> _segmentAddrs;
    };

    struct DefDataOpen
    {
        enum OpenMode {OpenRead, OpenWrite, OpenAppend, OpenUpdateRW, OpenCreateRW, OpenAppendR};

        int _device;
        int _id;
        std::string _path;
        std::string _name;
        OpenMode _mode;
    };

    struct DefDataBlit
    {
        int _id;
        std::string _filename;
        uint16_t _width, _height;
        uint16_t _numColumns, _numStripesPerCol;
        uint16_t _numStripeChunks, _remStripeChunks;
        std::vector<uint16_t> _stripeAddrs;
        std::vector<uint8_t> _data;
        BlitFlipType _flipType = NoFlip;
        bool _isInstanced = false;
    };
    struct BlitsAddrLut
    {
        uint16_t _address;
        std::vector<uint16_t> _addrs;
    };

    struct DefDataFont
    {
        int id;
        std::string _filename;
        uint16_t _width, _height;
        std::vector<uint16_t> _charAddrs;
        std::vector<std::vector<uint8_t>> _data;
        uint16_t _mapAddr;
        std::vector<uint8_t> _mapping;
        uint16_t _baseAddr;
        uint16_t _fgbgColour;
    };
    struct FontsAddrLut
    {
        uint16_t _address;
        std::vector<uint16_t> _addrs;
    };

    struct DefDataSprite
    {
        int _id;
        std::string _filename;
        uint16_t _width, _height;
        uint16_t _address;
        std::vector<uint8_t> _data;
    };
    struct SpritesAddrLut
    {
        uint16_t _address;
        uint16_t _tmpAddr;
        uint16_t _indicesAddr;
        uint16_t _xPosAddr;
        uint16_t _xPosTmpAddr;
        uint16_t _yPosAddr;
        uint16_t _yPosTmpAddr;
        uint16_t _heightAddr;
        std::vector<uint8_t> _addrs;
        bool _constantHeight = false;
    };

    struct DefDataPattern
    {
        int _id;
        std::string _filename;
        uint16_t _width, _height;
        std::vector<uint16_t> _addrs;
        std::vector<uint8_t> _data;
    };
    struct PatternAddrLut
    {
        int _id;
        uint16_t _address;
        std::vector<uint16_t> _addrs;
    };
    struct PatternsAddrLut
    {
        uint16_t _address;
        std::vector<uint16_t> _addrs;
    };

    struct DefFunction
    {
        std::string _name;
        std::string _function;
        std::vector<std::string> _params;
    };

    struct DataObject
    {
        DataType _dataType;
        uint16_t _address = 0x0000;
    };
    struct DataInt : DataObject
    {
        DataInt(int16_t data) {_dataType = DataInteger; _data = data;}

        int16_t _data = 0;
    };
    struct DataStr : DataObject
    {
        DataStr(const std::string& data) {_dataType = DataString; _data = data;}

        std::string _data;
    };


    uint16_t getVasmPC(void);
    uint16_t getUserCodeStart(void);
    uint16_t getRuntimeEnd(void);
    uint16_t getRuntimeStart(void);
    uint16_t getArraysStart(void);
    uint16_t getStringsStart(void);
    uint16_t getTempVarStart(void);
    uint16_t getTempVarSize(void);
    uint16_t getRegWorkArea(void);
    uint16_t getGprintfVarsAddr(void);
    uint16_t getStrWorkArea(void);
    uint16_t getStrWorkArea(int index);
    uint16_t getBlitsAddrLutAddress(void);
    uint16_t getBlitStripeChunks(void);
    uint16_t getBlitStripeMinAddress(void);
    uint16_t getBlitStripeMaxAddress(void);
    Memory::FitType getBlitStripeFitType(void);
    CodeOptimiseType getCodeOptimiseType(void);
    Cpu::RomType getCodeRomType(void);
    const std::map<std::string, int>& getBranchTypes(void);
    bool getArrayIndiciesOne(void);
    bool getCreateTimeData(void);
    int getCurrentLabelIndex(void);
    int getCurrentCodeLineIndex(void); 
    int getNumNumericLabels(void);
    int getCodeLineStart(int index);
    const std::string& getRuntimePath(void);
    const std::string& getTempVarStartStr(void);

    const InternalLabel& getNextInternalLabel(void);
    
    void setAssignVar(bool assignVar);
    void setCodeIsAsm(bool codeIsAsm);
    void setUserCodeStart(uint16_t userCodeStart);
    void setRuntimeEnd(uint16_t runtimeEnd);
    void setRuntimePath(const std::string& runtimePath);
    void setRuntimeStart(uint16_t runtimeStart);
    void setArraysStart(uint16_t arraysStart);
    void setStringsStart(uint16_t stringsStart);
    void setTempVarStart(uint16_t tempVarStart);
    void setTempVarSize(uint16_t tempVarSize);
    void setRegWorkArea(uint16_t regWorkArea);
    void setGprintfVarsAddr(uint16_t gprintfVarsAddr);
    void setStrWorkArea(uint16_t strWorkArea, int index=0);
    void setBlitsAddrLutAddress(uint16_t blitsAddrLutAddress);
    void setBlitStripeChunks(uint16_t blitStripeChunks);
    void setBlitStripeMinAddress(uint16_t blitStripeMinAddress);
    void setBlitStripeMaxAddress(uint16_t blitStripeMaxAddress);
    void setBlitStripeFitType(Memory::FitType blitStripeFitType);
    void setCodeOptimiseType(CodeOptimiseType codeOptimiseType);
    void setCodeRomType(Cpu::RomType codeRomType);
    void setCreateNumericLabelLut(bool createNumericLabelLut);
    void setCreateTimeData(bool createTimeArrays);
    void setArrayIndiciesOne(bool arrayIndiciesOne);

    void nextStrWorkArea(void);

    int getNextJumpFalseUniqueId(void);

    std::vector<Label>& getLabels(void);
    std::vector<Constant>& getConstants(void);
    std::vector<CodeLine>& getCodeLines(void);
    std::vector<ModuleLine>& getModuleLines(void);
    std::vector<IntegerVar>& getIntegerVars(void);
    std::vector<StringVar>& getStringVars(void);
    std::map<std::string, TypeData>& getTypeDatas(void);
    std::vector<InternalLabel>& getInternalLabels(void);
    std::vector<InternalLabel>& getDiscardedLabels(void);
    std::vector<std::string>& getOutput(void);
    std::vector<std::string>& getRuntime(void);
    std::vector<DefDataByte>& getDefDataBytes(void);
    std::vector<DefDataWord>& getDefDataWords(void);
    std::vector<DefDataImage>& getDefDataImages(void);
    std::vector<DefDataLoaderImageChunk>& getDefDataLoaderImageChunks(void);

    std::map<int, DefDataMidi>& getDefDataMidis(void);

    std::map<int, DefDataOpen>& getDefDataOpens(void);
    std::map<std::string, uint16_t>& getOpenDevices(void);

    std::map<int, DefDataBlit>& getDefDataBlits(void);
    BlitsAddrLut& getBlitsAddrLut(void);

    std::map<int, DefDataFont>& getDefDataFonts(void);
    FontsAddrLut& getFontsAddrLut(void);

    std::map<int, DefDataSprite>& getDefDataSprites(void);
    SpritesAddrLut& getSpritesAddrLut(void);

    std::map<int, DefDataPattern>& getDefDataPatterns(void);

    std::map<std::string, DefFunction>& getDefFunctions(void);
    std::vector<std::unique_ptr<DataObject>>& getDataObjects(void);

    std::map<std::string, MacroIndexEntry>& getMacroIndexEntries(void);
    
    std::stack<ForNextData>& getForNextDataStack(void);
    std::stack<ElseIfData>& getElseIfDataStack(void);
    std::stack<WhileWendData>& getWhileWendDataStack(void);
    std::stack<RepeatUntilData>& getRepeatUntilDataStack(void);

    std::map<std::string, CallData>& getCallDataMap(void);

    std::stack<ProcData>& getProcDataStack(void);
    std::map<std::string, ProcData>& getProcDataMap(void);

    void setNextInternalLabelAddress(uint16_t address);
    void setNextInternalLabel(const std::string& labelName);
    void adjustDiscardedLabels(const std::string& labelName, uint16_t address);
    void adjustLabelAddresses(uint16_t address, int offset);
    void adjustVasmAddresses(int codeLineIndex, uint16_t address, int offset);

    bool moveVblankVars(const Compiler::CodeLine& codeLine, int codeLineIndex);
    bool setBuildPath(const std::string& buildpath, const std::string& filepath);

    bool initialise(void);
    bool initialiseMacros(void);

    void enableSysInitFunc(const std::string& sysInitFunc);
    void enableOptimiser(bool enable);

    bool parsePragmas(std::vector<Input>& input, int numLines);
    bool parseLabels(std::vector<Input>& input, int numLines);

    Expression::Numeric expression(bool returnAddress=false);

    int findLabel(const std::string& labelName);
    int findLabel(uint16_t address);
    int findInternalLabel(const std::string& labelName);
    int findInternalLabel(uint16_t address);
    int findConst(std::string& constName);
    int findVar(std::string& varName, bool subAlpha=true);
    int findVar(std::string& varName, std::string& oldName, bool subAlpha=true);
    int findStr(std::string& strName);

    Compiler::VarType convertVarType(Expression::VarType varTypeE);
    Expression::VarType convertVarType(Compiler::VarType varTypeC);

    void writeArrayVarNoAssign(CodeLine& codeLine, int codeLineIndex, int varIndex);
    bool writeArrayStrNoAssign(std::string& arrText, int codeLineIndex, int strIndex);

    bool createCodeLine(const std::string& code, int codeLineStart, int labelIndex, int varIndex, Expression::Int16Byte int16Byte, bool vars, CodeLine& codeLine, const std::string& moduleName=MODULE_MAIN);
    void createLabel(uint16_t address, const std::string& name, int codeLineIndex, Label& label, bool numeric=false, bool addUnderscore=true, bool pageJump=false, bool gosub=false, bool noPush=false);
    bool createIntVar(const std::string& varName, int16_t data, int16_t init, CodeLine& codeLine, int codeLineIndex, bool containsVars, int& varIndex);
    void createProcIntVar(const std::string& varName, int16_t data, int16_t init, CodeLine& codeLine, int codeLineIndex, bool containsVars, uint16_t address, int& varIndex);
    void createArrIntVar(const std::string& varName, int16_t data, int16_t init, CodeLine& codeLine, int codeLineIndex, bool containsVars, bool isInit, int& varIndex, VarType varType, int intSize,
                         uint16_t address, std::vector<uint16_t>& arrSizes, const std::vector<int16_t>& arrInits, std::vector<std::vector<uint16_t>>& arrAddrs, std::vector<uint16_t>& arrLut);
    int getOrCreateString(CodeLine& codeLine, int codeLineIndex, const std::string& str, std::string& name, uint16_t& address, uint8_t maxSize=USER_STR_SIZE, bool constString=true, VarType varType=VarStr);
    uint16_t getOrCreateConstString(const std::string& input, int& index);
    uint16_t getOrCreateConstString(ConstStrType constStrType, int16_t input, int& index);
    uint16_t getOrCreateConstString(ConstStrType constStrType, const std::string& input, int8_t length, uint8_t offset, int& index);
    int createStringArray(CodeLine& codeLine, int codeLineIndex, const std::string& name, uint8_t size, bool isInit, std::vector<std::string>& arrInits, std::vector<uint16_t>& arrAddrs);
    void getOrCreateString(const Expression::Numeric& numeric, std::string& name, uint16_t& addr, int& index);
    void emitStringAddress(const Expression::Numeric& numeric, uint16_t address);

    void updateIntVar(int16_t data, CodeLine& codeLine, int varIndex, bool containsVars);

    bool findMacroText(const std::string& macroName, const std::string& text);
    int getMacroSize(const std::string& macroName);

    bool eraseVcpuAsm(int codeLineIdx);
    int createVcpuAsm(const std::string& opcodeStr, const std::string& operandStr, int codeLineIdx, std::string& line);
    std::pair<int, int> emitVcpuAsm(const std::string& opcodeStr, const std::string& operandStr, bool nextTempVar, int codeLineIdx=-1, const std::string& labelName="", bool pageJump=false);
    int16_t emitVcpuAsmForLiteral(Expression::Numeric& numeric, bool useHex);
    int16_t emitVcpuAsmForLiteral(const std::string& opcode, Expression::Numeric& numeric, bool useHex);
    int16_t emitVcpuAsmForLiteral(Expression::Numeric& numeric, const std::string& operand, bool useHex);
    void createVcpuAsmLabel(int codeLineIdxBra, int vcpuAsmBra, int codeLineIdxDst, int vcpuAsmDst, const std::string& label);
    bool emitVcpuAsmUserVar(const std::string& opcodeStr, Expression::Numeric& numeric, bool nextTempVar);
    void emitVcpuPreProcessingCmd(const std::string& cmdStr);
    void getNextTempVar(void);

    uint32_t isExpression(std::string& input, int& varIndex, int& constIndex, int& strIndex);
    OperandType parseStaticExpression(int codeLineIndex, std::string& expression, std::string& operand, Expression::Numeric& numeric);
    uint32_t parseExpression(int codeLineIndex, std::string& expression, Expression::Numeric& numeric);
    uint32_t handleExpression(int codeLineIndex, std::string& expression, Expression::Numeric numeric);
    StatementResult parseMultiStatements(const std::string& code, int codeLineIndex, int codeLineStart, int& varIndex, int& strIndex);

    void addLabelToBooleanCC(Compiler::VasmLine& vasm, const std::string& opcode, const std::string& label);
    void addLabelToJumpCC(std::vector<VasmLine>& vasm, const std::string& label);
    void addLabelToJump(std::vector<VasmLine>& vasm, const std::string& label);

    bool compile(const std::string& inputFilename, const std::string& outputFilename);
}

#endif