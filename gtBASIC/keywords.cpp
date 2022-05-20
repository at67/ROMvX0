#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <cmath>
#include <ctime>
#include <fstream>
#include <algorithm>
#include <random>

#include "memory.h"
#include "cpu.h"
#include "loader.h"
#include "image.h"
#include "expression.h"
#include "assembler.h"
#include "load.h"
#include "keywords.h"
#include "functions.h"
#include "operators.h"
#include "linker.h"
#include "midi.h"


namespace Keywords
{
    enum InitTypes {InitTime, InitMidi, InitMidiV, InitUser};
    enum MidiTypes {MidiNone, Midi, MidiV, MidiId, MidiIdV};

    struct Gprintf
    {
        int codeLineIndex = 0;
        Assembler::Gprintf _gprintfAsm;
    };


    uint64_t _uniqueJumpId = 0;
    bool _constDimStrArray = false;
    int _numNumericGotosGosubs = 0;
    MidiTypes _midiType = MidiNone;
    std::string _userRoutine;

    std::map<std::string, Keyword> _keywords;
    std::map<std::string, std::string> _equalsKeywords;

    std::map<std::string, Keyword>& getKeywords(void)           {return _keywords;      }
    std::map<std::string, std::string>& getEqualsKeywords(void) {return _equalsKeywords;}

    std::vector<Gprintf> _gprintfs;


    void reset(void)
    {
        _midiType = MidiNone;
        _userRoutine = "";
        _gprintfs.clear();
    }

    void restart(void)
    {
        _constDimStrArray = false;
        _numNumericGotosGosubs = 0;
    }

    bool initialise(void)
    {
        restart();

        _uniqueJumpId = 0;

        // Equals keywords
        _equalsKeywords["CONST" ] = "CONST";
        _equalsKeywords["DIM"   ] = "DIM";
        _equalsKeywords["DEF"   ] = "DEF";
        _equalsKeywords["FOR"   ] = "FOR";
        _equalsKeywords["IF"    ] = "IF";
        _equalsKeywords["ELSEIF"] = "ELSEIF";
        _equalsKeywords["WHILE" ] = "WHILE";
        _equalsKeywords["UNTIL" ] = "UNTIL";

        // Keywords
        _keywords["END"     ] = {"END",      END,      Compiler::SingleStatementParsed};
        _keywords["INC"     ] = {"INC",      INC,      Compiler::SingleStatementParsed};
        _keywords["DEC"     ] = {"DEC",      DEC,      Compiler::SingleStatementParsed};
        _keywords["XCHG"    ] = {"XCHG",     XCHG,     Compiler::SingleStatementParsed};
        _keywords["SWAP"    ] = {"SWAP",     SWAP,     Compiler::SingleStatementParsed};
        _keywords["DBNE"    ] = {"DBNE",     DBNE,     Compiler::SingleStatementParsed};
        _keywords["DBGE"    ] = {"DBGE",     DBGE,     Compiler::SingleStatementParsed};
        _keywords["ON"      ] = {"ON",       ON,       Compiler::SingleStatementParsed};
        _keywords["GOTO"    ] = {"GOTO",     GOTO,     Compiler::SingleStatementParsed};
        _keywords["GOSUB"   ] = {"GOSUB",    GOSUB,    Compiler::SingleStatementParsed};
        _keywords["RETURN"  ] = {"RETURN",   RETURN,   Compiler::SingleStatementParsed};
        _keywords["RET"     ] = {"RET",      RET,      Compiler::SingleStatementParsed};
        _keywords["CLS"     ] = {"CLS",      CLS,      Compiler::SingleStatementParsed};
        _keywords["?"       ] = {"?",        PRINT,    Compiler::SingleStatementParsed};
        _keywords["PRINT"   ] = {"PRINT",    PRINT,    Compiler::SingleStatementParsed};
        _keywords["INPUT"   ] = {"INPUT",    INPUT,    Compiler::SingleStatementParsed};
        _keywords["FOR"     ] = {"FOR",      FOR,      Compiler::SingleStatementParsed};
        _keywords["NEXT"    ] = {"NEXT",     NEXT,     Compiler::SingleStatementParsed};
        _keywords["IF"      ] = {"IF",       IF,       Compiler::MultiStatementParsed };
        _keywords["ELSEIF"  ] = {"ELSEIF",   ELSEIF,   Compiler::SingleStatementParsed};
        _keywords["ELSE"    ] = {"ELSE",     ELSE,     Compiler::SingleStatementParsed};
        _keywords["ENDIF"   ] = {"ENDIF",    ENDIF,    Compiler::SingleStatementParsed};
        _keywords["WHILE"   ] = {"WHILE",    WHILE,    Compiler::SingleStatementParsed};
        _keywords["WEND"    ] = {"WEND",     WEND,     Compiler::SingleStatementParsed};
        _keywords["REPEAT"  ] = {"REPEAT",   REPEAT,   Compiler::SingleStatementParsed};
        _keywords["UNTIL"   ] = {"UNTIL",    UNTIL,    Compiler::SingleStatementParsed};
        _keywords["FOREVER" ] = {"FOREVER",  FOREVER,  Compiler::SingleStatementParsed};
        _keywords["&FOREVER"] = {"&FOREVER", FOREVER,  Compiler::SingleStatementParsed};
        _keywords["AS"      ] = {"AS",       AS,       Compiler::SingleStatementParsed};
        _keywords["TYPE"    ] = {"TYPE",     TYPE,     Compiler::SingleStatementParsed};
        _keywords["CALL"    ] = {"CALL",     CALL,     Compiler::SingleStatementParsed};
        _keywords["PROC"    ] = {"PROC",     PROC,     Compiler::SingleStatementParsed};
        _keywords["ENDPROC" ] = {"ENDPROC",  ENDPROC,  Compiler::SingleStatementParsed};
        _keywords["LOCAL"   ] = {"LOCAL",    LOCAL,    Compiler::SingleStatementParsed};
        _keywords["CONST"   ] = {"CONST",    CONST,    Compiler::SingleStatementParsed};
        _keywords["DIM"     ] = {"DIM",      DIM,      Compiler::SingleStatementParsed};
        _keywords["DEF"     ] = {"DEF",      DEF,      Compiler::SingleStatementParsed};
        _keywords["DATA"    ] = {"DATA",     DATA,     Compiler::SingleStatementParsed};
        _keywords["READ"    ] = {"READ",     READ,     Compiler::SingleStatementParsed};
        _keywords["RESTORE" ] = {"RESTORE",  RESTORE,  Compiler::SingleStatementParsed};
        _keywords["ALLOC"   ] = {"ALLOC",    ALLOC,    Compiler::SingleStatementParsed};
        _keywords["FREE"    ] = {"FREE",     FREE,     Compiler::SingleStatementParsed};
        _keywords["CLEAR"   ] = {"CLEAR",    CLEAR,    Compiler::SingleStatementParsed};
        _keywords["AT"      ] = {"AT",       AT,       Compiler::SingleStatementParsed};
        _keywords["PUT"     ] = {"PUT",      PUT,      Compiler::SingleStatementParsed};
        _keywords["MODE"    ] = {"MODE",     MODE,     Compiler::SingleStatementParsed};
        _keywords["WAIT"    ] = {"WAIT",     WAIT,     Compiler::SingleStatementParsed};
        _keywords["PSET"    ] = {"PSET",     PSET,     Compiler::SingleStatementParsed};
        _keywords["LINE"    ] = {"LINE",     LINE,     Compiler::SingleStatementParsed};
        _keywords["HLINE"   ] = {"HLINE",    HLINE,    Compiler::SingleStatementParsed};
        _keywords["VLINE"   ] = {"VLINE",    VLINE,    Compiler::SingleStatementParsed};
        _keywords["CIRCLE"  ] = {"CIRCLE",   CIRCLE,   Compiler::SingleStatementParsed};
        _keywords["CIRCLEF" ] = {"CIRCLEF",  CIRCLEF,  Compiler::SingleStatementParsed};
        _keywords["RECT"    ] = {"RECT",     RECT,     Compiler::SingleStatementParsed};
        _keywords["RECTF"   ] = {"RECTF",    RECTF,    Compiler::SingleStatementParsed};
        _keywords["POLY"    ] = {"POLY",     POLY,     Compiler::SingleStatementParsed};
        _keywords["POLYR"   ] = {"POLYR",    POLYR,    Compiler::SingleStatementParsed};
        _keywords["FILL"    ] = {"FILL",     FILL,     Compiler::SingleStatementParsed};
        _keywords["TCLIP"   ] = {"TCLIP",    TCLIP,    Compiler::SingleStatementParsed};
        _keywords["TSCROLL" ] = {"TSCROLL",  TSCROLL,  Compiler::SingleStatementParsed};
        _keywords["TFNT4X6" ] = {"TFNT4X6",  TFNT4X6,  Compiler::SingleStatementParsed};
        _keywords["HSCROLL" ] = {"HSCROLL",  HSCROLL,  Compiler::SingleStatementParsed};
        _keywords["VSCROLL" ] = {"VSCROLL",  VSCROLL,  Compiler::SingleStatementParsed};
        _keywords["POKE"    ] = {"POKE",     POKE,     Compiler::SingleStatementParsed};
        _keywords["DOKE"    ] = {"DOKE",     DOKE,     Compiler::SingleStatementParsed};
        _keywords["INIT"    ] = {"INIT",     INIT,     Compiler::SingleStatementParsed};
        _keywords["TICK"    ] = {"TICK",     TICK,     Compiler::SingleStatementParsed};
        _keywords["PLAY"    ] = {"PLAY",     PLAY,     Compiler::SingleStatementParsed};
        _keywords["LOAD"    ] = {"LOAD",     LOAD,     Compiler::SingleStatementParsed};
        _keywords["BLIT"    ] = {"BLIT",     BLIT,     Compiler::SingleStatementParsed};
        _keywords["SPRITE"  ] = {"SPRITE",   SPRITE,   Compiler::SingleStatementParsed};
        _keywords["SPRITES" ] = {"SPRITES",  SPRITES,  Compiler::SingleStatementParsed};
        _keywords["SOUND"   ] = {"SOUND",    SOUND,    Compiler::SingleStatementParsed};
        _keywords["SET"     ] = {"SET",      SET,      Compiler::SingleStatementParsed};
        _keywords["ASM"     ] = {"ASM",      ASM,      Compiler::SingleStatementParsed};
        _keywords["ENDASM"  ] = {"ENDASM",   ENDASM,   Compiler::SingleStatementParsed};
        _keywords["BCDADD"  ] = {"BCDADD",   BCDADD,   Compiler::SingleStatementParsed};
        _keywords["BCDSUB"  ] = {"BCDSUB",   BCDSUB,   Compiler::SingleStatementParsed};
        _keywords["BCDINT"  ] = {"BCDINT",   BCDINT,   Compiler::SingleStatementParsed};
        _keywords["BCDCPY"  ] = {"BCDCPY",   BCDCPY,   Compiler::SingleStatementParsed};
        _keywords["VEC8"    ] = {"VEC8",     VEC8,     Compiler::SingleStatementParsed};
        _keywords["EXEC"    ] = {"EXEC",     EXEC,     Compiler::SingleStatementParsed};
        _keywords["OPEN"    ] = {"OPEN",     OPEN,     Compiler::SingleStatementParsed};
        _keywords["BRKPNT"  ] = {"BRKPNT",   BRKPNT,   Compiler::SingleStatementParsed};
        _keywords["GPRINTF" ] = {"GPRINTF",  GPRINTF,  Compiler::SingleStatementParsed};
        _keywords["OPTIMISE"] = {"OPTIMISE", OPTIMISE, Compiler::SingleStatementParsed};

        return true;
    }


    int emitJumpType(int codeLineIndex, const std::string& opcode, const std::string& labelText, Expression::CCType ccType)
    {
        int vasmIndex = -1;
        std::string uniqueLabel = std::to_string(_uniqueJumpId++);
        Compiler::CodeLine& codeLine = Compiler::getCodeLines()[codeLineIndex];
        
        bool dontMakeUnique = (ccType == Expression::FastCC  ||  (Compiler::getCodeRomType() >= Cpu::ROMvX0  &&  Compiler::getCodeRomType() < Cpu::SDCARD));
        std::string label = (dontMakeUnique) ? labelText : labelText + " " + uniqueLabel;

        switch(ccType)
        {
            case Expression::BooleanCC: vasmIndex = Compiler::emitVcpuAsm(opcode, label, false).second;     break;
            case Expression::NormalCC:  vasmIndex = Compiler::addLabelToJumpCC(codeLine._vasm, label);      break;
            case Expression::FastCC:    vasmIndex = Compiler::addLabelToJumpCC(codeLine._vasm, labelText);  break; // always not unique

            default: break;
        }

        return vasmIndex;
    }

    bool findKeyword(std::string code, const std::string& keyword, size_t& foundPos)
    {
        Expression::strToUpper(code);
        foundPos = code.find(keyword);
        if(foundPos != std::string::npos)
        {
            foundPos += keyword.size();
            return true;
        }
        return false;
    }

    KeywordResult handleKeywords(Compiler::CodeLine& codeLine, const std::string& keyword, int codeLineIndex, int tokenIndex, KeywordFuncResult& result)
    {
        size_t foundPos;

        std::string key = keyword;
        Expression::strToUpper(key);
        if(_keywords.find(key) == _keywords.end()) return KeywordNotFound;

        // Handle keyword in code line
        if(findKeyword(key, _keywords[key]._name, foundPos)  &&  _keywords[key]._func)
        {
            // Line index taking into account modules
            int codeLineStart = Compiler::getCodeLineStart(codeLineIndex);

            // Keyword
            bool success = _keywords[key]._func(codeLine, codeLineIndex, codeLineStart, tokenIndex, foundPos, result);
            return (!success) ? KeywordError : KeywordFound;
        }

        return KeywordFound;
    }

#ifndef STAND_ALONE
    bool addGprintf(const std::string& lineToken, const std::string& formatText, const std::vector<std::string>& variables, uint16_t address, int codeLineIndex)
    {
        std::vector<Assembler::Gprintf::Var> vars;
        std::vector<std::string> subs;
        Assembler::parseGprintfFormat(formatText, variables, vars, subs);

        Gprintf gprintf = {codeLineIndex, {Compiler::getVasmPC(), codeLineIndex, lineToken, formatText, vars, subs}};
        for(int i=0; i<int(gprintf._gprintfAsm._vars.size()); i++)
        {
            gprintf._gprintfAsm._vars[i]._indirection = 2;
            gprintf._gprintfAsm._vars[i]._data = uint16_t(address  + i*2);
        }

        _gprintfs.push_back(gprintf);

        return true;
    }

    bool convertGprintGbasToGprintfAsm(void)
    {
        for(int i=0; i<int(_gprintfs.size()); i++)
        {
            int codeLineIndex = _gprintfs[i].codeLineIndex;
            const Compiler::CodeLine& codeLine = Compiler::getCodeLines()[codeLineIndex];
            const Compiler::VasmLine& vasmLine = codeLine._vasm.back(); 
            Assembler::Gprintf& gprintfAsm = _gprintfs[i]._gprintfAsm;
            uint16_t address = uint16_t(vasmLine._address);

            gprintfAsm._address = address;
            if(!Assembler::addGprintf(gprintfAsm, address))
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::fixGprintfAddresses() : '%s:%d' : Assembler::addGprintf() at '0x%04x' already exists : %s\n", codeLine._moduleName.c_str(), codeLineIndex,
                                                                                                                                                                 address, codeLine._text.c_str());
                return false;
            }
        }

        return true;
    }
#endif

    Expression::Int16Byte getByteConfig(std::string& token)
    {
        Expression::Int16Byte int16Byte = Expression::Int16Both;
        size_t dot = token.find('.');
        if(dot != std::string::npos)
        {
            std::string dotName = token.substr(dot);
            token = token.substr(0, dot);
            Expression::strToUpper(dotName);
            if(dotName == ".LO") int16Byte = Expression::Int16Low;
            if(dotName == ".HI") int16Byte = Expression::Int16High;
        }

        return int16Byte;
    }


    // ********************************************************************************************
    // Keywords
    // ********************************************************************************************
    bool END(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(foundPos);
        UNREFERENCED_PARAM(tokenIndex);
        UNREFERENCED_PARAM(codeLineStart);
        UNREFERENCED_PARAM(codeLine);

        Compiler::emitVcpuAsm("HALT", "", false, codeLineIndex);

        return true;
    }

    bool INC(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(tokenIndex);
        UNREFERENCED_PARAM(codeLineIndex);

        // Byte configuration
        std::string varToken = codeLine._code.substr(foundPos);
        Expression::stripWhitespace(varToken);
        Expression::Int16Byte int16Byte = getByteConfig(varToken);

        // Operand must be an integer var
        int varIndex = Compiler::findVar(varToken, false);
        if(varIndex < 0)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::INC() : '%s:%d' : syntax error, integer variable '%s' not found : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                                        varToken.c_str(), codeLine._text.c_str());
            return false;
        }

        if(Compiler::getCodeRomType() >= Cpu::ROMvX0  &&  Compiler::getCodeRomType() < Cpu::SDCARD)
        {
            switch(int16Byte)
            {
                case Expression::Int16Low:  Compiler::emitVcpuAsm("INC",  "_" + Compiler::getIntegerVars()[varIndex]._name,          false); break;
                case Expression::Int16High: Compiler::emitVcpuAsm("INC",  "_" + Compiler::getIntegerVars()[varIndex]._name + " + 1", false); break;
                case Expression::Int16Both: Compiler::emitVcpuAsm("INCW", "_" + Compiler::getIntegerVars()[varIndex]._name,          false); break;

                default: break;
            }
        }
        else
        {
            switch(int16Byte)
            {
                case Expression::Int16Low:  Compiler::emitVcpuAsm("INC", "_" + Compiler::getIntegerVars()[varIndex]._name,          false); break;
                case Expression::Int16High: Compiler::emitVcpuAsm("INC", "_" + Compiler::getIntegerVars()[varIndex]._name + " + 1", false); break;

                case Expression::Int16Both:
                {
                    Compiler::emitVcpuAsm("LDW",  "_" + Compiler::getIntegerVars()[varIndex]._name, false);
                    Compiler::emitVcpuAsm("ADDI", "1", false);
                    Compiler::emitVcpuAsm("STW",  "_" + Compiler::getIntegerVars()[varIndex]._name, false);
                }
                break;

                default: break;
            }
        }

        return true;
    }

    bool DEC(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(tokenIndex);
        UNREFERENCED_PARAM(codeLineIndex);

        // Byte configuration
        std::string varToken = codeLine._code.substr(foundPos);
        Expression::stripWhitespace(varToken);
        Expression::Int16Byte int16Byte = getByteConfig(varToken);

        // Operand must be an integer var
        int varIndex = Compiler::findVar(varToken, false);
        if(varIndex < 0)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::DEC() : '%s:%d' : syntax error, integer variable '%s' not found : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                                        varToken.c_str(), codeLine._text.c_str());
            return false;
        }

        if(Compiler::getCodeRomType() >= Cpu::ROMvX0  &&  Compiler::getCodeRomType() < Cpu::SDCARD)
        {
            switch(int16Byte)
            {
                case Expression::Int16Low:  Compiler::emitVcpuAsm("DEC",  "_" + Compiler::getIntegerVars()[varIndex]._name,          false); break;
                case Expression::Int16High: Compiler::emitVcpuAsm("DEC",  "_" + Compiler::getIntegerVars()[varIndex]._name + " + 1", false); break;
                case Expression::Int16Both: Compiler::emitVcpuAsm("DECW", "_" + Compiler::getIntegerVars()[varIndex]._name,          false); break;

                default: break;
            }
        }
        else
        {
            if(int16Byte != Expression::Int16Both)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::DEC() : '%s:%d' : version error, ROM version too low for byte access : %s\n", codeLine._moduleName.c_str(),
                                                                                                                                                 codeLineStart, codeLine._text.c_str());
                return false;
            }

            Compiler::emitVcpuAsm("LDW",  "_" + Compiler::getIntegerVars()[varIndex]._name, false);
            Compiler::emitVcpuAsm("SUBI", "1", false);
            Compiler::emitVcpuAsm("STW",  "_" + Compiler::getIntegerVars()[varIndex]._name, false);
        }

        return true;
    }

    bool XCHG(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(tokenIndex);
        UNREFERENCED_PARAM(codeLineIndex);

        std::vector<std::string> tokens = Expression::tokenise(codeLine._code.substr(foundPos), ',', false);
        if(tokens.size() != 2)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::XCHG() : '%s:%d' : syntax error, wrong number of parameters : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        // Operands must be integer vars
        std::string varToken0 = tokens[0];
        std::string varToken1 = tokens[1];
        Expression::stripWhitespace(varToken0);
        Expression::stripWhitespace(varToken1);

        // Byte configuration
        Expression::Int16Byte int16Byte0 = getByteConfig(varToken0);
        Expression::Int16Byte int16Byte1 = getByteConfig(varToken1);
        if((int16Byte0 == Expression::Int16Both  ||  int16Byte1 == Expression::Int16Both)  &&  int16Byte0 != int16Byte1)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::XCHG() : '%s:%d' : syntax error, integer variable's byte configuration widths do not match : %s\n", codeLine._moduleName.c_str(),
                                                                                                                                                                   codeLineStart, codeLine._text.c_str());
            return false;
        }

        int varIndex0 = Compiler::findVar(varToken0, false);
        int varIndex1 = Compiler::findVar(varToken1, false);
        if(varIndex0 < 0)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::XCHG() : '%s:%d' : syntax error, integer variable '%s' not found : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                                         varToken0.c_str(), codeLine._text.c_str());
            return false;
        }
        if(varIndex1 < 0)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::XCHG() : '%s:%d' : syntax error, integer variable '%s' not found : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                                         varToken1.c_str(), codeLine._text.c_str());
            return false;
        }

        std::string varName0 = Compiler::getIntegerVars()[varIndex0]._name;
        std::string varName1 = Compiler::getIntegerVars()[varIndex1]._name;

        if(Compiler::getCodeRomType() >= Cpu::ROMvX0  &&  Compiler::getCodeRomType() < Cpu::SDCARD)
        {
            // Build emit strings
            std::string operand0 = (int16Byte0 == Expression::Int16High) ? "_" + varName0 + " + 1" : "_" + varName0;
            std::string operand1 = (int16Byte1 == Expression::Int16High) ? "_" + varName1 + " + 1" : "_" + varName1;
            if(int16Byte0 == Expression::Int16Both  &&  int16Byte1 == Expression::Int16Both)
            {
                Compiler::emitVcpuAsm("XCHGW", operand0 + ", " + operand1, false);
            }
            else
            {
                Compiler::emitVcpuAsm("XCHGB", operand0 + ", " + operand1, false);
            }
        }
        else
        {
            switch(int16Byte0)
            {
                case Expression::Int16Low:  Compiler::emitVcpuAsm("LDI", "_" + varName0, false);          break;
                case Expression::Int16High: Compiler::emitVcpuAsm("LDI", "_" + varName0 + " + 1", false); break;
                case Expression::Int16Both: Compiler::emitVcpuAsm("LDI", "_" + varName0, false);          break;
            }
            Compiler::emitVcpuAsm("STW", "swpSrcAddr", false);

            switch(int16Byte1)
            {
                case Expression::Int16Low:  Compiler::emitVcpuAsm("LDI", "_" + varName1, false);          break;
                case Expression::Int16High: Compiler::emitVcpuAsm("LDI", "_" + varName1 + " + 1", false); break;
                case Expression::Int16Both: Compiler::emitVcpuAsm("LDI", "_" + varName1, false);          break;
            }
            Compiler::emitVcpuAsm("STW", "swpDstAddr", false);

            (int16Byte1 != Expression::Int16Both) ? Compiler::emitVcpuAsm("%SwapByte", "swpSrcAddr swpDstAddr", false) : Compiler::emitVcpuAsm("%SwapWord", "swpSrcAddr swpDstAddr", false);
        }

        return true;
    }

    bool SWAP(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(tokenIndex);
        UNREFERENCED_PARAM(codeLineIndex);

        std::vector<std::string> tokens = Expression::tokenise(codeLine._code.substr(foundPos), ',', false);
        if(tokens.size() != 4)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::SWAP() : '%s:%d' : syntax error, wrong number of parameters, 'SWAP BYTE/WORD, <addr0>, <addr1>, <count>', where count is 1..65535 : %s\n",
                                                    codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        bool isByte = true;
        std::string isByteToken = tokens[0];
        Expression::stripWhitespace(isByteToken);
        Expression::strToUpper(isByteToken);
        if(isByteToken != "BYTE"  &&  isByteToken != "WORD")
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::SWAP() : '%s:%d' : syntax error, expecting 'BYTE' or 'WORD', found %s : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                                              isByteToken.c_str(), codeLine._text.c_str());
            return false;
        }
        isByte = (isByteToken == "BYTE");

        // Src address
        std::string addrToken0 = tokens[1];
        Expression::stripWhitespace(addrToken0);
        Expression::Numeric addrNumeric0;
        if(Compiler::parseExpression(codeLineIndex, addrToken0, addrNumeric0) == Expression::IsInvalid)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::SWAP() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, addrToken0.c_str(), codeLine._text.c_str());
            return false;
        }
        Compiler::emitVcpuAsm("STW", "swpSrcAddr", false);

        // Dst address
        std::string addrToken1 = tokens[2];
        Expression::stripWhitespace(addrToken1);
        Expression::Numeric addrNumeric1;
        if(Compiler::parseExpression(codeLineIndex, addrToken1, addrNumeric1) == Expression::IsInvalid)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::SWAP() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, addrToken1.c_str(), codeLine._text.c_str());
            return false;
        }
        Compiler::emitVcpuAsm("STW", "swpDstAddr", false);

        // Count
        std::string countToken = tokens[3];
        Expression::stripWhitespace(countToken);
        Expression::Numeric countNumeric;
        if(Compiler::parseExpression(codeLineIndex, countToken, countNumeric) == Expression::IsInvalid)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::SWAP() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, countToken.c_str(), codeLine._text.c_str());
            return false;
        }
        Compiler::emitVcpuAsm("STW", "swapCount", false);

        (isByte) ? Compiler::emitVcpuAsm("%SwapBytes", "", false) : Compiler::emitVcpuAsm("%SwapWords", "", false);

        return true;
    }

    bool DBNE(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(tokenIndex);
        UNREFERENCED_PARAM(codeLineIndex);

        std::vector<std::string> tokens = Expression::tokenise(codeLine._code.substr(foundPos), ',', false);
        if(tokens.size() != 2)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::DBNE() : '%s:%d' : syntax error, wrong number of parameters, 'DBNE <var>, <label>' : %s\n", codeLine._moduleName.c_str(),
                                                                                                                                                           codeLineStart, codeLine._text.c_str());
            return false;
        }

        // Var
        std::string varToken = tokens[0];
        Expression::stripWhitespace(varToken);
        std::string varOperand;
        Expression::Numeric varNumeric;
        if(Compiler::parseStaticExpression(codeLineIndex, varToken, varOperand, varNumeric) != Compiler::OperandVar)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::DBNE() : '%s:%d' : syntax error, invalid var '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, varToken.c_str(), codeLine._text.c_str());
            return false;
        }
        if(varNumeric._int16Byte == Expression::Int16High) varOperand = varOperand + " + 1";
        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD)
        {
            Compiler::emitVcpuAsm("LD", "_" + varOperand, false);
        }

        // Label
        std::string labelStr = tokens[1];
        Expression::stripWhitespace(labelStr);
        int labelIndex = Compiler::findLabel(labelStr);
        if(labelIndex == -1)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::DBNE() : '%s:%d' : invalid label '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, labelStr.c_str(), codeLine._text.c_str());
            return false;
        }

        if(Compiler::getCodeRomType() >= Cpu::ROMvX0  &&  Compiler::getCodeRomType() < Cpu::SDCARD)
        {
            Compiler::emitVcpuAsm("DBNE", "_" + varOperand + ", _" + labelStr, false);
        }
        else
        {
            Compiler::emitVcpuAsm("SUBI", "1", false);
            Compiler::emitVcpuAsm("ST", "_" + varOperand, false);
            Compiler::emitVcpuAsm("BNE", "_" + labelStr, false);
        }

        return true;
    }

    bool DBGE(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(tokenIndex);
        UNREFERENCED_PARAM(codeLineIndex);

        std::vector<std::string> tokens = Expression::tokenise(codeLine._code.substr(foundPos), ',', false);
        if(tokens.size() != 2)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::DBGE() : '%s:%d' : syntax error, wrong number of parameters, 'DBGE <var>, <label>' : %s\n", codeLine._moduleName.c_str(),
                                                                                                                                                           codeLineStart, codeLine._text.c_str());
            return false;
        }

        // Var
        std::string varToken = tokens[0];
        Expression::stripWhitespace(varToken);
        std::string varOperand;
        Expression::Numeric varNumeric;
        if(Compiler::parseStaticExpression(codeLineIndex, varToken, varOperand, varNumeric) != Compiler::OperandVar)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::DBGE() : '%s:%d' : syntax error, invalid var '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, varToken.c_str(), codeLine._text.c_str());
            return false;
        }
        if(varNumeric._int16Byte == Expression::Int16High) varOperand = varOperand + " + 1";
        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD)
        {
            Compiler::emitVcpuAsm("LD", "_" + varOperand, false);
        }

        // Label
        std::string labelStr = tokens[1];
        Expression::stripWhitespace(labelStr);
        int labelIndex = Compiler::findLabel(labelStr);
        if(labelIndex == -1)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::DBGE() : '%s:%d' : invalid label '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, labelStr.c_str(), codeLine._text.c_str());
            return false;
        }

        if(Compiler::getCodeRomType() >= Cpu::ROMvX0  &&  Compiler::getCodeRomType() < Cpu::SDCARD)
        {
            Compiler::emitVcpuAsm("DBGE", "_" + varOperand + ", _" + labelStr, false);
        }
        else
        {
            Compiler::emitVcpuAsm("SUBI", "1", false);
            Compiler::emitVcpuAsm("ST", "_" + varOperand, false);
            Compiler::emitVcpuAsm("BGE", "_" + labelStr, false);
        }

        return true;
    }

    bool ON(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(tokenIndex);

        std::string code = codeLine._code;
        Expression::strToUpper(code);
        size_t gotoOffset = code.find("GOTO");
        size_t gosubOffset = code.find("GOSUB");
        if(gotoOffset == std::string::npos  &&  gosubOffset == std::string::npos)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::ON() : '%s:%d' : syntax error : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        size_t gSize = (gotoOffset != std::string::npos) ? 4 : 5;
        size_t gOffset = (gotoOffset != std::string::npos) ? gotoOffset : gosubOffset;

        // Parse ON field
        Expression::Numeric onValue;
        std::string onToken = codeLine._code.substr(foundPos, gOffset - (foundPos + 1));
        if(Compiler::parseExpression(codeLineIndex, onToken, onValue) == Expression::IsInvalid)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::ON() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, onToken.c_str(), codeLine._text.c_str());
            return false;
        }
        Compiler::emitVcpuAsm("LSLW", "", false);

        // Parse labels
        std::vector<size_t> gOffsets;
        std::vector<std::string> gTokens = Expression::tokeniseOffsets(codeLine._code.substr(gOffset + gSize), ',', gOffsets, false);
        if(gTokens.size() < 1)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::ON() : '%s:%d' : syntax error, must have at least one label after GOTO/GOSUB : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        // Create on goto/gosub label LUT
        Compiler::getCodeLines()[codeLineIndex]._onGotoGosubLut._lut.clear();
        for(int i=0; i<int(gTokens.size()); i++)
        {
            std::string gLabel = gTokens[i];
            Expression::stripWhitespace(gLabel);

            // Optimised gosub has no PUSH, (i.e. leaf function, VBI handlers, ASM code, etc)
            bool usePush = true;
            if(gLabel[0] == '&')
            {
                usePush = false;
                gLabel.erase(0, 1);
            }

            int labelIndex = Compiler::findLabel(gLabel);
            if(labelIndex == -1)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::ON() : '%s:%d' : invalid label %s in slot %d : %s\n", codeLine._moduleName.c_str(), codeLineStart, gLabel.c_str(), i, codeLine._text.c_str());
                Compiler::getCodeLines()[codeLineIndex]._onGotoGosubLut._lut.clear();
                return false;
            }
                
            // Only ON GOSUB needs a PUSH, (emitted in createVasmCode())
            if(gosubOffset != std::string::npos) Compiler::getLabels()[labelIndex]._gosub = usePush;

            // Create lookup table out of label addresses
            Compiler::getCodeLines()[codeLineIndex]._onGotoGosubLut._lut.push_back(labelIndex);
        }

        // Allocate giga memory for LUT
        int size = int(gTokens.size()) * 2;
        uint16_t address;
        if(!Memory::getFreePageRAM(Memory::FitDescending, size, USER_CODE_START, Compiler::getRuntimeStart(), address, true))
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::ON() : '%s:%d' : not enough RAM for onGotoGosub LUT of size %d : %s\n", codeLine._moduleName.c_str(), codeLineStart, size, codeLine._text.c_str());
            return false;
        }
        Compiler::getCodeLines()[codeLineIndex]._onGotoGosubLut._address = address;
        Compiler::getCodeLines()[codeLineIndex]._onGotoGosubLut._name = "_lut_onAddrs_" + Expression::wordToHexString(address);

        Compiler::emitVcpuAsm("STW",  "register0", false);
        Compiler::emitVcpuAsm("LDWI", Compiler::getCodeLines()[codeLineIndex]._onGotoGosubLut._name, false);
        Compiler::emitVcpuAsm("ADDW", "register0", false);
        if(Compiler::getArrayIndiciesOne())
        {
            Compiler::emitVcpuAsm("SUBI", "2", false);  // enable this to start at 1 instead of 0
        }
        Compiler::emitVcpuAsm("DEEK", "", false);
        Compiler::emitVcpuAsm("CALL", "giga_vAC", false);

        return true;
    }

    bool GOTO(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(tokenIndex);

        // Parse labels
        std::vector<size_t> gotoOffsets;
        std::vector<std::string> gotoTokens = Expression::tokeniseOffsets(codeLine._code.substr(foundPos), ',', gotoOffsets, false);
        if(gotoTokens.size() < 1  ||  gotoTokens.size() > 2)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::GOTO() : '%s:%d' : syntax error, must have one or two parameters, e.g. 'GOTO 200' or 'GOTO k+1,default' : %s\n", codeLine._moduleName.c_str(),
                                                                                                                                                                                codeLineStart, codeLine._text.c_str());
            return false;
        }

        // Parse GOTO field
        Expression::Numeric gotoValue;
        std::string gotoToken = gotoTokens[0];
        Expression::stripWhitespace(gotoToken);

        bool useBRA = false;
        if(gotoToken[0] == '&')
        {
            useBRA = true;
            gotoToken.erase(0, 1);
        }

        int labelIndex = Compiler::findLabel(gotoToken, Compiler::getVasmPC());
        if(labelIndex == -1)
        {
            if(Expression::isNumber(gotoToken))
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::GOTO() : '%s:%d' : numeric label '%s' does not exist : %s\n", codeLine._moduleName.c_str(), codeLineStart, gotoToken.c_str(), codeLine._text.c_str());
                return false;
            }
            if(++_numNumericGotosGosubs > Compiler::getNumNumericLabels())
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::GOTO() : '%s:%d' : numeric label '%s' does not exist : %s\n", codeLine._moduleName.c_str(), codeLineStart, gotoToken.c_str(), codeLine._text.c_str());
                return false;
            }

            Compiler::setCreateNumericLabelLut(true);
            if(Compiler::parseExpression(codeLineIndex, gotoToken, gotoValue) == Expression::IsInvalid)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::GOTO() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, gotoToken.c_str(), codeLine._text.c_str());
                return false;
            }
            Compiler::emitVcpuAsm("STW", "numericLabel", false);

            // Default label exists
            if(gotoTokens.size() == 2)
            {
                std::string defaultToken = gotoTokens[1];
                Expression::stripWhitespace(defaultToken);
                labelIndex = Compiler::findLabel(defaultToken);
                if(labelIndex == -1)
                {
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::GOTO() : '%s:%d' : default label does not exist : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
                    return false;
                }

                Compiler::emitVcpuAsm("LDWI", "_" + Compiler::getLabels()[labelIndex]._name, false);
            }
            // No default label
            else
            {
                Compiler::emitVcpuAsm("LDI", "0", false);
            }
            Compiler::emitVcpuAsm("STW", "defaultLabel", false);

            // Call gotoNumericLabel
            Compiler::emitVcpuAsm("%GotoNumeric", "", false);

            return true;
        }

        // Within same page, (validation check on same page branch may fail after outputCode(), user will be warned)
        if(useBRA)
        {
            Compiler::emitVcpuAsm("BRA", "_" + gotoToken, false);
        }
        // Long jump
        else
        {
            if(Compiler::getCodeRomType() >= Cpu::ROMv5a)
            {
                Compiler::emitVcpuAsm("CALLI", "_" + gotoToken, false);
            }
            else
            {
                Compiler::emitVcpuAsm("LDWI", "_" + gotoToken, false);
                Compiler::emitVcpuAsm("CALL", "giga_vAC",      false);
            }
        }

        return true;
    }

    bool GOSUB(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(tokenIndex);

        // Parse labels
        std::vector<size_t> gosubOffsets;
        std::vector<std::string> gosubTokens = Expression::tokeniseOffsets(codeLine._code.substr(foundPos), ',', gosubOffsets, false);
        if(gosubTokens.size() < 1  ||  gosubTokens.size() > 2)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::GOSUB() : '%s:%d' : syntax error, must have one or two parameters, e.g. 'GOSUB <label>' or 'GOSUB <expression>, <default label>' : %s\n",
                                                    codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }
        if(gosubTokens[0].size() == 0)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::GOSUB() : '%s:%d' : syntax error, invalid label : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        // Parse GOSUB field
        Expression::Numeric gosubValue;
        std::string gosubToken = gosubTokens[0];
        Expression::stripWhitespace(gosubToken);

        // Optimised gosub has no PUSH, (i.e. leaf function, VBI handlers, ASM code, etc)
        bool usePush = true;
        if(gosubToken[0] == '&')
        {
            usePush = false;
            gosubToken.erase(0, 1);
        }

        int labelIndex = Compiler::findLabel(gosubToken);
        if(labelIndex == -1)
        {
            if(Expression::isNumber(gosubToken))
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::GOSUB() : '%s:%d' : numeric label '%s' does not exist : %s\n", codeLine._moduleName.c_str(), codeLineStart, gosubToken.c_str(), codeLine._text.c_str());
                return false;
            }
            if(++_numNumericGotosGosubs > Compiler::getNumNumericLabels())
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::GOSUB() : '%s:%d' : numeric label '%s' does not exist : %s\n", codeLine._moduleName.c_str(), codeLineStart, gosubToken.c_str(), codeLine._text.c_str());
                return false;
            }
            if(!usePush)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::GOSUB() : '%s:%d' : can't use optimised GOSUB with numeric labels : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
                return false;
            }

            Compiler::setCreateNumericLabelLut(true);
            if(Compiler::parseExpression(codeLineIndex, gosubToken, gosubValue) == Expression::IsInvalid)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::GOSUB() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, gosubToken.c_str(), codeLine._text.c_str());
                return false;
            }
            Compiler::emitVcpuAsm("STW", "numericLabel", false);

            // Default label exists
            if(gosubTokens.size() == 2)
            {
                std::string defaultToken = gosubTokens[1];
                Expression::stripWhitespace(defaultToken);
                labelIndex = Compiler::findLabel(defaultToken);
                if(labelIndex == -1)
                {
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::GOSUB() : '%s:%d' : default label does not exist : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
                    return false;
                }

                Compiler::getLabels()[labelIndex]._gosub = true;
                Compiler::emitVcpuAsm("LDWI", "_" + Compiler::getLabels()[labelIndex]._name, false);
            }
            // No default label
            else
            {
                Compiler::emitVcpuAsm("LDI", "0", false);
            }
            Compiler::emitVcpuAsm("STW", "defaultLabel", false);

            // Call gosubNumericLabel
            Compiler::emitVcpuAsm("%GosubNumeric", "", false);

            return true;
        }

        // CALL label
        Compiler::getLabels()[labelIndex]._gosub = usePush;

        if(Compiler::getCodeRomType() >= Cpu::ROMv5a)
        {
            Compiler::emitVcpuAsm("CALLI", "_" + gosubToken, false);
        }
        else
        {
            Compiler::emitVcpuAsm("LDWI", "_" + gosubToken, false);
            Compiler::emitVcpuAsm("CALL", "giga_vAC", false);
        }

        return true;
    }

    bool RETURN(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(foundPos);
        UNREFERENCED_PARAM(tokenIndex);
        UNREFERENCED_PARAM(codeLineIndex);
        UNREFERENCED_PARAM(codeLineStart);
        UNREFERENCED_PARAM(codeLine);

        // Use a macro instead of separate "POP" and "RET", otherwise page jumps could be inserted in between the "POP" and "RET" causing mayhem and havoc
        Compiler::emitVcpuAsm("%Return", "", false);

        return true;
    }

    bool RET(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(foundPos);
        UNREFERENCED_PARAM(tokenIndex);
        UNREFERENCED_PARAM(codeLineIndex);
        UNREFERENCED_PARAM(codeLineStart);
        UNREFERENCED_PARAM(codeLine);

        Compiler::emitVcpuAsm("RET", "", false);

        return true;
    }

    bool CLS(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(foundPos);
        UNREFERENCED_PARAM(tokenIndex);

        std::vector<std::string> tokens = Expression::tokenise(codeLine._code.substr(foundPos), ',', false);
        if(tokens.size() > 3)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::CLS() : '%s:%d' : syntax error, expected 'CLS INIT' or 'CLS <address>, <optional width>, <optional height>' : %s\n",
                                                    codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        Expression::Numeric param;
        if(tokens.size() == 1  &&  tokens[0].size())
        {
            std::string token = tokens[0];
            Expression::strToUpper(token);
            Expression::stripWhitespace(token);
            if(token == "INIT")
            {
                Compiler::emitVcpuAsm("%ResetVideoTable", "", false);
            }
            else
            {
                if(Compiler::parseExpression(codeLineIndex, tokens[0], param) == Expression::IsInvalid)
                {
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::CLS() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, tokens[0].c_str(), codeLine._text.c_str());
                    return false;
                }
                if(param._varType == Expression::Number  &&  uint16_t(std::lround(param._value)) < DEFAULT_EXEC_ADDRESS)
                {
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::CLS() : '%s:%d' : address field must be above &h%04x, found %s : %s\n",
                                                            codeLine._moduleName.c_str(), codeLineStart, DEFAULT_EXEC_ADDRESS, tokens[0].c_str(), codeLine._text.c_str());
                    return false;
                }
                Compiler::emitVcpuAsm("STW", "clsAddress", false);
                Compiler::emitVcpuAsm("%ClearScreen", "",  false);
            }
        }
        else if(tokens.size() > 1)
        {
            if(Compiler::parseExpression(codeLineIndex, tokens[0], param) == Expression::IsInvalid)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::CLS() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, tokens[0].c_str(), codeLine._text.c_str());
                return false;
            }
            Compiler::emitVcpuAsm("STW", "clrAddress", false);
            if(Compiler::parseExpression(codeLineIndex, tokens[1], param) == Expression::IsInvalid)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::CLS() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, tokens[1].c_str(), codeLine._text.c_str());
                return false;
            }
            Compiler::emitVcpuAsm("STW", "clrWidth", false); // runtime uses clrWidth in word instructions, so make sure all of it is valid

            if(tokens.size() == 2)
            {
                Compiler::emitVcpuAsm("LDI", "120", false);
            }
            else
            {
                if(Compiler::parseExpression(codeLineIndex, tokens[2], param) == Expression::IsInvalid)
                {
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::CLS() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, tokens[2].c_str(), codeLine._text.c_str());
                    return false;
                }
            }

            Compiler::emitVcpuAsm("STW", "clrLines", false); // runtime uses clrLines in word instructions, so make sure all of it is valid
            Compiler::emitVcpuAsm("%ClearRect", "",  false);
        }
        else
        {
            Compiler::emitVcpuAsm("%ClearVertBlinds", "", false);
        }

        return true;
    }

    bool PRINT(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(tokenIndex);
        UNREFERENCED_PARAM(codeLineStart);

        // Parse print tokens
        std::vector<std::string> tokens = Expression::tokenise(codeLine._code.substr(foundPos), ';', false, false);

RESTART_PRINT:
        for(int i=0; i<int(tokens.size()); i++)
        {
            if(tokens[i].size() == 0  ||  Expression::hasOnlyWhiteSpace(tokens[i]))
            {
                tokens.erase(tokens.begin() + i);
                goto RESTART_PRINT;
            }

            Expression::Numeric numeric;
            int varIndex = -1, constIndex = -1, strIndex = -1;
            uint32_t expressionType = Compiler::isExpression(tokens[i], varIndex, constIndex, strIndex);

#if 1
            if((expressionType & Expression::HasStringKeywords)  &&  (expressionType & Expression::HasOptimisedPrint))
            {
                // Prints text on the fly without creating strings
                Expression::setEnableOptimisedPrint(true);
                if(!Expression::parse(tokens[i], codeLineIndex, numeric))
                {
                    Expression::setEnableOptimisedPrint(false);
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::PRINT() : '%s:%d' : syntax error in '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                         Expression::getExpression(), codeLine._text.c_str());
                    return false;
                }
                Expression::setEnableOptimisedPrint(false);
            }
#else
            // TODO: Fix this, (checks for syntax errors)
            if((expressionType & Expression::HasStringKeywords))
            {
                if(expressionType & Expression::HasOptimisedPrint)
                {
                    // Prints text on the fly without creating strings
                    Expression::setEnableOptimisedPrint(true);
                    if(!Expression::parse(tokens[i], codeLineIndex, numeric))
                    {
                        Expression::setEnableOptimisedPrint(false);
                        Cpu::reportError(Cpu::KwdError, stderr, "Keywords::PRINT() : '%s:%d' : syntax error in '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                             Expression::getExpression(), codeLine._text.c_str());
                        return false;
                    }
                    Expression::setEnableOptimisedPrint(false);
                }
                // Leading chars before a string function
                else
                {
                    //Cpu::reportError(Cpu::KwdError, stderr, "Keywords::PRINT() : '%s:%d' : syntax error in '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, tokens[i].c_str(), codeLine._text.c_str());
                    //return false;
                }
            }
#endif
            // Arrays are handled as functions
            else if(expressionType & Expression::HasFunctions)
            {
                if(!Expression::parse(tokens[i], codeLineIndex, numeric))
                {
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::PRINT() : '%s:%d' : syntax error in '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, Expression::getExpression(), codeLine._text.c_str());
                    return false;
                }
                if(numeric._varType == Expression::Number)
                {
                    Compiler::emitVcpuAsm("%PrintInt16", Expression::wordToHexString(int16_t(std::lround(numeric._value))), false);
                }
                else
                {
                    Compiler::emitVcpuAsm("LDW", Expression::byteToHexString(uint8_t(Compiler::getTempVarStart())), false);
                    Compiler::emitVcpuAsm("%PrintAcInt16", "", false);
                }
            }
            else if((expressionType & Expression::HasStrVars)  &&  (expressionType & Expression::HasOperators))
            {
                if(!Expression::parse(tokens[i], codeLineIndex, numeric))
                {
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::PRINT() : '%s:%d' : syntax error in '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, Expression::getExpression(), codeLine._text.c_str());
                    return false;
                }
                if(numeric._varType == Expression::Number)
                {
                    Compiler::emitVcpuAsm("%PrintInt16", Expression::wordToHexString(int16_t(std::lround(numeric._value))), false);
                }
                else if(numeric._varType == Expression::Str2Var)
                {
                    Compiler::emitVcpuAsm("LDW", Expression::byteToHexString(uint8_t(Compiler::getTempVarStart())), false);
                    Compiler::emitVcpuAsm("%PrintAcString", "", false);
                }
                else if(numeric._varType == Expression::TmpStrVar)
                {
                    Compiler::emitVcpuAsm("LDWI", Expression::wordToHexString(Compiler::getStrWorkArea()), false);
                    Compiler::emitVcpuAsm("%PrintAcString", "", false);
                }
                else
                {
                    Compiler::emitVcpuAsm("LDW", Expression::byteToHexString(uint8_t(Compiler::getTempVarStart())), false);
                    Compiler::emitVcpuAsm("%PrintAcInt16", "", false);
                }
            }
            else if((expressionType & Expression::HasIntVars)  &&  (expressionType & Expression::HasOperators))
            {
                if(!Expression::parse(tokens[i], codeLineIndex, numeric))
                {
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::PRINT() : '%s:%d' : syntax error in '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, Expression::getExpression(), codeLine._text.c_str());
                    return false;
                }
                if(numeric._varType == Expression::Number)
                {
                    Compiler::emitVcpuAsm("%PrintInt16", Expression::wordToHexString(int16_t(std::lround(numeric._value))), false);
                }
                else
                {
                    if(numeric._varType != Expression::Str2Var)
                    {
                        Compiler::emitVcpuAsm("LDW", Expression::byteToHexString(uint8_t(Compiler::getTempVarStart())), false);
                        Compiler::emitVcpuAsm("%PrintAcInt16", "", false);
                    }
                    // String array with variable index
                    else
                    {
                        Compiler::emitVcpuAsm("LDW", Expression::byteToHexString(uint8_t(Compiler::getTempVarStart())), false);
                        Compiler::emitVcpuAsm("%PrintAcString", "", false);
                    }
                }
            }
            else if(expressionType & Expression::HasIntVars)
            {
                if(!Expression::parse(tokens[i], codeLineIndex, numeric))
                {
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::PRINT() : '%s:%d' : syntax error in '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, Expression::getExpression(), codeLine._text.c_str());
                    return false;
                }
                if(varIndex >= 0)
                {
                    if(numeric._varType != Expression::Str2Var)
                    {
                        switch(numeric._int16Byte)
                        {
                            case Expression::Int16Low:  Compiler::emitVcpuAsm("LD",  "_" + Compiler::getIntegerVars()[varIndex]._name,          false); break;
                            case Expression::Int16High: Compiler::emitVcpuAsm("LD",  "_" + Compiler::getIntegerVars()[varIndex]._name + " + 1", false); break;
                            case Expression::Int16Both: Compiler::emitVcpuAsm("LDW", "_" + Compiler::getIntegerVars()[varIndex]._name,          false); break;

                            default: break;
                        }

                        Compiler::emitVcpuAsm("%PrintAcInt16", "", false);
                    }
                    // String array with variable index
                    else
                    {
                        Compiler::emitVcpuAsm("LDW", Expression::byteToHexString(uint8_t(Compiler::getTempVarStart())), false);
                        Compiler::emitVcpuAsm("%PrintAcString", "", false);
                    }
                }
                else
                {
                    Compiler::emitVcpuAsm("%PrintAcInt16", "", false);
                }
            }
            else if(expressionType & Expression::HasStrVars)
            {
                if(strIndex >= 0)
                {
                    if(Compiler::getStringVars()[strIndex]._varType != Compiler::VarStr2)
                    {
                        std::string strName = Compiler::getStringVars()[strIndex]._name;
                        Compiler::emitVcpuAsm("%PrintString", "_" + strName, false);
                    }
                    // String array with literal index
                    else
                    {
                        if(!Expression::parse(tokens[i], codeLineIndex, numeric))
                        {
                            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::PRINT() : '%s:%d' : syntax error in '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                                 Expression::getExpression(), codeLine._text.c_str());
                            return false;
                        }
                        Compiler::emitVcpuAsm("LDW", Expression::byteToHexString(uint8_t(Compiler::getTempVarStart())), false);
                        Compiler::emitVcpuAsm("%PrintAcString", "", false);
                    }
                }
            }
            else if(expressionType & Expression::HasKeywords)
            {
                if(!Expression::parse(tokens[i], codeLineIndex, numeric))
                {
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::PRINT() : '%s:%d' : syntax error in '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, Expression::getExpression(), codeLine._text.c_str());
                    return false;
                }
                Compiler::emitVcpuAsm("LDW", Expression::byteToHexString(uint8_t(Compiler::getTempVarStart())), false);
                Compiler::emitVcpuAsm("%PrintAcInt16", "", false);
            }
            else if(expressionType & Expression::HasOperators)
            {
                if(!Expression::parse(tokens[i], codeLineIndex, numeric))
                {
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::PRINT() : '%s:%d' : syntax error in '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, Expression::getExpression(), codeLine._text.c_str());
                    return false;
                }
                Compiler::emitVcpuAsm("%PrintInt16", Expression::wordToHexString(int16_t(std::lround(numeric._value))), false);
            }
            else if(expressionType & Expression::HasStrings)
            {
                size_t lquote = tokens[i].find_first_of("\"");
                size_t rquote = tokens[i].find_last_of("\"");
#if 1
                // TODO: Test this thoroughly
                if(lquote > 0)
                {
                    // If there are leading chars that are not whitespace, then syntax error
                    for(size_t j=0; j<lquote; j++)
                    {
                        if(!isspace(tokens[i][j]))
                        {
                            std::string error = tokens[i].substr(0, lquote);
                            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::PRINT() : '%s:%d' : syntax error in '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, error.c_str(), codeLine._text.c_str());
                            return false;
                        }
                    }
                }
                if(rquote < tokens[i].size() - 1)
                {
                    // If there are trailing chars left over and they are not whitespace, then syntax error
                    for(size_t j=rquote+1; j<tokens[i].size(); j++)
                    {
                        if(!isspace(tokens[i][j]))
                        {
                            std::string error = tokens[i].substr(rquote + 1);
                            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::PRINT() : '%s:%d' : syntax error in '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, error.c_str(), codeLine._text.c_str());
                            return false;
                        }
                    }
                }
#endif
                if(lquote != std::string::npos  &&  rquote != std::string::npos)
                {
                    if(rquote == lquote + 1) continue; // skip empty strings
                    std::string str = tokens[i].substr(lquote + 1, rquote - (lquote + 1));

                    // Create string
                    std::string name;
                    uint16_t address;
                    if(Compiler::getOrCreateString(codeLine, codeLineIndex, str, name, address) == -1) return false;

                    // Print string
                    Compiler::emitVcpuAsm("%PrintString", "_" + name, false);
                }
            }
            else if(expressionType == Expression::HasStrConsts  &&  constIndex > -1)
            {
                // Print constant string
                std::string internalName = Compiler::getConstants()[constIndex]._internalName;
                Compiler::emitVcpuAsm("%PrintString", "_" + internalName, false);
            }
            else if(expressionType == Expression::HasIntConsts  &&  constIndex > -1)
            {
                // Print constant int
                if(Compiler::getConstants()[constIndex]._lazyUpdate)
                {
                    std::string name = Compiler::getConstants()[constIndex]._name;
                    Compiler::emitVcpuAsm("%PrintInt16", "_" + name, false);
                }
                else
                {
                    int16_t data = Compiler::getConstants()[constIndex]._data;
                    Compiler::emitVcpuAsm("%PrintInt16", Expression::wordToHexString(data), false);
                }
            }
            else if(expressionType == Expression::HasNumbers)
            {
                if(!Expression::parse(tokens[i], codeLineIndex, numeric))
                {
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::PRINT() : '%s:%d' : syntax error in '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, Expression::getExpression(), codeLine._text.c_str());
                    return false;
                }
                Compiler::emitVcpuAsm("%PrintInt16", Expression::wordToHexString(int16_t(std::lround(numeric._value))), false);
            }
        }

        // New line
        if(codeLine._code[codeLine._code.size() - 1] != ';'  &&  codeLine._code[codeLine._code.size() - 1] != ',')
        {
            Compiler::emitVcpuAsm("%NewLine", "", false);
        }

        return true;
    }

    bool INPUT(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(tokenIndex);

        // Tokenise string and vars
        std::vector<std::string> strings;
        std::string text = codeLine._code.substr(foundPos);
        Expression::stripNonStringWhitespace(text);
        std::vector<std::string> tokens = Expression::tokenise(text, ',', false, false);
        std::string code = Expression::stripStrings(text, strings, true);
        std::vector<std::string> varTokens = Expression::tokenise(code, ',', false, false);

        if(varTokens.size() < 1  ||  (strings.size() > varTokens.size() + 1))
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::INPUT() : '%s:%d' : syntax error, use 'INPUT <optional BEEP>, <heading>, <int/str var0>, <prompt0>, ... <int/str varN>, <promptN>' : %s\n",
                                                    codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        // Print heading string
        bool foundHeadingString = false;
        if(tokens.size()  &&  Expression::isStringValid(tokens[0]))
        {
            size_t lquote = tokens[0].find_first_of("\"");
            size_t rquote = tokens[0].find_last_of("\"");
            if(lquote != std::string::npos  &&  rquote != std::string::npos)
            {
                // Skip empty strings
                if(rquote > lquote + 1)
                {
                    std::string str = tokens[0].substr(lquote + 1, rquote - (lquote + 1));

                    // Create string
                    std::string name;
                    uint16_t address;
                    if(Compiler::getOrCreateString(codeLine, codeLineIndex, str, name, address) == -1) return false;

                    // Print string
                    Compiler::emitVcpuAsm("%PrintString", "_" + name, false);
                    foundHeadingString = true;
                }
            }
        }

        // INPUT vars/strs/types LUTs, (extra 0x0000 delimiter used by VASM runtime)
        std::vector<uint16_t> varsLut(varTokens.size());
        std::vector<uint16_t> strsLut(varTokens.size());
        std::vector<uint16_t> typesLut(varTokens.size() + 1, 0x0000);

        // Loop through vars
        for(int i=0; i<int(varTokens.size()); i++)
        {
            // Int var exists
            bool isStrVar = false;
            int intVar = Compiler::findVar(varTokens[i]);
            if(intVar >= 0)
            {
                varsLut[i] = Compiler::getIntegerVars()[intVar]._address;
                typesLut[i] = Compiler::VarInt16;
                continue;
            }
            // Str var exists
            else
            {
                if(Expression::isStrNameValid(varTokens[i]))
                {
                    isStrVar = true;
                    int strIndex = Compiler::findStr(varTokens[i]);
                    if(strIndex >= 0)
                    {
                        varsLut[i] = Compiler::getStringVars()[strIndex]._address;
                        typesLut[i] = Compiler::VarStr;
                        continue;
                    }
                }
            }

            // Create int var
            int varIndex = -1;
            if(!isStrVar)
            {
                if(!Compiler::createIntVar(varTokens[i], 0, 0, codeLine, codeLineIndex, false, varIndex))
                {
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::INPUT() : '%s:%d' : couldn't create integer var '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                                     varTokens[i].c_str(), codeLine._text.c_str());
                    return false;
                }
                varsLut[i] = Compiler::getIntegerVars()[varIndex]._address;
                typesLut[i] = Compiler::VarInt16;
            }
            // Create str var
            else
            {
                uint16_t address;
                varIndex = getOrCreateString(codeLine, codeLineIndex, "", varTokens[i], address, USER_STR_SIZE, false);
                if(varIndex == -1)
                {
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::INPUT() : '%s:%d' : couldn't create string var '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                                    varTokens[i].c_str(), codeLine._text.c_str());
                    return false;
                }
                varsLut[i] = Compiler::getStringVars()[varIndex]._address;
                typesLut[i] = Compiler::VarStr;
            }
        }

        // Loop through strs
        for(int i=0; i<int(varTokens.size()); i++)
        {
            // Create string
            std::string name;
            uint16_t address;
            int index = (foundHeadingString) ? i + 1 : i;
            std::string str = (index < int(strings.size())) ? strings[index] : "\"?\";;";
            size_t fquote = str.find_first_of('"');
            size_t lquote = str.find_last_of('"');

            // Semicolons
            if(str.size() > lquote + 1  &&  str[lquote + 1] != ';') typesLut[i] |= 0x40;
            if(str.size() > lquote + 1  &&  str[lquote + 1] == ';') str.erase(lquote + 1, 1);
            if(str.back() != ';') typesLut[i] |= 0x80;
            if(str.back() == ';') str.erase(str.size() - 1, 1);

            // Text length field
            uint8_t length = USER_STR_SIZE;
            if(str.size() > lquote + 1  &&  isdigit((unsigned char)str[lquote + 1]))
            {
                std::string field = str.substr(lquote + 1);
                if(!Expression::stringToU8(field, length))
                {
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::INPUT() : '%s:%d' : syntax error in text size field of string '%s' of INPUT statement : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                                                                      str.c_str(), codeLine._text.c_str());
                    return false;
                }
                if(length > USER_STR_SIZE)
                {
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::INPUT() : '%s:%d' : text size field > %d of string '%s' of INPUT statement : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                                                           USER_STR_SIZE, str.c_str(), codeLine._text.c_str());
                    return false;
                }

                str.erase(lquote + 1, field.size());
            }
            typesLut[i] |= (length + 1) << 8; // increment length as INPUT VASM code counts cursor
        
            // Remove quotes, (remove last quote first)
            str.erase(lquote, 1);
            str.erase(fquote, 1);

            if(Compiler::getOrCreateString(codeLine, codeLineIndex, str, name, address) == -1) return false;
            strsLut[i] = address;
        }

        // Allocate memory for register work area if it hasn't been allocated already
        if(Compiler::getRegWorkArea() == 0x0000)
        {
            uint16_t regAddr;
            if(!Memory::getFreePageRAM(Memory::FitDescending, REG_WORK_SIZE, USER_CODE_START, Compiler::getRuntimeStart(), regAddr, true))
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::INPUT() : '%s:%d' : not enough RAM for register work area of size %d : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                                                 REG_WORK_SIZE, codeLine._text.c_str());
                return false;
            }
            Compiler::setRegWorkArea(regAddr);
        }

        // INPUT LUTs
        const int lutSize = 3;
        uint16_t lutAddr, varsAddr, strsAddr, typesAddr;
        if(!Memory::getFreePageRAM(Memory::FitDescending, lutSize*2, USER_CODE_START, Compiler::getRuntimeStart(), lutAddr, true))
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::INPUT() : '%s:%d' : not enough RAM for INPUT LUT of size %d : %s\n", codeLine._moduleName.c_str(), codeLineStart, lutSize*2, codeLine._text.c_str());
            return false;
        }
        if(!Memory::getFreePageRAM(Memory::FitDescending, int(varsLut.size()*2), USER_CODE_START, Compiler::getRuntimeStart(), varsAddr, true))
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::INPUT() : '%s:%d' : not enough RAM for INPUT Vars LUT of size %d : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                                         int(varsLut.size()*2), codeLine._text.c_str());
            return false;
        }
        if(!Memory::getFreePageRAM(Memory::FitDescending, int(strsLut.size()*2), USER_CODE_START, Compiler::getRuntimeStart(), strsAddr, true))
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::INPUT() : '%s:%d' : not enough RAM for INPUT Strings LUT of size %d : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                                            int(strsLut.size()*2), codeLine._text.c_str());
            return false;
        }
        if(!Memory::getFreePageRAM(Memory::FitDescending, int(typesLut.size()*2), USER_CODE_START, Compiler::getRuntimeStart(), typesAddr, true))
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::INPUT() : '%s:%d' : not enough RAM for INPUT Var Types LUT of size %d : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                                              int(typesLut.size()*2), codeLine._text.c_str());
            return false;
        }
        Compiler::getCodeLines()[codeLineIndex]._inputLut = {lutAddr, varsAddr, strsAddr, typesAddr, varsLut, strsLut, typesLut}; // save LUT in global codeLine not local copy
        Compiler::emitVcpuAsm("LDWI", Expression::wordToHexString(lutAddr), false);
        Compiler::emitVcpuAsm("%Input", "", false);

        return true;
    }

    bool FOR(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(tokenIndex);

        bool optimise = true;
        bool hasVarStart = false;
        int varIndex, constIndex, strIndex;
        uint32_t expressionType;

        // Parse first line of FOR loop
        std::string code = codeLine._code;
        Expression::strToUpper(code);
        size_t equals, to, step;
        if((equals = code.find("=")) == std::string::npos)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::FOR() : '%s:%d' : syntax error, missing '=' : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        // TO uses INC/ADD, UPTO uses INC/ADD, DOWNTO uses DEC/SUB; &TO/&UPTO/&DOWNTO are optimised BRA versions
        Compiler::ForNextType type = Compiler::AutoTo;
        type = (code.find("UPTO")   != std::string::npos) ? Compiler::UpTo   : type;
        type = (code.find("DOWNTO") != std::string::npos) ? Compiler::DownTo : type;
        bool farJump = (code.find("&TO") == std::string::npos)  &&  (code.find("&UPTO") == std::string::npos)  &&  (code.find("&DOWNTO") == std::string::npos);
        if((to = code.find("TO")) == std::string::npos)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::FOR() : '%s:%d' : syntax error, missing 'TO' or 'DOWNTO' : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }
        step = code.find("STEP");

        // Maximum of 4 nested loops
        if(Compiler::getForNextDataStack().size() == MAX_NESTED_LOOPS)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::FOR() : '%s:%d' : syntax error, maximum nested loops is 4 : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        // Nested loops temporary variables
        uint16_t offset = uint16_t(Compiler::getForNextDataStack().size()) * LOOP_VARS_SIZE;
        uint16_t varEnd = LOOP_VAR_START + offset;
        uint16_t varStep = LOOP_VAR_START + offset + sizeof(uint16_t);

        // Adjust 'to' based on length of TO keyword
        int16_t loopStart = 0;
        int toOffset = (farJump) ? 0 : 0 - int(sizeof('&'));
        toOffset = (type == Compiler::UpTo)   ? toOffset - (sizeof("UP")-1)   : toOffset;
        toOffset = (type == Compiler::DownTo) ? toOffset - (sizeof("DOWN")-1) : toOffset;

        // Loop start
        std::string startToken = codeLine._code.substr(equals + sizeof('='), to - (equals + sizeof('=')) + toOffset);
        expressionType = Compiler::isExpression(startToken, varIndex, constIndex, strIndex);
        if((expressionType & Expression::HasIntVars)  ||  (expressionType & Expression::HasKeywords)  ||  (expressionType & Expression::HasFunctions)) hasVarStart = true;

        // Var counter, (create or update if being reused)
        std::string var = codeLine._code.substr(foundPos, equals - foundPos);
        Expression::stripWhitespace(var);
        int varCounter = Compiler::findVar(var);
        if(varCounter == -1)
        {
            if(!Compiler::createIntVar(var, loopStart, 0, codeLine, codeLineIndex, false, varCounter)) return false;
        }
        else
        {
            Compiler::updateIntVar(loopStart, codeLine, varCounter, false);
        }

        // Loop end, (optimise fails for loop end is a var and rom != ROMvX0)
        int16_t loopEnd = 0;
        size_t end = (step == std::string::npos) ? codeLine._code.size() : step;
        std::string endToken = codeLine._code.substr(to + sizeof("TO")-1, end - (to + sizeof("TO")-1));
        expressionType = Compiler::isExpression(endToken, varIndex, constIndex, strIndex);
        if((expressionType & Expression::HasIntVars)  ||  (expressionType & Expression::HasKeywords)  ||  (expressionType & Expression::HasFunctions))
        {
            if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD) optimise = false;
        }

        // Loop step
        int16_t loopStep = 1;
        std::string stepToken;
        if(step != std::string::npos)
        {
            end = codeLine._code.size();
            stepToken = codeLine._code.substr(step + sizeof("STEP")-1, end - (step + sizeof("STEP")-1));
            expressionType = Compiler::isExpression(stepToken, varIndex, constIndex, strIndex);
            if((expressionType & Expression::HasIntVars)  ||  (expressionType & Expression::HasKeywords)  ||  (expressionType & Expression::HasFunctions)) optimise = false;
        }

        bool romvx = (Compiler::getCodeRomType() >= Cpu::ROMvX0  &&  Compiler::getCodeRomType() < Cpu::SDCARD);

        Expression::Numeric startNumeric, endNumeric, stepNumeric;
        if(optimise)
        {
            // Parse start
            if(!Expression::parse(startToken, codeLineIndex, startNumeric))
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::FOR() : '%s:%d' : syntax error in : '%s'\n", codeLine._moduleName.c_str(), codeLineStart, Expression::getExpression());
                return false;
            }
            loopStart = int16_t(std::lround(startNumeric._value));

            // Parse end
            if(!Expression::parse(endToken, codeLineIndex, endNumeric))
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::FOR() : '%s:%d' : syntax error in : '%s'\n", codeLine._moduleName.c_str(), codeLineStart, Expression::getExpression());
                return false;
            }
            loopEnd = int16_t(std::lround(endNumeric._value));
            bool constEnd = (endNumeric._varType == Expression::Number  ||  endNumeric._varType == Expression::Constant);

            // Parse step
            if(stepToken.size())
            {
                if(!Expression::parse(stepToken, codeLineIndex, stepNumeric))
                {
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::FOR() : '%s:%d' : syntax error in : '%s'\n", codeLine._moduleName.c_str(), codeLineStart, Expression::getExpression());
                    return false;
                }
                loopStep = int16_t(std::lround(stepNumeric._value));
                if(loopStep < 1  ||  loopStep > 255) optimise = false;
            }

            // Variable start
            if(optimise  &&  hasVarStart  &&  endNumeric._isValid  &&  constEnd  &&  loopEnd >= 0  &&  loopEnd <= 255)
            {
                if(Compiler::parseExpression(codeLineIndex, startToken, startNumeric) == Expression::IsInvalid)
                {
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::FOR() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, startToken.c_str(), codeLine._text.c_str());
                    return false;
                }
                loopStart = int16_t(std::lround(startNumeric._value));
                Compiler::emitVcpuAsm("STW", "_" + Compiler::getIntegerVars()[varCounter]._name, false);
            }
            // ROMvX0
            else if(romvx  &&  optimise  &&  abs(loopStep) == 1)
            {
                // Loop start
                if(hasVarStart)
                {
                    // Variable start
                    if(Compiler::parseExpression(codeLineIndex, startToken, startNumeric) == Expression::IsInvalid)
                    {
                        Cpu::reportError(Cpu::KwdError, stderr, "Keywords::FOR() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, endToken.c_str(), codeLine._text.c_str());
                        return false;
                    }
                    loopStart = int16_t(std::lround(startNumeric._value));
                    Compiler::emitVcpuAsm("STW", "_" + Compiler::getIntegerVars()[varCounter]._name, false);
                }
                else
                {
                    Expression::Numeric numericTmp = Expression::Numeric(int16_t(loopStart), -1, true, false, false, Expression::Number, {false, Expression::BooleanCC}, Expression::Int16Both, std::string(""), std::string(""));
                    Compiler::emitVcpuAsmForLiteral(numericTmp, "_" + Compiler::getIntegerVars()[varCounter]._name, false);
                }

                // ROMvX0 INCW/DECW with constants and loop end = 0,1
                if(startNumeric._isValid  &&  endNumeric._isValid  &&  constEnd  &&  (loopEnd == 0  ||  loopEnd == 1))
                {
                }
                // ROMvX0 INC/DEC with constants loop end = 2 to 255
                else if(startNumeric._isValid  &&  endNumeric._isValid  &&  constEnd  &&  loopEnd > 1  &&  loopEnd <= 255)
                {
                }
                // ROMvX0 INCW/DECW with loop end var
                else
                {
                    optimise = false;

                    // Variable end
                    if(Compiler::parseExpression(codeLineIndex, endToken, endNumeric) == Expression::IsInvalid)
                    {
                        Cpu::reportError(Cpu::KwdError, stderr, "Keywords::FOR() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, endToken.c_str(), codeLine._text.c_str());
                        return false;
                    }
                    loopEnd = int16_t(std::lround(endNumeric._value));
                    Compiler::emitVcpuAsm("STW", Expression::byteToHexString(uint8_t(varEnd)), false);
                }
            }
            // 8bit constants
            else if(optimise  &&  startNumeric._isValid  &&  loopStart >= 0  &&  loopStart <= 255  &&  endNumeric._isValid  &&  loopEnd >= 0  &&  loopEnd <= 255)
            {
                Expression::Numeric numericTmp = Expression::Numeric(int16_t(loopStart), -1, true, false, false, Expression::Number, {false, Expression::BooleanCC}, Expression::Int16Both, std::string(""), std::string(""));
                Compiler::emitVcpuAsmForLiteral(numericTmp, "_" + Compiler::getIntegerVars()[varCounter]._name, false);
            }
            // 16bit constants
            else
            {
                optimise = false;

                // Loop start
                Expression::Numeric numericTmp = Expression::Numeric(int16_t(loopStart), -1, true, false, false, Expression::Number, {false, Expression::BooleanCC}, Expression::Int16Both, std::string(""), std::string(""));
                Compiler::emitVcpuAsmForLiteral(numericTmp, "_" + Compiler::getIntegerVars()[varCounter]._name, false);
                
                // Loop end
                numericTmp = Expression::Numeric(int16_t(loopEnd), -1, true, false, false, Expression::Number, {false, Expression::BooleanCC}, Expression::Int16Both, std::string(""), std::string(""));
                Compiler::emitVcpuAsmForLiteral(numericTmp, Expression::byteToHexString(uint8_t(varEnd)), false);

                // Loop step
                numericTmp = Expression::Numeric(int16_t(loopStep), -1, true, false, false, Expression::Number, {false, Expression::BooleanCC}, Expression::Int16Both, std::string(""), std::string(""));
                Compiler::emitVcpuAsmForLiteral(numericTmp, Expression::byteToHexString(uint8_t(varStep)), false);
            }
        }
        else
        {
            // Parse start
            if(Compiler::parseExpression(codeLineIndex, startToken, startNumeric) == Expression::IsInvalid)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::FOR() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, startToken.c_str(), codeLine._text.c_str());
                return false;
            }
            loopStart = int16_t(std::lround(startNumeric._value));
            Compiler::emitVcpuAsm("STW", "_" + Compiler::getIntegerVars()[varCounter]._name, false);

            // Parse end
            if(Compiler::parseExpression(codeLineIndex, endToken, endNumeric) == Expression::IsInvalid)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::FOR() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, endToken.c_str(), codeLine._text.c_str());
                return false;
            }
            loopEnd = int16_t(std::lround(endNumeric._value));
            Compiler::emitVcpuAsm("STW", Expression::byteToHexString(uint8_t(varEnd)), false);

            // Parse step
            if(stepToken.size())
            {
                if(Compiler::parseExpression(codeLineIndex, stepToken, stepNumeric) == Expression::IsInvalid)
                {
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::FOR() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, stepToken.c_str(), codeLine._text.c_str());
                    return false;
                }
                loopStep = int16_t(std::lround(stepNumeric._value));
            }
            else
            {
                loopStep = 1;
                Compiler::emitVcpuAsm("LDI", std::to_string(loopStep), false);
            }
            Compiler::emitVcpuAsm("STW", Expression::byteToHexString(uint8_t(varStep)), false);
        }

        // Label and stack
        Compiler::Label label;
        std::string forLabelName = "_next_" + Expression::wordToHexString(uint16_t(Compiler::getNextFlowControlUniqueId()));
        int labelIndex = Compiler::createLabel(Compiler::getVasmPC(), forLabelName, codeLineIndex, label, true, false, false);
        Compiler::setNextVasmLabelIndex(labelIndex);
        Compiler::getForNextDataStack().push({varCounter, forLabelName, loopStart, loopEnd, loopStep, varEnd, varStep, type, farJump, optimise, romvx, codeLineIndex});

        return true;
    }

    bool NEXT(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(tokenIndex);
        UNREFERENCED_PARAM(codeLineIndex);

        if(codeLine._tokens.size() != 2)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::NEXT() : '%s:%d' : syntax error, wrong number of tokens : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        std::string var = codeLine._code.substr(foundPos);
        int varIndex = Compiler::findVar(codeLine._tokens[1]);
        if(varIndex < 0)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::NEXT() : '%s:%d' : syntax error, bad var : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        // Pop stack for this nested loop
        if(Compiler::getForNextDataStack().empty())
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::NEXT() : '%s:%d' : syntax error, missing FOR statement : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }
        Compiler::ForNextData forNextData = Compiler::getForNextDataStack().top();
        Compiler::getForNextDataStack().pop();

        if(varIndex != forNextData._varIndex)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::NEXT() : '%s:%d' : syntax error, wrong var : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        std::string varName = Compiler::getIntegerVars()[varIndex]._name;
        std::string labName = forNextData._labelName;
        int16_t loopStart = forNextData._loopStart;
        int16_t loopEnd = forNextData._loopEnd;
        int16_t loopStep = forNextData._loopStep;
        uint16_t varEnd = forNextData._varEnd;
        uint16_t varStep = forNextData._varStep;
        Compiler::ForNextType type = forNextData._type;
        bool farJump = forNextData._farJump;
        bool optimise = forNextData._optimise;
        bool romvx = forNextData._romvx;

        std::string forNextCmd;
        if(optimise)
        {
            if(abs(loopStep) == 1)
            {
                switch(type)
                {
                    case Compiler::AutoTo:
                    case Compiler::UpTo:
                    {
                        // ROMvX0 16bit
                        if(romvx)
                        {
                            // Increment to 0
                            if(loopEnd == 0)
                            {
                                Compiler::emitVcpuAsm("%ForNextIncZero", "_" + varName + " " + labName, false);
                            }
                            // Increment to 1
                            else if(loopEnd == 1)
                            {
                                Compiler::emitVcpuAsm("%ForNextIncOne", "_" + varName + " " + labName, false);
                            }
                            // Increment to constant, (2 to 255)
                            else
                            {
                                Compiler::emitVcpuAsm("%ForNextInc", "_" + varName + " " + labName + " " + std::to_string(loopEnd), false);
                            }
                        }
                        // rom != ROMvX0, (can't use INC when counting upto 0 or 255)
                        else if(loopEnd == 0  ||  loopEnd == 255)
                        {
                            forNextCmd = (farJump) ? "%ForNextFarAdd" : "%ForNextAdd";
                            Compiler::emitVcpuAsm(forNextCmd, "_" + varName + " " + labName + " " + std::to_string(loopEnd) + " " + std::to_string(abs(loopStep)), false);
                        }
                        // rom != ROMvX0, increment to constant
                        else
                        {
                            forNextCmd = (farJump) ? "%ForNextFarInc" : "%ForNextInc";
                            Compiler::emitVcpuAsm(forNextCmd, "_" + varName + " " + labName + " " + std::to_string(loopEnd), false);
                        }
                    }
                    break;

                    case Compiler::DownTo:
                    {
                        // ROMvX0 16bit
                        if(romvx)
                        {
                            // Decrement to 0
                            if(loopEnd == 0)
                            {
                                if(!farJump  &&  loopStart > 0  &&  loopStart <= 255)
                                {
                                    Compiler::emitVcpuAsm("%ForNextDBGE", "_" + varName + " " + labName, false);
                                }
                                else
                                {
                                    Compiler::emitVcpuAsm("%ForNextDJGE", "_" + varName + " " + labName, false);
                                }
                            }
                            // Decrement to 1
                            else if(loopEnd == 1)
                            {
                                if(!farJump  &&  loopStart > 0  &&  loopStart <= 255)
                                {
                                    Compiler::emitVcpuAsm("%ForNextDBNE", "_" + varName + " " + labName, false);
                                }
                                else
                                {
                                    Compiler::emitVcpuAsm("%ForNextDJNE", "_" + varName + " " + labName, false);
                                }
                            }
                            // Decrement to constant, (2 to 255)
                            else
                            {
                                Compiler::emitVcpuAsm("%ForNextDec", "_" + varName + " " + labName + " " + std::to_string(loopEnd), false);
                            }
                        }
                        // rom != ROMvX0
                        else
                        {
                            // Decrement to 0
                            if(loopEnd == 0)
                            {
                                forNextCmd = (farJump) ? "%ForNextFarDecZero" : "%ForNextDecZero";
                                Compiler::emitVcpuAsm(forNextCmd, "_" + varName + " " + labName, false);
                            }
                            // Decrement to 1
                            else if(loopEnd == 1)
                            {
                                forNextCmd = (farJump) ? "%ForNextFarDecOne" : "%ForNextDecOne";
                                Compiler::emitVcpuAsm(forNextCmd, "_" + varName + " " + labName, false);
                            }
                            // Decrement to constant, (2 to 255)
                            else
                            {
                                forNextCmd = (farJump) ? "%ForNextFarDec" : "%ForNextDec";
                                Compiler::emitVcpuAsm(forNextCmd, "_" + varName + " " + labName + " " + std::to_string(loopEnd), false);
                            }
                        }
                    }
                    break;

                    default: break;
                }
            }
            // Additive/subtractive step
            else
            {
                // ADDVI/SUBVI to 0
                if(romvx  &&  loopEnd == 0)
                {
                    switch(type)
                    {
                        case Compiler::AutoTo:
                        case Compiler::UpTo:   forNextCmd = "%ForNextAddZero"; break;
                        case Compiler::DownTo: forNextCmd = "%ForNextSubZero"; break;

                        default: break;
                    }
                    Compiler::emitVcpuAsm(forNextCmd, "_" + varName + " " + labName + " " + std::to_string(loopEnd) + " " + std::to_string(abs(loopStep)), false);
                }
                // Everything else
                else
                {
                    switch(type)
                    {
                        case Compiler::AutoTo:
                        case Compiler::UpTo:   forNextCmd = (farJump) ? "%ForNextFarAdd" : "%ForNextAdd"; break;
                        case Compiler::DownTo: forNextCmd = (farJump) ? "%ForNextFarSub" : "%ForNextSub"; break;

                        default: break;
                    }
                    Compiler::emitVcpuAsm(forNextCmd, "_" + varName + " " + labName + " " + std::to_string(loopEnd) + " " + std::to_string(abs(loopStep)), false);
                }
            }
        }
        // Positive/negative variable step
        else
        {
            if(romvx  &&  abs(loopStep) == 1)
            {
                switch(type)
                {
                    case Compiler::AutoTo:
                    case Compiler::UpTo:   Compiler::emitVcpuAsm("%ForNextVarInc", "_" + varName + " " + labName + " " + Expression::byteToHexString(uint8_t(varEnd)), false); break;
                    case Compiler::DownTo: Compiler::emitVcpuAsm("%ForNextVarDec", "_" + varName + " " + labName + " " + Expression::byteToHexString(uint8_t(varEnd)), false); break;

                    default: break;
                }
            }
            else
            {
                switch(type)
                {
                    case Compiler::AutoTo:
                    case Compiler::UpTo:   forNextCmd = (farJump) ? "%ForNextFarVarAdd" : "%ForNextVarAdd"; break;
                    case Compiler::DownTo: forNextCmd = (farJump) ? "%ForNextFarVarSub" : "%ForNextVarSub"; break;

                    default: break;
                }
                Compiler::emitVcpuAsm(forNextCmd, "_" + varName + " " + labName + " " + Expression::byteToHexString(uint8_t(varEnd)) + " " + Expression::byteToHexString(uint8_t(varStep)), false);
            }
        }

        return true;
    }

    bool subStrFlowCtrl(const std::string& replace, std::string& inout)
    {
        std::size_t underscore0 = inout.find('_');
        if(underscore0 == std::string::npos) return false;
        std::size_t underscore1 = inout.find('_', underscore0 + 1);
        if(underscore1 == std::string::npos) return false;

        std::string subText = inout.substr(underscore0, underscore1 - underscore0 + 1);

        return (Expression::replaceText(inout, subText, replace) > 0);
    }
    bool fixFlowCtrlLabel(int codeIndex, int vasmIndex, const std::string& subStr, std::string& inout, Expression::CCType ccType)
    {
        inout = Expression::getVasmOperand(inout);
        if(subStrFlowCtrl(subStr, inout) == 0) return false;

        bool dontMakeUnique = (ccType == Expression::FastCC  ||  (Compiler::getCodeRomType() >= Cpu::ROMvX0  &&  Compiler::getCodeRomType() < Cpu::SDCARD));

        std::string opcode = Compiler::getCodeLines()[codeIndex]._vasm[vasmIndex]._opcode;
        if(opcode[0] == '%') opcode.erase(0, 1);
        Compiler::getCodeLines()[codeIndex]._vasm[vasmIndex]._operand = (dontMakeUnique) ? inout : inout + " " + std::to_string(_uniqueJumpId++);
        Compiler::getCodeLines()[codeIndex]._vasm[vasmIndex]._code = Expression::createPaddedString(opcode, OPCODE_TRUNC_SIZE, ' ') + Compiler::getCodeLines()[codeIndex]._vasm[vasmIndex]._operand;

        return true;
    }
    void fixEndIflLabel(int codeIndex, int vasmIndex, const std::string& label)
    {
        std::string opcode = Compiler::getCodeLines()[codeIndex]._vasm[vasmIndex]._opcode;
        if(opcode[0] == '%') opcode.erase(0, 1);
        Compiler::getCodeLines()[codeIndex]._vasm[vasmIndex]._operand = label;
        Compiler::getCodeLines()[codeIndex]._vasm[vasmIndex]._code = Expression::createPaddedString(opcode, OPCODE_TRUNC_SIZE, ' ') + Compiler::getCodeLines()[codeIndex]._vasm[vasmIndex]._operand;
    }
    bool IF(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(tokenIndex);

        bool ifElseEndif = false;

        // IF
        std::string code = Compiler::getCodeLines()[codeLineIndex]._code;
        Expression::strToUpper(code);
        size_t offsetIF = code.find("IF");

        // THEN
        code = codeLine._code;
        Expression::strToUpper(code);
        size_t offsetTHEN = code.find("THEN");
        if(offsetTHEN == std::string::npos) ifElseEndif = true;

        // Condition
        Expression::Numeric condition;
        std::string conditionToken = codeLine._code.substr(foundPos, offsetTHEN - foundPos);
        if(Compiler::parseExpression(codeLineIndex, conditionToken, condition) == Expression::IsInvalid)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::IF() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, conditionToken.c_str(), codeLine._text.c_str());
            return false;
        }

        // Bail early as we assume this is an IF ELSE ENDIF block
        if(ifElseEndif)
        {
            std::string endIfLabelName = "_endif_" + Expression::wordToHexString(uint16_t(Compiler::getNextFlowControlUniqueId()));
            int vasmIndex = emitJumpType(codeLineIndex, "%JumpFalse", endIfLabelName, condition._condCode._ccType);
            if(vasmIndex < 0)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::IF() : '%s:%d' : internal error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, conditionToken.c_str(), codeLine._text.c_str());
                return false;
            }

            std::stack<Compiler::FixUpLabel> endIfFixUps;
            Compiler::getElseIfDataStack().push({codeLineIndex, vasmIndex, endIfLabelName, Compiler::IfBlock, condition._condCode, endIfFixUps});
            return true;
        }

        std::string ifLabel = "_if_" + Expression::wordToHexString(uint16_t(Compiler::getNextFlowControlUniqueId()));
        if(emitJumpType(codeLineIndex, "%JumpFalse", ifLabel, condition._condCode._ccType) < 0)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::IF() : '%s:%d' : internal error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, conditionToken.c_str(), codeLine._text.c_str());
            return false;
        }

        // Action
        std::string actionToken = Compiler::getCodeLines()[codeLineIndex]._code.substr(offsetIF + offsetTHEN + 4);
        if(actionToken.size() == 0)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::IF() : '%s:%d' : syntax error, missing action in 'IF THEN <action>' : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }
        Expression::trimWhitespace(actionToken);
        std::string actionText = Expression::collapseWhitespaceNotStrings(actionToken);

        // Short circuit GOTO if action is a literal constant or a label
        uint16_t res = 0;
        if(Expression::stringToU16(actionText, res)  ||  Compiler::findLabel(actionText) != -1)
        {
            actionText = "GOTO " + actionText;
        }

        // Multi-statements
        int varIndex, strIndex;
        if(Compiler::parseMultiStatements(actionText, codeLineIndex, codeLineStart, varIndex, strIndex) == Compiler::StatementError) return false;

        Compiler::Label label;
        int labelIndex = Compiler::createLabel(Compiler::getVasmPC(), ifLabel, codeLineIndex, label, true, false, false);
        Compiler::setNextVasmLabelIndex(labelIndex);

        return true;
    }

    bool ELSEIF(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(tokenIndex);

        // Check stack for this IF ELSE ENDIF block
        if(Compiler::getElseIfDataStack().empty())
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::ELSEIF() : '%s:%d' : syntax error, missing IF statement : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        // Check stack for this IF ELSE ENDIF block
        if(Compiler::getElseIfDataStack().empty())
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::ELSEIF() : '%s:%d' : syntax error, missing IF statement : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        Compiler::ElseIfData elseIfData = Compiler::getElseIfDataStack().top();
        int vasmIndex = elseIfData._vasmIndex;
        int codeIndex = elseIfData._codeIndex;
        std::string elseIfLabelName = elseIfData._labelName;
        Compiler::IfElseEndType ifElseEndType = elseIfData._ifElseEndType;
        Expression::CCType ccType = elseIfData._condCode._ccType;
        std::stack<Compiler::FixUpLabel> endIfFixUps = elseIfData._endIfFixUps;
        Compiler::getElseIfDataStack().pop();

        if(ifElseEndType != Compiler::IfBlock  &&  ifElseEndType != Compiler::ElseIfBlock)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::ELSEIF() : '%s:%d' : syntax error, ELSEIF follows IF or ELSEIF : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        // Fixup flow control label
        if(!fixFlowCtrlLabel(codeIndex, vasmIndex, "_elseif_", elseIfLabelName, ccType))
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::ELSEIF() : '%s:%d' : internal error, couldn't find '_elseif_' : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        // Jump to endif label
        int endifVasmIndex = -1;
        if(Compiler::getCodeRomType() >= Cpu::ROMv5a)
        {
            endifVasmIndex = Compiler::emitVcpuAsm("CALLI", "ENDIF", false).second;
        }
        else
        {
            endifVasmIndex = Compiler::emitVcpuAsm("LDWI", "ENDIF",  false).second;
            Compiler::emitVcpuAsm("CALL", "giga_vAC", false);
        }
        endIfFixUps.push({codeLineIndex, endifVasmIndex, {false, ccType}});

        // Create elseIf label on next line of vasm code
        Compiler::Label elseIfLabel;
        int elseifLabelIndex = Compiler::createLabel(Compiler::getVasmPC(), elseIfLabelName, codeLineIndex, elseIfLabel, true, false, false);
        Compiler::setNextVasmLabelIndex(elseifLabelIndex);

        // Condition
        Expression::Numeric condition;
        std::string conditionToken = codeLine._code.substr(foundPos);
        if(Compiler::parseExpression(codeLineIndex, conditionToken, condition) == Expression::IsInvalid)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::ELSEIF() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, conditionToken.c_str(), codeLine._text.c_str());
            return false;
        }

        // Push elseif, else or endif onto stack, (default to endif, gets fixed by fixFlowCtrlLabel())
        std::string chainedLabel = "_endif_" + Expression::wordToHexString(uint16_t(Compiler::getNextFlowControlUniqueId()));
        int chainedVasmIndex = emitJumpType(codeLineIndex, "%JumpFalse", chainedLabel, condition._condCode._ccType);
        if(chainedVasmIndex < 0)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::ELSEIF() : '%s:%d' : internal error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, conditionToken.c_str(), codeLine._text.c_str());
            return false;
        }

        Compiler::getElseIfDataStack().push({codeLineIndex, chainedVasmIndex, chainedLabel, Compiler::ElseIfBlock, condition._condCode, endIfFixUps});

        return true;
    }

    bool ELSE(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(foundPos);
        UNREFERENCED_PARAM(tokenIndex);

        if(codeLine._tokens.size() != 1)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::ELSE() : '%s:%d' : syntax error, wrong number of tokens : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        // Check stack for this IF ELSE ENDIF block
        if(Compiler::getElseIfDataStack().empty())
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::ELSE() : '%s:%d' : syntax error, missing IF statement : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        Compiler::ElseIfData elseIfData = Compiler::getElseIfDataStack().top();
        int vasmIndex = elseIfData._vasmIndex;
        int codeIndex = elseIfData._codeIndex;
        std::string elseLabelName = elseIfData._labelName;
        Expression::CCType ccType = elseIfData._condCode._ccType;
        std::stack<Compiler::FixUpLabel> endIfFixUps = elseIfData._endIfFixUps;
        Compiler::getElseIfDataStack().pop();

        // Fixup flow control label
        if(!fixFlowCtrlLabel(codeIndex, vasmIndex, "_else_", elseLabelName, ccType))
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::ELSEIF() : '%s:%d' : internal error, couldn't find '_elseif_' : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        // Jump to endif label
        int endifVasmIndex = -1;
        if(Compiler::getCodeRomType() >= Cpu::ROMv5a)
        {
            endifVasmIndex = Compiler::emitVcpuAsm("CALLI", "ENDIF", false).second;
        }
        else
        {
            endifVasmIndex = Compiler::emitVcpuAsm("LDWI", "ENDIF",  false).second;
            Compiler::emitVcpuAsm("CALL", "giga_vAC", false);
        }
        endIfFixUps.push({codeLineIndex, endifVasmIndex, {false, ccType}});

        // Create else label on next line of vasm code
        Compiler::Label label;
        int labelIndex = Compiler::createLabel(Compiler::getVasmPC(), elseLabelName, codeLineIndex, label, true, false, false);
        Compiler::setNextVasmLabelIndex(labelIndex);

        // Push endif onto stack, (must be endif for else)
        std::string endifLabel = "_endif_" + Expression::wordToHexString(uint16_t(Compiler::getNextFlowControlUniqueId()));
        Compiler::getElseIfDataStack().push({codeLineIndex, endifVasmIndex, endifLabel, Compiler::ElseBlock, elseIfData._condCode, endIfFixUps});

        return true;
    }

    bool ENDIF(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(foundPos);
        UNREFERENCED_PARAM(tokenIndex);
        UNREFERENCED_PARAM(codeLineIndex);

        if(codeLine._tokens.size() != 1)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::ENDIF() : '%s:%d' : syntax error, wrong number of tokens : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        // Check stack for this IF ELSE ENDIF block
        if(Compiler::getElseIfDataStack().empty())
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::ENDIF() : '%s:%d' : syntax error, missing IF statement : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        Compiler::ElseIfData elseIfData = Compiler::getElseIfDataStack().top();
        int vasmIndex = elseIfData._vasmIndex;
        int codeIndex = elseIfData._codeIndex;
        std::string endifLabelName = elseIfData._labelName;
        Expression::CCType ccType = elseIfData._condCode._ccType;
        std::stack<Compiler::FixUpLabel> endIfFixUps = elseIfData._endIfFixUps;
        Compiler::getElseIfDataStack().pop();

        // Update if's/elseif's jump to this new label
        Compiler::Label label;
        int labelIndex = Compiler::createLabel(Compiler::getVasmPC(), endifLabelName, codeLineIndex, label, true, false, false);
        Compiler::setNextVasmLabelIndex(labelIndex);

        // Fixup endif labels
        while(!endIfFixUps.empty())
        {
            codeIndex = endIfFixUps.top()._codeIndex;
            vasmIndex = endIfFixUps.top()._vasmIndex;
            ccType = endIfFixUps.top()._condCode._ccType;

            fixEndIflLabel(codeIndex, vasmIndex, endifLabelName);

            endIfFixUps.pop();
        }
        return true;
    }

    bool WHILE(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(tokenIndex);
        UNREFERENCED_PARAM(codeLineStart);

        // Condition
        std::string condToken = codeLine._code.substr(foundPos);

        // Condition label
        std::string condLabel = "_wcond_" + Expression::wordToHexString(uint16_t(Compiler::getNextFlowControlUniqueId()));
        if(Compiler::getCodeRomType() >= Cpu::ROMv5a)
        {
            Compiler::emitVcpuAsm("CALLI", condLabel, false);
        }
        else
        {
            Compiler::emitVcpuAsm("LDWI", condLabel,  false);
            Compiler::emitVcpuAsm("CALL", "giga_vAC", false);
        }

        Compiler::Label label;
        std::string whileLabelName = "_while_" + Expression::wordToHexString(uint16_t(Compiler::getNextFlowControlUniqueId()));
        int labelIndex = Compiler::createLabel(Compiler::getVasmPC(), whileLabelName, codeLineIndex, label, true, false, false);
        Compiler::setNextVasmLabelIndex(labelIndex);
        Compiler::getWhileWendDataStack().push({whileLabelName, condLabel, condToken, codeLineIndex});

        return true;
    }

    bool WEND(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(foundPos);
        UNREFERENCED_PARAM(tokenIndex);
        UNREFERENCED_PARAM(codeLineIndex);

        // Pop stack for this WHILE loop
        if(Compiler::getWhileWendDataStack().empty())
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::WEND() : '%s:%d' : syntax error, missing WHILE statement : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }
        Compiler::WhileWendData whileWendData = Compiler::getWhileWendDataStack().top();
        Compiler::getWhileWendDataStack().pop();

        Compiler::Label label;
        int labelIndex = Compiler::createLabel(Compiler::getVasmPC(), whileWendData._condName, codeLineIndex, label, true, false, false);
        Compiler::setNextVasmLabelIndex(labelIndex);

        // Generate code for condition
        Expression::Numeric condition;
        condition._condCode._positiveLogic = true;
        if(Compiler::parseExpression(codeLineIndex, whileWendData._condToken, condition) == Expression::IsInvalid)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::WEND() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, whileWendData._condToken.c_str(), codeLine._text.c_str());
            return false;
        }

        if(emitJumpType(codeLineIndex, "%JumpTrue", whileWendData._whileName, condition._condCode._ccType) < 0)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::WEND() : '%s:%d' : internal error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, whileWendData._condToken.c_str(), codeLine._text.c_str());
            return false;
        }

        return true;
    }

    bool REPEAT(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(foundPos);
        UNREFERENCED_PARAM(tokenIndex);
        UNREFERENCED_PARAM(codeLineStart);
        UNREFERENCED_PARAM(codeLine);

        Compiler::Label label;
        std::string repeatlabelName = "_repeat_" + Expression::wordToHexString(uint16_t(Compiler::getNextFlowControlUniqueId()));
        int labelIndex = Compiler::createLabel(Compiler::getVasmPC(), repeatlabelName, codeLineIndex, label, true, false, false);
        Compiler::setNextVasmLabelIndex(labelIndex);
        Compiler::getRepeatUntilDataStack().push({repeatlabelName, codeLineIndex});

        return true;
    }

    bool UNTIL(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(tokenIndex);

        // Pop stack for this REPEAT loop
        if(Compiler::getRepeatUntilDataStack().empty())
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::UNTIL() : '%s:%d' : syntax error, missing REPEAT statement : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }
        Compiler::RepeatUntilData repeatUntilData = Compiler::getRepeatUntilDataStack().top();
        Compiler::getRepeatUntilDataStack().pop();

        // Condition
        Expression::Numeric condition;
        std::string conditionToken = codeLine._code.substr(foundPos);
        if(Compiler::parseExpression(codeLineIndex, conditionToken, condition) == Expression::IsInvalid)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::UNTIL() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, conditionToken.c_str(), codeLine._text.c_str());
            return false;
        }

        // Branch if condition false to instruction after REPEAT
        if(emitJumpType(codeLineIndex, "%JumpFalse", repeatUntilData._labelName, condition._condCode._ccType) < 0)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::WEND() : '%s:%d' : internal error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, conditionToken.c_str(), codeLine._text.c_str());
            return false;
        }

        return true;
    }

    bool FOREVER(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(foundPos);
        UNREFERENCED_PARAM(codeLineIndex);

        // Pop stack for this REPEAT loop
        if(Compiler::getRepeatUntilDataStack().empty())
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::FOREVER() : '%s:%d' : syntax error, missing REPEAT statement : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }
        Compiler::RepeatUntilData repeatUntilData = Compiler::getRepeatUntilDataStack().top();
        Compiler::getRepeatUntilDataStack().pop();
        std::string gotoLabel = repeatUntilData._labelName;

        // Check for optimised branch
        bool useBRA = (codeLine._tokens[tokenIndex][0] == '&') ? true : false;

        // Within same page, (validation check on same page branch may fail after outputCode(), user will be warned)
        if(useBRA)
        {
            Compiler::emitVcpuAsm("BRA", repeatUntilData._labelName, false);
        }
        // Long jump
        else
        {
            if(Compiler::getCodeRomType() >= Cpu::ROMv5a)
            {
                Compiler::emitVcpuAsm("CALLI", repeatUntilData._labelName, false);
            }
            else
            {
                Compiler::emitVcpuAsm("LDWI", repeatUntilData._labelName, false);
                Compiler::emitVcpuAsm("CALL", "giga_vAC",      false);
            }
        }

        return true;
    }

    bool AS(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(foundPos);
        UNREFERENCED_PARAM(tokenIndex);
        UNREFERENCED_PARAM(codeLineStart);
        UNREFERENCED_PARAM(codeLineIndex);
        UNREFERENCED_PARAM(codeLine);

        return true;
    }

    bool typeHelper(const std::string& input, Compiler::CodeLine& codeLine, int codeLineStart, std::string& name, uint16_t& address, Compiler::TypeVarType& varType, int& paramNum)
    {
        name = input;
        address = 0x0000;
        Expression::stripWhitespace(name);

        std::string token = input;
        Expression::strToUpper(token);
        size_t dimPos = token.find("DIM ");
        if(dimPos == std::string::npos) return true;

        size_t lbra, rbra;
        token = input.substr(dimPos + 3);
        Expression::stripWhitespace(token);
        if(Expression::findMatchingBrackets(token, 0, lbra, rbra, '(', name, paramNum))
        {
            // Byte
            if(varType == Compiler::Byte  &&  (paramNum >= 1  &&  paramNum <= 3))
            {
                varType = Compiler::TypeVarType(Compiler::ArrayB + paramNum - 1);
            }
            // Word
            if(varType == Compiler::Word  &&  (paramNum >= 1  &&  paramNum <= 3))
            {
                varType = Compiler::TypeVarType(Compiler::ArrayW + paramNum - 1);
            }
            // String
            else if(varType == Compiler::String  &&  paramNum == 1)
            {
                varType = Compiler::ArrayS;
            }
            else
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::TYPE() : '%s:%d' : syntax error, 'TYPE' var array does not have the correct number of dimensions : %s'\n",
                                                        codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
                return false;
            }
        }

        return true;
    }
    bool TYPE(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(tokenIndex);
        UNREFERENCED_PARAM(codeLineIndex);

        std::vector<std::string> tokens = Expression::tokenise(codeLine._code.substr(foundPos), ' ', true);
        if(tokens.size() < 3  ||  tokens[1] != "=")
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::TYPE() : '%s:%d' : syntax error, 'TYPE' requires the following format 'TYPE <NAME> = <varType1>, ... <varTypeN>' : %s\n",
                                                    codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        // Type name
        std::string typeName = tokens[0];
        Expression::stripWhitespace(typeName);
        std::map<std::string, Compiler::TypeData>& typeDatas = Compiler::getTypeDatas();
        if(typeDatas.find(typeName) != typeDatas.end())
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::TYPE() : '%s:%d' : type '%s' already exists : %s\n", codeLine._moduleName.c_str(), codeLineStart, typeName.c_str(), codeLine._text.c_str());
            return false;
        }

        // Variables
        size_t equals = codeLine._code.find_first_of('=');
        std::string vars = codeLine._code.substr(equals + 1);
        tokens = Expression::tokenise(vars, ',', true);
        if(tokens.size() == 0)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::TYPE() : '%s:%d' : missing variable types, 'TYPE' requires the following format 'TYPE <NAME> = <varType1>, ... <varTypeN>' : %s\n",
                                                    codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        // Type data
        Compiler::TypeData typeData;
        for(int i=0; i<int(tokens.size()); i++)
        {
            int paramNum = 0;
            std::string varName;
            uint16_t varAddr = 0x0000;
            Compiler::TypeVarType varType;

            // Byte
            if(tokens[i].find('%') != std::string::npos)
            {
                varType = Compiler::Byte;
                if(!typeHelper(tokens[i], codeLine, codeLineStart, varName, varAddr, varType, paramNum)) return false;
            }
            // String
            else if(tokens[i].find('$') != std::string::npos)
            {
                varType = Compiler::String;
                if(!typeHelper(tokens[i], codeLine, codeLineStart, varName, varAddr, varType, paramNum)) return false;
            }
            // Word
            else
            {
                varType = Compiler::Word;
                if(!typeHelper(tokens[i], codeLine, codeLineStart, varName, varAddr, varType, paramNum)) return false;
            }

            if(typeData._vars.find(varName) != typeData._vars.end())
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::TYPE() : '%s:%d' : var '%s' already exists : %s\n", codeLine._moduleName.c_str(), codeLineStart, tokens[i].c_str(), codeLine._text.c_str());
                return false;
            }

            typeData._vars[varName] = {varAddr, varType};
        }

        typeDatas[typeName] = typeData;

        //// Constant string array
        //std::string token = tokens[0];
        //Expression::strToUpper(token);
        //size_t dimPos = token.find("DIM ");
        //if(dimPos != std::string::npos)
        //{
        //    size_t strPos = token.find("$", dimPos + 3);
        //    if(strPos != std::string::npos)
        //    {
        //        size_t lbra, rbra;
        //        if(Expression::findMatchingBrackets(token, strPos + 1, lbra, rbra))
        //        {
        //            _constDimStrArray = true;
        //            return DIM(codeLine, codeLineIndex, codeLineStart, tokenIndex, foundPos + dimPos + 3, result);
        //        }
        //    }
        //}

        return true;
    }

    bool callHelper(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, std::string& token, uint16_t localVarsAddr)
    {
        UNREFERENCED_PARAM(codeLine);

        // If valid expression
        Expression::Numeric numeric;
        if(!Expression::parse(token, codeLineIndex, numeric))
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::CALL() : '%s:%d' : syntax error in '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, Expression::getExpression(), codeLine._text.c_str());
            return false;
        }

        if(numeric._varType == Expression::Number)
        {
            Compiler::emitVcpuAsmForLiteral(numeric, true);
        }
        else if(numeric._varType == Expression::IntVar16)
        {
            switch(numeric._int16Byte)
            {
                case Expression::Int16Low:  Compiler::emitVcpuAsm("LD",  "_" + Compiler::getIntegerVars()[numeric._index]._name,          false); break;
                case Expression::Int16High: Compiler::emitVcpuAsm("LD",  "_" + Compiler::getIntegerVars()[numeric._index]._name + " + 1", false); break;
                case Expression::Int16Both: Compiler::emitVcpuAsm("LDW", "_" + Compiler::getIntegerVars()[numeric._index]._name,          false); break;

                default: break;
            }
        }
        else
        {
            Compiler::emitVcpuAsm("LDW", Expression::byteToHexString(uint8_t(Compiler::getTempVarStart())), false);
        }

        Compiler::emitVcpuAsm("STW", Expression::byteToHexString(uint8_t(localVarsAddr)), false);

        return true;
    }
    bool CALL(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(tokenIndex);

        std::vector<std::string> callTokens = Expression::tokenise(codeLine._code.substr(foundPos + 1), ',', true);
        if(callTokens.size() == 0)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::CALL() : '%s:%d' : syntax error, 'CALL' requires a 'NAME' and optional parameters, 'CALL <NAME>, <param0, param1, ... paramN>' : %s\n",
                                                    codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        // Proc name
        Expression::stripWhitespace(callTokens[0]);
        std::string procName = callTokens[0];

        // Params
        int numParams = int(callTokens.size()) - 1;
        uint16_t localVarsAddr = LOCAL_VAR_START;
        if(callTokens.size() > 1)
        {
            for(int i=1; i<int(callTokens.size()); i++)
            {
                if(localVarsAddr >= TEMP_VAR_START)
                {
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::CALL() : '%s:%d' : syntax error, maximum number of parameters exceeded : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
                    return false;
                }

                if(!callHelper(codeLine, codeLineIndex, codeLineStart, callTokens[i], localVarsAddr))
                {
                    return false;
                }

                localVarsAddr += 2;
            }
        }

        // Save for later validation
        Compiler::CallData callData = {numParams, codeLineIndex, procName};
        Compiler::getCallDataMap()[procName] = callData;

        if(Compiler::getCodeRomType() < Cpu::ROMv5a)
        {
            Compiler::emitVcpuAsm("LDWI", "_" + procName, false);
            Compiler::emitVcpuAsm("CALL", "giga_vAC", false);
        }
        else
        {
            Compiler::emitVcpuAsm("CALLI", "_" + procName, false);
        }

        return true;
    }

    bool PROC(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(tokenIndex);

        std::vector<std::string> procTokens = Expression::tokenise(codeLine._code.substr(foundPos + 1), ',', true);
        if(procTokens.size() == 0)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::PROC() : '%s:%d' : syntax error, 'PROC' requires a 'NAME' and optional parameters, 'PROC <NAME>, <param0, param1, ... paramN>' : %s\n",
                                                    codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        if(!Compiler::getProcDataStack().empty())
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::PROC() : '%s:%d' : syntax error, 'PROC' can NOT be nested : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        Compiler::ProcData procData;
        uint16_t localVarsAddr = LOCAL_VAR_START;

        // Proc name
        Expression::stripWhitespace(procTokens[0]);
        procData._name = procTokens[0];
        procData._codeLineIndex = codeLineIndex;
        procData._numParams = int(procTokens.size()) - 1;

        // Params
        if(procTokens.size() > 1)
        {
            for(int i=1; i<int(procTokens.size()); i++)
            {
                if(localVarsAddr >= TEMP_VAR_START)
                {
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::PROC() : '%s:%d' : syntax error, maximum number of parameters exceeded : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
                    return false;
                }

                Expression::stripWhitespace(procTokens[i]);
                if(Expression::isVarNameValid(procTokens[i]) == Expression::Invalid)
                {
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::PROC() : '%s:%d' : syntax error, parameter types can only be integer : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
                    return false;
                }

                int localVarIndex = -1;
                std::string localVarName = procData._name + "_" + procTokens[i];
                Compiler::createProcIntVar(localVarName, 0, 0, codeLine, codeLineIndex, false, localVarsAddr, localVarIndex);
                if(localVarIndex == -1)
                {
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::PROC() : '%s:%d' : can't create local integer var '%s'\n", codeLine._moduleName.c_str(), codeLineStart, localVarName.c_str());
                    return false;
                }

                // Accessing variables within a PROC requires a translation from local var name to source var name
                procData._localVarNameMap[procTokens[i]] = localVarName;

                localVarsAddr += 2;
            }
        }

        Compiler::getProcDataStack().push(procData);
        Compiler::getProcDataMap()[procData._name] = procData;

        Compiler::emitVcpuAsm("PUSH", "", false, codeLineIndex, procData._name);

        return true;
    }

    bool ENDPROC(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(foundPos);
        UNREFERENCED_PARAM(tokenIndex);
        UNREFERENCED_PARAM(codeLineIndex);

        if(Compiler::getProcDataStack().empty())
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::ENDPROC() : '%s:%d' : syntax error, missing PROC statement : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        if(Compiler::getProcDataStack().size() != 1)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::ENDPROC() : '%s:%d' : syntax error, 'PROC' can NOT be nested : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        // Pop stack for current PROC
        Compiler::getProcDataStack().pop();

        Compiler::emitVcpuAsm("%Return", "", false);

        return true;
    }

    bool LOCAL(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(foundPos);
        UNREFERENCED_PARAM(tokenIndex);

        std::vector<std::string> localTokens = Expression::tokenise(codeLine._code.substr(foundPos + 1), ',', true);
        if(localTokens.size() < 1)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::LOCAL() : '%s:%d' : syntax error, 'LOCAL' requires at least one '<VAR>' : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        if(Compiler::getProcDataStack().empty())
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::LOCAL() : '%s:%d' : syntax error, 'LOCAL' can only be used within a 'PROC/ENDPROC' pair : %s\n", codeLine._moduleName.c_str(),
                                                                                                                                                                codeLineStart, codeLine._text.c_str());
            return false;
        }

        if(Compiler::getProcDataStack().size() != 1)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::LOCAL() : '%s:%d' : syntax error, 'LOCAL' can NOT be used in nested 'PROC's' : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        Compiler::ProcData procData = Compiler::getProcDataStack().top();
        procData._numLocals = int(localTokens.size());
        uint16_t localVarsAddr = LOCAL_VAR_START + uint16_t(procData._numParams)*2;

        // Local vars
        for(int i=0; i<int(localTokens.size()); i++)
        {
            if(localVarsAddr >= TEMP_VAR_START)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::LOCAL() : '%s:%d' : error, maximum number of local vars exceeded : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
                return false;
            }

            Expression::stripWhitespace(localTokens[i]);
            if(Expression::isVarNameValid(localTokens[i]) == Expression::Invalid)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::LOCAL() : '%s:%d' : syntax error, local var types can only be integer : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
                return false;
            }

            int localVarIndex = -1;
            std::string localVarName = procData._name + "_" + localTokens[i];
            Compiler::createProcIntVar(localVarName, 0, 0, codeLine, codeLineIndex, false, localVarsAddr, localVarIndex);
            if(localVarIndex == -1)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::LOCAL() : '%s:%d' : couldn't create local integer var '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                                       localVarName.c_str(), codeLine._text.c_str());
                return false;
            }

            // Accessing variables within a PROC requires a translation from local var name to source var name
            procData._localVarNameMap[localTokens[i]] = localVarName;

            localVarsAddr += 2;
        }

        Compiler::getProcDataStack().top() = procData;
        Compiler::getProcDataMap()[procData._name] = procData;

        return true;
    }

    bool CONST(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(tokenIndex);

        std::vector<std::string> tokens = Expression::tokenise(codeLine._code.substr(foundPos), '=', true);
        if(tokens.size() != 2)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::CONST() : '%s:%d' : syntax error, use CONST a=50 or CONST a$=\"doggy\" or const dim arr$(2) = \"One\", \"Two\", \"Three\" : %s\n",
                                                    codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        // Constant string array, (only string arrays can be constant, byte and integer arrays are excluded)
        std::string token = tokens[0];
        Expression::strToUpper(token);
        size_t dimPos = token.find("DIM ");
        if(dimPos != std::string::npos)
        {
            size_t strPos = token.find("$", dimPos + 3);
            if(strPos != std::string::npos)
            {
                size_t lbra, rbra;
                if(Expression::findMatchingBrackets(token, strPos + 1, lbra, rbra))
                {
                    _constDimStrArray = true;
                    return DIM(codeLine, codeLineIndex, codeLineStart, tokenIndex, foundPos + dimPos + 3, result);
                }
            }
        }

        // Variable string array
        Expression::stripWhitespace(tokens[0]);
        if(Expression::isVarNameValid(tokens[0]) == Expression::Invalid  &&  Expression::isStrNameValid(tokens[0]) == Expression::Invalid)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::CONST() : '%s:%d' : syntax error, name MUST contain only alphanumerics and '$' : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        // String
        if(Expression::isStrNameValid(tokens[0]) == Expression::Variable)
        {
            // Strip whitespace
            Expression::stripNonStringWhitespace(tokens[1]);
            if(Expression::isStringValid(tokens[1]))
            {
                uint16_t address;
                std::string internalName;

                // Strip quotes
                tokens[1].erase(0, 1);
                tokens[1].erase(tokens[1].size()-1, 1);

                // Don't count escape char '\'
                int escCount = 0;
                int strLength = int(tokens[1].size());
                for(int i=0; i<strLength; i++)
                {
                    if(tokens[1][i] == '\\') escCount++;
                }
                strLength -= escCount;

                Compiler::getOrCreateString(codeLine, codeLineIndex, tokens[1], internalName, address);
                Compiler::getConstants().push_back({uint8_t(strLength), 0, address, tokens[1], tokens[0], internalName, Compiler::ConstStr});
            }
            // String keyword
            else
            {
                size_t lbra, rbra;
                if(!Expression::findMatchingBrackets(tokens[1], 0, lbra, rbra))
                {
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::CONST() : '%s:%d' : syntax error, invalid string or keyword : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
                    return false;
                }

                std::string funcToken = tokens[1].substr(0, lbra);
                std::string paramToken = tokens[1].substr(lbra + 1, rbra - (lbra + 1));
                Expression::strToUpper(funcToken);
                if(Functions::getStringFunctions().find(funcToken) == Functions::getStringFunctions().end())
                {
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::CONST() : '%s:%d' : syntax error, invalid string or keyword : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
                    return false;
                }

                int16_t param;
                if(!Expression::stringToI16(paramToken, param))
                {
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::CONST() : '%s:%d' : syntax error, keyword param must be a constant number : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
                    return false;
                }

                // Create constant string
                int index;
                uint8_t length = 0;
                uint16_t address = 0x0000;
                if(funcToken == "CHR$")      {length = 1; address = Compiler::getOrCreateConstString(Compiler::StrChar, param, index);}
                else if(funcToken == "HEX$") {length = 4; address = Compiler::getOrCreateConstString(Compiler::StrHex,  param, index);}

                // Create constant
                if(address)
                {
                    std::string internalName = Compiler::getStringVars().back()._name;
                    Compiler::getConstants().push_back({length, 0, address, Compiler::getStringVars().back()._text, tokens[0], internalName, Compiler::ConstStr});
                }
            }
        }
        // Integer
        else
        {
            Expression::Numeric numeric(true); // true = allow static init
            if(!Expression::parse(tokens[1], codeLineIndex, numeric))
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::CONST() : '%s:%d' : syntax error in '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, Expression::getExpression(), codeLine._text.c_str());
                return false;
            }
            if(tokens[1].size() == 0  ||  !numeric._isValid  ||  numeric._varType == Expression::TmpVar  ||  numeric._varType == Expression::IntVar16)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::CONST() : '%s:%d' : syntax error, invalid constant expression : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
                return false;
            }

            if(numeric._lazyUpdate)
            {
                // Constant doesn't exist yet, so mark it for a lazy update
                Compiler::getConstants().push_back({2, 0x0000, 0x0000, "", tokens[0], "_" + tokens[0], Compiler::ConstInt16, numeric._lazyUpdate});
            }
            else
            {
                // Add constant that exists
                Compiler::getConstants().push_back({2, int16_t(std::lround(numeric._value)), 0x0000, "", tokens[0], "_" + tokens[0], Compiler::ConstInt16});
            }
        }

        return true;
    }

    bool initDIM(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, std::string& varName, int arrSizeTotal, int16_t& intInit, std::vector<int16_t>& intInits, bool& isInit)
    {
        std::string constName = varName;
        if(Compiler::findConst(constName) >= 0)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::initDIM() : '%s:%d' : const '%s' already exists : %s\n", codeLine._moduleName.c_str(), codeLineStart, varName.c_str(), codeLine._text.c_str());
            return false;
        }
        if(Compiler::findVar(varName) >= 0)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::initDIM() : '%s:%d' : var '%s' already exists : %s\n", codeLine._moduleName.c_str(), codeLineStart, varName.c_str(), codeLine._text.c_str());
            return false;
        }

        // Optional array int init values
        isInit = false;
        size_t equalsPos = codeLine._code.find("=");
        if(equalsPos != std::string::npos)
        {
            std::string initText = codeLine._code.substr(equalsPos + 1);
            Expression::stripWhitespace(initText);
            std::vector<std::string> initTokens = Expression::tokenise(initText, ',', true);
            if(initTokens.size() == 0)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::initDIM() : '%s:%d' : initial value must be a constant, found '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                                               initText.c_str(), codeLine._text.c_str());
                return false;
            }
            else if(initTokens.size() == 1)
            {
                std::string operand;
                Expression::Numeric numeric(true); // true = allow static init
                if(Compiler::parseStaticExpression(codeLineIndex, initTokens[0], operand, numeric) == Compiler::OperandInvalid)
                {
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::initDIM() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, initTokens[0].c_str(), codeLine._text.c_str());
                    return false;
                }
                intInit = int16_t(std::lround(numeric._value));
            }
            else if(int(initTokens.size()) > arrSizeTotal)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::initDIM() : '%s:%d' : too many initialisation values for size of array, found %d for a size of %d : %s\n",
                                                        codeLine._moduleName.c_str(), codeLineStart, int(initTokens.size()), arrSizeTotal, codeLine._text.c_str());
                return false;
            }

            // Multiple initialisation values, (if there are less init values than array size, then array is padded with last init value)
            std::string operand;
            intInits.resize(initTokens.size());
            std::vector<Expression::Numeric> funcParams(initTokens.size(), Expression::Numeric(true)); // true = allow static init
            for(int i=0; i<int(initTokens.size()); i++)
            {
                if(Compiler::parseStaticExpression(codeLineIndex, initTokens[i], operand, funcParams[i]) == Compiler::OperandInvalid)
                {
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::initDIM() : '%s:%d' : bad initialiser %s at index %d : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                                     Expression::getExpression(), i, codeLine._text.c_str());
                    return false;
                }
                intInits[i] = int16_t(std::lround(funcParams[i]._value));
            }
            intInit = intInits.back();

            isInit = true;
        }

        return true;
    }
    bool allocDIM(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, uint16_t& address, Compiler::VarType& varType,
                  std::vector<uint16_t>& arrLut, std::vector<uint16_t>& arrSizes, std::vector<std::vector<uint16_t>>& arrAddrs)
    {
        UNREFERENCED_PARAM(codeLineIndex);

        int intSize = 0;
        Memory::ParityType parityType = Memory::ParityNone;
        switch(varType)
        {
            case Compiler::Var1Arr8:  intSize = 1; parityType = Memory::ParityNone; break;
            case Compiler::Var1Arr16: intSize = 2; parityType = Memory::ParityEven; break;

            default: Cpu::reportError(Cpu::KwdError, stderr, "Keywords::allocDIM() : '%s:%d' : unknown var type : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
                     break;
        }

        // Allocate memory for k * j * i of intSize byte values
        int iSizeBytes = arrSizes[2] * intSize;
        for(int k=0; k<arrSizes[0]; k++)
        {
            for(int j=0; j<arrSizes[1]; j++)
            {
                // TODO: mark within variable if array crosses page boundary
                // 16bit arrays must start on even addresses in case they overflow a page, (ops such as DEEK and DOKE must be word aligned if crossing a page boundary)
                if(!Memory::getFreePageRAM(Memory::FitDescending, iSizeBytes, USER_CODE_START, Compiler::getArraysStart(), arrAddrs[k][j], false, parityType)) // arrays do not need to be contained within pages
                {
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::allocDIM() : '%s:%d' : not enough RAM for int array of size %d : %s\n", codeLine._moduleName.c_str(), codeLineStart, iSizeBytes, codeLine._text.c_str());
                    return false;
                }
            }
        }

        // 1D array
        if(arrSizes[0] == 1  &&  arrSizes[1] == 1)
        {
            address = arrAddrs[0][0];
            varType = (varType == Compiler::Var1Arr8) ? Compiler::Var1Arr8 : Compiler::Var1Arr16;
        }
        // 2D array
        else if(arrSizes[0] == 1)
        {
            // 16bit arrays must start on even addresses in case they overflow a page, (ops such as DEEK and DOKE must be word aligned if crossing a page boundary)
            int jSizeBytes = arrSizes[1] * 2;
            if(!Memory::getFreePageRAM(Memory::FitDescending, jSizeBytes, USER_CODE_START, Compiler::getArraysStart(), address, false, Memory::ParityEven)) // arrays do not need to be contained within pages
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::allocDIM() : '%s:%d' : not enough RAM for int array of size %d : %s\n", codeLine._moduleName.c_str(), codeLineStart, jSizeBytes, codeLine._text.c_str());
                return false;
            }

            varType = (varType == Compiler::Var1Arr8) ? Compiler::Var2Arr8 : Compiler::Var2Arr16;

            // Enable system internal sub intialiser and mark system internal sub to be loaded, (init functions are not needed for ROMv5a and higher as CALLI is able to directly CALL them)
            if(intSize == 1)
            {
                if(Compiler::getCodeRomType() < Cpu::ROMv5a) Compiler::enableSysInitFunc("Init8Array2d");
                Linker::setInternalSubToLoad("convert8Arr2d");
            }
            else if(intSize == 2)
            {
                if(Compiler::getCodeRomType() < Cpu::ROMv5a) Compiler::enableSysInitFunc("Init16Array2d");
                Linker::setInternalSubToLoad("convert16Arr2d");
            }
        }
        // 3D array
        else
        {
            int jSizeBytes = arrSizes[1] * 2;
            for(int k=0; k<arrSizes[0]; k++)
            {
                // 16bit arrays must start on even addresses in case they overflow a page, (ops such as DEEK and DOKE must be word aligned if crossing a page boundary)
                if(!Memory::getFreePageRAM(Memory::FitDescending, jSizeBytes, USER_CODE_START, Compiler::getArraysStart(), arrLut[k], false, Memory::ParityEven)) // arrays do not need to be contained within pages
                {
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::allocDIM() : '%s:%d' : not enough RAM for int array of size %d : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                                               jSizeBytes, codeLine._text.c_str());
                    return false;
                }
            }

            // 16bit arrays must start on even addresses in case they overflow a page, (ops such as DEEK and DOKE must be word aligned if crossing a page boundary)
            int kSizeBytes = arrSizes[2] * 2;
            if(!Memory::getFreePageRAM(Memory::FitDescending, kSizeBytes, USER_CODE_START, Compiler::getArraysStart(), address, false, Memory::ParityEven)) // arrays do not need to be contained within pages
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::allocDIM() : '%s:%d' : not enough RAM for int array of size %d : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                                           kSizeBytes, codeLine._text.c_str());
                return false;
            }

            varType = (varType == Compiler::Var1Arr8) ? Compiler::Var3Arr8 : Compiler::Var3Arr16;

            // Enable system internal sub intialiser and mark system internal sub to be loaded, (init functions are not needed for ROMv5a and higher as CALLI is able to directly CALL them)
            if(intSize == 1)
            {
                if(Compiler::getCodeRomType() < Cpu::ROMv5a) Compiler::enableSysInitFunc("Init8Array3d");
                Linker::setInternalSubToLoad("convert8Arr3d");
            }
            else if(intSize == 2)
            {
                if(Compiler::getCodeRomType() < Cpu::ROMv5a) Compiler::enableSysInitFunc("Init16Array3d");
                Linker::setInternalSubToLoad("convert16Arr3d");
            }
        }

        return true;
    }
    bool DIM(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(tokenIndex);

        size_t lbra, rbra;
        if(!Expression::findMatchingBrackets(codeLine._code, foundPos, lbra, rbra))
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::DIM() : '%s:%d' : syntax error in DIM statement, must be DIM <var>(<n1>, <optional n2>) : %s\n", codeLine._moduleName.c_str(),
                                                                                                                                                                codeLineStart, codeLine._text.c_str());
            return false;
        }

        // Dimensions
        std::vector<uint16_t> arrSizes;
        std::vector<std::string> sizeTokens = Expression::tokenise(codeLine._code.substr(lbra + 1, rbra - (lbra + 1)), ',', true);
        if(sizeTokens.size() > MAX_ARRAY_DIMS)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::DIM() : '%s:%d' : maximum of %d dimensions, found %d : %s\n", codeLine._moduleName.c_str(), codeLineStart, MAX_ARRAY_DIMS,
                                                                                                                             int(sizeTokens.size()), codeLine._text.c_str());
            return false;
        }

        int arrSizeTotal = 1;
        for(int i=0; i<int(sizeTokens.size()); i++)
        {
            int varIndex = -1, constIndex = -1, strIndex = -1;
            std::string sizeToken = sizeTokens[i];
            Expression::stripWhitespace(sizeToken);
            uint32_t expressionType = Compiler::isExpression(sizeToken, varIndex, constIndex, strIndex);

            // Constant dimension
            if((expressionType & Expression::HasIntConsts)  &&  constIndex > -1)
            {
                std::string operand;
                Expression::Numeric numeric(true); // true = allow static init
                if(Compiler::parseStaticExpression(codeLineIndex, sizeToken, operand, numeric) == Compiler::OperandInvalid)
                {
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::DIM() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, sizeToken.c_str(), codeLine._text.c_str());
                    return false;
                }
                int16_t data = int16_t(std::lround(numeric._value));
                arrSizes.push_back(data);
            }
            // Literal dimension
            else
            {
                uint16_t arrSize = 0;
                if(!Expression::stringToU16(sizeToken, arrSize)  ||  arrSize <= 0)
                {
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::DIM() : '%s:%d' : array dimensions must be a positive constant, found '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                                                           sizeToken.c_str(), codeLine._text.c_str());
                    return false;
                }
                arrSizes.push_back(arrSize);
            }

            // Most BASIC's declared 0 to n elements, hence size = n + 1
            if(!Compiler::getArrayIndiciesOne())
            {
                arrSizes.back()++;
            }

            arrSizeTotal *= arrSizes.back();
        }

        std::string varName = codeLine._code.substr(foundPos, lbra - foundPos);
        Expression::stripWhitespace(varName);

        int16_t intInit = 0;
        bool isIntInit = false;
        std::vector<int16_t> intInits;
        std::vector<std::string> strInits;
        Compiler::VarType varType = Compiler::Var1Arr16;

        // Str array
        bool isStrInit = false;
        if(Expression::isStrNameValid(varName))
        {
            if(Compiler::findStr(varName) >= 0)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::DIM() : '%s:%d' : str %s already exists : %s\n", codeLine._moduleName.c_str(), codeLineStart, varName.c_str(), codeLine._text.c_str());
                return false;
            }
            if(arrSizes.size() != 1)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::DIM() : '%s:%d' : require 1 string dimension, found %d : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                                   int(arrSizes.size()), codeLine._text.c_str());
                return false;
            }

            // String array 2nd dimension is always USER_STR_SIZE, (actual arrSizes data structure looks like this, [n][USER_STR_SIZE])
            arrSizes.push_back(USER_STR_SIZE);

            // Optional array str init values
            size_t equalsPos = codeLine._code.find("=");
            if(equalsPos != std::string::npos)
            {
                std::string initText = codeLine._code.substr(equalsPos + 1);
                Expression::stripNonStringWhitespace(initText);
                std::vector<std::string> initTokens = Expression::tokenise(initText, ',', true);
                if(initTokens.size() == 0)
                {
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::DIM() : '%s:%d' : initial value must be a string, found '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                                             initText.c_str(), codeLine._text.c_str());
                    return false;
                }
                else if(int(initTokens.size()) > arrSizes[0])
                {
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::DIM() : '%s:%d' : too many initialisation strings for size of array, found %d for a size of %d : %s\n",
                                                            codeLine._moduleName.c_str(), codeLineStart, int(initTokens.size()), arrSizes[0], codeLine._text.c_str());
                    return false;
                }

                // Multiple initialisation values, (if there are less strings than array size, then array is padded with last string)
                for(int i=0; i<int(initTokens.size()); i++)
                {
                    if(initTokens[i].size() - 2 > USER_STR_SIZE)
                    {
                        Cpu::reportError(Cpu::KwdError, stderr, "Keywords::DIM() : '%s:%d' : initialisation string '%s' is larger than %d chars : %s\n",
                                                                codeLine._moduleName.c_str(), codeLineStart, initTokens[i].c_str(), USER_STR_SIZE, codeLine._text.c_str());
                        return false;
                    }

                    if(!Expression::isStringValid(initTokens[i]))
                    {
                        Cpu::reportError(Cpu::KwdError, stderr, "Keywords::DIM() : '%s:%d' : invalid string initialiser, found '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                                             initTokens[i].c_str(), codeLine._text.c_str());
                        return false;
                    }

                    // Strip quotes
                    initTokens[i].erase(0, 1);
                    initTokens[i].erase(initTokens[i].size() - 1, 1);
                    strInits.push_back(initTokens[i]);
                }

                isStrInit = true;
            }

            varType = Compiler::VarStr2;
        }
        // Int8 array
        else if(varName.back() == '%')
        {
            // % is only used within DIM keyword
            varName.erase(varName.size()-1, 1);
            if(!initDIM(codeLine, codeLineIndex, codeLineStart, varName, arrSizeTotal, intInit, intInits, isIntInit)) return false;
            varType = Compiler::Var1Arr8;
        }
        // Int16 array
        else
        {
            if(!initDIM(codeLine, codeLineIndex, codeLineStart, varName, arrSizeTotal, intInit, intInits, isIntInit)) return false;
            varType = Compiler::Var1Arr16;
        }

        // Represent 1 or 2 dimensional arrays as a 3 dimensional array with dimensions of 1 for the missing dimensions
        while(arrSizes.size() != 3)
        {
            arrSizes.insert(arrSizes.begin(), 1);
        }

        // Array lut
        std::vector<uint16_t> arrLut;
        arrLut.resize(arrSizes[0]);

        // Array addresses
        std::vector<std::vector<uint16_t>> arrAddrs;
        arrAddrs.resize(arrSizes[0]);
        for(int i=0; i<arrSizes[0]; i++)
        {
            arrAddrs[i].resize(arrSizes[1]);
        }

        uint16_t address = 0x0000;

        switch(varType)
        {
            // Str arrays
            case Compiler::VarStr2:
            {
                std::vector<uint16_t> strAddrs;
                strAddrs.resize(arrSizes[1]);
                if(_constDimStrArray)
                {
                    // Constant array of strings
                    _constDimStrArray = false;
                    if(Compiler::createStringArray(codeLine, codeLineIndex, varName, 0, isStrInit, strInits, strAddrs) == -1) return false;
                }
                else
                {
                    // Variable array of strings
                    if(Compiler::createStringArray(codeLine, codeLineIndex, varName, USER_STR_SIZE, isStrInit, strInits, strAddrs) == -1) return false;
                }
            }
            break;

            // Int8 arrays, (allocDIM returns var type based on arrSizes)
            case Compiler::Var1Arr8:
            {
                int intIndex = -1;
                if(!allocDIM(codeLine, codeLineIndex, codeLineStart, address, varType, arrLut, arrSizes, arrAddrs)) return false;
                Compiler::createArrIntVar(varName, 0, intInit, codeLine, codeLineIndex, false, isIntInit, intIndex, varType, Compiler::Int8, address, arrSizes, intInits, arrAddrs, arrLut);
            }
            break;

            // Int16 arrays, (allocDIM returns var type based on arrSizes)
            case Compiler::Var1Arr16:
            {
                int intIndex = -1;
                if(!allocDIM(codeLine, codeLineIndex, codeLineStart, address, varType, arrLut, arrSizes, arrAddrs)) return false;
                Compiler::createArrIntVar(varName, 0, intInit, codeLine, codeLineIndex, false, isIntInit, intIndex, varType, Compiler::Int16, address, arrSizes, intInits, arrAddrs, arrLut);
            }
            break;

            default: Cpu::reportError(Cpu::KwdError, stderr, "Keywords::DIM() : '%s:%d' : unknown array type %d : %s\n", codeLine._moduleName.c_str(), codeLineStart, varType, codeLine._text.c_str());
                     return false;
        }

        return true;
    }

    // Not used, implemented as a function, (Compiler::userFunc())
    bool FUNC(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(foundPos);
        UNREFERENCED_PARAM(tokenIndex);

        size_t lbra, rbra;
        std::string fnText = codeLine._expression;
        Expression::strToUpper(fnText);
        size_t fnPos = fnText.find("FUNC");
        std::string funcText = codeLine._expression.substr(fnPos);
        if(!Expression::findMatchingBrackets(funcText, 0, lbra, rbra))
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::FUNC() : '%s:%d' : syntax error, invalid parenthesis in FN : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }
        funcText = funcText.substr(0, rbra + 1);

        // Name
        std::string name = funcText.substr(sizeof("FUNC")-1, lbra - (sizeof("FUNC")-1));
        Expression::stripWhitespace(name);
        if(Compiler::getDefFunctions().find(name) == Compiler::getDefFunctions().end())
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::FUNC() : '%s:%d' : syntax error, FN %s can't be found : %s\n", codeLine._moduleName.c_str(), codeLineStart, name.c_str(), codeLine._text.c_str());
            return false;
        }
        int varIndex = Compiler::findVar(name);
        if(varIndex >= 0)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::FUNC() : '%s:%d' : syntax error, name collision with var %s : %s\n", codeLine._moduleName.c_str(), codeLineStart, name.c_str(), codeLine._text.c_str());
            return false;
        }

        // Params
        std::vector<std::string> params = Expression::tokenise(funcText.substr(lbra + 1, rbra - (lbra + 1)), ',', true);
        if(params.size() == 0)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::FUNC() : '%s:%d' : syntax error, need at least one parameter : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }
        int paramsSize = int(Compiler::getDefFunctions()[name]._params.size());
        if(paramsSize != int(params.size()))
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::FUNC() : '%s:%d' : syntax error, wrong number of parameters, expecting %d : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                                                  paramsSize, codeLine._text.c_str());
            return false;
        }
        std::string func = Compiler::getDefFunctions()[name]._function;
        for(int i=0; i<int(params.size()); i++)
        {
            Expression::stripWhitespace(params[i]);
            Expression::replaceText(func, Compiler::getDefFunctions()[name]._params[i], params[i]);
        }

        // Replace DEF FN with FUNC
        Expression::replaceText(codeLine._code,       funcText, func);
        Expression::replaceText(codeLine._text,       funcText, func);
        Expression::replaceText(codeLine._expression, funcText, func);
        Expression::replaceText(Compiler::getCodeLines()[codeLineIndex]._code,       funcText, func);
        Expression::replaceText(Compiler::getCodeLines()[codeLineIndex]._text,       funcText, func);
        Expression::replaceText(Compiler::getCodeLines()[codeLineIndex]._expression, funcText, func);
        std::vector<size_t> offsets;
        std::vector<std::string> tokens = Expression::tokeniseLineOffsets(Compiler::getCodeLines()[codeLineIndex]._code, " (),=", offsets);
        codeLine._tokens = tokens;
        codeLine._offsets = offsets;
        Compiler::getCodeLines()[codeLineIndex]._tokens = tokens;
        Compiler::getCodeLines()[codeLineIndex]._offsets = offsets;

        return true;
    }

    bool createDEFFN(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, std::string& defFunc)
    {
        UNREFERENCED_PARAM(codeLineIndex);

        size_t lbra, rbra;
        if(!Expression::findMatchingBrackets(defFunc, 0, lbra, rbra))
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::functionDEF() : '%s:%d' : syntax error, invalid parenthesis : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        // Name
        if(lbra == 0)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::functionDEF() : '%s:%d' : syntax error, missing name : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }
        std::string name = defFunc.substr(0, lbra);
        Expression::stripWhitespace(name);
        Expression::strToUpper(name);
        int varIndex = Compiler::findVar(name);
        if(varIndex >= 0)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::functionDEF() : '%s:%d' : syntax error, name collision with var %s : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                                           name.c_str(), codeLine._text.c_str());
            return false;
        }

        // Function
        size_t equalsPos = defFunc.find("=", rbra + 1);
        if(equalsPos == std::string::npos)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::functionDEF() : '%s:%d' : syntax error, missing equals sign : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }
        std::string function = defFunc.substr(equalsPos + 1);
        Expression::trimWhitespace(function);

        // Params
        std::vector<std::string> params = Expression::tokenise(defFunc.substr(lbra + 1, rbra - (lbra + 1)), ',', true);
        if(params.size() == 0)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::functionDEF() : '%s:%d' : syntax error, need at least one parameter : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }
        for(int i=0; i<int(params.size()); i++)
        {
            Expression::stripWhitespace(params[i]);
            if(function.find(params[i]) == std::string::npos)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::functionDEF() : '%s:%d' : syntax error, parameter %s missing from function %s : %s\n",
                                                        codeLine._moduleName.c_str(), codeLineStart, params[i].c_str(), function.c_str(), codeLine._text.c_str());
                return false;
            }
        }


        Compiler::DefFunction defFunction = {name, function, params};
        if(Compiler::getDefFunctions().find(name) != Compiler::getDefFunctions().end())
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::functionDEF() : '%s:%d' : syntax error, DEF FN %s has been defined more than once : %s\n",
                                                    codeLine._moduleName.c_str(), codeLineStart, name.c_str(), codeLine._text.c_str());
            return false;
        }
        Compiler::getDefFunctions()[name] = defFunction;

        return true;
    }
    bool DEF(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(tokenIndex);

        std::string defText = codeLine._code.substr(foundPos);
        std::string fnText = defText;
        Expression::strToUpper(fnText);

        // FUNC
        size_t defPos = std::string::npos;
        if((defPos = fnText.find("FN")) != std::string::npos)
        {
            std::string defFunc = defText.substr(defPos + sizeof("FN") - 1);
            return createDEFFN(codeLine, codeLineIndex, codeLineStart, defFunc);
        }

        // Equals
        size_t equalsPos = codeLine._code.find("=");
        if(equalsPos == std::string::npos)
        {
            // Integer var definition list
            std::vector<std::string> varTokens = Expression::tokenise(defText, ',', true);
            if(varTokens.size())
            {
                std::map<std::string, int> varMap;
                for(int i=0; i<int(varTokens.size()); i++)
                {
                    // Check for valid var name
                    Expression::stripWhitespace(varTokens[i]);
                    if(Expression::isVarNameValid(varTokens[i]) == Expression::Invalid)
                    {
                        Cpu::reportError(Cpu::KwdError, stderr, "Keywords::DEF() : '%s:%d' : syntax error in variable definition '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                                               varTokens[i].c_str(), codeLine._text.c_str());
                        return false;
                    }

                    // Check for duplicate vars
                    if(varMap.find(varTokens[i]) != varMap.end())
                    {
                        Cpu::reportError(Cpu::KwdError, stderr, "Keywords::DEF() : '%s:%d' : duplicate variable definition '%s : %s'\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                                         varTokens[i].c_str(), codeLine._text.c_str());
                        return false;
                    }

                    // Create var, (no vASM code, i.e. uninitialised)
                    varMap[varTokens[i]] = i;
                    int varIndex = -1;
                    if(!Compiler::createIntVar(varTokens[i], 0, 0, codeLine, codeLineIndex, false, varIndex)) return false;
                }

                return true;
            }

            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::DEF() : '%s:%d' : syntax error, missing equals sign : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        // Address field
        size_t typePos, lbra, rbra;
        uint16_t address = 0;
        bool foundAddress = false;
        bool foundLutGenerator = false;
        std::string addrText, operand;
        std::vector<std::string> addrTokens;
        Expression::Numeric addrNumeric(true);  // true = allow static init
        if(Expression::findMatchingBrackets(codeLine._code, foundPos, lbra, rbra))
        {
            // Check for LUT generator
            addrText = codeLine._code.substr(lbra + 1, rbra - (lbra + 1));
            addrTokens = Expression::tokenise(addrText, ',', true);
            if(addrTokens.size() >= 4)
            {
                foundLutGenerator = true;
            }

            // Parse address field
            if(addrTokens.size() == 0)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::DEF() : '%s:%d' : address field does not exist, found %s : %s\n", codeLine._moduleName.c_str(), codeLineStart, addrText.c_str(), codeLine._text.c_str());
                return false;
            }
            if(Compiler::parseStaticExpression(codeLineIndex, addrTokens[0], operand, addrNumeric) == Compiler::OperandInvalid)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::DEF() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, addrTokens[0].c_str(), codeLine._text.c_str());
                return false;
            }
            address = uint16_t(std::lround(addrNumeric._value));
            typePos = lbra;
            foundAddress = true;
        }
        else
        {
            typePos = equalsPos;
        }

        // Type field
        std::string typeText = codeLine._code.substr(foundPos, typePos - foundPos);
        Expression::stripWhitespace(typeText);
        Expression::strToUpper(typeText);
        if(typeText != "BYTE"  &&  typeText != "WORD")
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::DEF() : '%s:%d' : type field must be either BYTE or WORD, found %s : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                                           typeText.c_str(), codeLine._text.c_str());
            return false;
        }

        // Address offset field
        uint16_t addrOffset = 0;
        if(addrTokens.size() == 2  &&  !foundLutGenerator)
        {
            Expression::Numeric offsetNumeric(true);  // true = allow static init
            if(Compiler::parseStaticExpression(codeLineIndex, addrTokens[1], operand, offsetNumeric) == Compiler::OperandInvalid)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::DEF() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, addrTokens[1].c_str(), codeLine._text.c_str());
                return false;
            }
            addrOffset = uint16_t(std::lround(offsetNumeric._value));
        }
        
        // ************************************************************************************************************
        // LUT generator
        if(foundLutGenerator)
        {
            if(addrTokens.size() < 4  ||  addrTokens.size() > 6)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::DEF() : '%s:%d' : LUT generator must have 4 to 6 parameters, '(ADDR, <VAR>, START, STOP, SIZE, <OFFSET>)', (<VAR> and <OFFSET> are optional), found %d : %s\n",
                                                        codeLine._moduleName.c_str(), codeLineStart, int(addrTokens.size()), codeLine._text.c_str());
                return false;
            }

            for(int i=0; i<int(addrTokens.size()); i++) Expression::stripWhitespace(addrTokens[i]);

            std::string lutGenerator = codeLine._code.substr(equalsPos + 1);
            Expression::stripWhitespace(lutGenerator);
            if(lutGenerator.size() == 0)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::DEF() : '%s:%d' : LUT generator '%s' is invalid : %s\n", codeLine._moduleName.c_str(), codeLineStart, lutGenerator.c_str(), codeLine._text.c_str());
                return false;
            }

            // Parse LUT generator variable
            bool foundVar = false;
            std::string lutGenVar;
            size_t varPos = 0;
            std::vector<size_t> varPositions;
            if(addrTokens.size() >= 5)
            {
                lutGenVar = addrTokens[1];
                if(Expression::isVarNameValid(lutGenVar) == Expression::Variable)
                {
                    foundVar = true;
                    bool foundVarFirstTime = false;

                    for(;;)
                    {
                        varPos = lutGenerator.find(lutGenVar, varPos);
                        if(varPos == std::string::npos)
                        {
                            if(!foundVarFirstTime)
                            {
                                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::DEF() : '%s:%d' : LUT generator variable '%s' invalid : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                                                  lutGenVar.c_str(), codeLine._text.c_str());
                                return false;
                            }

                            // Found all occurenced of LUT generator variable
                            break;
                        }
                        lutGenerator.erase(varPos, lutGenVar.size());
                        varPositions.push_back(varPos);
                        foundVarFirstTime = true;
                        varPos++;
                    }
                }
            }

            // Parse LUT generator parameters
            int paramsOffset = (foundVar) ? 2 : 1;
            std::vector<Expression::Numeric> lutGenParams = {Expression::Numeric(true), Expression::Numeric(true), Expression::Numeric(true)}; // true = allow static init
            for(int i=0; i<int(lutGenParams.size()); i++)
            {
                if(Compiler::parseStaticExpression(codeLineIndex, addrTokens[i + paramsOffset], operand, lutGenParams[i]) == Compiler::OperandInvalid)
                {
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::DEF() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                     addrTokens[i + paramsOffset].c_str(), codeLine._text.c_str());
                    return false;
                }
            }
            if(lutGenParams[2]._value <= 0.0)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::DEF() : '%s:%d' : LUT size must be greater than zero, found '%d' : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                                             int(lutGenParams[2]._value), codeLine._text.c_str());
                return false;
            }

            // Parse LUT generator address offset, if it exists
            if(addrTokens.size() == 6  ||  (addrTokens.size() == 5  &&  !foundVar))
            {
                Expression::Numeric offsetNumeric;
                if(Compiler::parseStaticExpression(codeLineIndex, addrTokens[3 + paramsOffset], operand, offsetNumeric) == Compiler::OperandInvalid)
                {
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::DEF() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                     addrTokens[3 + paramsOffset].c_str(), codeLine._text.c_str());
                    return false;
                }
                addrOffset = uint16_t(std::lround(offsetNumeric._value));
            }

            // Evaluate LUT generator
            double start = lutGenParams[0]._value;
            double end = lutGenParams[1]._value;
            double count = fabs(lutGenParams[2]._value);
            double step = (end - start) / count;
            std::vector<int16_t> lutGenData;
            for(double d=start; d<end; d+=step)
            {
                std::string var;
                if(foundVar)
                {
                    // Insert substitute values into var's positions
                    var = std::to_string(d);
                    for(int i=0; i<int(varPositions.size()); i++)
                    {
                        lutGenerator.insert(varPositions[i] + i*var.size(), var);
                    }
                }

                Expression::Numeric lutGenResult = Expression::Numeric(true); // true = allow static init
                if(Compiler::parseStaticExpression(codeLineIndex, lutGenerator, operand, lutGenResult) == Compiler::OperandInvalid)
                {
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::DEF() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, lutGenerator.c_str(), codeLine._text.c_str());
                    return false;
                }
                lutGenData.push_back(int16_t(std::lround(lutGenResult._value)));

                if(foundVar)
                {
                    // Erase every occurence of substitute values
                    for(int i=0; i<int(varPositions.size()); i++)
                    {
                        lutGenerator.erase(varPositions[i], var.size());
                    }
                }
            }

            // Allows for byte allocating and setting of interlaced memory, (audio memory is not allocated, but can still be set)
            if(typeText == "BYTE")
            {
                uint16_t addr = address;
                std::vector<uint8_t> dataBytes(int(count), 0);
                for(int i=0; i<int(dataBytes.size()); i++)
                {
                    dataBytes[i] = uint8_t(lutGenData[i]);
                    if(addrOffset != 0)
                    {
                        if(addr < RAM_AUDIO_START  ||  addr > RAM_AUDIO_END)
                        {
                            if(!Memory::takeFreeRAM(addr, 1, false))
                            {
                                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::DEF() : '%s:%d' : memory error, byte chunk allocation at '0x%04x' of size '%d' failed : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                                                                                  addr, 1, codeLine._text.c_str());
                                return false;
                            }
                            addr += addrOffset;
                        }
                    }
                }
                if(addrOffset == 0)
                {
                    // If RAM is not free, still allow it to be initialised
                    if(Memory::isFreeRAM(address, int(dataBytes.size())))
                    {
                        if(!Memory::takeFreeRAM(address, int(dataBytes.size()), false))
                        {
                            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::DEF() : '%s:%d' : memory error, byte chunk allocation at '0x%04x' of size '%d' failed : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                                                                              address, int(dataBytes.size()), codeLine._text.c_str());
                            return false;
                        }
                    }
                }
                Compiler::getDefDataBytes().push_back({address, addrOffset, dataBytes});
            }
            // Allows for word allocating and setting of interlaced memory, (audio memory is not allocated, but can still be set)
            else if(typeText == "WORD")
            {
                uint16_t addr = address;
                std::vector<int16_t> dataWords(int(count), 0);
                for(int i=0; i<int(dataWords.size()); i++)
                {
                    dataWords[i] = int16_t(lutGenData[i]);
                    if(addrOffset != 0)
                    {
                        if(addr < RAM_AUDIO_START  ||  addr > RAM_AUDIO_END)
                        {
                            if(!Memory::takeFreeRAM(addr, 2, false))
                            {
                                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::DEF() : '%s:%d' : memory error, byte chunk allocation at '0x%04x' of size '%d' failed : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                                                                                  addr, 2, codeLine._text.c_str());
                                return false;
                            }
                            addr += addrOffset * 2;
                        }
                    }
                }
                if(addrOffset == 0)
                {
                    // If RAM is not free, still allow it to be initialised
                    if(Memory::isFreeRAM(address, int(dataWords.size()) * 2))
                    {
                        if(!Memory::takeFreeRAM(address, int(dataWords.size()) * 2, false))
                        {
                            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::DEF() : '%s:%d' : memory error, byte chunk allocation at '0x%04x' of size '%d' failed : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                                                                              address, int(dataWords.size()), codeLine._text.c_str());
                            return false;
                        }
                    }
                }
                Compiler::getDefDataWords().push_back({address, addrOffset, dataWords});
            }

            return true;
        }

        // ************************************************************************************************************
        // Data fields
        std::vector<std::string> dataTokens = Expression::tokenise(codeLine._code.substr(equalsPos + 1), ',', true);
        if(dataTokens.size() == 0)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::DEF() : '%s:%d' : syntax error, require at least one data field : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        // BYTE data
        if(typeText == "BYTE")
        {
            // Parse byte fields
            std::vector<uint8_t> dataBytes;
            for(int i=0; i<int(dataTokens.size()); i++)
            {
                Expression::Numeric numeric(true); // true = allow static init
                if(Compiler::parseStaticExpression(codeLineIndex, dataTokens[i], operand, numeric) == Compiler::OperandInvalid)
                {
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::DEF() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, dataTokens[i].c_str(), codeLine._text.c_str());
                    return false;
                }
                dataBytes.push_back(uint8_t(std::lround(numeric._value)));
            }

            // If no new address is found, then it is recalculated
            if(!foundAddress)
            {
                addrOffset = Compiler::getDefDataBytes().back()._offset;
                uint16_t offset = (addrOffset != 0) ? uint16_t(Compiler::getDefDataBytes().back()._data.size()) * addrOffset : uint16_t(Compiler::getDefDataBytes().back()._data.size());
                address = Compiler::getDefDataBytes().back()._address + offset;
            }

            // No address offset so take memory as one chunk
            if(addrOffset == 0)
            {
                if(address >= DEFAULT_EXEC_ADDRESS  &&  !Memory::takeFreeRAM(address, int(dataBytes.size()), false))
                {
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::DEF() : '%s:%d' : memory error, byte chunk allocation at '0x%04x' of size '%d' failed : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                                                                      address, int(dataBytes.size()), codeLine._text.c_str());
                    return false;
                }
            }
            // Valid address offset, so take memory as individual bytes
            else
            {
                uint16_t addr = address;
                for(int i=0; i<int(dataBytes.size()); i++)
                {
                    if(addr < RAM_AUDIO_START  ||  addr > RAM_AUDIO_END)
                    {
                        if(!Memory::takeFreeRAM(addr, 1, false))
                        {
                            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::DEF() : '%s:%d' : memory error, byte allocation at '0x%04x of size '1' failed : %s\n", codeLine._moduleName.c_str(),
                                                                                                                                                                      codeLineStart, address, codeLine._text.c_str());
                            return false;
                        }
                        addr += addrOffset;
                    }
                }
            }
            Compiler::getDefDataBytes().push_back({address, addrOffset, dataBytes});
        }
        // WORD data
        else if(typeText == "WORD")
        {
            // Parse word fields
            std::vector<int16_t> dataWords;
            for(int i=0; i<int(dataTokens.size()); i++)
            {
                Expression::Numeric numeric(true); // true = allow static init
                if(Compiler::parseStaticExpression(codeLineIndex, dataTokens[i], operand, numeric) == Compiler::OperandInvalid)
                {
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::DEF() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, dataTokens[i].c_str(), codeLine._text.c_str());
                    return false;
                }
                dataWords.push_back(int16_t(std::lround(numeric._value)));
            }

            // If no new address is found, then it is recalculated
            if(!foundAddress)
            {
                addrOffset = Compiler::getDefDataWords().back()._offset;
                uint16_t offset = (addrOffset != 0) ? uint16_t(Compiler::getDefDataWords().back()._data.size()) * addrOffset * 2 : uint16_t(Compiler::getDefDataWords().back()._data.size()) * 2;
                address = Compiler::getDefDataWords().back()._address + offset;
            }

            // No address offset so take memory as one chunk
            if(addrOffset == 0)
            {
                if(address >= DEFAULT_EXEC_ADDRESS  &&  !Memory::takeFreeRAM(address, int(dataWords.size()) * 2, false))
                {
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::DEF() : '%s:%d' : memory error, word chunk allocation at '0x%04x' of size '%d' failed : %s\n",
                                                            codeLine._moduleName.c_str(), codeLineStart, address, int(dataWords.size()) * 2, codeLine._text.c_str());
                    return false;
                }
            }
            // Valid address offset, so take memory as individual words
            else
            {
                uint16_t addr = address;
                for(int i=0; i<int(dataWords.size()); i++)
                {
                    if(addr < RAM_AUDIO_START  ||  addr > RAM_AUDIO_END)
                    {
                        if(!Memory::takeFreeRAM(addr, 2, false))
                        {
                            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::DEF() : '%s:%d' : memory error, word allocation at '0x%04x of size '2' failed : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                                                                      address, codeLine._text.c_str());
                            return false;
                        }
                        addr += addrOffset * 2;
                    }
                }
            }
            Compiler::getDefDataWords().push_back({address, addrOffset, dataWords});
        }

        return true;
    }

    bool DATA(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(tokenIndex);

        // Data fields
        std::vector<std::string> dataTokens = Expression::tokenise(codeLine._code.substr(foundPos + 1), ',', true);
        if(dataTokens.size() == 0)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::DATA() : '%s:%d' : syntax error, require at least one data field : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        // Parse data
        std::string operand;
        for(int i=0; i<int(dataTokens.size()); i++)
        {
            std::string dataToken = dataTokens[i];
            Expression::stripNonStringWhitespace(dataToken);
            if(Expression::isStringValid(dataToken))
            {
                // Strip quotes
                dataToken.erase(0, 1);
                dataToken.erase(dataToken.size()-1, 1);

                // Add str to list
                std::unique_ptr<Compiler::DataObject> pObject = std::make_unique<Compiler::DataStr>(dataToken);
                Compiler::getDataObjects().push_back(std::move(pObject));
            }
            else
            {
                // Parse and add int to list, (ints can be constants, complex expressions or functions that return statics, hence the parsing)
                Expression::Numeric numeric(true); // true = allow static init
                if(Compiler::parseStaticExpression(codeLineIndex, dataTokens[i], operand, numeric) == Compiler::OperandInvalid)
                {
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::DATA() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, dataTokens[i].c_str(), codeLine._text.c_str());
                    return false;
                }
                int16_t data = int16_t(std::lround(numeric._value));
                std::unique_ptr<Compiler::DataObject> pObject = std::make_unique<Compiler::DataInt>(data);
                Compiler::getDataObjects().push_back(std::move(pObject));
            }
        }

        return true;
    }

    bool READ(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(tokenIndex);

        std::vector<std::string> tokens = Expression::tokenise(codeLine._code.substr(foundPos), ',', false);
        if(tokens.size() < 1)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::READ() : '%s:%d' : syntax error, use 'READ <var0, var1, var2...varN>', requires at least one variable : %s\n", codeLine._moduleName.c_str(),
                                                                                                                                                                              codeLineStart, codeLine._text.c_str());
            return false;
        }

        // Search for variables
        for(int i=0; i<int(tokens.size()); i++)
        {
            std::string varToken = tokens[i];
            Expression::stripWhitespace(varToken);

            int varIndex = Compiler::findVar(varToken);
            int strIndex = Compiler::findStr(varToken);

            size_t lbra, rbra;
            std::string arrText;
            Compiler::VarType varType = Compiler::VarInt16;
            if(varToken.find("$") != std::string::npos) varType = Compiler::VarStr;
            if(Expression::findMatchingBrackets(tokens[i], 0, lbra, rbra))
            {
                arrText = tokens[i].substr(lbra + 1, rbra - (lbra + 1));

                switch(varType)
                {
                    case Compiler::VarInt16: varType = Compiler::Var1Arr16; break;
                    case Compiler::VarStr:   varType = Compiler::VarStr2;   break;

                    default:
                    {
                        Cpu::reportError(Cpu::KwdError, stderr, "Keywords::READ() : '%s:%d' : syntax error, only single dimensional arrays are supported in '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                                                                          varToken.c_str(), codeLine._text.c_str());
                        return false;
                    }
                    break;
                }
            }

            // Int array must have already been dimensioned
            if(varIndex == -1  &&  varType == Compiler::Var1Arr16)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::READ() : '%s:%d' : integer array is not dimensioned in '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                                        varToken.c_str(), codeLine._text.c_str());
                return false;
            }

            // Str array must have already been dimensioned
            if(strIndex == -1  &&  varType == Compiler::VarStr2)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::READ() : '%s:%d' : string array is not dimensioned in '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                                       varToken.c_str(), codeLine._text.c_str());
                return false;
            }

            // Create int var
            if(varIndex == -1  &&  varType == Compiler::VarInt16)
            {
                if(!Compiler::createIntVar(varToken, 0, 0, codeLine, codeLineIndex, false, varIndex))
                {
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::READ() : '%s:%d' : couldn't create integer var '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                                    varToken.c_str(), codeLine._text.c_str());
                    return false;
                }
            }
            // Create str var
            else if(strIndex == -1  &&  varType == Compiler::VarStr)
            {
                uint16_t address;
                strIndex = getOrCreateString(codeLine, codeLineIndex, "", varToken, address, USER_STR_SIZE, false);
                if(strIndex == -1)
                {
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::READ() : '%s:%d' : couldn't create string var '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                                   varToken.c_str(), codeLine._text.c_str());
                    return false;
                }
            }

            // Copy read into int var
            if(varIndex >= 0)
            {
                Compiler::emitVcpuAsm("%ReadIntVar", "", false);
                Compiler::emitVcpuAsm("DEEK", "", false);

                if(varType == Compiler::VarInt16)
                {
                    Compiler::emitVcpuAsm("STW", "_" + Compiler::getIntegerVars()[varIndex]._name, false);
                }
                else if(varType == Compiler::Var1Arr16)
                {
                    Compiler::writeArrayVarNoAssign(codeLine, codeLineIndex, varIndex);
                }
                else
                {
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::READ() : '%s:%d' : integer var '%s' is invalid : %s\n", codeLine._moduleName.c_str(), codeLineStart, varToken.c_str(), codeLine._text.c_str());
                    return false;
                }
            }
            // Copy read into str var
            else if(strIndex >= 0)
            {
                if(varType == Compiler::VarStr)
                {
                    Compiler::emitVcpuAsm("LDWI", "_" + Compiler::getStringVars()[strIndex]._name, false);
                    Compiler::emitVcpuAsm("STW", "strDstAddr", false);
                    Compiler::emitVcpuAsm("%ReadStrVar", "", false);
                }
                else if(varType == Compiler::VarStr2)
                {
                    Compiler::writeArrayStrNoAssign(arrText, codeLineIndex, strIndex);
                    Compiler::emitVcpuAsm("STW", "strDstAddr", false);
                    Compiler::emitVcpuAsm("%ReadStrVar", "", false);
                }
                else
                { 
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::READ() : '%s:%d' : string var '%s' is invalid : %s\n", codeLine._moduleName.c_str(), codeLineStart, varToken.c_str(), codeLine._text.c_str());
                    return false;
                }
            }
            else
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::READ() : '%s:%d' : var '%s' is invalid : %s\n", codeLine._moduleName.c_str(), codeLineStart, varToken.c_str(), codeLine._text.c_str());
                return false;
            }
        }

        return true;
    }

    bool RESTORE(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(tokenIndex);

        std::vector<std::string> tokens = Expression::tokenise(codeLine._code.substr(foundPos), ',', false);
        if(tokens.size() > 1)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::RESTORE() : '%s:%d' : syntax error, use 'RESTORE <optional index>' : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        Compiler::emitVcpuAsm("LDWI", "_dataIndex_", false);
        Compiler::emitVcpuAsm("STW", "memAddr",      false);
        
        if(tokens.size() == 1)
        {
            Expression::Numeric numeric;
            if(Compiler::parseExpression(codeLineIndex, tokens[0], numeric) == Expression::IsInvalid)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::RESTORE() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, tokens[0].c_str(), codeLine._text.c_str());
                return false;
            }
        }
        else
        {
            Compiler::emitVcpuAsm("LDI", "0", false);
        }

        Compiler::emitVcpuAsm("DOKE", "memAddr", false);

        return true;
    }

    bool ALLOC(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(tokenIndex);

        std::vector<std::string> tokens = Expression::tokenise(codeLine._code.substr(foundPos), ',', false);
        if(tokens.size() < 1  &&  tokens.size() > 4)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::ALLOC() : '%s:%d' : syntax error, use 'ALLOC <address>, <optional size>, <optional count>, <optional offset=0x0100>' : %s\n",
                                                    codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        int count = 1;
        uint16_t address, end, size = 0x0000, offset = 0x0100;
        std::string addrOperand, sizeOperand, countOperand, offsetOperand;
        Expression::Numeric addrNumeric(true), sizeNumeric(true), countNumeric(true), offsetNumeric(true);  // true = allow static init
        if(Compiler::parseStaticExpression(codeLineIndex, tokens[0], addrOperand, addrNumeric) == Compiler::OperandInvalid)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::ALLOC() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, tokens[0].c_str(), codeLine._text.c_str());
            return false;
        }
        if(tokens.size() >= 2)
        {
            if(Compiler::parseStaticExpression(codeLineIndex, tokens[1], sizeOperand, sizeNumeric) == Compiler::OperandInvalid)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::ALLOC() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, tokens[1].c_str(), codeLine._text.c_str());
                return false;
            }
            size = uint16_t(std::lround(sizeNumeric._value));
        }
        if(tokens.size() >= 3)
        {
            if(Compiler::parseStaticExpression(codeLineIndex, tokens[2], countOperand, countNumeric) == Compiler::OperandInvalid)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::ALLOC() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, tokens[2].c_str(), codeLine._text.c_str());
                return false;
            }
            count = std::lround(countNumeric._value);
        }
        if(tokens.size() >= 4)
        {
            if(Compiler::parseStaticExpression(codeLineIndex, tokens[3], offsetOperand, offsetNumeric) == Compiler::OperandInvalid)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::ALLOC() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, tokens[3].c_str(), codeLine._text.c_str());
                return false;
            }
            offset = uint16_t(std::lround(offsetNumeric._value));
            if(count == 0  ||  offset == 0)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::ALLOC() : '%s:%d' : count and offset must both be non zero, found %d and 0x%04x : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                                                            count, offset, codeLine._text.c_str());
                return false;
            }
        }

        address = uint16_t(std::lround(addrNumeric._value));
        if(address < DEFAULT_EXEC_ADDRESS)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::ALLOC() : '%s:%d' : address field must be above &h%04x, found %s : %s\n", codeLine._moduleName.c_str(), codeLineStart, DEFAULT_EXEC_ADDRESS,
                                                                                                                                         tokens[0].c_str(), codeLine._text.c_str());
            return false;
        }

        end = (size == 0) ? 0xFFFF : address + size;
        for(int i=0; i<count; i++)
        {
            //Cpu::reportError(Cpu::NoError, stderr, "0x%04x 0x%04x %d\n", address, end, end - address);
            for(uint16_t j=address; j<end; j++)
            {
                if(!Memory::takeFreeRAM(j, 1, false))
                {
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::ALLOC() : '%s:%d' : trying to allocate already allocated memory at 0x%04x : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                                                          j, codeLine._text.c_str());
                    return false;
                }
            }
            address += offset;
            end += offset;
        }
    
        //Memory::printFreeRamList(Memory::NoSort);

        return true;
    }

    bool FREE(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(tokenIndex);

        std::vector<std::string> tokens = Expression::tokenise(codeLine._code.substr(foundPos), ',', false);
        if(tokens.size() < 1  ||  tokens.size() > 4)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::FREE() : '%s:%d' : syntax error, use 'FREE <address>, <size>, <optional count>, <optional offset=0x0100> or FREE STRINGWORKAREA' : %s\n",
                                                    codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        // Free string work area
        if(tokens.size() == 1)
        {
            std::string token = Expression::strUpper(tokens[0]);
            Expression::stripWhitespace(token);
            if(token != "STRINGWORKAREA")
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::FREE() : '%s:%d' : syntax error, expecting 'STRINGWORKAREA', found '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                                                    token.c_str(), codeLine._text.c_str());
                return false;
            }

            for(int i=0; i<NUM_STR_WORK_AREAS; i++)
            {
                if(!Memory::giveFreeRAM(Compiler::getStrWorkArea(i), USER_STR_SIZE + 2))
                {
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::FREE() : '%s:%d' : RAM at '0x%04x' is already free : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                                   Compiler::getStrWorkArea(i), codeLine._text.c_str());
                    return false;
                }
            }

            return true;
        }

        std::string addrOperand, sizeOperand, countOperand, offsetOperand;;
        Expression::Numeric addrNumeric(true), sizeNumeric(true), countNumeric(true), offsetNumeric(true);  // true = allow static init
        if(Compiler::parseStaticExpression(codeLineIndex, tokens[0], addrOperand, addrNumeric) == Compiler::OperandInvalid)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::FREE() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, tokens[0].c_str(), codeLine._text.c_str());
            return false;
        }
        if(Compiler::parseStaticExpression(codeLineIndex, tokens[1], sizeOperand, sizeNumeric) == Compiler::OperandInvalid)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::FREE() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, tokens[1].c_str(), codeLine._text.c_str());
            return false;
        }
        countNumeric._value = 1;
        if(tokens.size() >= 3)
        {
            if(Compiler::parseStaticExpression(codeLineIndex, tokens[2], countOperand, countNumeric) == Compiler::OperandInvalid)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::FREE() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, tokens[2].c_str(), codeLine._text.c_str());
                return false;
            }
        }
        offsetNumeric._value = 0x0100;
        if(tokens.size() >= 4)
        {
            if(Compiler::parseStaticExpression(codeLineIndex, tokens[3], offsetOperand, offsetNumeric) == Compiler::OperandInvalid)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::FREE() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, tokens[3].c_str(), codeLine._text.c_str());
                return false;
            }
        }

        uint16_t address = uint16_t(std::lround(addrNumeric._value));
        uint16_t size = uint16_t(std::lround(sizeNumeric._value));
        uint16_t count = uint16_t(std::lround(countNumeric._value));
        uint16_t offset = uint16_t(std::lround(offsetNumeric._value));

        if(count == 0  ||  offset == 0)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::FREE() : '%s:%d' : count and offset must both be non zero, found %d and 0x%04x : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                                                       count, offset, codeLine._text.c_str());
            return false;
        }

        //Memory::printFreeRamList(Memory::NoSort);
        for(int i=0; i<count; i++)
        {
            if(!Memory::giveFreeRAM(address, size))
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::FREE() : '%s:%d' : RAM at '0x%04x' is already free : %s\n", codeLine._moduleName.c_str(), codeLineStart, address, codeLine._text.c_str());
                return false;
            }
            address += offset;
        }
        //Memory::printFreeRamList(Memory::NoSort);

        return true;
    }

    void usageCLEAR(Compiler::CodeLine& codeLine, int codeLineStart)
    {
        Cpu::reportError(Cpu::KwdError, stderr, "Keywords::CLEAR() : '%s:%d' : syntax error, use 'CLEAR VARS, <optional start var or var address> : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
        Cpu::reportError(Cpu::KwdError, stderr, "Keywords::CLEAR() : '%s:%d' : syntax error, use 'CLEAR RAM, <address>, <length> : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
    }
    bool CLEAR(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(tokenIndex);

        std::vector<std::string> tokens = Expression::tokenise(codeLine._code.substr(foundPos), ',', false);
        if(tokens.size() < 1  ||  tokens.size() > 3)
        {
            usageCLEAR(codeLine, codeLineStart);
            return false;
        }

        Expression::stripWhitespace(tokens[0]);
        std::string token = tokens[0];
        Expression::strToUpper(token);
        if(token != "VARS"  &&  token != "RAM")
        {
            usageCLEAR(codeLine, codeLineStart);
            return false;
        }

        // Parse VARS
        if(token == "VARS")
        {
            std::string varToken;
            if(tokens.size() > 1)
            {
                Expression::stripWhitespace(tokens[1]);
                token = tokens[1];
                varToken = token;
            }

            // Parse VARS label or address
            if(varToken.size())
            {
                int varIndex = Compiler::findVar(varToken, false);
                if(varIndex >= 0)
                {
                    Compiler::emitVcpuAsm("%ResetVars", "_" + varToken, false);
                }
                else
                {
                    Expression::Numeric numeric(true); // true = allow static init
                    if(!Expression::parse(varToken, codeLineIndex, numeric))
                    {
                        Cpu::reportError(Cpu::KwdError, stderr, "Keywords::CLEAR() : '%s:%d' : syntax error in '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                             Expression::getExpression(), codeLine._text.c_str());
                        return false;
                    }

                    uint16_t varAddr = uint16_t(std::lround(numeric._value));
                    if(varAddr < USER_VAR_START)
                    {
                        Cpu::reportError(Cpu::KwdError, stderr, "Keywords::CLEAR() : '%s:%d' : var address error in '0x%04x' : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                                      varAddr, codeLine._text.c_str());
                        return false;
                    }
                    Compiler::emitVcpuAsm("%ResetVars", Expression::byteToHexString(uint8_t(varAddr)), false);
                }
            }
            else
            {
                Compiler::emitVcpuAsm("%ResetVars", "giga_User", false);
            }

            return true;
        }

        // Parse MEM
        std::string memToken;
        if(tokens.size() != 3)
        {
            usageCLEAR(codeLine, codeLineStart);
            return false;
        }

        // Parse MEM address
        Expression::Numeric numericAddr(true); // true = allow static init
        if(!Expression::parse(tokens[1], codeLineIndex, numericAddr))
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::CLEAR() : '%s:%d' : syntax error in '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                 Expression::getExpression(), codeLine._text.c_str());
            return false;
        }

        // Parse MEM count
        Expression::Numeric numericCount(true); // true = allow static init
        if(!Expression::parse(tokens[2], codeLineIndex, numericCount))
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::CLEAR() : '%s:%d' : syntax error in '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                 Expression::getExpression(), codeLine._text.c_str());
            return false;
        }

        uint16_t memAddr = uint16_t(std::lround(numericAddr._value));
        uint16_t memCount = uint16_t(std::lround(numericCount._value));
        Compiler::emitVcpuAsm("%ResetMem", Expression::wordToHexString(memAddr) + ' ' + Expression::wordToHexString(memCount), false);

        return true;
    }

    bool AT(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(tokenIndex);

        std::vector<std::string> tokens = Expression::tokenise(codeLine._code.substr(foundPos), ',', false);
        if(tokens.size() < 1  ||  tokens.size() > 2)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::AT() : '%s:%d' : syntax error, use 'AT <x>' or 'AT <x>, <y>' : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        Expression::Numeric numeric;
        if(Compiler::parseExpression(codeLineIndex, tokens[0], numeric) == Expression::IsInvalid)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::AT() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, tokens[0].c_str(), codeLine._text.c_str());
            return false;
        }
        Compiler::emitVcpuAsm("ST", "cursorXY", false);

        if(tokens.size() == 2)
        {
            if(Compiler::parseExpression(codeLineIndex, tokens[1], numeric) == Expression::IsInvalid)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::AT() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, tokens[1].c_str(), codeLine._text.c_str());
                return false;
            }
            Compiler::emitVcpuAsm("ST", "cursorXY + 1", false);
        }

        Compiler::emitVcpuAsm("%AtTextCursor", "", false);
        return true;
    }

    bool PUT(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(tokenIndex);

        std::vector<std::string> tokens = Expression::tokenise(codeLine._code.substr(foundPos), ',', false);
        if(tokens.size() != 1)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::PUT() : '%s:%d' : syntax error, use 'PUT <ascii>' : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        Expression::Numeric numeric;
        if(Compiler::parseExpression(codeLineIndex, tokens[0], numeric) == Expression::IsInvalid)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::PUT() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, tokens[0].c_str(), codeLine._text.c_str());
            return false;
        }
        Compiler::emitVcpuAsm("%PrintAcChr", "", false);

        return true;
    }

    bool MODE(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(tokenIndex);

        if(Compiler::getCodeRomType() < Cpu::ROMv2)
        {
            std::string romTypeStr;
            getRomTypeStr(Compiler::getCodeRomType(), romTypeStr);
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::MODE() : '%s:%d' : version error, 'MODE' requires ROMv2 or higher, you are trying to link against '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                                                                               romTypeStr.c_str(), codeLine._text.c_str());
            return false;
        }

        std::vector<std::string> tokens = Expression::tokenise(codeLine._code.substr(foundPos), ',', false);
        if(tokens.size() != 1)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::MODE() : '%s:%d' : syntax error, use 'MODE <0 - 3>' : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        Expression::Numeric numeric;
        if(Compiler::parseExpression(codeLineIndex, tokens[0], numeric) == Expression::IsInvalid)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::MODE() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, tokens[0].c_str(), codeLine._text.c_str());
            return false;
        }
        Compiler::emitVcpuAsm("STW", "graphicsMode", false);
        Compiler::emitVcpuAsm("%ScanlineMode", "",   false);

        return true;
    }

    bool WAIT(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(tokenIndex);

        std::vector<std::string> tokens = Expression::tokenise(codeLine._code.substr(foundPos), ',', false);
        if(tokens.size() > 1)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::WAIT() : '%s:%d' : syntax error, use 'WAIT <optional vblank count>' : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        if(tokens.size() == 0)
        {
            Compiler::emitVcpuAsm("%WaitVBlank", "", false);
            return true;
        }

        Expression::Numeric numeric;
        if(Compiler::parseExpression(codeLineIndex, tokens[0], numeric) == Expression::IsInvalid)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::WAIT() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, tokens[0].c_str(), codeLine._text.c_str());
            return false;
        }
        Compiler::emitVcpuAsm("STW", "waitVBlankNum", false);
        Compiler::emitVcpuAsm("%WaitVBlanks", "",     false);

        return true;
    }

    bool PSET(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(tokenIndex);

        std::vector<std::string> tokens = Expression::tokenise(codeLine._code.substr(foundPos), ',', false);
        if(tokens.size() < 2  ||  tokens.size() > 3)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::PSET() : '%s:%d' : syntax error, use 'PSET <x>, <y>' or 'PSET <x>, <y>, <colour>' : %s\n", codeLine._moduleName.c_str(),
                                                                                                                                                          codeLineStart, codeLine._text.c_str());
            return false;
        }

        Expression::Numeric numeric;
        if(Compiler::getCodeRomType() >= Cpu::ROMvX0  &&  Compiler::getCodeRomType() < Cpu::SDCARD)
        {
            if(Compiler::parseExpression(codeLineIndex, tokens[0], numeric) == Expression::IsInvalid)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::PSET() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, tokens[0].c_str(), codeLine._text.c_str());
                return false;
            }
            Compiler::emitVcpuAsm("ST", "drawPixel_xy", false);
            if(Compiler::parseExpression(codeLineIndex, tokens[1], numeric) == Expression::IsInvalid)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::PSET() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, tokens[1].c_str(), codeLine._text.c_str());
                return false;
            }
            Compiler::emitVcpuAsm("ST", "drawPixel_xy + 1", false);

            if(tokens.size() == 3)
            {
                if(Compiler::parseExpression(codeLineIndex, tokens[2], numeric) == Expression::IsInvalid)
                {
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::PSET() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, tokens[2].c_str(), codeLine._text.c_str());
                    return false;
                }
                Compiler::emitVcpuAsm("ST", "fgbgColour + 1", false);
            }

            Compiler::emitVcpuAsm("LDW", "drawPixel_xy", false);
            Compiler::emitVcpuAsm("STPX", "fgbgColour+1", false);
        }
        else
        {
            if(Compiler::parseExpression(codeLineIndex, tokens[0], numeric) == Expression::IsInvalid)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::PSET() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, tokens[0].c_str(), codeLine._text.c_str());
                return false;
            }
            Compiler::emitVcpuAsm("ST", "drawPixel_xy", false);
            if(Compiler::parseExpression(codeLineIndex, tokens[1], numeric) == Expression::IsInvalid)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::PSET() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, tokens[1].c_str(), codeLine._text.c_str());
                return false;
            }
            Compiler::emitVcpuAsm("ST", "drawPixel_xy + 1", false);

            if(tokens.size() == 3)
            {
                if(Compiler::parseExpression(codeLineIndex, tokens[2], numeric) == Expression::IsInvalid)
                {
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::PSET() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, tokens[2].c_str(), codeLine._text.c_str());
                    return false;
                }
                Compiler::emitVcpuAsm("ST", "fgbgColour + 1", false);
            }

            Compiler::emitVcpuAsm("%DrawPixel", "", false);
        }

        return true;
    }

    bool LINE(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(tokenIndex);

        std::vector<std::string> tokens = Expression::tokenise(codeLine._code.substr(foundPos), ',', false);
        if(tokens.size() != 2  &&  tokens.size() != 4)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::LINE() : '%s:%d' : syntax error, use 'LINE <x>, <y>' or 'LINE <x1>, <y1>, <x2>, <y2>' : %s\n", codeLine._moduleName.c_str(),
                                                                                                                                                              codeLineStart, codeLine._text.c_str());
            return false;
        }

        if(tokens.size() == 2)
        {
            std::vector<Expression::Numeric> params = {Expression::Numeric(), Expression::Numeric()};
            for(int i=0; i<int(tokens.size()); i++)
            {
                if(Compiler::parseExpression(codeLineIndex, tokens[i], params[i]) == Expression::IsInvalid)
                {
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::LINE() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, tokens[i].c_str(), codeLine._text.c_str());
                    return false;
                }
                switch(i)
                {
                    case 0: Compiler::emitVcpuAsm("STW", "drawLine_x2", false); break;
                    case 1: Compiler::emitVcpuAsm("STW", "drawLine_y2", false); break;

                    default: break;
                }
            }

            Compiler::emitVcpuAsm("%AtLineCursor", "", false);
            Compiler::emitVcpuAsm("%DrawVTLine",   "", false);
        }
        else
        {
            std::vector<Expression::Numeric> params = {Expression::Numeric(), Expression::Numeric(), Expression::Numeric(), Expression::Numeric()};
            for(int i=0; i<int(tokens.size()); i++)
            {
                if(Compiler::parseExpression(codeLineIndex, tokens[i], params[i]) == Expression::IsInvalid)
                {
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::LINE() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, tokens[i].c_str(), codeLine._text.c_str());
                    return false;
                }
                switch(i)
                {
                    case 0: Compiler::emitVcpuAsm("STW", "drawLine_x1", false); break;
                    case 1: Compiler::emitVcpuAsm("STW", "drawLine_y1", false); break;
                    case 2: Compiler::emitVcpuAsm("STW", "drawLine_x2", false); break;
                    case 3: Compiler::emitVcpuAsm("STW", "drawLine_y2", false); break;

                    default: break;
                }
            }

            Compiler::emitVcpuAsm("%DrawLine", "", false);
        }

        return true;
    }

    bool HLINE(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(tokenIndex);

        std::vector<std::string> tokens = Expression::tokenise(codeLine._code.substr(foundPos), ',', false);
        if(tokens.size() != 3)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::HLINE() : '%s:%d' : syntax error, use 'HLINE <x1>, <y>, <x2>' : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        std::vector<Expression::Numeric> params = {Expression::Numeric(), Expression::Numeric(), Expression::Numeric()};
        for(int i=0; i<int(tokens.size()); i++)
        {
            if(Compiler::parseExpression(codeLineIndex, tokens[i], params[i]) == Expression::IsInvalid)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::HLINE() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, tokens[i].c_str(), codeLine._text.c_str());
                return false;
            }
            switch(i)
            {
                case 0: Compiler::emitVcpuAsm("STW", "drawHLine_x1", false); break;
                case 1: Compiler::emitVcpuAsm("STW", "drawHLine_y1", false); break;
                case 2: Compiler::emitVcpuAsm("STW", "drawHLine_x2", false); break;

                default: break;
            }
        }

        Compiler::emitVcpuAsm("%DrawHLine", "", false);

        return true;
    }

    bool VLINE(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(tokenIndex);

        std::vector<std::string> tokens = Expression::tokenise(codeLine._code.substr(foundPos), ',', false);
        if(tokens.size() != 3)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::VLINE() : '%s:%d' : syntax error, use 'VLINE <x>, <y1>, <y2>' : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        std::vector<Expression::Numeric> params = {Expression::Numeric(), Expression::Numeric(), Expression::Numeric()};
        for(int i=0; i<int(tokens.size()); i++)
        {
            if(Compiler::parseExpression(codeLineIndex, tokens[i], params[i]) == Expression::IsInvalid)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::VLINE() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, tokens[i].c_str(), codeLine._text.c_str());
                return false;
            }
            switch(i)
            {
                case 0: Compiler::emitVcpuAsm("STW", "drawVLine_x1", false); break;
                case 1: Compiler::emitVcpuAsm("STW", "drawVLine_y1", false); break;
                case 2: Compiler::emitVcpuAsm("STW", "drawVLine_y2", false); break;

                default: break;
            }
        }

        Compiler::emitVcpuAsm("%DrawVLine", "", false);

        return true;
    }

    bool CIRCLE(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(tokenIndex);

        std::vector<std::string> tokens = Expression::tokenise(codeLine._code.substr(foundPos), ',', false);
        if(tokens.size() != 3)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::CIRCLE() : '%s:%d' : syntax error, use 'CIRCLE <x>, <y>, <radius>' : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        std::vector<Expression::Numeric> params = {Expression::Numeric(), Expression::Numeric(), Expression::Numeric()};
        for(int i=0; i<int(tokens.size()); i++)
        {
            if(Compiler::parseExpression(codeLineIndex, tokens[i], params[i]) == Expression::IsInvalid)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::CIRCLE() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, tokens[i].c_str(), codeLine._text.c_str());
                return false;
            }
            switch(i)
            {
                case 0: Compiler::emitVcpuAsm("ST",   "drawCircle_cycx", false);                                                            break;
                case 1: Compiler::emitVcpuAsm("ADDI", "8",               false); Compiler::emitVcpuAsm("ST", "drawCircle_cycx + 1", false); break;
                case 2: Compiler::emitVcpuAsm("STW",  "drawCircle_r",    false);                                                            break;

                default: break;
            }
        }

        Compiler::emitVcpuAsm("%DrawCircle", "", false);

        return true;
    }

    bool CIRCLEF(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(tokenIndex);

        std::vector<std::string> tokens = Expression::tokenise(codeLine._code.substr(foundPos), ',', false);
        if(tokens.size() != 3)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::CIRCLEF() : '%s:%d' : syntax error, use 'CIRCLEF <x>, <y>, <radius>' : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        std::vector<Expression::Numeric> params = {Expression::Numeric(), Expression::Numeric(), Expression::Numeric()};
        for(int i=0; i<int(tokens.size()); i++)
        {
            if(Compiler::parseExpression(codeLineIndex, tokens[i], params[i]) == Expression::IsInvalid)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::CIRCLEF() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, tokens[i].c_str(), codeLine._text.c_str());
                return false;
            }
            switch(i)
            {
                case 0: Compiler::emitVcpuAsm("STW", "drawCircleF_cx", false); break;
                case 1: Compiler::emitVcpuAsm("STW", "drawCircleF_cy", false); break;
                case 2: Compiler::emitVcpuAsm("STW", "drawCircleF_r",  false); break;

                default: break;
            }
        }

        Compiler::emitVcpuAsm("%DrawCircleF", "", false);

        return true;
    }

    bool RECT(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(tokenIndex);

        std::vector<std::string> tokens = Expression::tokenise(codeLine._code.substr(foundPos), ',', false);
        if(tokens.size() != 4)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::RECT() : '%s:%d' : syntax error, use 'RECT <x1>, <y1>, <x2>, <y2>'' : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        std::vector<Expression::Numeric> params = {Expression::Numeric(), Expression::Numeric(), Expression::Numeric(), Expression::Numeric()};
        for(int i=0; i<int(tokens.size()); i++)
        {
            if(Compiler::parseExpression(codeLineIndex, tokens[i], params[i]) == Expression::IsInvalid)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::RECT() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, tokens[i].c_str(), codeLine._text.c_str());
                return false;
            }
            switch(i)
            {
                case 0: Compiler::emitVcpuAsm("STW", "drawRect_x1", false); break;
                case 1: Compiler::emitVcpuAsm("STW", "drawRect_y1", false); break;
                case 2: Compiler::emitVcpuAsm("STW", "drawRect_x2", false); break;
                case 3: Compiler::emitVcpuAsm("STW", "drawRect_y2", false); break;

                default: break;
            }
        }

        Compiler::emitVcpuAsm("%DrawRect", "", false);

        return true;
    }

    bool RECTF(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(tokenIndex);

        std::vector<std::string> tokens = Expression::tokenise(codeLine._code.substr(foundPos), ',', false);
        if(tokens.size() != 4)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::RECTF() : '%s:%d' : syntax error, use 'RECTF <x1>, <y1>, <x2>, <y2>' : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        std::vector<Expression::Numeric> params = {Expression::Numeric(), Expression::Numeric(), Expression::Numeric(), Expression::Numeric()};
        for(int i=0; i<int(tokens.size()); i++)
        {
            if(Compiler::parseExpression(codeLineIndex, tokens[i], params[i]) == Expression::IsInvalid)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::RECTF() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, tokens[i].c_str(), codeLine._text.c_str());
                return false;
            }
            switch(i)
            {
                case 0: Compiler::emitVcpuAsm("STW", "drawRectF_x1", false); break;
                case 1: Compiler::emitVcpuAsm("STW", "drawRectF_y1", false); break;
                case 2: Compiler::emitVcpuAsm("STW", "drawRectF_x2", false); break;
                case 3: Compiler::emitVcpuAsm("STW", "drawRectF_y2", false); break;

                default: break;
            }
        }

        Compiler::emitVcpuAsm("%DrawRectF", "", false);

        return true;
    }

    bool POLY(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(tokenIndex);

        std::vector<std::string> tokens = Expression::tokenise(codeLine._code.substr(foundPos), ',', false);
        if(tokens.size() != 1)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::POLY() : '%s:%d' : syntax error, use 'POLY <coords address>' : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        Expression::Numeric param;
        if(Compiler::parseExpression(codeLineIndex, tokens[0], param) == Expression::IsInvalid)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::POLY() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, tokens[0].c_str(), codeLine._text.c_str());
            return false;
        }
        Compiler::emitVcpuAsm("STW", "drawPoly_addr", false);
        Compiler::emitVcpuAsm("%DrawPoly", "",        false);

        return true;
    }

    bool POLYR(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(tokenIndex);

        std::vector<std::string> tokens = Expression::tokenise(codeLine._code.substr(foundPos), ',', false);
        if(tokens.size() < 1  ||  tokens.size() > 2)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::POLYR() : '%s:%d' : syntax error, use 'POLYR <coords address> <optional FLIPX/FLIPY/FLIPXY>' : %s\n", codeLine._moduleName.c_str(),
                                                                                                                                                                     codeLineStart, codeLine._text.c_str());
            return false;
        }

        // Flip type
        if(tokens.size() == 2)
        {
            std::string flipToken = tokens[1];
            Expression::stripWhitespace(flipToken);
            Expression::strToUpper(flipToken);
        
            if(flipToken != "FLIPX"  &&  flipToken != "FLIPY"  &&  flipToken != "FLIPXY")
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::POLYR() : '%s:%d' : syntax error, use one of the correct flip types, 'POLY <coords address> <optional FLIPX/FLIPY/FLIPXY>' : %s\n",
                                                        codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
                return false;
            }

             // SUBW mode
            Compiler::emitVcpuAsm("LDI", "0xB8", false);
            Compiler::emitVcpuAsm("ST", "drawPoly_mode", false);

            // FlipX
            if(flipToken == "FLIPX")
            {
                Compiler::emitVcpuAsm("%SetPolyRelFlipX", "", false);
            }
            // FlipY
            else if(flipToken == "FLIPY")
            {
                Compiler::emitVcpuAsm("%SetPolyRelFlipY", "", false);
            }
            // FlipXY
            else
            {
                Compiler::emitVcpuAsm("%SetPolyRelFlipX", "", false);
                Compiler::emitVcpuAsm("%SetPolyRelFlipY", "", false);
            }
        }

        Expression::Numeric param;
        if(Compiler::parseExpression(codeLineIndex, tokens[0], param) == Expression::IsInvalid)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::POLYR() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, tokens[0].c_str(), codeLine._text.c_str());
            return false;
        }
        Compiler::emitVcpuAsm("STW", "drawPoly_addr", false);
        Compiler::emitVcpuAsm("%DrawPolyRel", "", false);

        return true;
    }

    bool FILL(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(tokenIndex);

        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::FILL() : '%s:%d' : version error, 'FILL' only works with ROMvX0 : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }
        std::vector<std::string> tokens = Expression::tokenise(codeLine._code.substr(foundPos), ',', false);
        if(tokens.size() != 8)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::FILL() : '%s:%d' : syntax error, use 'FILL <x>, <y>, <xCount>, <yCount>, <fill colour>, <border colour>, <replace colour>, <fill toggle>' : %s\n",
                                                    codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        // X
        Expression::Numeric param;
        std::string xToken = tokens[0];
        if(Compiler::parseExpression(codeLineIndex, xToken, param) == Expression::IsInvalid)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::FILL() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, xToken.c_str(), codeLine._text.c_str());
            return false;
        }
        Compiler::emitVcpuAsm("ST", "giga_sysArg2", false);

        // Y
        std::string yToken = tokens[1];
        if(Compiler::parseExpression(codeLineIndex, yToken, param) == Expression::IsInvalid)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::FILL() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, yToken.c_str(), codeLine._text.c_str());
            return false;
        }
        Compiler::emitVcpuAsm("ADDI", "8", false);
        Compiler::emitVcpuAsm("ST", "giga_sysArg3", false);

        // X count
        std::string xCountToken = tokens[2];
        if(Compiler::parseExpression(codeLineIndex, xCountToken, param) == Expression::IsInvalid)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::FILL() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, xCountToken.c_str(), codeLine._text.c_str());
            return false;
        }
        Compiler::emitVcpuAsm("ST", "giga_sysArg4", false);

        // Y count
        std::string yCountToken = tokens[3];
        if(Compiler::parseExpression(codeLineIndex, yCountToken, param) == Expression::IsInvalid)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::FILL() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, yCountToken.c_str(), codeLine._text.c_str());
            return false;
        }
        Compiler::emitVcpuAsm("ST", "giga_sysArg5", false);

        // Fill colour
        std::string fColourToken = tokens[4];
        if(Compiler::parseExpression(codeLineIndex, fColourToken, param) == Expression::IsInvalid)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::FILL() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, fColourToken.c_str(), codeLine._text.c_str());
            return false;
        }
        Compiler::emitVcpuAsm("ST", "giga_sysArg1", false);

        // Border colour
        std::string bColourToken = tokens[5];
        if(Compiler::parseExpression(codeLineIndex, bColourToken, param) == Expression::IsInvalid)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::FILL() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, bColourToken.c_str(), codeLine._text.c_str());
            return false;
        }
        Compiler::emitVcpuAsm("ST", "giga_sysArg0", false);

        // Replace colour
        std::string rColourToken = tokens[6];
        if(Compiler::parseExpression(codeLineIndex, rColourToken, param) == Expression::IsInvalid)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::FILL() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, rColourToken.c_str(), codeLine._text.c_str());
            return false;
        }
        Compiler::emitVcpuAsm("ST", "giga_sysArg7", false);

        // Fill toggle
        std::string fToggleToken = tokens[7];
        if(Compiler::parseExpression(codeLineIndex, fToggleToken, param) == Expression::IsInvalid)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::FILL() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, fToggleToken.c_str(), codeLine._text.c_str());
            return false;
        }
        Compiler::emitVcpuAsm("ANDI", "1", false);
        Compiler::emitVcpuAsm("ST", "giga_sysArg6", false);

        Compiler::emitVcpuAsm("%ParityFill", "", false); 

        return true;
    }

    bool TCLIP(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(tokenIndex);
        UNREFERENCED_PARAM(codeLineIndex);

        std::vector<std::string> tokens = Expression::tokenise(codeLine._code.substr(foundPos), ',', false);
        if(tokens.size() != 1)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::TCLIP() : '%s:%d' : syntax error, use 'TCLIP ON' or 'TCLIP OFF' : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        std::string tclipToken = Expression::strToUpper(tokens[0]);
        Expression::stripWhitespace(tclipToken);
        if(tclipToken != "ON"  &&  tclipToken != "OFF")
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::TCLIP() : '%s:%d' : syntax error, use 'TCLIP ON' or 'TCLIP OFF' : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        Compiler::emitVcpuAsm("LD", "miscFlags", false);
        if(tclipToken == "ON")
        {
            Compiler::emitVcpuAsm("ANDI", Expression::byteToHexString(MISC_DISABLE_CLIP_MSK), false);
        }
        else
        {
            Compiler::emitVcpuAsm("ORI", Expression::byteToHexString(MISC_DISABLE_CLIP_BIT), false);
        }
        Compiler::emitVcpuAsm("ST", "miscFlags", false);

        return true;
    }

    bool TSCROLL(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(tokenIndex);
        UNREFERENCED_PARAM(codeLineIndex);

        std::vector<std::string> tokens = Expression::tokenise(codeLine._code.substr(foundPos), ',', false);
        if(tokens.size() != 1)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::TSCROLL() : '%s:%d' : syntax error, use 'TSCROLL ON' or 'TSCROLL OFF' : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        std::string scrollToken = Expression::strToUpper(tokens[0]);
        Expression::stripWhitespace(scrollToken);
        if(scrollToken != "ON"  &&  scrollToken != "OFF")
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::TSCROLL() : '%s:%d' : syntax error, use 'TSCROLL ON' or 'TSCROLL OFF'' : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        Compiler::emitVcpuAsm("LD", "miscFlags", false);
        if(scrollToken == "ON")
        {
            Compiler::emitVcpuAsm("ORI", Expression::byteToHexString(MISC_ENABLE_SCROLL_BIT), false);
        }
        else
        {
            Compiler::emitVcpuAsm("ANDI", Expression::byteToHexString(MISC_ENABLE_SCROLL_MSK), false);
        }
        Compiler::emitVcpuAsm("ST", "miscFlags", false);

        return true;
    }

    bool TFNT4X6(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(tokenIndex);
        UNREFERENCED_PARAM(codeLineIndex);

        std::vector<std::string> tokens = Expression::tokenise(codeLine._code.substr(foundPos), ',', false);
        if(tokens.size() != 1)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::TFNT4X6() : '%s:%d' : syntax error, use 'TFNT4X6 ON' or 'TFNT4X6 OFF' : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        std::string scrollToken = Expression::strToUpper(tokens[0]);
        Expression::stripWhitespace(scrollToken);
        if(scrollToken != "ON"  &&  scrollToken != "OFF")
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::TFNT4X6() : '%s:%d' : syntax error, use 'TFNT4X6 ON' or 'TFNT4X6 OFF'' : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        if(scrollToken == "ON")
        {
            Compiler::emitVcpuAsm("LDI", "0", false);
            Compiler::emitVcpuAsm("STW", "cursorXY", false);
            Compiler::emitVcpuAsm("LD", "miscFlags", false);
            Compiler::emitVcpuAsm("ORI", Expression::byteToHexString(MISC_ENABLE_FNT4X6_BIT), false);
        }
        else
        {
            Compiler::emitVcpuAsm("LD", "miscFlags", false);
            Compiler::emitVcpuAsm("ANDI", Expression::byteToHexString(MISC_ENABLE_FNT4X6_MSK), false);
        }
        Compiler::emitVcpuAsm("ST", "miscFlags", false);

        return true;
    }

    bool HSCROLL(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(foundPos);
        UNREFERENCED_PARAM(tokenIndex);
        UNREFERENCED_PARAM(codeLineIndex);

        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::HSCROLL() : '%s:%d' : version error, 'HSCROLL' only works with ROMvX0 : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        return true;
    }

    bool VSCROLL(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(tokenIndex);
        UNREFERENCED_PARAM(codeLineIndex);

        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::VSCROLL() : '%s:%d' : version error, 'VSCROLL' only works with ROMvX0 : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        std::vector<std::string> tokens = Expression::tokenise(codeLine._code.substr(foundPos), ',', false);
        if(tokens.size() != 1  &&  tokens.size() != 3)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::VSCROLL() : '%s:%d' : syntax error, use 'VSCROLL <scroll -31 to 31>, <optional start>, <optional count>' : %s\n",
                                                    codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        // Scroll offset
        std::string scrollOffsetToken = tokens[0];
        Expression::Numeric scrollOffsetNumeric;
        if(Compiler::parseExpression(codeLineIndex, scrollOffsetToken, scrollOffsetNumeric) == Expression::IsInvalid)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::VSCROLL() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, scrollOffsetToken.c_str(), codeLine._text.c_str());
            return false;
        }
        if(tokens.size() == 1)
        {
            Compiler::emitVcpuAsm("%ScrollV", "", false);
            return true;
        }

        Compiler::emitVcpuAsm("STW", "giga_sysArg0", false);

        // Scroll start
        std::string vtableToken = tokens[1];
        Expression::Numeric vtableNumeric;
        if(Compiler::parseExpression(codeLineIndex, vtableToken, vtableNumeric) == Expression::IsInvalid)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::VSCROLL() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, vtableToken.c_str(), codeLine._text.c_str());
            return false;
        }
        Compiler::emitVcpuAsm("STW", "register0", false);

        // Scroll count
        std::string scanCountToken = tokens[2];
        Expression::Numeric scanCountNumeric;
        if(Compiler::parseExpression(codeLineIndex, scanCountToken, scanCountNumeric) == Expression::IsInvalid)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::VSCROLL() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, scanCountToken.c_str(), codeLine._text.c_str());
            return false;
        }
        Compiler::emitVcpuAsm("STW", "register1", false);

        Compiler::emitVcpuAsm("%ScrollRectV", "", false);

        return true;
    }

    std::string pokeOpcodeHelper(const Expression::Numeric& numeric)
    {
        std::string output;

        switch(numeric._int16Byte)
        {
            case Expression::Int16Low:  output = "LD";  break;
            case Expression::Int16High: output = "LD";  break;
            case Expression::Int16Both: output = "LDW"; break;
        
            default: break;
        }

        return output;
    }
    std::string pokeOperandHelper(Compiler::OperandType operandType, const Expression::Numeric& numeric, const std::string& operand)
    {
        std::string output = operand;

        if(operandType == Compiler::OperandVar)
        {
            switch(numeric._int16Byte)
            {
                case Expression::Int16Low:  output = "_" + operand;          break;
                case Expression::Int16High: output = "_" + operand + " + 1"; break;
                case Expression::Int16Both: output = "_" + operand;          break;
        
                default: break;
            }
        }

        return output;
    }
    bool POKE(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(tokenIndex);

        std::vector<std::string> tokens = Expression::tokenise(codeLine._code.substr(foundPos), ',', false);
        if(tokens.size() != 2)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::POKE() : '%s:%d' : syntax error, use 'POKE <address>, <value>' : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        std::vector<std::string> operands = {"", ""};
        std::vector<Expression::Numeric> numerics = {Expression::Numeric(), Expression::Numeric()};
        std::vector<Compiler::OperandType> operandTypes {Compiler::OperandConst, Compiler::OperandConst};

        for(int i=0; i<int(tokens.size()); i++)
        {
            operandTypes[i] = Compiler::parseStaticExpression(codeLineIndex, tokens[i], operands[i], numerics[i]);
            if(operandTypes[i] == Compiler::OperandInvalid)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::POKE() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, tokens[i].c_str(), codeLine._text.c_str());
                return false;
            }
        }

        // ROMvX0
        std::string opcode, operand;
        if(Compiler::getCodeRomType() >= Cpu::ROMvX0  &&  Compiler::getCodeRomType() < Cpu::SDCARD)
        {
            if(operandTypes[1] == Compiler::OperandTemp  &&  (operandTypes[0] == Compiler::OperandVar  ||  operandTypes[0] == Compiler::OperandTemp))
            {
                operand = pokeOperandHelper(operandTypes[0], numerics[0], operands[0]);
                Compiler::emitVcpuAsm("LDW", operands[1], false);
                Compiler::emitVcpuAsm("POKE", operand, false);
            }
            else if(operandTypes[0] == Compiler::OperandTemp  &&  (operandTypes[1] == Compiler::OperandVar  ||  operandTypes[1] == Compiler::OperandTemp))
            {
                operand = pokeOperandHelper(operandTypes[1], numerics[1], operands[1]);
                Compiler::eraseVcpuAsm(codeLineIndex); // erase previous STW to temp var
                Compiler::emitVcpuAsm("POKEA", operand, false);
            }
            else if(operandTypes[0] == Compiler::OperandVar  &&  operandTypes[1] == Compiler::OperandVar)
            {
                opcode = pokeOpcodeHelper(numerics[0]);
                operand = pokeOperandHelper(operandTypes[1], numerics[1], operands[1]);
                Compiler::emitVcpuAsm(opcode, "_" + operands[0], false);
                Compiler::emitVcpuAsm("POKEA", operand, false);
            }
            else if(operandTypes[1] == Compiler::OperandConst  &&  (operandTypes[0] == Compiler::OperandVar  ||  operandTypes[0] == Compiler::OperandTemp))
            {
                opcode = pokeOpcodeHelper(numerics[0]);
                operand = pokeOperandHelper(operandTypes[0], numerics[0], operands[0]);
                Compiler::emitVcpuAsm(opcode, operand, false);
                Compiler::emitVcpuAsm("POKEI", operands[1], false);
            }
            else if(operandTypes[0] == Compiler::OperandConst  &&  (operandTypes[1] == Compiler::OperandVar  ||  operandTypes[1] == Compiler::OperandTemp))
            {
                uint16_t addr;
                operand = pokeOperandHelper(operandTypes[1], numerics[1], operands[1]);
                (Expression::stringToU16(operands[0], addr)  &&  addr < 0x0100) ? Compiler::emitVcpuAsm("LDI", operands[0], false) : Compiler::emitVcpuAsm("LDWI", operands[0], false);
                Compiler::emitVcpuAsm("POKEA", operand, false);
            }
            else
            {
                // Optimise for page 0
                uint16_t addr;
                if(Expression::stringToU16(operands[0], addr)  &&  addr < 0x0100)
                {
                    Compiler::emitVcpuAsm("LDI", operands[1], false);
                    Compiler::emitVcpuAsm("ST",  operands[0], false);
                }
                // All other pages
                else
                {
                    Compiler::emitVcpuAsm("LDWI",  operands[0], false);
                    Compiler::emitVcpuAsm("POKEI", operands[1], false);
                }
            }
        }
        // ROMv1 to ROMv5a
        else
        {
            opcode = pokeOpcodeHelper(numerics[1]);
            operand = pokeOperandHelper(operandTypes[1], numerics[1], operands[1]);

            if((operandTypes[0] == Compiler::OperandVar  ||  operandTypes[0] == Compiler::OperandTemp)  &&  (operandTypes[1] == Compiler::OperandVar  || operandTypes[1] == Compiler::OperandTemp))
            {
                (operandTypes[1] == Compiler::OperandVar) ? Compiler::emitVcpuAsm(opcode, operand, false, codeLineIndex) : Compiler::emitVcpuAsm("LDW", operands[1], false);
                (operandTypes[0] == Compiler::OperandVar) ? Compiler::emitVcpuAsm("POKE", "_" + operands[0], false, codeLineIndex) : Compiler::emitVcpuAsm("POKE", operands[0], false);
            }
            else if((operandTypes[0] == Compiler::OperandVar  ||  operandTypes[0] == Compiler::OperandTemp)  &&  operandTypes[1] == Compiler::OperandConst)
            {
                Compiler::emitVcpuAsm("LDI", operands[1], false);
                (operandTypes[0] == Compiler::OperandVar) ? Compiler::emitVcpuAsm("POKE", "_" + operands[0], false, codeLineIndex) : Compiler::emitVcpuAsm("POKE", operands[0], false);
            }
            else if(operandTypes[0] == Compiler::OperandConst  &&  (operandTypes[1] == Compiler::OperandVar  ||  operandTypes[1] == Compiler::OperandTemp))
            {
                uint16_t addr;
                if(Expression::stringToU16(operands[0], addr)  &&  addr < 0x0100)
                {
                    (operandTypes[1] == Compiler::OperandVar) ? Compiler::emitVcpuAsm(opcode, operand, false, codeLineIndex) : Compiler::emitVcpuAsm("LDW", operands[1], false);
                    Compiler::emitVcpuAsm("ST", operands[0], false);
                }
                else
                {
                    Compiler::emitVcpuAsm("LDWI", operands[0], false);
                    Compiler::emitVcpuAsm("STW", "register0", false);
                    (operandTypes[1] == Compiler::OperandVar) ? Compiler::emitVcpuAsm(opcode, operand, false, codeLineIndex) : Compiler::emitVcpuAsm("LDW", operands[1], false);
                    Compiler::emitVcpuAsm("POKE", "register0", false);
                }
            }
            else
            {
                // Optimise for page 0
                uint16_t addr;
                if(Expression::stringToU16(operands[0], addr)  &&  addr < 0x0100)
                {
                    Compiler::emitVcpuAsm("LDI", operands[1], false);
                    Compiler::emitVcpuAsm("ST",  operands[0], false);
                }
                // All other pages
                else
                {
                    Compiler::emitVcpuAsm("LDWI", operands[0], false);
                    Compiler::emitVcpuAsm("STW",  "register0", false);
                    Compiler::emitVcpuAsm("LDI",  operands[1], false);
                    Compiler::emitVcpuAsm("POKE", "register0", false);
                }
            }
        }

        return true;
    }

    bool DOKE(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(tokenIndex);

        std::vector<std::string> tokens = Expression::tokenise(codeLine._code.substr(foundPos), ',', false);
        if(tokens.size() != 2)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::DOKE() : '%s:%d' : syntax error, use 'DOKE <address>, <value>' : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        std::vector<std::string> operands = {"", ""};
        std::vector<Expression::Numeric> numerics = {Expression::Numeric(), Expression::Numeric()};
        std::vector<Compiler::OperandType> operandTypes {Compiler::OperandConst, Compiler::OperandConst};

        for(int i=0; i<int(tokens.size()); i++)
        {
            operandTypes[i] = Compiler::parseStaticExpression(codeLineIndex, tokens[i], operands[i], numerics[i]);
            if(operandTypes[i] == Compiler::OperandInvalid)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::DOKE() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, tokens[i].c_str(), codeLine._text.c_str());
                return false;
            }
        }

        // ROMvX0
        std::string opcode, operand;
        if(Compiler::getCodeRomType() >= Cpu::ROMvX0  &&  Compiler::getCodeRomType() < Cpu::SDCARD)
        {
            if(operandTypes[1] == Compiler::OperandTemp  &&  (operandTypes[0] == Compiler::OperandVar  ||  operandTypes[0] == Compiler::OperandTemp))
            {
                operand = pokeOperandHelper(operandTypes[0], numerics[0], operands[0]);
                Compiler::emitVcpuAsm("LDW", operands[1], false);
                Compiler::emitVcpuAsm("DOKE", operand, false);
            }
            else if(operandTypes[0] == Compiler::OperandTemp  &&  (operandTypes[1] == Compiler::OperandVar  ||  operandTypes[1] == Compiler::OperandTemp))
            {
                operand = pokeOperandHelper(operandTypes[1], numerics[1], operands[1]);
                Compiler::eraseVcpuAsm(codeLineIndex); // erase previous STW to temp var
                Compiler::emitVcpuAsm("DOKEA", operand, false);
            }
            else if(operandTypes[0] == Compiler::OperandVar  &&  operandTypes[1] == Compiler::OperandVar)
            {
                opcode = pokeOpcodeHelper(numerics[0]);
                operand = pokeOperandHelper(operandTypes[1], numerics[1], operands[1]);
                Compiler::emitVcpuAsm(opcode, "_" + operands[0], false);
                Compiler::emitVcpuAsm("DOKEA", operand, false);
            }
            else if(operandTypes[1] == Compiler::OperandConst  &&  (operandTypes[0] == Compiler::OperandVar  ||  operandTypes[0] == Compiler::OperandTemp))
            {
                opcode = pokeOpcodeHelper(numerics[0]);
                operand = pokeOperandHelper(operandTypes[0], numerics[0], operands[0]);
                Compiler::emitVcpuAsm(opcode, operand, false);
                Compiler::emitVcpuAsm("DOKEI", operands[1], false);
            }
            else if(operandTypes[0] == Compiler::OperandConst  &&  (operandTypes[1] == Compiler::OperandVar  ||  operandTypes[1] == Compiler::OperandTemp))
            {
                uint16_t addr;
                operand = pokeOperandHelper(operandTypes[1], numerics[1], operands[1]);
                (Expression::stringToU16(operands[0], addr)  &&  addr < 0x0100) ? Compiler::emitVcpuAsm("LDI", operands[0], false) : Compiler::emitVcpuAsm("LDWI", operands[0], false);
                Compiler::emitVcpuAsm("DOKEA", operand, false);
            }
            else
            {
                // Optimise for page 0
                uint16_t addr;
                if(Expression::stringToU16(operands[0], addr)  &&  addr < 0x0100)
                {
                    Compiler::emitVcpuAsm("LDI", operands[1], false);
                    Compiler::emitVcpuAsm("STW",  operands[0], false);
                }
                // All other pages
                else
                {
                    Compiler::emitVcpuAsm("LDWI",  operands[0], false);
                    Compiler::emitVcpuAsm("DOKEI", operands[1], false);
                }
            }
        }
        // ROMv1 to ROMv5a
        else
        {
            opcode = pokeOpcodeHelper(numerics[1]);
            operand = pokeOperandHelper(operandTypes[1], numerics[1], operands[1]);

            if((operandTypes[0] == Compiler::OperandVar  ||  operandTypes[0] == Compiler::OperandTemp)  &&  (operandTypes[1] == Compiler::OperandVar  ||  operandTypes[1] == Compiler::OperandTemp))
            {
                (operandTypes[1] == Compiler::OperandVar) ? Compiler::emitVcpuAsm(opcode, operand, false, codeLineIndex) : Compiler::emitVcpuAsm("LDW", "" + operands[1], false);
                (operandTypes[0] == Compiler::OperandVar) ? Compiler::emitVcpuAsm("DOKE", "_" + operands[0], false, codeLineIndex) : Compiler::emitVcpuAsm("DOKE", "" + operands[0], false);
            }
            else if((operandTypes[0] == Compiler::OperandVar  ||  operandTypes[0] == Compiler::OperandTemp)  &&  operandTypes[1] == Compiler::OperandConst)
            {
                Compiler::emitVcpuAsm("LDWI", operands[1], false);
                (operandTypes[0] == Compiler::OperandVar) ? Compiler::emitVcpuAsm("DOKE", "_" + operands[0], false, codeLineIndex) : Compiler::emitVcpuAsm("DOKE", "" + operands[0], false);
            }
            else if(operandTypes[0] == Compiler::OperandConst  &&  (operandTypes[1] == Compiler::OperandVar  ||  operandTypes[1] == Compiler::OperandTemp))
            {
                uint16_t addr;
                if(Expression::stringToU16(operands[0], addr)  &&  addr < 0x0100)
                {
                    (operandTypes[1] == Compiler::OperandVar) ? Compiler::emitVcpuAsm(opcode, operand, false, codeLineIndex) : Compiler::emitVcpuAsm("LDW", "" + operands[1], false);
                    Compiler::emitVcpuAsm("STW", operands[0], false);
                }
                else
                {
                    Compiler::emitVcpuAsm("LDWI", operands[0], false);
                    Compiler::emitVcpuAsm("STW", "register0", false);
                    (operandTypes[1] == Compiler::OperandVar) ? Compiler::emitVcpuAsm(opcode, operand, false, codeLineIndex) : Compiler::emitVcpuAsm("LDW", "" + operands[1], false);
                    Compiler::emitVcpuAsm("DOKE", "register0", false);
                }
            }
            else
            {
                // Optimise for page 0
                uint16_t addr;
                if(Expression::stringToU16(operands[0], addr)  &&  addr < 0x0100)
                {
                    Compiler::emitVcpuAsm("LDWI", operands[1], false);
                    Compiler::emitVcpuAsm("STW",  operands[0], false);
                }
                // All other pages
                else
                {
                    Compiler::emitVcpuAsm("LDWI", operands[0], false);
                    Compiler::emitVcpuAsm("STW",  "register0", false);
                    Compiler::emitVcpuAsm("LDWI", operands[1], false);
                    Compiler::emitVcpuAsm("DOKE", "register0", false);
                }
            }
        }

        return true;
    }

    void timeInit(bool noReset)
    {
        // Init time proc
        Compiler::setCreateTimeData(true);

        if(noReset) return;

        // ROM's 1 to 4
        if(Compiler::getCodeRomType() < Cpu::ROMv5a)
        {
            Compiler::emitVcpuAsm("LDI", "0",               false);
            Compiler::emitVcpuAsm("STW", "timerTick",       false);
            Compiler::emitVcpuAsm("ST",  "timerJiff",       false);
            Compiler::emitVcpuAsm("LD",  "giga_frameCount", false);
            Compiler::emitVcpuAsm("ST",  "timerJiff + 1",   false);
        }
        // ROMv5a and friends
        else if((Compiler::getCodeRomType() >= Cpu::ROMv5a  &&  Compiler::getCodeRomType() < Cpu::ROMvX0)  ||  Compiler::getCodeRomType() >= Cpu::SDCARD)
        {
            Compiler::emitVcpuAsm("LDI", "0",         false);
            Compiler::emitVcpuAsm("STW", "timerTick", false);
            Compiler::emitVcpuAsm("STW", "timerJiff", false);
        }
        // ROMvX0
        else if(Compiler::getCodeRomType() >= Cpu::ROMvX0  &&  Compiler::getCodeRomType() < Cpu::SDCARD)
        {
            Compiler::emitVcpuAsm("LDI", "0",                  false);
            Compiler::emitVcpuAsm("ST",  "giga_jiffiesTick",   false);
            Compiler::emitVcpuAsm("STW", "giga_secondsTickLo", false);
        }
    }
    void midiInit(void)
    {
        _midiType = Midi;
    }
    void midivInit(void)
    {
        _midiType = MidiV;
    }
    void userInit(const std::string& label)
    {
        _userRoutine = label;
    }
    void timeAddr(int index)
    {
        Compiler::emitVcpuAsm("LDWI", "tickTime",                             false);
        Compiler::emitVcpuAsm("STW",  "realTimeProc" + std::to_string(index), false);
    }
    void midiAddr(int index)
    {
        Compiler::emitVcpuAsm("LDWI", "playMidi",                             false);
        Compiler::emitVcpuAsm("STW",  "realTimeProc" + std::to_string(index), false);
    }
    void midivAddr(int index)
    {
        Compiler::emitVcpuAsm("LDWI", "playMidiVol",                          false);
        Compiler::emitVcpuAsm("STW",  "realTimeProc" + std::to_string(index), false);
    }
    void userAddr(const std::string& label, int index)
    {
        Compiler::emitVcpuAsm("LDWI", "_" + label,                            false);
        Compiler::emitVcpuAsm("STW",  "realTimeProc" + std::to_string(index), false);
    }
    void usageINIT(Compiler::CodeLine& codeLine, int codeLineStart)
    {
        Cpu::reportError(Cpu::KwdError, stderr, "Keywords::INIT() : '%s:%d' : syntax error, use 'INIT TIME, MIDI/MIDIV, <user proc>, NOUPDATE, NORESET : %s\n",
                                                codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
    }
    bool INIT(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(tokenIndex);

        static std::map<std::string, InitTypes> initTypesMap = {{"TIME", InitTime}, {"MIDI", InitMidi}, {"MIDIV", InitMidiV}};

        std::vector<std::string> tokens = Expression::tokenise(codeLine._code.substr(foundPos), ',', false);
        if(tokens.size() < 1  ||  tokens.size() > 4)
        {
            usageINIT(codeLine, codeLineStart);
            return false;
        }

        if(tokens.size() == 1  ||  Compiler::getCodeRomType() > Cpu::ROMv4)
        {
            for(int i=0; i<int(tokens.size()); i++)
            {
                Expression::stripWhitespace(tokens[i]);
                std::string token = tokens[i];
                Expression::strToUpper(token);
                if(token == "NOUPDATE")
                {
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::INIT() : '%s:%d' : syntax error, 'NOUPDATE' must be used with 'INIT TIME, MIDI/MIDIV, <user proc>' and only on ROMv4 or lower : %s\n",
                                                            codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
                    return false;
                }
            }
        }

        // Search for init types and labels
        bool noUpdate = false;
        bool noReset = false;
        std::vector<InitTypes> initTypes;
        for(int i=0; i<int(tokens.size()); i++)
        {
            Expression::stripWhitespace(tokens[i]);
            std::string token = tokens[i];
            Expression::strToUpper(token);

            // Init type not found, so search for label
            if(initTypesMap.find(token) == initTypesMap.end())
            {
                if(token == "NOUPDATE")
                {
                    noUpdate = true;
                    continue;
                }

                if(token == "NORESET")
                {
                    noReset = true;
                    continue;
                }

                int labIndex = Compiler::findLabel(tokens[i]);
                if(labIndex == -1)
                {
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::INIT() : '%s:%d' : syntax error, label '%s' not found : %s\n",
                                                            codeLine._moduleName.c_str(), codeLineStart, tokens[i].c_str(), codeLine._text.c_str());
                    return false;
                }
                initTypes.push_back(InitUser);
            }
            else
            {
                if(initTypesMap[token] == InitMidi  ||  initTypesMap[token] == InitMidiV)
                {
                    if(_midiType != MidiNone)
                    {
                        Cpu::reportError(Cpu::KwdError, stderr, "Keywords::INIT() : '%s:%d' : syntax error, can only init one instance of MIDI or MIDIV, use 'INIT TIME, MIDI/MIDIV, <user proc> : %s\n",
                                                                codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
                        return false;
                    }
                }
                initTypes.push_back(initTypesMap[token]);
            }
        }

        // Proc and addr init
        for(int i=0; i<int(initTypes.size()); i++)
        {
            switch(initTypes[i])
            {
                case InitTime:  timeInit(noReset);   timeAddr(i);            break;
                case InitMidi:  midiInit();          midiAddr(i);            break;
                case InitMidiV: midivInit();         midivAddr(i);           break;
                case InitUser:  userInit(tokens[i]); userAddr(tokens[i], i); break;

                default: break;
            }
        }

        if(initTypes.size())
        {
            // ROM's 1 to 4, (time sliced code)
            if(Compiler::getCodeRomType() < Cpu::ROMv5a)
            {
                // If 'NOUPDATE" is specified then user must call tick himself
                if(!noUpdate)
                {
                    Compiler::enableSysInitFunc("InitRealTimeStub");
                    Linker::setInternalSubToLoad("realTimeStub");
                    Compiler::emitVcpuPreProcessingCmd("%define TIME_SLICING");
                    Compiler::emitVcpuAsm("LDWI", "setRealTimeProc" + std::to_string(initTypes.size() - 1), false);
                    Compiler::emitVcpuAsm("CALL", "giga_vAC",                                               false);
                }
            }
            // ROMv5a and higher, (vertical blank interrupt)
            else
            {
                Compiler::emitVcpuPreProcessingCmd("%define VBLANK_INTERRUPT");

                // Vertical blank interrupt uses 0x30-0x33 for vPC and vAC save/restore, so we must move any variables found there
                if(!Compiler::moveVblankVars(codeLine, codeLineIndex)) return false;

                // Build chain of vertical blank interrupt handlers, (up to 3)
                Compiler::emitVcpuAsm("CALLI", "setRealTimeProc" + std::to_string(initTypes.size() - 1), false);

                // ROMvX0
                if(Compiler::getCodeRomType() >= Cpu::ROMvX0  &&  Compiler::getCodeRomType() < Cpu::SDCARD)
                {
                    Compiler::emitVcpuAsm("MOVQB",  "giga_frameCount, 0xFF", false);
                }
                // ROMv5a/DEVROM
                else
                {
                    Compiler::emitVcpuAsm("LDI",  "0xFF"           , false);
                    Compiler::emitVcpuAsm("ST",   "giga_frameCount", false);
                }

                // Start vertical blank interrupt
                Compiler::emitVcpuAsm("LDWI", "giga_vblankProc", false);
                Compiler::emitVcpuAsm("STW",  "register0"      , false);
                Compiler::emitVcpuAsm("LDWI", "realTimeStub"   , false);
                Compiler::emitVcpuAsm("DOKE", "register0"      , false);
            }
        }

        return true;
    }

    void usageTICK(Compiler::CodeLine& codeLine, int codeLineStart)
    {
        Cpu::reportError(Cpu::KwdError, stderr, "Keywords::INIT() : '%s:%d' : syntax error, use one or more of, 'TICK TIME, MIDI, MIDIV, USER' : %s\n", codeLine._moduleName.c_str(),
                                                                                                                                                        codeLineStart, codeLine._text.c_str());
    }
    bool TICK(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(tokenIndex);
        UNREFERENCED_PARAM(codeLineIndex);

        if(Compiler::getCodeRomType() > Cpu::ROMv4)
        {
            std::string romTypeStr;
            getRomTypeStr(Compiler::getCodeRomType(), romTypeStr);
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::TICK() : '%s:%d' : version error, 'TICK' requires ROMv4 or lower, you are trying to link against '%s' : %s\n",
                                                    codeLine._moduleName.c_str(), codeLineStart, romTypeStr.c_str(), codeLine._text.c_str());
            return false;
        }

        std::vector<std::string> tokens = Expression::tokenise(codeLine._code.substr(foundPos), ',', false);
        if(tokens.size() < 1  ||  tokens.size() > 4)
        {
            usageTICK(codeLine, codeLineStart);
            return false;
        }

        std::map<std::string, InitTypes> initTypesMap;
        for(int i=0; i<int(tokens.size()); i++)
        {
            std::string token = Expression::strToUpper(tokens[i]);
            Expression::stripWhitespace(token);
            if(initTypesMap.find(token) == initTypesMap.end())
            {
                if(token == "TIME")
                {
                    if(!Compiler::getCreateTimeData())
                    {
                        Cpu::reportError(Cpu::KwdError, stderr, "Keywords::TICK() : '%s:%d' : TIME not initialised using INIT : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
                        return false;
                    }

                    initTypesMap[token] = InitTime;
                    Compiler::emitVcpuAsm("%TickTime", "", false);
                }
                else if(token == "MIDI")
                {
                    if(_midiType != Midi)
                    {
                        Cpu::reportError(Cpu::KwdError, stderr, "Keywords::TICK() : '%s:%d' : MIDI not initialised using INIT : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
                        return false;
                    }

                    initTypesMap[token] = InitMidi;
                    Compiler::emitVcpuAsm("%TickMidi", "", false);
                }
                else if(token == "MIDIV")
                {
                    if(_midiType != MidiV)
                    {
                        Cpu::reportError(Cpu::KwdError, stderr, "Keywords::TICK() : '%s:%d' : MIDIV not initialised using INIT : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
                        return false;
                    }

                    initTypesMap[token] = InitMidiV;
                    Compiler::emitVcpuAsm("%TickMidiV", "", false);
                }
                else if(token == "USER")
                {
                    if(_userRoutine == "")
                    {
                        Cpu::reportError(Cpu::KwdError, stderr, "Keywords::TICK() : '%s:%d' : USER routine not initialised using INIT : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
                        return false;
                    }

                    initTypesMap[token] = InitUser;
                    if(Compiler::getCodeRomType() < Cpu::ROMv5a)
                    {
                        Compiler::emitVcpuAsm("LDWI", "_" + _userRoutine, false);
                        Compiler::emitVcpuAsm("CALL", "giga_vAC",         false);
                    }
                    else
                    {
                        Compiler::emitVcpuAsm("CALLI", "_" + _userRoutine, false);
                    }
                }
                else
                {
                    usageTICK(codeLine, codeLineStart);
                    return false;
                }
            }
            else
            {
                usageTICK(codeLine, codeLineStart);
                return false;
            }
        }

        return true;
    }

    void usagePLAY(Compiler::CodeLine& codeLine, int codeLineStart)
    {
        Cpu::reportError(Cpu::KwdError, stderr, "Keywords::PLAY() : '%s:%d' : syntax error, use 'PLAY <TYPE>, <id/address>, <optional waveType>', where <TYPE> = 'MIDI', 'MIDID', 'MIDIV', 'MIDIDV' or 'MUSIC' : %s\n",
                                                codeLine._moduleName.c_str(),  codeLineStart, codeLine._text.c_str());
    }
    bool PLAY(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(tokenIndex);

        std::vector<std::string> tokens = Expression::tokenise(codeLine._code.substr(foundPos), ',', false);
        if(tokens.size() < 2  ||  tokens.size() > 3)
        {
            usagePLAY(codeLine, codeLineStart);
            return false;
        }

        // Default wave type
        if(tokens.size() == 2)
        {
            Compiler::emitVcpuAsm("LDI", "2",           false);
            Compiler::emitVcpuAsm("ST", "waveType + 1", false);
        }
        // Midi wave type, (optional)
        else if(tokens.size() == 3)
        {
            std::string waveTypeToken = tokens[2];
            Expression::Numeric waveTypeNumeric;
            if(Compiler::parseExpression(codeLineIndex, waveTypeToken, waveTypeNumeric) == Expression::IsInvalid)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::PLAY() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, waveTypeToken.c_str(), codeLine._text.c_str());
                return false;
            }
            Compiler::emitVcpuAsm("ST", "waveType + 1", false);
        }

        // Midi stream address
        std::string addressToken = tokens[1];
        Expression::Numeric addressNumeric;
        if(Compiler::parseExpression(codeLineIndex, tokens[1], addressNumeric) == Expression::IsInvalid)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::PLAY() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, tokens[1].c_str(), codeLine._text.c_str());
            return false;
        }

        std::string midiToken = Expression::strToUpper(tokens[0]);
        Expression::stripWhitespace(midiToken);
        if(midiToken == "MIDI")
        {
            Compiler::emitVcpuAsm("%PlayMidi", "", false);
            return true;
        }

        if(midiToken == "MIDID")
        {
            Compiler::emitVcpuAsm("%SetMidiStream", "", false);
            Compiler::emitVcpuAsm("%PlayMidi", "",      false);
            return true;
        }
        
        if(midiToken == "MIDIV")
        {
            Compiler::emitVcpuAsm("%PlayMidiV", "", false);
            return true;
        }

        if(midiToken == "MIDIDV")
        {
            Compiler::emitVcpuAsm("%SetMidiStream", "", false);
            Compiler::emitVcpuAsm("%PlayMidiV", "",     false);
            return true;
        }

        if(midiToken == "MUSIC")
        {
            Compiler::emitVcpuAsm("%PlayMusic", "", false);
            return true;
        }

        usagePLAY(codeLine, codeLineStart);
        return false;
    }

    bool LOAD(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(tokenIndex);

        std::vector<std::string> tokens = Expression::tokenise(codeLine._code.substr(foundPos), ',', false);
        if(tokens.size() < 2  ||  tokens.size() > 5)
        {
            Load::loadUsage(Load::LoadType, codeLine, codeLineStart);
            return false;
        }

        // Type
        Expression::strToUpper(tokens[0]);
        Expression::stripWhitespace(tokens[0]);

        // Load RAW
        if(tokens[0] == "RAW")
        {
            return Load::loadRaw(codeLine, codeLineIndex, codeLineStart, tokens);
        }

        // Load WAVE
        if(tokens[0] == "WAVE")
        {
            return Load::loadWave(codeLine, codeLineIndex, codeLineStart, tokens);
        }

        // Load Midi
        if(tokens[0] == "MIDI")
        {
            return Load::loadMidi(codeLine, codeLineIndex, codeLineStart, tokens);
        }

        // Load Image, Blit and Font
        if(tokens[0] == "IMAGE"  ||  tokens[0] == "BLIT"  ||  tokens[0] == "FONT"  ||  tokens[0] == "SPRITE"  ||  tokens[0] == "PATTERN")
        {
            std::string filename = tokens[1];
            Expression::stripWhitespace(filename);
            std::string ext = filename;
            Expression::strToUpper(ext);
            if(ext.find(".TGA") != std::string::npos)
            {
                std::string filepath = Loader::getFilePath();
                size_t slash = filepath.find_last_of("\\/");
                filepath = (slash != std::string::npos) ? filepath.substr(0, slash) : ".";
                filename = filepath + "/" + filename;
                Image::TgaFile tgaFile;

                //Cpu::reportError(Cpu::NoError, stderr, "\nKeywords::LOAD() : %s\n", filename.c_str());

                if(!Image::loadTgaFile(filename, tgaFile))
                {
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::LOAD() : '%s:%d' : file '%s' failed to load : %s\n", codeLine._moduleName.c_str(), codeLineStart, filename.c_str(), codeLine._text.c_str());
                    return false;
                }

                // Load image/blit/font/sprite/pattern
                std::vector<uint8_t> data;
                std::vector<uint16_t> optional;
                Image::GtRgbFile gtRgbFile{{GTRGB_IDENTIFIER, Image::GT_RGB_222, tgaFile._header._width, tgaFile._header._height}, data, optional};

                if(tokens[0] == "IMAGE")   return Load::loadImage(codeLine, codeLineIndex, codeLineStart, tokens, filename, tgaFile, gtRgbFile);
                if(tokens[0] == "BLIT")    return Load::loadBlit(codeLine, codeLineIndex, codeLineStart, tokens, filename, tgaFile, gtRgbFile);
                if(tokens[0] == "FONT")    return Load::loadFont(codeLine, codeLineIndex, codeLineStart, tokens, filename, tgaFile, gtRgbFile);
                if(tokens[0] == "SPRITE")  return Load::loadSprite(codeLine, codeLineIndex, codeLineStart, tokens, filename, tgaFile, gtRgbFile);
                if(tokens[0] == "PATTERN") return Load::loadPattern(codeLine, codeLineIndex, codeLineStart, tokens, filename, tgaFile, gtRgbFile);
            }
        }

        Load::loadUsage(Load::LoadType, codeLine, codeLineStart);
        return false;
    }

    bool BLIT(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(tokenIndex);

        if(Compiler::getCodeRomType() < Cpu::ROMv3)
        {
            std::string romTypeStr;
            getRomTypeStr(Compiler::getCodeRomType(), romTypeStr);
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::BLIT() : '%s:%d' : version error, 'BLIT' requires ROMv3 or higher, you are trying to link against '%s' : %s\n",
                                                    codeLine._moduleName.c_str(), codeLineStart, romTypeStr.c_str(), codeLine._text.c_str());
            return false;
        }

        std::vector<std::string> tokens = Expression::tokenise(codeLine._code.substr(foundPos), ',', false);
        if(tokens.size() != 4)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::BLIT() : '%s:%d' : syntax error, use 'BLIT <NOFLIP/FLIPX/FLIPY/FLIPXY>, <id>, <x pos>, <y pos>' : %s\n",
                                                    codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        // Flip type
        static std::map<std::string, Compiler::BlitFlipType> flipType = {{"NOFLIP", Compiler::NoFlip}, {"FLIPX", Compiler::FlipX}, {"FLIPY", Compiler::FlipY}, {"FLIPXY", Compiler::FlipXY}};
        std::string flipToken = tokens[0];
        Expression::stripWhitespace(flipToken);
        Expression::strToUpper(flipToken);
        if(flipType.find(flipToken) == flipType.end())
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::BLIT() : '%s:%d' : syntax error, use one of the correct flip types, 'BLIT <NOFLIP/FLIPX/FLIPY/FLIPXY>, <id>, <x pos>, <y pos>' : %s\n",
                                                    codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        // Blit identifier
        std::string idToken = tokens[1];
        Expression::Numeric idParam;
        if(Compiler::parseExpression(codeLineIndex, idToken, idParam) == Expression::IsInvalid)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::BLIT() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, idToken.c_str(), codeLine._text.c_str());
            return false;
        }
        Compiler::emitVcpuAsm("STW", "blitId", false);

        // Blit X position
        std::string xposToken = tokens[2];
        Expression::Numeric xposParam;
        if(Compiler::parseExpression(codeLineIndex, xposToken, xposParam) == Expression::IsInvalid)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::BLIT() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, xposToken.c_str(), codeLine._text.c_str());
            return false;
        }
        Compiler::emitVcpuAsm("ST", "blitXY", false);

        // Blit Y position
        std::string yposToken = tokens[3];
        Expression::Numeric yposParam;
        if(Compiler::parseExpression(codeLineIndex, yposToken, yposParam) == Expression::IsInvalid)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::BLIT() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, yposToken.c_str(), codeLine._text.c_str());
            return false;
        }
        Compiler::emitVcpuAsm("ADDI", "8", false);
        Compiler::emitVcpuAsm("ST", "blitXY + 1", false);

        // Draw blit
        switch(flipType[flipToken])
        {
            case Compiler::NoFlip: Compiler::emitVcpuAsm("%DrawBlit",   "", false); break;
            case Compiler::FlipX:  Compiler::emitVcpuAsm("%DrawBlitX",  "", false); break;
            case Compiler::FlipY:  Compiler::emitVcpuAsm("%DrawBlitY",  "", false); break;
            case Compiler::FlipXY: Compiler::emitVcpuAsm("%DrawBlitXY", "", false); break;
        }
 
        return true;
    }

    bool SPRITE(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(tokenIndex);

        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD)
        {
            std::string romTypeStr;
            getRomTypeStr(Compiler::getCodeRomType(), romTypeStr);
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::SPRITE() : '%s:%d' : version error, 'SPRITE' requires ROMvX0, you are trying to link against '%s' : %s\n",
                                                    codeLine._moduleName.c_str(), codeLineStart, romTypeStr.c_str(), codeLine._text.c_str());
            return false;
        }

        std::vector<std::string> tokens = Expression::tokenise(codeLine._code.substr(foundPos), ',', false);
        if(tokens.size() < 3  ||  tokens.size() > 5)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::SPRITE() : '%s:%d' : syntax error, use 'SPRITE <id>, <x pos>, <y pos>, <pattern id>, <enable>' : %s\n",
                                                    codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::SPRITE()                           use 'SPRITE MOVE, <id>, <x pos>, <y pos>'\n");
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::SPRITE()                           use 'SPRITE PATTERN, <id>, <pattern id>'\n");
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::SPRITE()                           use 'SPRITE ENABLE, <id>, <enable>'\n");
            return false;
        }

        // Sprite command
        static std::map<std::string, Compiler::SpriteCmd> spriteCmdMap = {{"MOVE", Compiler::SpriteMove}, {"PATTERN", Compiler::SpritePattern}, {"SHOW", Compiler::SpriteShow}};
        std::string cmdToken = tokens[0];
        Expression::stripWhitespace(cmdToken);
        Expression::strToUpper(cmdToken);
        auto it = spriteCmdMap.find(cmdToken);
        if(it != spriteCmdMap.end())
        {
            // Sprite identifier
            std::string idToken = tokens[1];
            Expression::Numeric idParam;
            if(Compiler::parseExpression(codeLineIndex, idToken, idParam) == Expression::IsInvalid)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::SPRITE() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, idToken.c_str(), codeLine._text.c_str());
                return false;
            }
            Compiler::emitVcpuAsm("STW", "spriteId", false);

            Compiler::SpriteCmd spriteCmd = it->second;
            switch(spriteCmd)
            {
                case Compiler::SpriteMove:
                {
                    // Sprite X position
                    std::string xposToken = tokens[2];
                    Expression::Numeric xposParam;
                    if(Compiler::parseExpression(codeLineIndex, xposToken, xposParam) == Expression::IsInvalid)
                    {
                        Cpu::reportError(Cpu::KwdError, stderr, "Keywords::SPRITE() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, xposToken.c_str(), codeLine._text.c_str());
                        return false;
                    }
                    Compiler::emitVcpuAsm("ST", "spriteXY", false);

                    // Sprite Y position
                    std::string yposToken = tokens[3];
                    Expression::Numeric yposParam;
                    if(Compiler::parseExpression(codeLineIndex, yposToken, yposParam) == Expression::IsInvalid)
                    {
                        Cpu::reportError(Cpu::KwdError, stderr, "Keywords::SPRITE() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, yposToken.c_str(), codeLine._text.c_str());
                        return false;
                    }
                    //Compiler::emitVcpuAsm("ADDI", "8", false);
                    Compiler::emitVcpuAsm("ST", "spriteXY + 1", false);
                    Compiler::emitVcpuAsm("%MoveSprite", "", false);
                    return true;
                }
                break;

                case Compiler::SpritePattern:
                {
                    // Pattern identifier
                    std::string patternToken = tokens[2];
                    Expression::Numeric patternParam;
                    if(Compiler::parseExpression(codeLineIndex, patternToken, patternParam) == Expression::IsInvalid)
                    {
                        Cpu::reportError(Cpu::KwdError, stderr, "Keywords::SPRITE() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, patternToken.c_str(), codeLine._text.c_str());
                        return false;
                    }
                    Compiler::emitVcpuAsm("STW", "spatternId", false);
                    Compiler::emitVcpuAsm("LDI", "_SH/2", false);
                    Compiler::emitVcpuAsm("ST", "giga_sysArg3", false);
                    Compiler::emitVcpuAsm("%AnimateSprite", "", false);
                    return true;
                }
                break;

                case Compiler::SpriteShow:
                {
                    // Show/Hide
                    std::string showToken = tokens[2];
                    Expression::Numeric showParam;
                    if(Compiler::parseExpression(codeLineIndex, showToken, showParam) == Expression::IsInvalid)
                    {
                        Cpu::reportError(Cpu::KwdError, stderr, "Keywords::SPRITE() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, showToken.c_str(), codeLine._text.c_str());
                        return false;
                    }
                    Compiler::emitVcpuAsm("CONDII", "0x80, 0x00", false);
                    Compiler::emitVcpuAsm("ST", "spriteEnable", false);
                    Compiler::emitVcpuAsm("%EnableSprite", "", false);
                    return true;
                }
                break;

                default: break;
            }
        }

        // Sprite identifier
        std::string idToken = tokens[0];
        Expression::Numeric idParam;
        if(Compiler::parseExpression(codeLineIndex, idToken, idParam) == Expression::IsInvalid)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::SPRITE() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, idToken.c_str(), codeLine._text.c_str());
            return false;
        }
        Compiler::emitVcpuAsm("STW", "spriteId", false);

        // Sprite X position
        std::string xposToken = tokens[1];
        Expression::Numeric xposParam;
        if(Compiler::parseExpression(codeLineIndex, xposToken, xposParam) == Expression::IsInvalid)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::SPRITE() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, xposToken.c_str(), codeLine._text.c_str());
            return false;
        }
        Compiler::emitVcpuAsm("ST", "spriteXY", false);

        // Sprite Y position
        std::string yposToken = tokens[2];
        Expression::Numeric yposParam;
        if(Compiler::parseExpression(codeLineIndex, yposToken, yposParam) == Expression::IsInvalid)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::SPRITE() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, yposToken.c_str(), codeLine._text.c_str());
            return false;
        }
        Compiler::emitVcpuAsm("ADDI", "8", false);
        Compiler::emitVcpuAsm("ST", "spriteXY + 1", false);
        Compiler::emitVcpuAsm("%MoveSprite", "", false);
        return true;
    }

    bool SPRITES(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(tokenIndex);
        UNREFERENCED_PARAM(codeLineIndex);

        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD)
        {
            std::string romTypeStr;
            getRomTypeStr(Compiler::getCodeRomType(), romTypeStr);
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::SPRITES() : '%s:%d' : version error, 'SPRITES' requires ROMvX0, you are trying to link against '%s' : %s\n",
                                                    codeLine._moduleName.c_str(), codeLineStart, romTypeStr.c_str(), codeLine._text.c_str());
            return false;
        }

        std::vector<std::string> tokens = Expression::tokenise(codeLine._code.substr(foundPos), ',', false);
        if(tokens.size() < 1  ||  tokens.size() > 2)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::SPRITES() : '%s:%d' : syntax error, use 'SPRITES DRAW, <optional height>' : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::SPRITES()                           use 'SPRITES RESTORE, <optional height>'\n");
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::SPRITES()                           use 'SPRITES ENABLE, <enable>'\n");
            return false;
        }

        // Sprites command
        static std::map<std::string, Compiler::SpritesCmd> spritesCmdMap = {{"INIT", Compiler::SpritesInit}, {"DRAW", Compiler::SpritesDraw}, {"RESTORE", Compiler::SpritesRestore}, {"SHOW", Compiler::SpritesShow}};
        std::string cmdToken = tokens[0];
        Expression::stripWhitespace(cmdToken);
        Expression::strToUpper(cmdToken);
        auto it = spritesCmdMap.find(cmdToken);
        if(it == spritesCmdMap.end())
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::SPRITES() : '%s:%d' : syntax error in : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        Compiler::SpritesCmd spritesCmd = it->second;
        switch(spritesCmd)
        {
            case Compiler::SpritesInit:
            {
                Compiler::emitVcpuAsm("LDI", std::to_string(uint8_t(Compiler::getDefDataSprites().size())), false);
                Compiler::emitVcpuAsm("ST", "spritesCount", false);
                Compiler::emitVcpuAsm("%InitSprites", "", false);
            }
            break;

            case Compiler::SpritesDraw:
            {
                Compiler::emitVcpuAsm("LDI", std::to_string(uint8_t(Compiler::getDefDataSprites().size())), false);
                Compiler::emitVcpuAsm("ST", "spritesCount", false);
                Compiler::emitVcpuAsm("%SortSpritesLut", "", false);

                if(tokens.size() == 1)
                {
                    Compiler::emitVcpuAsm("LDI", std::to_string(uint8_t(Compiler::getDefDataSprites().size())), false);
                    Compiler::emitVcpuAsm("ST", "spritesCount", false);
                    Compiler::emitVcpuAsm("%DrawSprites", "", false);
                }
                // Optional constant height
                else
                {
                    //std::string heightToken = tokens[1];
                    //Expression::Numeric heightParam;
                    //if(Compiler::parseExpression(codeLineIndex, heightToken, heightParam) == Expression::IsInvalid)
                    //{
                    //    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::SPRITES() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, heightToken.c_str(), codeLine._text.c_str());
                    //    return false;
                    //}
                    //Compiler::emitVcpuAsm("ST", "spritesCntHt + 1", false);

                    Compiler::emitVcpuAsm("LDI", std::to_string(uint8_t(Compiler::getDefDataSprites().size())), false);
                    Compiler::emitVcpuAsm("ST", "spritesCount", false);
                    Compiler::emitVcpuAsm("%DrawSpritesH", "", false);
                }
            }
            break;

            case Compiler::SpritesRestore:
            {
                Compiler::emitVcpuAsm("LDI", std::to_string(uint8_t(Compiler::getDefDataSprites().size())), false);
                Compiler::emitVcpuAsm("ST", "spritesCount", false);
                Compiler::emitVcpuAsm("%SortSprites", "", false);

                // Optional constant height
                if(tokens.size() == 2)
                {
                    //std::string heightToken = tokens[1];
                    //Expression::Numeric heightParam;
                    //if(Compiler::parseExpression(codeLineIndex, heightToken, heightParam) == Expression::IsInvalid)
                    //{
                    //    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::SPRITES() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, heightToken.c_str(), codeLine._text.c_str());
                    //    return false;
                    //}
                    //Compiler::emitVcpuAsm("STW", "giga_sysArg1", false);

                    Compiler::emitVcpuAsm("LDI", std::to_string(uint8_t(Compiler::getDefDataSprites().size())), false);
                    Compiler::emitVcpuAsm("ST", "spritesCount", false);
                    Compiler::emitVcpuAsm("LDWI", "_spritesTmpLut_ - 2 + " + std::to_string(uint8_t(Compiler::getDefDataSprites().size())*4), false);
                    Compiler::emitVcpuAsm("%RestoreSpritesH", "", false);
                }
            }
            break;

            case Compiler::SpritesShow:
            {
            }
            break;

            default:
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::SPRITES() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, cmdToken.c_str(), codeLine._text.c_str());
                return false;
            }
            break;
        }

        return true;
    }

    void usageSOUND(int msgType, Compiler::CodeLine& codeLine, int codeLineStart)
    {
        switch(msgType)
        {
            case 0: Cpu::reportError(Cpu::KwdError, stderr, "Keywords::SOUND() : '%s:%d' : syntax error, use 'SOUND <TYPE>, <params>, where <TYPE> = 'ON', 'MOD' or 'OFF' : %s\n",
                                                            codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str()); break;
            case 1: Cpu::reportError(Cpu::KwdError, stderr, "Keywords::SOUND() : '%s:%d' : syntax error, use 'SOUND ON, <channel>, <frequency>, <optional volume>, <optional waveform>' : %s\n",
                                                            codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str()); break;
            case 2: Cpu::reportError(Cpu::KwdError, stderr, "Keywords::SOUND() : '%s:%d' : syntax error, use 'SOUND MOD, <channel>, <wavX>, <optional wavA>' : %s\n",
                                                            codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str()); break;
            case 3: Cpu::reportError(Cpu::KwdError, stderr, "Keywords::SOUND() : '%s:%d' : syntax error, use 'SOUND OFF, <optional channel>' : %s\n",
                                                            codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str()); break;
            default: break;
        }
    }
    bool SOUND(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(tokenIndex);

        std::vector<std::string> tokens = Expression::tokenise(codeLine._code.substr(foundPos), ',', false);
        if(tokens.size() < 1  ||  tokens.size() > 5)
        {
            usageSOUND(0, codeLine, codeLineStart);
            return false;
        }

        // Sound state
        std::string stateToken = tokens[0];
        Expression::stripWhitespace(stateToken);
        Expression::strToUpper(stateToken);

        // ROMvX0
        if(Compiler::getCodeRomType() >= Cpu::ROMvX0  &&  Compiler::getCodeRomType() < Cpu::SDCARD)
        {
            // SOUND OFF, <constant/literal>
            if(tokens.size() == 2  &&  stateToken == "OFF")
            {
                int varIndex = -1, constIndex = -1, strIndex = -1;
                std::string chanToken = tokens[1];
                Expression::stripWhitespace(chanToken);
                uint32_t expressionType = Compiler::isExpression(chanToken, varIndex, constIndex, strIndex);

                // Constant channel
                if((expressionType & Expression::HasIntConsts)  &&  constIndex > -1)
                {
                    std::string operand;
                    Expression::Numeric numeric(true); // true = allow static init
                    if(Compiler::parseStaticExpression(codeLineIndex, chanToken, operand, numeric) == Compiler::OperandInvalid)
                    {
                        Cpu::reportError(Cpu::KwdError, stderr, "Keywords::SOUND() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                               chanToken.c_str(), codeLine._text.c_str());
                        return false;
                    }
                    int16_t channel = int16_t(std::lround(numeric._value));
                    if(channel < 1  ||  channel > 4)
                    {
                        Cpu::reportError(Cpu::KwdError, stderr, "Keywords::SOUND() : '%s:%d' : channel number must be in the range, [1 to 4], found '%s' : %s\n",
                                                                codeLine._moduleName.c_str(), codeLineStart, chanToken.c_str(), codeLine._text.c_str());
                        return false;
                    }

                    Compiler::emitVcpuAsm("LDI", "0", false);
                    Compiler::emitVcpuAsm("FREQI", std::to_string(channel - 1), false);
                    return true;
                }
                // Literal channel
                else
                {
                    uint16_t channel = 0;
                    if(!Expression::stringToU16(chanToken, channel)  ||  channel < 1  ||  channel > 4)
                    {
                        Cpu::reportError(Cpu::KwdError, stderr, "Keywords::SOUND() : '%s:%d' : channel number must be in the range, [1 to 4], found '%s' : %s\n",
                                                                codeLine._moduleName.c_str(), codeLineStart, chanToken.c_str(), codeLine._text.c_str());
                        return false;
                    }
                    Compiler::emitVcpuAsm("LDI", "0", false);
                    Compiler::emitVcpuAsm("FREQI", std::to_string(channel - 1), false);
                    return true;
                }
            }
        }

        // Sound channel, (has to be between 1 and 4, saves an ADD/INC, no checking done)
        if(tokens.size() >= 2)
        {
            std::string chanToken = tokens[1];
            Expression::Numeric chanParam;
            if(Compiler::parseExpression(codeLineIndex, chanToken, chanParam) == Expression::IsInvalid)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::SOUND() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                       chanToken.c_str(), codeLine._text.c_str());
                return false;
            }
            Compiler::emitVcpuAsm("ST", "sndChannel + 1", false);
        }
        
        // Sound channels on
        if(stateToken == "ON")
        {
            if(tokens.size() < 3)
            {
                usageSOUND(1, codeLine, codeLineStart);
                return false;
            }

            std::string freqToken = tokens[2];
            Expression::Numeric freqParam;
            if(Compiler::parseExpression(codeLineIndex, freqToken, freqParam) == Expression::IsInvalid)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::SOUND() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, freqToken.c_str(), codeLine._text.c_str());
                return false;
            }
            Compiler::emitVcpuAsm("STW", "sndFrequency", false);

            if(tokens.size() == 3)
            {
                Compiler::emitVcpuAsm("%SoundOn", "", false);
                return true;
            }

            std::string volToken = tokens[3];
            Expression::Numeric volParam;
            if(Compiler::parseExpression(codeLineIndex, volToken, volParam) == Expression::IsInvalid)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::SOUND() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, volToken.c_str(), codeLine._text.c_str());
                return false;
            }
            Compiler::emitVcpuAsm("STW", "sndVolume", false);

            if(tokens.size() == 4)
            {
                Compiler::emitVcpuAsm("LDI", "2",           false);
                Compiler::emitVcpuAsm("STW", "sndWaveType", false);
                Compiler::emitVcpuAsm("%SoundOnV", "",      false);
                return true;
            }

            std::string wavToken = tokens[4];
            Expression::Numeric wavParam;
            if(Compiler::parseExpression(codeLineIndex, wavToken, wavParam) == Expression::IsInvalid)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::SOUND() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, wavToken.c_str(), codeLine._text.c_str());
                return false;
            }
            Compiler::emitVcpuAsm("STW", "sndWaveType", false);
            Compiler::emitVcpuAsm("%SoundOnV", "",      false);

            return true;
        }

        // Sound channels modulation
        if(stateToken == "MOD")
        {
            if(tokens.size() < 3  ||  tokens.size() > 4)
            {
                usageSOUND(2, codeLine, codeLineStart);
                return false;
            }

            std::string waveXToken = tokens[2];
            Expression::Numeric waveXParam;
            if(Compiler::parseExpression(codeLineIndex, waveXToken, waveXParam) == Expression::IsInvalid)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::SOUND() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, waveXToken.c_str(), codeLine._text.c_str());
                return false;
            }
            Compiler::emitVcpuAsm("ST", "sndWaveType + 1", false);

            if(tokens.size() == 4)
            {
                std::string waveAToken = tokens[3];
                Expression::Numeric waveAParam;
                if(Compiler::parseExpression(codeLineIndex, waveAToken, waveAParam) == Expression::IsInvalid)
                {
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::SOUND() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, waveAToken.c_str(), codeLine._text.c_str());
                    return false;
                }
                Compiler::emitVcpuAsm("ST", "sndWaveType", false);
            }
            // Reset waveA
            else
            {
                Compiler::emitVcpuAsm("LDI", "0",          false);
                Compiler::emitVcpuAsm("ST", "sndWaveType", false);
            }

            Compiler::emitVcpuAsm("%SoundMod", "", false);
            return true;
        }

        // Sound channels off
        if(stateToken == "OFF")
        {
            if(tokens.size() > 2)
            {
                usageSOUND(3, codeLine, codeLineStart);
                return false;
            }

            // All sound channels off
            if(tokens.size() == 1)
            {
                Compiler::emitVcpuAsm("%SoundAllOff", "", false);
                return true;
            }
            // Single channel off
            else
            {
                Compiler::emitVcpuAsm("%SoundOff", "", false);
                return true;
            }
        }

        usageSOUND(0, codeLine, codeLineStart);
        return false;
    }

    void usageSET(Compiler::CodeLine& codeLine, int codeLineStart)
    {
        Cpu::reportError(Cpu::KwdError, stderr, "Keywords::SET() : '%s:%d' : syntax error, use 'SET <VAR NAME>, <PARAM>' : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
    }
    bool SET(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(tokenIndex);

        std::vector<std::string> tokens = Expression::tokenise(codeLine._code.substr(foundPos), ',', false);
        if(tokens.size() < 1  ||  tokens.size() > 3)
        {
            usageSET(codeLine, codeLineStart);
            return false;
        }

        // System variable to set
        std::string sysVarName = tokens[0];
        Expression::stripWhitespace(sysVarName);
        Expression::strToUpper(sysVarName);

        // First parameter after system var name
        std::string token1;
        Expression::Numeric param1;
        if(tokens.size() >= 2) token1 = tokens[1];

        // Second parameter after system var name
        std::string token2;
        Expression::Numeric param2;
        if(tokens.size() >= 3) token2 = tokens[2];

        // Font id variable
        if(sysVarName == "FONT_ID"  &&  tokens.size() == 2)
        {
            if(Compiler::getCodeRomType() < Cpu::ROMv3)
            {
                std::string romTypeStr;
                getRomTypeStr(Compiler::getCodeRomType(), romTypeStr);
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::SET() : '%s:%d' : version error, 'SET FONT_ID' requires ROMv3 or higher, you are trying to link against '%s' : %s\n",
                                                        codeLine._moduleName.c_str(), codeLineStart, romTypeStr.c_str(), codeLine._text.c_str());
                return false;
            }

            Compiler::emitVcpuAsm("LDWI", "_fontId_", false);
            Compiler::emitVcpuAsm("STW", "register0", false);
            if(Compiler::parseExpression(codeLineIndex, token1, param1) == Expression::IsInvalid)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::SET() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, token1.c_str(), codeLine._text.c_str());
                return false;
            }
            Compiler::emitVcpuAsm("POKE", "register0", false);
            return true;
        }
        else if(sysVarName == "TIME_MODE")
        {
            Compiler::emitVcpuAsm("LDWI", "handleT_mode + 1", false);
            Compiler::emitVcpuAsm("STW", "register0", false);
            if(Compiler::parseExpression(codeLineIndex, token1, param1) == Expression::IsInvalid)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::SET() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, token1.c_str(), codeLine._text.c_str());
                return false;
            }
            Compiler::emitVcpuAsm("POKE", "register0", false);
            return true;
        }
        else if(sysVarName == "TIME_EPOCH")
        {
            Compiler::emitVcpuAsm("LDWI", "handleT_epoch + 1", false);
            Compiler::emitVcpuAsm("STW", "register0", false);
            if(Compiler::parseExpression(codeLineIndex, token1, param1) == Expression::IsInvalid)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::SET() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, token1.c_str(), codeLine._text.c_str());
                return false;
            }
            Compiler::emitVcpuAsm("POKE", "register0", false);
            return true;
        }
        else if(sysVarName == "TIME_S")
        {
            Compiler::emitVcpuAsm("LDWI", "_timeArray_ + 0", false);
            Compiler::emitVcpuAsm("STW", "register0", false);
            if(Compiler::parseExpression(codeLineIndex, token1, param1) == Expression::IsInvalid)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::SET() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, token1.c_str(), codeLine._text.c_str());
                return false;
            }
            Compiler::emitVcpuAsm("POKE", "register0", false);
            return true;
        }
        else if(sysVarName == "TIME_M")
        {
            Compiler::emitVcpuAsm("LDWI", "_timeArray_ + 1", false);
            Compiler::emitVcpuAsm("STW", "register0", false);
            if(Compiler::parseExpression(codeLineIndex, token1, param1) == Expression::IsInvalid)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::SET() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, token1.c_str(), codeLine._text.c_str());
                return false;
            }
            Compiler::emitVcpuAsm("POKE", "register0", false);
            return true;
        }
        else if(sysVarName == "TIME_H")
        {
            Compiler::emitVcpuAsm("LDWI", "_timeArray_ + 2", false);
            Compiler::emitVcpuAsm("STW", "register0", false);
            if(Compiler::parseExpression(codeLineIndex, token1, param1) == Expression::IsInvalid)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::SET() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, token1.c_str(), codeLine._text.c_str());
                return false;
            }
            Compiler::emitVcpuAsm("POKE", "register0", false);
            return true;
        }
        else if(sysVarName == "JIFFIES")
        {
            if(Compiler::parseExpression(codeLineIndex, token1, param1) == Expression::IsInvalid)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::SET() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, token1.c_str(), codeLine._text.c_str());
                return false;
            }
            
            if(Compiler::getCodeRomType() >= Cpu::ROMvX0  &&  Compiler::getCodeRomType() < Cpu::SDCARD)
            {
                Compiler::emitVcpuAsm("ST", "giga_jiffiesTick", false);
            }
            else
            {
                Compiler::emitVcpuAsm("STW", "timerJiff", false);
            }
            return true;
        }
        else if(sysVarName == "TIMER")
        {
            if(Compiler::parseExpression(codeLineIndex, token1, param1) == Expression::IsInvalid)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::SET() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, token1.c_str(), codeLine._text.c_str());
                return false;
            }
            
            if(Compiler::getCodeRomType() >= Cpu::ROMvX0  &&  Compiler::getCodeRomType() < Cpu::SDCARD)
            {
                Compiler::emitVcpuAsm("STW", "giga_secondsTickLo", false);
                Compiler::emitVcpuAsm("MOVQB", "giga_jiffiesTick, 0", false);
            }
            else
            {
                Compiler::emitVcpuAsm("STW", "timerTick", false);
            }
            return true;
        }
        else if(sysVarName == "VBLANK_PROC")
        {
            if(Compiler::getCodeRomType() < Cpu::ROMv5a)
            {
                std::string romTypeStr;
                getRomTypeStr(Compiler::getCodeRomType(), romTypeStr);
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::SET() : '%s:%d' : version error, 'SET VBLANK_PROC' requires ROMv5a or higher, you are trying to link against '%s' : %s\n",
                                                        codeLine._moduleName.c_str(), codeLineStart, romTypeStr.c_str(), codeLine._text.c_str());
                return false;
            }

            Compiler::emitVcpuAsm("LDWI", Expression::wordToHexString(VBLANK_PROC), false);
            Compiler::emitVcpuAsm("STW", "register0", false);
            if(Compiler::parseExpression(codeLineIndex, token1, param1) == Expression::IsInvalid)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::SET() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, token1.c_str(), codeLine._text.c_str());
                return false;
            }
            Compiler::emitVcpuAsm("DOKE", "register0", false);
            return true;
        }
        else if(sysVarName == "VBLANK_FREQ")
        {
            if(Compiler::getCodeRomType() < Cpu::ROMv5a)
            {
                std::string romTypeStr;
                getRomTypeStr(Compiler::getCodeRomType(), romTypeStr);
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::SET() : '%s:%d' : version error, 'SET VBLANK_FREQ' requires ROMv5a or higher, you are trying to link against '%s' : %s\n",
                                                        codeLine._moduleName.c_str(), codeLineStart, romTypeStr.c_str(), codeLine._text.c_str());
                return false;
            }

#if 1
            // (256 - n) = vblank interrupt frequency, where n = 1 to 255
            Compiler::emitVcpuAsm("LDWI", "realTS_rti + 2", false);
            Compiler::emitVcpuAsm("STW", "register0", false);
            if(Compiler::parseExpression(codeLineIndex, token1, param1) == Expression::IsInvalid)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::SET() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, token1.c_str(), codeLine._text.c_str());
                return false;
            }
            Compiler::emitVcpuAsm("POKE", "register0", false);
#endif
            return true;
        }
        else if(sysVarName == "CURSOR_X"  &&  tokens.size() == 2)
        {
            if(Compiler::parseExpression(codeLineIndex, token1, param1) == Expression::IsInvalid)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::SET() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, token1.c_str(), codeLine._text.c_str());
                return false;
            }
            Compiler::emitVcpuAsm("ST", "cursorXY", false);
            return true;
        }
        else if(sysVarName == "CURSOR_Y"  &&  tokens.size() == 2)
        {
            if(Compiler::parseExpression(codeLineIndex, token1, param1) == Expression::IsInvalid)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::SET() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, token1.c_str(), codeLine._text.c_str());
                return false;
            }
            Compiler::emitVcpuAsm("ST", "cursorXY + 1", false);
            return true;
        }
        else if(sysVarName == "CURSOR_XY"  &&  tokens.size() == 2)
        {
            if(Compiler::parseExpression(codeLineIndex, token1, param1) == Expression::IsInvalid)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::SET() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, token1.c_str(), codeLine._text.c_str());
                return false;
            }
            Compiler::emitVcpuAsm("STW", "cursorXY", false);
            return true;
        }
        else if(sysVarName == "FG_COLOUR"  &&  tokens.size() == 2)
        {
            if(Compiler::parseExpression(codeLineIndex, token1, param1) == Expression::IsInvalid)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::SET() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, token1.c_str(), codeLine._text.c_str());
                return false;
            }
            Compiler::emitVcpuAsm("ST", "fgbgColour + 1", false);
            return true;
        }
        else if(sysVarName == "BG_COLOUR"  &&  tokens.size() == 2)
        {
            if(Compiler::parseExpression(codeLineIndex, token1, param1) == Expression::IsInvalid)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::SET() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, token1.c_str(), codeLine._text.c_str());
                return false;
            }
            Compiler::emitVcpuAsm("ST", "fgbgColour", false);
            return true;
        }
        else if(sysVarName == "FGBG_COLOUR"  &&  tokens.size() == 2)
        {
            if(Compiler::parseExpression(codeLineIndex, token1, param1) == Expression::IsInvalid)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::SET() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, token1.c_str(), codeLine._text.c_str());
                return false;
            }
            Compiler::emitVcpuAsm("STW", "fgbgColour", false);
            return true;
        }
        else if(sysVarName == "MIDI_STREAM"  &&  tokens.size() == 2)
        {
            if(Compiler::parseExpression(codeLineIndex, token1, param1) == Expression::IsInvalid)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::SET() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, token1.c_str(), codeLine._text.c_str());
                return false;
            }
            Compiler::emitVcpuAsm("STW", "midiStream", false);
            return true;
        }
        else if(sysVarName == "VIDEO_TOP"  &&  tokens.size() == 2)
        {
            Compiler::emitVcpuAsm("LDWI", "giga_videoTop", false);
            Compiler::emitVcpuAsm("STW", "register0", false);
            if(Compiler::parseExpression(codeLineIndex, token1, param1) == Expression::IsInvalid)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::SET() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, token1.c_str(), codeLine._text.c_str());
                return false;
            }
            Compiler::emitVcpuAsm("POKE", "register0", false);
            return true;
        }
        else if(sysVarName == "LED_TEMPO"  &&  tokens.size() == 2)
        {
            if(Compiler::parseExpression(codeLineIndex, token1, param1) == Expression::IsInvalid)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::SET() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, token1.c_str(), codeLine._text.c_str());
                return false;
            }
            Compiler::emitVcpuAsm("ST", "giga_ledTempo", false);
            return true;
        }
        else if(sysVarName == "LED_STATE"  &&  tokens.size() == 2)
        {
            if(Compiler::parseExpression(codeLineIndex, token1, param1) == Expression::IsInvalid)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::SET() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, token1.c_str(), codeLine._text.c_str());
                return false;
            }
            Compiler::emitVcpuAsm("ST", "giga_ledState", false);
            return true;
        }
        else if(sysVarName == "SOUND_TIMER"  &&  tokens.size() == 2)
        {
            if(Compiler::parseExpression(codeLineIndex, token1, param1) == Expression::IsInvalid)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::SET() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, token1.c_str(), codeLine._text.c_str());
                return false;
            }
            Compiler::emitVcpuAsm("ST", "giga_soundTimer", false);
            return true;
        }
        else if(sysVarName == "CHANNEL_MASK"  &&  tokens.size() == 2)
        {
            if(Compiler::parseExpression(codeLineIndex, token1, param1) == Expression::IsInvalid)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::SET() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, token1.c_str(), codeLine._text.c_str());
                return false;
            }
            Compiler::emitVcpuAsm("ST", "giga_channelMask", false);
            return true;
        }
        else if(sysVarName == "XOUT_MASK"  &&  tokens.size() == 2)
        {
            if(Compiler::parseExpression(codeLineIndex, token1, param1) == Expression::IsInvalid)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::SET() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, token1.c_str(), codeLine._text.c_str());
                return false;
            }
            Compiler::emitVcpuAsm("ST", "giga_xoutMask", false);
            return true;
        }
        else if(sysVarName == "BUTTON_STATE"  &&  tokens.size() == 2)
        {
            if(Compiler::parseExpression(codeLineIndex, token1, param1) == Expression::IsInvalid)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::SET() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, token1.c_str(), codeLine._text.c_str());
                return false;
            }
            Compiler::emitVcpuAsm("ST", "giga_buttonState", false);
            return true;
        }
        else if(sysVarName == "FRAME_COUNT"  &&  tokens.size() == 2)
        {
            if(Compiler::parseExpression(codeLineIndex, token1, param1) == Expression::IsInvalid)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::SET() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, token1.c_str(), codeLine._text.c_str());
                return false;
            }
            Compiler::emitVcpuAsm("ST", "giga_frameCount", false);
            return true;
        }

        usageSET(codeLine, codeLineStart);
        return false;
    }

    bool ASM(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(foundPos);
        UNREFERENCED_PARAM(tokenIndex);
        UNREFERENCED_PARAM(codeLineIndex);
        UNREFERENCED_PARAM(codeLineStart);
        UNREFERENCED_PARAM(codeLine);

        // If ASM is attached to a label, propagate it to the generated vCPU code, (if sub/proc/func already has a PUSH, then label is already valid, so ignore)
        if(codeLine._labelIndex > -1  &&  (!Compiler::getLabels()[codeLine._labelIndex]._gosub  ||  Compiler::getLabels()[codeLine._labelIndex]._noPush))
        {
            Compiler::setNextVasmLabelIndex(codeLine._labelIndex);
        }

        Compiler::setCodeIsAsm(true);

        return true;
    }

    bool ENDASM(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(foundPos);
        UNREFERENCED_PARAM(tokenIndex);
        UNREFERENCED_PARAM(codeLineIndex);
        UNREFERENCED_PARAM(codeLineStart);
        UNREFERENCED_PARAM(codeLine);

        Compiler::setCodeIsAsm(false);

        return true;
    }

    bool BCDADD(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(tokenIndex);

        std::vector<std::string> tokens = Expression::tokenise(codeLine._code.substr(foundPos), ',', false);
        if(tokens.size() != 3)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::BCDADD() : '%s:%d' : syntax error, use 'BCDADD <src bcd address>, <dst bcd address>, <length>' : %s\n",
                                                    codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        // BCD src address
        std::string srcToken = tokens[0];
        Expression::Numeric srcParam(true); // true = allow static init
        if(Compiler::parseExpression(codeLineIndex, srcToken, srcParam) == Expression::IsInvalid)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::BCDADD() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, srcToken.c_str(), codeLine._text.c_str());
            return false;
        }
        Compiler::emitVcpuAsm("STW", "bcdSrcAddr", false);

        // BCD dst address
        std::string dstToken = tokens[1];
        Expression::Numeric dstParam(true); // true = allow static init
        if(Compiler::parseExpression(codeLineIndex, dstToken, dstParam) == Expression::IsInvalid)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::BCDADD() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, dstToken.c_str(), codeLine._text.c_str());
            return false;
        }
        Compiler::emitVcpuAsm("STW", "bcdDstAddr", false);

        // BCD length
        std::string lenToken = tokens[2];
        Expression::Numeric lenParam;
        if(Compiler::parseExpression(codeLineIndex, lenToken, lenParam) == Expression::IsInvalid)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::BCDADD() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, lenToken.c_str(), codeLine._text.c_str());
            return false;
        }
        Compiler::emitVcpuAsm("%BcdAdd", "", false);

        return true;
    }

    bool BCDSUB(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(tokenIndex);

        std::vector<std::string> tokens = Expression::tokenise(codeLine._code.substr(foundPos), ',', false);
        if(tokens.size() != 3)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::BCDSUB() : '%s:%d' : syntax error, use 'BCDSUB <src bcd address>, <dst bcd address>, <length>' : %s\n",
                                                    codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        // BCD src address
        std::string srcToken = tokens[0];
        Expression::Numeric srcParam(true); // true = allow static init
        if(Compiler::parseExpression(codeLineIndex, srcToken, srcParam) == Expression::IsInvalid)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::BCDSUB() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, srcToken.c_str(), codeLine._text.c_str());
            return false;
        }
        Compiler::emitVcpuAsm("STW", "bcdSrcAddr", false);

        // BCD dst address
        std::string dstToken = tokens[1];
        Expression::Numeric dstParam(true); // true = allow static init
        if(Compiler::parseExpression(codeLineIndex, dstToken, dstParam) == Expression::IsInvalid)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::BCDSUB() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, dstToken.c_str(), codeLine._text.c_str());
            return false;
        }
        Compiler::emitVcpuAsm("STW", "bcdDstAddr", false);

        // BCD length
        std::string lenToken = tokens[2];
        Expression::Numeric lenParam;
        if(Compiler::parseExpression(codeLineIndex, lenToken, lenParam) == Expression::IsInvalid)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::BCDSUB() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, lenToken.c_str(), codeLine._text.c_str());
            return false;
        }
        Compiler::emitVcpuAsm("%BcdSub", "", false);

        return true;
    }

    bool BCDINT(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(tokenIndex);

        std::vector<std::string> tokens = Expression::tokenise(codeLine._code.substr(foundPos), ',', false);
        if(tokens.size() != 2)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::BCDINT() : '%s:%d' : syntax error, use 'BCDINT <dst bcd address>, <int>' bcd value MUST contain at least 5 digits : %s\n",
                                                    codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        // BCD dst address
        std::string srcToken = tokens[0];
        Expression::Numeric srcParam(true); // true = allow static init
        if(Compiler::parseExpression(codeLineIndex, srcToken, srcParam) == Expression::IsInvalid)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::BCDINT() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, srcToken.c_str(), codeLine._text.c_str());
            return false;
        }
        Compiler::emitVcpuAsm("STW", "bcdDstAddr", false);

        // Integer value, must be +ve, max value 42767, (32767 + 10000 because of how vASM sub Numeric::bcdInt works)
        std::string intToken = tokens[1];
        Expression::Numeric intParam;
        if(Compiler::parseExpression(codeLineIndex, intToken, intParam) == Expression::IsInvalid)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::BCDINT() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, intToken.c_str(), codeLine._text.c_str());
            return false;
        }
        Compiler::emitVcpuAsm("%BcdInt", "", false);

        return true;
    }

    bool BCDCPY(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(tokenIndex);

        std::vector<std::string> tokens = Expression::tokenise(codeLine._code.substr(foundPos), ',', false);
        if(tokens.size() != 3)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::BCDCPY() : '%s:%d' : syntax error, use 'BCDCPY <src bcd address>, <dst bcd address>, <length>' : %s\n",
                                                    codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        // BCD src address
        std::string srcToken = tokens[0];
        Expression::Numeric srcParam(true); // true = allow static init
        if(Compiler::parseExpression(codeLineIndex, srcToken, srcParam) == Expression::IsInvalid)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::BCDCPY() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, srcToken.c_str(), codeLine._text.c_str());
            return false;
        }
        Compiler::emitVcpuAsm("STW", "bcdSrcAddr", false);

        // BCD dst address
        std::string dstToken = tokens[1];
        Expression::Numeric dstParam(true); // true = allow static init
        if(Compiler::parseExpression(codeLineIndex, dstToken, dstParam) == Expression::IsInvalid)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::BCDCPY() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, dstToken.c_str(), codeLine._text.c_str());
            return false;
        }
        Compiler::emitVcpuAsm("STW", "bcdDstAddr", false);

        // BCD length
        std::string lenToken = tokens[2];
        Expression::Numeric lenParam;
        if(Compiler::parseExpression(codeLineIndex, lenToken, lenParam) == Expression::IsInvalid)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::BCDCPY() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, lenToken.c_str(), codeLine._text.c_str());
            return false;
        }
        Compiler::emitVcpuAsm("%BcdCpy", "", false);

        return true;
    }

    bool VEC8(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(tokenIndex);

        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD)
        {
            std::string romTypeStr;
            getRomTypeStr(Compiler::getCodeRomType(), romTypeStr);
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::VEC8() : '%s:%d' : version error, 'VEC8' requires ROMvX0, you are trying to link against '%s' : %s\n",
                                                    codeLine._moduleName.c_str(), codeLineStart, romTypeStr.c_str(), codeLine._text.c_str());
            return false;
        }

        std::vector<std::string> tokens = Expression::tokenise(codeLine._code.substr(foundPos), ',', false);
        if(tokens.size() < 5  ||  tokens.size() > 7)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::VEC8() : '%s:%d' : syntax error, use 'VEC8 ADD, <src addr>, <dst addr>, <size>, <index>' : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            //Cpu::reportError(Cpu::KwdError, stderr, "Keywords::VEC8()                           use 'VEC8 SUB, <src addr>, <dst addr>, <size>, <start>'\n");
            //Cpu::reportError(Cpu::KwdError, stderr, "Keywords::VEC8()                           use 'VEC8 MUL, <src addr>, <dst addr>, <size>, <start>'\n");
            //Cpu::reportError(Cpu::KwdError, stderr, "Keywords::VEC8()                           use 'VEC8 DIV, <src addr>, <dst addr>, <size>, <start>'\n");
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::VEC8()                           use 'VEC8 CHK, <src addr>, <sub addr>, <CLT>, <CGE>, <size>, <index>'\n");
            return false;
        }

        // Vec8 command
        static std::map<std::string, Compiler::Vec8Cmd> vec8CmdMap = {{"ADD", Compiler::Vec8Add}, {"SUB", Compiler::Vec8Sub}, {"MUL", Compiler::Vec8Mul}, {"DIV", Compiler::Vec8Div}, {"CHK", Compiler::Vec8Chk}};
        std::string cmdToken = tokens[0];
        Expression::stripWhitespace(cmdToken);
        Expression::strToUpper(cmdToken);
        auto it = vec8CmdMap.find(cmdToken);
        if(it == vec8CmdMap.end())
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::VEC8() : '%s:%d' : syntax error in : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        Compiler::Vec8Cmd vec8Cmd = it->second;
        switch(vec8Cmd)
        {
            case Compiler::Vec8Add:
            {
                // Src, Dst, Size, Start
                std::string srcToken = tokens[1], dstToken = tokens[2], sizeToken = tokens[3], indexToken = tokens[4];
                Expression::Numeric srcParam, dstParam, sizeParam, indexParam;
                if(!Expression::parse(srcToken, codeLineIndex, srcParam))
                {
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::VEC8() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, srcToken.c_str(), codeLine._text.c_str());
                    return false;
                }
                if(!Expression::parse(dstToken, codeLineIndex, dstParam))
                {
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::VEC8() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, srcToken.c_str(), codeLine._text.c_str());
                    return false;
                }
                if(!Expression::parse(sizeToken, codeLineIndex, sizeParam))
                {
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::VEC8() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, srcToken.c_str(), codeLine._text.c_str());
                    return false;
                }
                if(!Expression::parse(indexToken, codeLineIndex, indexParam))
                {
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::VEC8() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, srcToken.c_str(), codeLine._text.c_str());
                    return false;
                }

                if(srcParam._varType == Expression::Number  &&  dstParam._varType == Expression::Number  &&  sizeParam._varType == Expression::Number  &&  indexParam._varType == Expression::Number)
                {
                    uint16_t src   = uint16_t(std::lround(srcParam._value));
                    uint16_t dst   = uint16_t(std::lround(dstParam._value));
                    uint16_t size  = uint16_t(std::lround(sizeParam._value));
                    uint16_t index = uint16_t(std::lround(indexParam._value));
                    if(src >= USER_VAR_START  &&  src <= 0x00FE  &&  dst >= USER_VAR_START  &&  dst <= 0x00FE  &&  index == 0)
                    {
                        if(size == 2)
                        {
                            Compiler::emitVcpuAsm("VADDBW", Expression::byteToHexString(uint8_t(src)) + ", " + Expression::byteToHexString(uint8_t(dst)), false);
                            break;
                        }
                        
                        if(size == 4)
                        {
                            Compiler::emitVcpuAsm("VADDBL", Expression::byteToHexString(uint8_t(src)) + ", " + Expression::byteToHexString(uint8_t(dst)), false);
                            break;
                        }
                    }
                }

                // Src
                if(Compiler::handleExpression(codeLineIndex, srcToken, srcParam) == Expression::IsInvalid)
                {
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::VEC8() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, srcToken.c_str(), codeLine._text.c_str());
                    return false;
                }
                Compiler::emitVcpuAsm("STW", "giga_sysArg0", false);

                // Dst
                if(Compiler::handleExpression(codeLineIndex, dstToken, dstParam) == Expression::IsInvalid)
                {
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::VEC8() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, dstToken.c_str(), codeLine._text.c_str());
                    return false;
                }
                Compiler::emitVcpuAsm("STW", "giga_sysArg2", false);

                // Size in high byte
                if(Compiler::handleExpression(codeLineIndex, sizeToken, sizeParam) == Expression::IsInvalid)
                {
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::VEC8() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, sizeToken.c_str(), codeLine._text.c_str());
                    return false;
                }
                Compiler::emitVcpuAsm("ST", "giga_sysArg4 + 1", false);

                // Index in low byte
                if(Compiler::handleExpression(codeLineIndex, indexToken, indexParam) == Expression::IsInvalid)
                {
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::VEC8() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, indexToken.c_str(), codeLine._text.c_str());
                    return false;
                }
                Compiler::emitVcpuAsm("ST", "giga_sysArg4", false);

                Compiler::emitVcpuAsm("LDWI", "SYS_AddInt8Array_vX_40", false);
                Compiler::emitVcpuAsm("STW", "giga_sysFn", false);
                Compiler::emitVcpuAsm("SYS", "40", false);
            }
            break;

            case Compiler::Vec8Chk:
            {
                // Src address
                std::string srcToken = tokens[1];
                Expression::Numeric srcParam;
                if(Compiler::parseExpression(codeLineIndex, srcToken, srcParam) == Expression::IsInvalid)
                {
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::VEC8() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, srcToken.c_str(), codeLine._text.c_str());
                    return false;
                }
                Compiler::emitVcpuAsm("STW", "0xB8", false);

                // Subroutine address
                std::string subToken = tokens[2];
                Expression::stripWhitespace(subToken);
                int labelIndex = Compiler::findLabel(subToken);
                if(labelIndex > -1)
                {
                    Compiler::emitVcpuAsm("LDWI", "_" + subToken, false);
                }
                else
                {
                    Expression::Numeric subParam;
                    if(Compiler::parseExpression(codeLineIndex, subToken, subParam) == Expression::IsInvalid)
                    {
                        Cpu::reportError(Cpu::KwdError, stderr, "Keywords::VEC8() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, subToken.c_str(), codeLine._text.c_str());
                        return false;
                    }
                }
                Compiler::emitVcpuAsm("STW", "0xBA", false);

                // Condition LT, low byte
                std::string condLtToken = tokens[3];
                Expression::Numeric condLtParam;
                if(Compiler::parseExpression(codeLineIndex, condLtToken, condLtParam) == Expression::IsInvalid)
                {
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::VEC8() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, condLtToken.c_str(), codeLine._text.c_str());
                    return false;
                }
                Compiler::emitVcpuAsm("ST", "0xBC", false);

                // Condition GE, high byte
                std::string condGeToken = tokens[4];
                Expression::Numeric condGeParam;
                if(Compiler::parseExpression(codeLineIndex, condGeToken, condGeParam) == Expression::IsInvalid)
                {
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::VEC8() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, condGeToken.c_str(), codeLine._text.c_str());
                    return false;
                }
                Compiler::emitVcpuAsm("ST", "0xBD", false);

                // Size in high byte
                std::string sizeToken = tokens[5];
                Expression::Numeric sizeParam;
                if(Compiler::parseExpression(codeLineIndex, sizeToken, sizeParam) == Expression::IsInvalid)
                {
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::VEC8() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, sizeToken.c_str(), codeLine._text.c_str());
                    return false;
                }
                Compiler::emitVcpuAsm("ST", "0xBF", false);

                // Start index in low byte
                std::string startToken = tokens[6];
                Expression::Numeric startParam;
                if(Compiler::parseExpression(codeLineIndex, startToken, startParam) == Expression::IsInvalid)
                {
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::VEC8() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, startToken.c_str(), codeLine._text.c_str());
                    return false;
                }
                Compiler::emitVcpuAsm("ST", "0xBE", false);

                Compiler::emitVcpuAsm("LDWI", "SYS_CmpByteBounds_vX_54", false);
                Compiler::emitVcpuAsm("STW", "giga_sysFn", false);
                Compiler::emitVcpuAsm("SYS", "54", false);
            }
            break;

            default:
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::VEC8() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, cmdToken.c_str(), codeLine._text.c_str());
                return false;
            }
            break;
        }

        return true;
    }

    bool EXEC(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(tokenIndex);

        std::vector<std::string> tokens = Expression::tokenise(codeLine._code.substr(foundPos), ',', false);
        if(tokens.size() < 1  ||  tokens.size() > 2)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::EXEC() : '%s:%d' : syntax error, expected 'EXEC <rom address>, <optional ram address>' : %s\n",
                                                    codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        // ROM address to load from
        std::string romToken = tokens[0];
        Expression::Numeric romParam;
        if(Compiler::parseExpression(codeLineIndex, romToken, romParam) == Expression::IsInvalid)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::EXEC() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, romToken.c_str(), codeLine._text.c_str());
            return false;
        }
        Compiler::emitVcpuAsm("STW", "giga_sysArg0", false);

        // RAM address to execute at
        if(tokens.size() == 2)
        {
            std::string ramToken = tokens[1];
            Expression::Numeric ramParam;
            if(Compiler::parseExpression(codeLineIndex, ramToken, ramParam) == Expression::IsInvalid)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::EXEC() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, ramToken.c_str(), codeLine._text.c_str());
                return false;
            }
        }
        else
        {
            // Default execute address
            Compiler::emitVcpuAsm("LDWI", Expression::wordToHexString(DEFAULT_EXEC_ADDRESS), false);
        }

        // SYS_Exec_88
        Compiler::emitVcpuAsm("%RomExec", "",  false);

        return true;
    }

    void openUsage(Compiler::CodeLine& codeLine, int codeLineStart)
    {
        Cpu::reportError(Cpu::KwdError, stderr, "Keywords::OPEN() : '%s:%d' : usage, 'OPEN <device>, <id>, <\"path\">, <\"file\">, <mode>', where mode is one of 'r', 'w', 'a', 'r+', 'w+', 'a+' : %s\n",
                                                codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
    }
    bool OPEN(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(tokenIndex);

        std::vector<std::string> tokens = Expression::tokenise(codeLine._code.substr(foundPos), ',', false);
        if(tokens.size() != 5)
        {
            openUsage(codeLine, codeLineStart);
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::OPEN() : '%s:%d' : syntax error, wrong number of parameters : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        // device
        std::string devToken = tokens[0];
        Expression::stripWhitespace(devToken);
        Expression::strToUpper(devToken);
        if(Compiler::getOpenDevices().find(devToken) == Compiler::getOpenDevices().end())
        {
            openUsage(codeLine, codeLineStart);
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::OPEN() : '%s:%d' : device '%s' does not exist : %s\n", codeLine._moduleName.c_str(), codeLineStart, devToken.c_str(), codeLine._text.c_str());
            return false;
        }
        uint16_t deviceId = Compiler::getOpenDevices()[devToken];

        // file id
        std::string idToken = tokens[1];
        Expression::stripWhitespace(idToken);
        Expression::Numeric idNumeric;
        std::string idOperand;
        if(Compiler::parseStaticExpression(codeLineIndex, idToken, idOperand, idNumeric) == Compiler::OperandInvalid)
        {
            openUsage(codeLine, codeLineStart);
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::OPEN() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, idToken.c_str(), codeLine._text.c_str());
            return false;
        }
        int openId = int(std::lround(idNumeric._value));
        if(Compiler::getDefDataOpens().find(openId) != Compiler::getDefDataOpens().end())
        {
            openUsage(codeLine, codeLineStart);
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::OPEN() : '%s:%d' : id:%d is not unique : %s\n", codeLine._moduleName.c_str(), codeLineStart, openId, codeLine._text.c_str());
            return false;
        }

        // File path
        std::string pathToken = tokens[2];
        Expression::stripNonStringWhitespace(pathToken);
        if(pathToken == "") pathToken = "/";
        size_t fquote = pathToken.find_first_of('"');
        size_t lquote = pathToken.find_last_of('"');
        if(fquote == std::string::npos  ||  fquote == std::string::npos)
        {
            openUsage(codeLine, codeLineStart);
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::OPEN() : '%s:%d' : syntax error missing double quotes in '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                                      pathToken.c_str(), codeLine._text.c_str());
            return false;
        }
        pathToken.erase(lquote);
        pathToken.erase(0, fquote + 1);

        // File name
        std::string fileToken = tokens[3];
        Expression::stripNonStringWhitespace(fileToken);
        if(fileToken == "")
        {
            openUsage(codeLine, codeLineStart);
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::OPEN() : '%s:%d' : syntax error, <file> is empty : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }
        fquote = fileToken.find_first_of('"');
        lquote = fileToken.find_last_of('"');
        if(fquote == std::string::npos  ||  fquote == std::string::npos)
        {
            openUsage(codeLine, codeLineStart);
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::OPEN() : '%s:%d' : syntax error missing double quotes in '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                                      fileToken.c_str(), codeLine._text.c_str());
            return false;
        }
        fileToken.erase(lquote);
        fileToken.erase(0, fquote + 1);

        // Open mode
        Compiler::DefDataOpen::OpenMode openMode;
        std::string modeToken = tokens[4];
        Expression::stripWhitespace(modeToken);
        Expression::strToUpper(modeToken);
        if(modeToken == "R")       openMode = Compiler::DefDataOpen::OpenRead;
        else if(modeToken == "W")  openMode = Compiler::DefDataOpen::OpenWrite;
        else if(modeToken == "A")  openMode = Compiler::DefDataOpen::OpenAppend;
        else if(modeToken == "R+") openMode = Compiler::DefDataOpen::OpenUpdateRW;
        else if(modeToken == "W+") openMode = Compiler::DefDataOpen::OpenCreateRW;
        else if(modeToken == "A+") openMode = Compiler::DefDataOpen::OpenAppendR;
        else
        {
            openUsage(codeLine, codeLineStart);
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::OPEN() : '%s:%d' : syntax error, <mode> is expecting one of 'r', 'w', 'a', 'r+', 'w+', 'a+', : %s\n",
                                                    codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        Compiler::getDefDataOpens()[openId] = {deviceId, openId, pathToken, fileToken, openMode};

        return true;
    }

    bool BRKPNT(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(foundPos);
        UNREFERENCED_PARAM(tokenIndex);
        UNREFERENCED_PARAM(codeLineIndex);
        UNREFERENCED_PARAM(codeLineStart);
        UNREFERENCED_PARAM(codeLine);

        // Inserts a _breakpoint_ into the assembler source, (only works on the emulator)
        Compiler::emitVcpuAsm("_breakpoint_", "", false);
        return true;
    }

    bool GPRINTF(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        // Produces vCPU code and allocates Gigatron memory only for the emulator
#ifdef STAND_ALONE
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(foundPos);
        UNREFERENCED_PARAM(tokenIndex);
        UNREFERENCED_PARAM(codeLineIndex);
        UNREFERENCED_PARAM(codeLineStart);
        UNREFERENCED_PARAM(codeLine);
#else
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(tokenIndex);

        // Allocate memory for gprintf variable's addresses if it hasn't been allocated already
        uint16_t varsAddr = Compiler::getGprintfVarsAddr();
        if(varsAddr == 0x0000)
        {
            if(!Memory::getFreePageRAM(Memory::FitDescending, GPRINT_VAR_ADDRS*2, USER_CODE_START, Compiler::getRuntimeStart(), varsAddr, true))
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::GPRINTF() : '%s:%d' : not enough RAM for variables LUT of size %d : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                                              GPRINT_VAR_ADDRS*2, codeLine._text.c_str());
                return false;
            }
            Compiler::setGprintfVarsAddr(varsAddr);
        }

        std::vector<std::string> tokens = Expression::tokenise(codeLine._code.substr(foundPos), ',');
        if(tokens.size() < 2)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::GPRINTF() : '%s:%d' : syntax error, use 'GPRINTF \"<format string>\", <var1>, ... <varN>' : %s\n",
                                                    codeLine._text.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }
        if(tokens.size() > GPRINT_VAR_ADDRS)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::GPRINTF() : '%s:%d' : maximum number of vars is '%d', found '%d' vars : %s\n",
                                                    codeLine._moduleName.c_str(), codeLineStart, GPRINT_VAR_ADDRS, int(tokens.size()), codeLine._text.c_str());
            return false;
        }

        // Format string
        std::string formatStr = tokens[0];
        Expression::stripNonStringWhitespace(formatStr);
        if(formatStr[0] != '\"'  ||  formatStr.back() != '\"')
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::GPRINTF() : '%s:%d' : syntax error in string format, use 'GPRINTF \"<format string>\", <var1>, ... <varN>' : %s\n",
                                                    codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        // Vars
        Expression::Numeric numeric;
        std::vector<std::string> variables;
        for(int i=1; i<int(tokens.size()); i++)
        {
            uint16_t address = varsAddr + uint16_t((i-1)*2);
            if(i == 1)
            {
                Compiler::emitVcpuAsm("LDWI", Expression::wordToHexString(address), false);
                Compiler::emitVcpuAsm("STW", "memDstAddr", false);
            }
            else
            {
                Compiler::emitVcpuAsm("INC", "memDstAddr", false);
                Compiler::emitVcpuAsm("INC", "memDstAddr", false);
            }

            // Convert GBAS format to ASM format
            variables.push_back("*" + Expression::wordToHexString(address));

            if(!Expression::parse(tokens[i], codeLineIndex, numeric))
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::GPRINTF() : '%s:%d' : syntax error in '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, Expression::getExpression(), codeLine._text.c_str());
                return false;
            }
            if(numeric._varType == Expression::Number  ||  numeric._varType == Expression::IntVar16  ||  numeric._varType == Expression::StrVar)
            {
                Compiler::handleExpression(codeLineIndex, tokens[i], numeric);
            }

            Compiler::emitVcpuAsm("DOKE", "memDstAddr", false);
        }

        // Add a dummy instruction so that gprintf is called at the correct address
        Compiler::emitVcpuAsm("LSLW", "; dummy", false);

        return addGprintf(codeLine._code.substr(foundPos), formatStr, variables, varsAddr, codeLineIndex);
#endif

#ifdef STAND_ALONE
        return true;
#endif
    }

    bool OPTIMISE(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, int tokenIndex, size_t foundPos, KeywordFuncResult& result)
    {
        UNREFERENCED_PARAM(result);
        UNREFERENCED_PARAM(tokenIndex);
        UNREFERENCED_PARAM(codeLineIndex);

        std::vector<std::string> tokens = Expression::tokenise(codeLine._code.substr(foundPos), ',', false);
        if(tokens.size() != 1)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::OPTIMISE() : '%s:%d' : syntax error, use 'OPTIMISE ON' or 'OPTIMISE OFF' : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        std::string optimiseToken = Expression::strToUpper(tokens[0]);
        Expression::stripWhitespace(optimiseToken);
        if(optimiseToken != "ON"  &&  optimiseToken != "OFF")
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::OPTIMISE() : '%s:%d' : syntax error, use 'OPTIMISE ON' or 'OPTIMISE OFF' : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        Compiler::enableOptimiser((optimiseToken == "ON") ? true : false);

        return true;
    }
}