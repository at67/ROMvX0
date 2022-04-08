#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <cmath>
#include <algorithm>
#include <stdlib.h>
#include <time.h>

#include "memory.h"
#include "cpu.h"
#include "assembler.h"
#include "compiler.h"
#include "operators.h"
#include "linker.h"


namespace Operators
{
    bool _nextTempVar = true;

    std::vector<std::string> _operators;

    std::vector<std::string>& getOperators(void) {return _operators;}


    bool initialise(void)
    {
        _nextTempVar = true;

        // Operators
        _operators.push_back(" AND ");
        _operators.push_back(" XOR ");
        _operators.push_back(" OR " );
        _operators.push_back("NOT " );
        _operators.push_back(" MOD ");
        _operators.push_back(" LSL ");
        _operators.push_back(" LSR ");
        _operators.push_back(" ASR ");

        return true;
    }


    void changeToTmpVar(Expression::Numeric& numeric)
    {
        numeric._value = uint8_t(Compiler::getTempVarStart());
        numeric._varType = Expression::TmpVar;
        numeric._name = Compiler::getTempVarStartStr();
    }

    std::string getVarAsOperand(Expression::Numeric& numeric)
    {
        std::string operand;

        switch(numeric._varType)
        {
            // Temporary variable address
            case Expression::TmpVar: operand = Expression::byteToHexString(uint8_t(std::lround(numeric._value))); break;

            // User variable name
            case Expression::IntVar16: operand = "_" + numeric._name; break;

            default: break;
        }

        return operand;
    }   

    void createSingleOp(const std::string& opcode, Expression::Numeric& numeric)
    {
        switch(numeric._varType)
        {
            // Temporary variable address
            case Expression::TmpVar:
            {
                Compiler::emitVcpuAsm(opcode, Expression::byteToHexString(uint8_t(std::lround(numeric._value))), false);
            }
            break;

            // User variable name
            case Expression::IntVar16:
            {
                Compiler::emitVcpuAsmUserVar(opcode, numeric, false);
            }
            break;

            default: break;
        }
    }

    void createSingleOp(const std::string& opcode, Expression::Numeric& numeric, const std::string& operand)
    {
        switch(numeric._varType)
        {
            // Temporary variable address
            case Expression::TmpVar:
            {
                Compiler::emitVcpuAsm(opcode, Expression::byteToHexString(uint8_t(std::lround(numeric._value))) + operand, false);
            }
            break;

            // User variable name
            case Expression::IntVar16:
            {
                Compiler::emitVcpuAsm(opcode, "_" + numeric._name + operand, false);
            }
            break;

            default: break;
        }
    }

    void handleSingleOp(const std::string& opcode, Expression::Numeric& numeric)
    {
        createSingleOp(opcode, numeric);
        changeToTmpVar(numeric);
    }

    void selectSingleOp(const std::string& opcode, Expression::Numeric& numeric)
    {
        if(numeric._varType == Expression::Number)
        {
            Compiler::emitVcpuAsmForLiteral(numeric, false);
        }
        else
        {
            Operators::createSingleOp(opcode, numeric);
        }
    }

    bool handleDualOp(const std::string& opcodeStr, Expression::Numeric& lhs, Expression::Numeric& rhs, bool useHex)
    {
        std::string opcode = std::string(opcodeStr);

        // Swap left and right to take advantage of LDWI for 16bit numbers
        if((Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD)  &&  rhs._varType == Expression::Number  &&  (rhs._value < 0  ||  rhs._value > 255))
        {
            std::swap(lhs, rhs);
            if(opcode == "SUB")
            {
                opcode = "ADD";
                if(lhs._value > 0) lhs._value = -lhs._value;
            }
        }

        // LHS
        if(lhs._varType == Expression::Number)
        {
            // 8bit unsigned constants and 16bit signed constants
            Compiler::emitVcpuAsmForLiteral(lhs, useHex);
            _nextTempVar = true;
        }
        else
        {
            switch(lhs._varType)
            {
                // Temporary variable address
                case Expression::TmpVar:
                {
                    Compiler::emitVcpuAsm("LDW", Expression::byteToHexString(uint8_t(std::lround(lhs._value))), false);
                }
                break;

                // User variable name
                case Expression::IntVar16:
                {
                    if(!Compiler::emitVcpuAsmUserVar("LDW", lhs, true)) return false;
                    _nextTempVar = false;
                }
                break;

                default: break;
            }
        }

        // RHS
        if(rhs._varType == Expression::Number)
        {
            // Skip XOR if operand is 0, (n XOR 0 = n)
            if(rhs._value  ||  opcode != "XOR") 
            {
                // Literals are 16bit
                if(Compiler::getCodeRomType() >= Cpu::ROMvX0  &&  Compiler::getCodeRomType() < Cpu::SDCARD  &&  (rhs._value < 0  ||  rhs._value > 255))
                {
                    if(useHex)
                    {
                        Compiler::emitVcpuAsm(opcode + "WI", Expression::wordToHexString(uint16_t(std::lround(rhs._value))), false);
                    }
                    else
                    {
                        Compiler::emitVcpuAsm(opcode + "WI", std::to_string(int16_t(std::lround(rhs._value))), false);
                    }
                }
                // Literals are unsigned 8bit
                else
                {
                    if(useHex)
                    {
                        Compiler::emitVcpuAsm(opcode + "I", Expression::byteToHexString(uint8_t(std::lround(rhs._value))), false);
                    }
                    else
                    {
                        Compiler::emitVcpuAsm(opcode + "I", std::to_string(uint8_t(std::lround(rhs._value))), false);
                    }
                }
            }
        }
        else
        {
            switch(rhs._varType)
            {
                // Temporary variable address
                case Expression::TmpVar:
                {
                    Compiler::emitVcpuAsm(opcode + "W", Expression::byteToHexString(uint8_t(std::lround(rhs._value))), false);
                }
                break;

                // User variable name
                case Expression::IntVar16:
                {
                    if(!Compiler::emitVcpuAsmUserVar(opcode + "W", rhs, _nextTempVar)) return false;
                    _nextTempVar = false;
                }
                break;

                default: break;
            }
        }

        changeToTmpVar(lhs);
        Compiler::emitVcpuAsm("STW", Expression::byteToHexString(uint8_t(Compiler::getTempVarStart())), false);

        return true;
    }

    bool handleLogicalOp(const std::string& opcode, Expression::Numeric& lhs)
    {
        // SYS shift function needs this preamble, LSLW doesn't
        switch(lhs._varType)
        {
            // Temporary variable address
            case Expression::TmpVar:
            {
                Compiler::emitVcpuAsm("LDW", Expression::byteToHexString(uint8_t(std::lround(lhs._value))), false);
            }
            break;

            // User variable name
            case Expression::IntVar16:
            {
                if(!Compiler::emitVcpuAsmUserVar("LDW", lhs, true)) return false;
            }
            break;

            default: break;
        }

        // Inbuilt instructions or sys calls
        if(opcode != "LSLN"  &&  opcode != "LSLW"  &&  opcode != "LSRV"  &&  opcode != "LSRB") Compiler::emitVcpuAsm("STW", "mathShift", false);

        changeToTmpVar(lhs);

        return true;
    }

    void emitCcType(Expression::CCType ccType, std::string& cc)
    {
        switch(ccType)
        {
            case Expression::BooleanCC:
            {
                // Init functions are not needed for ROMvX0 as it has specific TCC instructions
                if(Compiler::getCodeRomType() >= Cpu::ROMvX0  &&  Compiler::getCodeRomType() < Cpu::SDCARD)
                {
                    Compiler::emitVcpuAsm("T" + Expression::strUpper(cc), "giga_vAC", false);
                }
                // Init functions are not needed for ROMv5a and higher as CALLI is able to directly CALL them
                else if(Compiler::getCodeRomType() >= Cpu::ROMv5a)
                {
                    Compiler::emitVcpuAsm("CALLI", "convert" + cc + "Op", false);
                }
                else
                // Enable system internal sub intialiser and mark system internal sub to be loaded
                {
                    Compiler::emitVcpuAsm("CALL", "convert" + cc + "OpAddr", false);
                    Compiler::enableSysInitFunc("Init" + cc + "Op");
                    Linker::setInternalSubToLoad("convert" + cc + "Op");
                }
            }
            break;

            case Expression::NormalCC: Compiler::emitVcpuAsm("%Jump" + Expression::strToUpper(cc), "", false); break;
            case Expression::FastCC: Compiler::emitVcpuAsm("B" + Expression::strToUpper(cc), "", false); break;

            default: break;
        }

        Compiler::emitVcpuAsm("STW", Expression::byteToHexString(uint8_t(Compiler::getTempVarStart())), false);
    }
    bool handleConditionOp(Expression::Numeric& lhs, Expression::Numeric& rhs, Expression::CCType ccType, bool& swappedLogic, bool& invertedLogic, const std::string& opcode="SUB")
    {
        // Swap left and right to take advantage of LDWI for 16bit numbers
        if(rhs._varType == Expression::Number  &&  (rhs._value < 0  ||  rhs._value > 255))
        {
            std::swap(lhs, rhs);
            swappedLogic = true;
        }

        // JumpCC and BCC are inverses of each other
        lhs._ccType = ccType;
        if(ccType == Expression::FastCC) invertedLogic = true;

        // LHS
        if(lhs._varType == Expression::Number)
        {
            // 8bit unsigned constants and 16bit signed constants
            Compiler::emitVcpuAsmForLiteral(lhs, false);
            _nextTempVar = true;
        }
        else
        {
            switch(lhs._varType)
            {
                // Temporary variable address
                case Expression::TmpVar:
                {
                    Compiler::emitVcpuAsm("LDW", Expression::byteToHexString(uint8_t(std::lround(lhs._value))), false);
                }
                break;

                // User variable name
                case Expression::IntVar16:
                {
                    if(!Compiler::emitVcpuAsmUserVar("LDW", lhs, true)) return false;
                    _nextTempVar = false;
                }
                break;

                default: break;
            }
        }

        // RHS
        if(rhs._varType == Expression::Number)
        {
            // Skip XOR if operand is 0, (n XOR 0 = n)
            if(rhs._value  ||  opcode != "XOR") Compiler::emitVcpuAsm(opcode + "I", std::to_string(uint8_t(std::lround(rhs._value))), false);
        }
        else        
        {
            switch(rhs._varType)
            {
                // Temporary variable address
                case Expression::TmpVar:
                {
                    Compiler::emitVcpuAsm(opcode + "W", Expression::byteToHexString(uint8_t(std::lround(rhs._value))), false);
                }
                break;

                // User variable name
                case Expression::IntVar16:
                {
                    if(!Compiler::emitVcpuAsmUserVar(opcode + "W", rhs, _nextTempVar)) return false;
                    _nextTempVar = false;
                }
                break;

                default: break;
            }
        }

        changeToTmpVar(lhs);

        return true;
    }

    bool handleStringCcOP(Expression::Numeric& lhs, Expression::Numeric& rhs, Expression::OPType opType)
    {
        if(lhs._varType != Expression::String  &&  lhs._varType != Expression::Constant  &&  lhs._varType != Expression::StrVar  &&  lhs._varType != Expression::Str2Var  &&
           lhs._varType != Expression::TmpStrVar) return false;
        if(rhs._varType != Expression::String  &&  rhs._varType != Expression::Constant  &&  rhs._varType != Expression::StrVar  &&  rhs._varType != Expression::Str2Var  &&
           rhs._varType != Expression::TmpStrVar) return false;

        if(lhs._varType == Expression::String  &&  rhs._varType == Expression::String)
        {
            switch(opType)
            {
                case Expression::EqOP: lhs._value = (int16_t(lhs._text.compare(rhs._text)) == 0); return true;
                case Expression::NeOP: lhs._value = (int16_t(lhs._text.compare(rhs._text)) != 0); return true;
                case Expression::LeOP: lhs._value = (int16_t(lhs._text.compare(rhs._text)) <= 0); return true;
                case Expression::GeOP: lhs._value = (int16_t(lhs._text.compare(rhs._text)) >= 0); return true;
                case Expression::LtOP: lhs._value = (int16_t(lhs._text.compare(rhs._text)) <  0); return true;
                case Expression::GtOP: lhs._value = (int16_t(lhs._text.compare(rhs._text)) >  0); return true;

                default: break;
            }
        }

        // Get addresses of strings to be compared
        int lhsIndex = int(lhs._index);
        int rhsIndex = int(rhs._index);
        std::string lhsName, rhsName;
        uint16_t lhsAddr, rhsAddr;

        // String inputs can be literal, const, var and temp
        if(lhs._varType == Expression::TmpStrVar)
        {
            lhsAddr = Compiler::getStrWorkArea();
        }
        else
        {
            Compiler::getOrCreateString(lhs, lhsName, lhsAddr, lhsIndex);
        }
        if(rhs._varType == Expression::TmpStrVar)
        {
            rhsAddr = Compiler::getStrWorkArea();
        }
        else
        {
            Compiler::getOrCreateString(rhs, rhsName, rhsAddr, rhsIndex);
        }

        // By definition this must be a match
        if(lhsAddr == rhsAddr)
        {
            Compiler::emitVcpuAsm("LDI", "1", false);
        }
        // Compare strings
        else
        {
            Compiler::emitStringAddress(lhs, lhsAddr);
            Compiler::emitVcpuAsm("STW",  "strSrcAddr", false);
            Compiler::emitStringAddress(rhs, rhsAddr);
            Compiler::emitVcpuAsm("%StringCmp", "", false);

            // 0, 1, 2, (lesser, equal, greater)
            switch(opType)
            {
                case Expression::EqOP:
                {
                    // 0, 1, 2 to 0, 1, 0
                    Compiler::emitVcpuAsm("ANDI", "1", false);
                }
                break;

                case Expression::NeOP:
                {
                    // 0, 1, 2 to 1, 0, 1
                    Compiler::emitVcpuAsm("ANDI", "1", false);
                    Compiler::emitVcpuAsm("XORI", "1", false);
                }
                break;

                case Expression::LeOP:
                {
                    // 0, 1 to 1 : 2 to 0
                    Compiler::emitVcpuAsm("XORI", "2", false);
                    if(Compiler::getCodeRomType() >= Cpu::ROMvX0  &&  Compiler::getCodeRomType() < Cpu::SDCARD)
                    {
                        Compiler::emitVcpuAsm("TNE", "giga_vAC", false);
                    }
                    else if(Compiler::getCodeRomType() >= Cpu::ROMv5a)
                    {
                        Compiler::emitVcpuAsm("CALLI", "convertNeOp", false);
                    }
                    else
                    {
                        Compiler::emitVcpuAsm("CALL", "convertNeOpAddr", false);
                        Compiler::enableSysInitFunc("InitNeOp");
                        Linker::setInternalSubToLoad("convertNeOp");
                    }
                }
                break;

                case Expression::GeOP:
                {
                    // 1, 2 to 1 : 0 to 0
                    if(Compiler::getCodeRomType() >= Cpu::ROMvX0  &&  Compiler::getCodeRomType() < Cpu::SDCARD)
                    {
                        Compiler::emitVcpuAsm("TNE", "giga_vAC", false);
                    }
                    else if(Compiler::getCodeRomType() >= Cpu::ROMv5a)
                    {
                        Compiler::emitVcpuAsm("CALLI", "convertNeOp", false);
                    }
                    else
                    {
                        Compiler::emitVcpuAsm("CALL", "convertNeOpAddr", false);
                        Compiler::enableSysInitFunc("InitNeOp");
                        Linker::setInternalSubToLoad("convertNeOp");
                    }
                }
                break;

                case Expression::LtOP:
                {
                    // 0 to 1 : 1, 2 to 0
                    if(Compiler::getCodeRomType() >= Cpu::ROMvX0  &&  Compiler::getCodeRomType() < Cpu::SDCARD)
                    {
                        Compiler::emitVcpuAsm("TEQ", "giga_vAC", false);
                    }
                    else if(Compiler::getCodeRomType() >= Cpu::ROMv5a)
                    {
                        Compiler::emitVcpuAsm("CALLI", "convertEqOp", false);
                    }
                    else
                    {
                        Compiler::emitVcpuAsm("CALL", "convertEqOpAddr", false);
                        Compiler::enableSysInitFunc("InitEqOp");
                        Linker::setInternalSubToLoad("convertEqOp");
                    }
                }
                break;

                case Expression::GtOP:
                {
                    // 0, 1 to 0 : 2 to 1
                    Compiler::emitVcpuAsm("ANDI",  "2", false);
                    if(Compiler::getCodeRomType() >= Cpu::ROMvX0  &&  Compiler::getCodeRomType() < Cpu::SDCARD)
                    {
                        Compiler::emitVcpuAsm("TNE", "giga_vAC", false);
                    }
                    else if(Compiler::getCodeRomType() >= Cpu::ROMv5a)
                    {
                        Compiler::emitVcpuAsm("CALLI", "convertNeOp", false);
                    }
                    else
                    {
                        Compiler::emitVcpuAsm("CALL", "convertNeOpAddr", false);
                        Compiler::enableSysInitFunc("InitNeOp");
                        Linker::setInternalSubToLoad("convertNeOp");
                    }
                }
                break;

                default: break;
            }
        }

        changeToTmpVar(lhs);
        Compiler::emitVcpuAsm("STW", Expression::byteToHexString(uint8_t(Compiler::getTempVarStart())), false);

        return true;
    }

    bool handleStringAdd(Expression::Numeric& lhs, Expression::Numeric& rhs)
    {
        if(lhs._varType != Expression::String  &&  lhs._varType != Expression::Constant  &&  lhs._varType != Expression::StrVar  &&  lhs._varType != Expression::Str2Var  &&
           lhs._varType != Expression::TmpStrVar) return false;
        if(rhs._varType != Expression::String  &&  rhs._varType != Expression::Constant  &&  rhs._varType != Expression::StrVar  &&  rhs._varType != Expression::Str2Var  &&
           rhs._varType != Expression::TmpStrVar) return false;

        // Get addresses of strings to be compared
        int lhsIndex = int(lhs._index);
        int rhsIndex = int(rhs._index);
        std::string lhsName, rhsName;
        uint16_t lhsAddr, rhsAddr;

        // String inputs can be literal, const, var and temp
        if(lhs._varType == Expression::TmpStrVar)
        {
            lhsAddr = Compiler::getStrWorkArea();
        }
        else
        {
            Compiler::getOrCreateString(lhs, lhsName, lhsAddr, lhsIndex);
        }
        if(rhs._varType == Expression::TmpStrVar)
        {
            // If both lhs and rhs are temporaries then get next string work area and swap lhs and rhs, (stringConcat requires dst = src0)
            if(lhs._varType == Expression::TmpStrVar) Compiler::nextStrWorkArea();
            rhsAddr = Compiler::getStrWorkArea();
            std::swap(lhs, rhs);
            std::swap(lhsAddr, rhsAddr);
        }
        else
        {
            Compiler::getOrCreateString(rhs, rhsName, rhsAddr, rhsIndex);
        }

        Compiler::emitStringAddress(lhs, lhsAddr);
        Compiler::emitVcpuAsm("STW",  "strSrcAddr", false);
        Compiler::emitStringAddress(rhs, rhsAddr);
        Compiler::emitVcpuAsm("STW",  "strSrcAddr2", false);
        Compiler::emitVcpuAsm("LDWI", Expression::wordToHexString(Compiler::getStrWorkArea()), false);
        Compiler::emitVcpuAsm("%StringConcat", "", false);

        lhs =  Expression::Numeric(0, -1, true, false, false, Expression::TmpStrVar, Expression::BooleanCC, Expression::Int16Both, "", std::string(""));

        return true;
    }

    bool handleMathOp(const std::string& opcode, const std::string& operand, Expression::Numeric& lhs, Expression::Numeric& rhs, bool isMod=false)
    {
        // LHS
        if(lhs._varType == Expression::Number)
        {
            // 8bit unsigned constants and 16bit signed constants
            Compiler::emitVcpuAsmForLiteral(lhs, false);
            _nextTempVar = true;
        }
        else
        {
            switch(lhs._varType)
            {
                // Temporary variable address
                case Expression::TmpVar:
                {
                    Compiler::emitVcpuAsm("LDW", Expression::byteToHexString(uint8_t(std::lround(lhs._value))), false);
                }
                break;

                // User variable name
                case Expression::IntVar16:
                {
                    if(!Compiler::emitVcpuAsmUserVar("LDW", lhs, true)) return false;
                    _nextTempVar = false;
                }
                break;

                default: break;
            }
        }

        (Compiler::getCodeRomType() >= Cpu::ROMvX0  &&  Compiler::getCodeRomType() < Cpu::SDCARD) ? Compiler::emitVcpuAsm("STW", "giga_sysArg0", false) : Compiler::emitVcpuAsm("STW", "mathX", false);

        // RHS
        if(rhs._varType == Expression::Number)
        {
            // 8bit unsigned constants and 16bit signed constants
            Compiler::emitVcpuAsmForLiteral(rhs, false);
        }
        else
        {
            switch(rhs._varType)
            {
                // Temporary variable address
                case Expression::TmpVar:
                {
                    Compiler::emitVcpuAsm("LDW", Expression::byteToHexString(uint8_t(std::lround(rhs._value))), false);
                }
                break;

                // User variable name
                case Expression::IntVar16:
                {
                    if(!Compiler::emitVcpuAsmUserVar("LDW", rhs, _nextTempVar)) return false;
                    _nextTempVar = false;
                }
                break;

                default: break;
            }
        }

        (Compiler::getCodeRomType() >= Cpu::ROMvX0  &&  Compiler::getCodeRomType() < Cpu::SDCARD) ? Compiler::emitVcpuAsm("STW", "giga_sysArg2", false) : Compiler::emitVcpuAsm("STW", "mathY", false);

        if(Compiler::getCodeRomType() >= Cpu::ROMv5a)
        {
            Compiler::emitVcpuAsm(opcode, operand, false);
        }
        else
        {
            Compiler::emitVcpuAsm("LDWI", operand, false);
            Compiler::emitVcpuAsm(opcode, "giga_vAC", false);
        }

        changeToTmpVar(lhs);
        
        if(isMod) (Compiler::getCodeRomType() >= Cpu::ROMvX0  &&  Compiler::getCodeRomType() < Cpu::SDCARD) ? Compiler::emitVcpuAsm("LDW", "giga_sysArg4", false) : Compiler::emitVcpuAsm("LDW", "mathRem", false);
        Compiler::emitVcpuAsm("STW", Expression::byteToHexString(uint8_t(Compiler::getTempVarStart())), false);

        return true;
    }

    uint32_t handleRevOp(uint32_t input, uint32_t n)
    {
        uint32_t output = 0;
        uint32_t bits = input & uint32_t(pow(2, n) - 1);
        for(uint32_t i=0; i<=n-1; i++)
        {
            output = (output << 1) | (bits & 1);
            bits = bits >> 1;
        }

        return output;
    }


    // ********************************************************************************************
    // Unary Operators
    // ********************************************************************************************
    Expression::Numeric POS(Expression::Numeric& numeric, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        UNREFERENCED_PARAM(moduleName);
        UNREFERENCED_PARAM(codeLineText);
        UNREFERENCED_PARAM(codeLineStart);

        if(numeric._varType == Expression::Number)
        {
            numeric._value = +numeric._value;
            return numeric;
        }

        Compiler::getNextTempVar();
        handleSingleOp("LDW", numeric);
        Compiler::emitVcpuAsm("STW", Expression::byteToHexString(uint8_t(Compiler::getTempVarStart())), false);
        
        return numeric;
    }

    Expression::Numeric NEG(Expression::Numeric& numeric, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        UNREFERENCED_PARAM(moduleName);
        UNREFERENCED_PARAM(codeLineText);
        UNREFERENCED_PARAM(codeLineStart);

        if(numeric._varType == Expression::Number)
        {
            numeric._value = -numeric._value;
            return numeric;
        }

        Compiler::getNextTempVar();
        if(Compiler::getCodeRomType() >= Cpu::ROMvX0  &&  Compiler::getCodeRomType() < Cpu::SDCARD)
        {
            handleSingleOp("LDW", numeric);
            Compiler::emitVcpuAsm("NEGW", "giga_vAC", false);
        }
        else
        {
            Compiler::emitVcpuAsm("LDI", std::to_string(0), false);
            handleSingleOp("SUBW", numeric);
        }
        Compiler::emitVcpuAsm("STW", Expression::byteToHexString(uint8_t(Compiler::getTempVarStart())), false);
        
        return numeric;
    }

    Expression::Numeric NOT(Expression::Numeric& numeric, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        UNREFERENCED_PARAM(moduleName);
        UNREFERENCED_PARAM(codeLineText);
        UNREFERENCED_PARAM(codeLineStart);

        if(numeric._varType == Expression::Number)
        {
            numeric._value = ~int16_t(std::lround(numeric._value));
            return numeric;
        }

        Compiler::getNextTempVar();
        if(Compiler::getCodeRomType() >= Cpu::ROMvX0  &&  Compiler::getCodeRomType() < Cpu::SDCARD)
        {
            handleSingleOp("LDW", numeric);
            Compiler::emitVcpuAsm("NOTW", "giga_vAC", false);
        }
        else
        {
            Compiler::emitVcpuAsm("LDWI", std::to_string(-1), false);
            handleSingleOp("XORW", numeric);
        }
        Compiler::emitVcpuAsm("STW", Expression::byteToHexString(uint8_t(Compiler::getTempVarStart())), false);

        return numeric;
    }


    // ********************************************************************************************
    // Unary Math Operators
    // ********************************************************************************************
    Expression::Numeric CEIL(Expression::Numeric& numeric, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        UNREFERENCED_PARAM(moduleName);
        UNREFERENCED_PARAM(codeLineText);
        UNREFERENCED_PARAM(codeLineStart);

        //if(!Expression::getOutputNumeric()._staticInit)
        //{
        //    Cpu::reportError(Cpu::OprError, stderr, "Operator::CEIL() : '%s:%d' : CEIL() can only be used in static initialisation : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
        //    numeric._isValid = false;
        //    return numeric;
        //}

        if(numeric._varType == Expression::Number)
        {
            numeric._value = ceil(numeric._value);
        }

        return numeric;
    }

    Expression::Numeric FLOOR(Expression::Numeric& numeric, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        UNREFERENCED_PARAM(moduleName);
        UNREFERENCED_PARAM(codeLineText);
        UNREFERENCED_PARAM(codeLineStart);

        //if(!Expression::getOutputNumeric()._staticInit)
        //{
        //    Cpu::reportError(Cpu::OprError, stderr, "Operator::FLOOR() : '%s:%d' : FLOOR() can only be used in static initialisation : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
        //    numeric._isValid = false;
        //    return numeric;
        //}

        if(numeric._varType == Expression::Number)
        {
            numeric._value = floor(numeric._value);
        }

        return numeric;
    }

    Expression::Numeric POWF(Expression::Numeric& numeric, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        UNREFERENCED_PARAM(moduleName);
        UNREFERENCED_PARAM(codeLineText);
        UNREFERENCED_PARAM(codeLineStart);

        //if(!Expression::getOutputNumeric()._staticInit)
        //{
        //    Cpu::reportError(Cpu::OprError, stderr, "Operator::POWF() : '%s:%d' : POWF() can only be used in static initialisation : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
        //    numeric._isValid = false;
        //    return numeric;
        //}

        if(numeric._varType == Expression::Number  &&  numeric._params.size() > 0  &&  numeric._params[0]._varType == Expression::Number)
        {
            numeric._value = pow(numeric._value, numeric._params[0]._value);
        }

        numeric._params.clear();
        return numeric;
    }

    Expression::Numeric SQRT(Expression::Numeric& numeric, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        UNREFERENCED_PARAM(moduleName);
        UNREFERENCED_PARAM(codeLineText);
        UNREFERENCED_PARAM(codeLineStart);

        //if(!Expression::getOutputNumeric()._staticInit)
        //{
        //    Cpu::reportError(Cpu::OprError, stderr, "Operator::SQRT() : '%s:%d' : SQRT() can only be used in static initialisation : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
        //    numeric._isValid = false;
        //    return numeric;
        //}

        if(numeric._varType == Expression::Number  &&  numeric._value > 0.0)
        {
            numeric._value = sqrt(numeric._value);
        }

        return numeric;
    }

    Expression::Numeric EXP(Expression::Numeric& numeric, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        UNREFERENCED_PARAM(moduleName);
        UNREFERENCED_PARAM(codeLineText);
        UNREFERENCED_PARAM(codeLineStart);

        //if(!Expression::getOutputNumeric()._staticInit)
        //{
        //    Cpu::reportError(Cpu::OprError, stderr, "Operator::EXP() : '%s:%d' : EXP() can only be used in static initialisation : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
        //    numeric._isValid = false;
        //    return numeric;
        //}

        if(numeric._varType == Expression::Number)
        {
            numeric._value = exp(numeric._value);
        }

        return numeric;
    }

    Expression::Numeric EXP2(Expression::Numeric& numeric, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        UNREFERENCED_PARAM(moduleName);
        UNREFERENCED_PARAM(codeLineText);
        UNREFERENCED_PARAM(codeLineStart);

        //if(!Expression::getOutputNumeric()._staticInit)
        //{
        //    Cpu::reportError(Cpu::OprError, stderr, "Operator::EXP2() : '%s:%d' : EXP2() can only be used in static initialisation : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
        //    numeric._isValid = false;
        //    return numeric;
        //}

        if(numeric._varType == Expression::Number)
        {
            numeric._value = exp2(numeric._value);
        }

        return numeric;
    }

    Expression::Numeric LOG(Expression::Numeric& numeric, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        UNREFERENCED_PARAM(moduleName);
        UNREFERENCED_PARAM(codeLineText);
        UNREFERENCED_PARAM(codeLineStart);

        //if(!Expression::getOutputNumeric()._staticInit)
        //{
        //    Cpu::reportError(Cpu::OprError, stderr, "Operator::LOG() : '%s:%d' : LOG() can only be used in static initialisation : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
        //    numeric._isValid = false;
        //    return numeric;
        //}

        if(numeric._varType == Expression::Number  &&  numeric._value > 0.0)
        {
            numeric._value = log(numeric._value);
        }

        return numeric;
    }

    Expression::Numeric LOG2(Expression::Numeric& numeric, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        UNREFERENCED_PARAM(moduleName);
        UNREFERENCED_PARAM(codeLineText);
        UNREFERENCED_PARAM(codeLineStart);

        //if(!Expression::getOutputNumeric()._staticInit)
        //{
        //    Cpu::reportError(Cpu::OprError, stderr, "Operator::LOG2() : '%s:%d' : LOG2() can only be used in static initialisation : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
        //    numeric._isValid = false;
        //    return numeric;
        //}

        if(numeric._varType == Expression::Number  &&  numeric._value > 0.0)
        {
            numeric._value = log2(numeric._value);
        }

        return numeric;
    }

    Expression::Numeric LOG10(Expression::Numeric& numeric, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        UNREFERENCED_PARAM(moduleName);
        UNREFERENCED_PARAM(codeLineText);
        UNREFERENCED_PARAM(codeLineStart);

        //if(!Expression::getOutputNumeric()._staticInit)
        //{
        //    Cpu::reportError(Cpu::OprError, stderr, "Operator::LOG10() : '%s:%d' : LOG10() can only be used in static initialisation : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
        //    numeric._isValid = false;
        //    return numeric;
        //}

        if(numeric._varType == Expression::Number  &&  numeric._value > 0.0)
        {
            numeric._value = log10(numeric._value);
        }

        return numeric;
    }

    Expression::Numeric SIN(Expression::Numeric& numeric, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        UNREFERENCED_PARAM(moduleName);
        UNREFERENCED_PARAM(codeLineText);
        UNREFERENCED_PARAM(codeLineStart);

        //if(!Expression::getOutputNumeric()._staticInit)
        //{
        //    Cpu::reportError(Cpu::OprError, stderr, "Operator::SIN() : '%s:%d' : SIN() can only be used in static initialisation : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
        //    numeric._isValid = false;
        //    return numeric;
        //}

        if(numeric._varType == Expression::Number)
        {
            numeric._value = sin(numeric._value*MATH_PI/180.0);
        }

        return numeric;
    }

    Expression::Numeric COS(Expression::Numeric& numeric, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        UNREFERENCED_PARAM(moduleName);
        UNREFERENCED_PARAM(codeLineText);
        UNREFERENCED_PARAM(codeLineStart);

        //if(!Expression::getOutputNumeric()._staticInit)
        //{
        //    Cpu::reportError(Cpu::OprError, stderr, "Operator::COS() : '%s:%d' : COS() can only be used in static initialisation : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
        //    numeric._isValid = false;
        //    return numeric;
        //}

        if(numeric._varType == Expression::Number)
        {
            numeric._value = cos(numeric._value*MATH_PI/180.0);
        }

        return numeric;
    }

    Expression::Numeric TAN(Expression::Numeric& numeric, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        UNREFERENCED_PARAM(moduleName);
        UNREFERENCED_PARAM(codeLineText);
        UNREFERENCED_PARAM(codeLineStart);

        //if(!Expression::getOutputNumeric()._staticInit)
        //{
        //    Cpu::reportError(Cpu::OprError, stderr, "Operator::TAN() : '%s:%d' : TAN() can only be used in static initialisation : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
        //    numeric._isValid = false;
        //    return numeric;
        //}

        if(numeric._varType == Expression::Number)
        {
            numeric._value = tan(numeric._value*MATH_PI/180.0);
        }

        return numeric;
    }

    Expression::Numeric ASIN(Expression::Numeric& numeric, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        UNREFERENCED_PARAM(moduleName);
        UNREFERENCED_PARAM(codeLineText);
        UNREFERENCED_PARAM(codeLineStart);

        //if(!Expression::getOutputNumeric()._staticInit)
        //{
        //    Cpu::reportError(Cpu::OprError, stderr, "Operator::ASIN() : '%s:%d' : ASIN() can only be used in static initialisation : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
        //    numeric._isValid = false;
        //    return numeric;
        //}

        if(numeric._varType == Expression::Number)
        {
            numeric._value = asin(numeric._value)/MATH_PI*180.0;
        }

        return numeric;
    }

    Expression::Numeric ACOS(Expression::Numeric& numeric, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        UNREFERENCED_PARAM(moduleName);
        UNREFERENCED_PARAM(codeLineText);
        UNREFERENCED_PARAM(codeLineStart);

        //if(!Expression::getOutputNumeric()._staticInit)
        //{
        //    Cpu::reportError(Cpu::OprError, stderr, "Operator::ACOS() : '%s:%d' : ACOS() can only be used in static initialisation : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
        //    numeric._isValid = false;
        //    return numeric;
        //}

        if(numeric._varType == Expression::Number)
        {
            numeric._value = acos(numeric._value)/MATH_PI*180.0;
        }

        return numeric;
    }

    Expression::Numeric ATAN(Expression::Numeric& numeric, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        UNREFERENCED_PARAM(moduleName);
        UNREFERENCED_PARAM(codeLineText);
        UNREFERENCED_PARAM(codeLineStart);

        //if(!Expression::getOutputNumeric()._staticInit)
        //{
        //    Cpu::reportError(Cpu::OprError, stderr, "Operator::ATAN() : '%s:%d' : ATAN() can only be used in static initialisation : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
        //    numeric._isValid = false;
        //    return numeric;
        //}

        if(numeric._varType == Expression::Number)
        {
            numeric._value = atan(numeric._value)/MATH_PI*180.0;
        }

        return numeric;
    }

    Expression::Numeric ATAN2(Expression::Numeric& numeric, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        UNREFERENCED_PARAM(moduleName);
        UNREFERENCED_PARAM(codeLineText);
        UNREFERENCED_PARAM(codeLineStart);

        //if(!Expression::getOutputNumeric()._staticInit)
        //{
        //    Cpu::reportError(Cpu::OprError, stderr, "Operator::ATAN2() : '%s:%d' : ATAN2() can only be used in static initialisation : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
        //    numeric._isValid = false;
        //    return numeric;
        //}

        if(numeric._varType == Expression::Number  &&  numeric._params.size() > 0  &&  numeric._params[0]._varType == Expression::Number)
        {
            if(numeric._value != 0.0  ||  numeric._params[0]._value != 0.0)
            {
                numeric._value = atan2(numeric._value, numeric._params[0]._value)/MATH_PI*180.0;
            }
        }

        numeric._params.clear();
        return numeric;
    }

    Expression::Numeric RAND(Expression::Numeric& numeric, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        UNREFERENCED_PARAM(moduleName);
        UNREFERENCED_PARAM(codeLineText);
        UNREFERENCED_PARAM(codeLineStart);

        //if(!Expression::getOutputNumeric()._staticInit)
        //{
        //    Cpu::reportError(Cpu::OprError, stderr, "Operator::RAND() : '%s:%d' : RAND() can only be used in static initialisation, try RND() : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
        //    numeric._isValid = false;
        //    return numeric;
        //}

        if(numeric._varType == Expression::Number)
        {
            long value = std::lround(numeric._value);
            numeric._value = (value <= 0) ? 0 : double(rand() % value);
        }

        return numeric;
    }

    Expression::Numeric REV16(Expression::Numeric& numeric, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        UNREFERENCED_PARAM(moduleName);
        UNREFERENCED_PARAM(codeLineText);
        UNREFERENCED_PARAM(codeLineStart);

        //if(!Expression::getOutputNumeric()._staticInit)
        //{
        //    Cpu::reportError(Cpu::OprError, stderr, "Operator::REV16() : '%s:%d' : REV16() can only be used in static initialisation : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
        //    numeric._isValid = false;
        //    return numeric;
        //}

        if(numeric._varType == Expression::Number)
        {
            numeric._value = double(handleRevOp(uint32_t(std::lround(numeric._value)), 16));
        }

        return numeric;
    }

    Expression::Numeric REV8(Expression::Numeric& numeric, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        UNREFERENCED_PARAM(moduleName);
        UNREFERENCED_PARAM(codeLineText);
        UNREFERENCED_PARAM(codeLineStart);

        //if(!Expression::getOutputNumeric()._staticInit)
        //{
        //    Cpu::reportError(Cpu::OprError, stderr, "Operator::REV8() : '%s:%d' : REV8() can only be used in static initialisation : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
        //    numeric._isValid = false;
        //    return numeric;
        //}

        if(numeric._varType == Expression::Number)
        {
            numeric._value = double(handleRevOp(uint32_t(std::lround(numeric._value)), 8));
        }

        return numeric;
    }

    Expression::Numeric REV4(Expression::Numeric& numeric, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        UNREFERENCED_PARAM(moduleName);
        UNREFERENCED_PARAM(codeLineText);
        UNREFERENCED_PARAM(codeLineStart);

        //if(!Expression::getOutputNumeric()._staticInit)
        //{
        //    Cpu::reportError(Cpu::OprError, stderr, "Operator::REV4() : '%s:%d' : REV4() can only be used in static initialisation : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
        //    numeric._isValid = false;
        //    return numeric;
        //}

        if(numeric._varType == Expression::Number)
        {
            numeric._value = double(handleRevOp(uint32_t(std::lround(numeric._value)), 4));
        }

        return numeric;
    }


    // ********************************************************************************************
    // Binary Operators
    // ********************************************************************************************
    Expression::Numeric ADD(Expression::Numeric& left, Expression::Numeric& right, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        UNREFERENCED_PARAM(moduleName);
        UNREFERENCED_PARAM(codeLineText);
        UNREFERENCED_PARAM(codeLineStart);

        if(handleStringAdd(left, right)) return left;

        if(left._varType == Expression::Number  &&  right._varType == Expression::Number)
        {
            left._value += right._value;
            return left;
        }

        left._isValid = handleDualOp("ADD", left, right, false);
        return left;
    }

    Expression::Numeric SUB(Expression::Numeric& left, Expression::Numeric& right, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        UNREFERENCED_PARAM(moduleName);
        UNREFERENCED_PARAM(codeLineText);
        UNREFERENCED_PARAM(codeLineStart);

        if(left._varType == Expression::Number  &&  right._varType == Expression::Number)
        {
            left._value -= right._value;
            return left;
        }

        left._isValid = handleDualOp("SUB", left, right, false);
        return left;
    }

    Expression::Numeric AND(Expression::Numeric& left, Expression::Numeric& right, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        UNREFERENCED_PARAM(moduleName);
        UNREFERENCED_PARAM(codeLineText);
        UNREFERENCED_PARAM(codeLineStart);

        if(left._varType == Expression::Number  &&  right._varType == Expression::Number)
        {
            left._value = int16_t(std::lround(left._value)) & int16_t(std::lround(right._value));
            return left;
        }

        left._isValid = handleDualOp("AND", left, right, true);
        return left;
    }

    Expression::Numeric XOR(Expression::Numeric& left, Expression::Numeric& right, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        UNREFERENCED_PARAM(moduleName);
        UNREFERENCED_PARAM(codeLineText);
        UNREFERENCED_PARAM(codeLineStart);

        if(left._varType == Expression::Number  &&  right._varType == Expression::Number)
        {
            left._value = int16_t(std::lround(left._value)) ^ int16_t(std::lround(right._value));
            return left;
        }

        left._isValid = handleDualOp("XOR", left, right, true);
        return left;
    }

    Expression::Numeric OR(Expression::Numeric& left, Expression::Numeric& right, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        UNREFERENCED_PARAM(moduleName);
        UNREFERENCED_PARAM(codeLineText);
        UNREFERENCED_PARAM(codeLineStart);

        if(left._varType == Expression::Number  &&  right._varType == Expression::Number)
        {
            left._value = int16_t(std::lround(left._value)) | int16_t(std::lround(right._value));
            return left;
        }

        left._isValid = handleDualOp("OR", left, right, true);
        return left;
    }


    // ********************************************************************************************
    // Logical Operators
    // ********************************************************************************************
    Expression::Numeric LSL(Expression::Numeric& left, Expression::Numeric& right, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        UNREFERENCED_PARAM(moduleName);
        UNREFERENCED_PARAM(codeLineText);
        UNREFERENCED_PARAM(codeLineStart);

        if(right._varType != Expression::Number)
        {
            Cpu::reportError(Cpu::OprError, stderr, "Operator::LSL() : '%s:%d' : right hand side operand must be a literal : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
            left._isValid = false;
            return left;
        }
        if(left._varType != Expression::TmpVar  &&  left._varType != Expression::IntVar16)
        {
            Cpu::reportError(Cpu::OprError, stderr, "Operator::LSL() : '%s:%d' : left hand side operand must be an int16 var or int16 expression : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
            left._isValid = false;
            return left;
        }

        // LHS and RHS are both literals
        if(left._varType == Expression::Number)
        {
            left._value = (int16_t(std::lround(left._value)) << int16_t(std::lround(right._value))) & 0x0000FFFF;
            return left;
        }

        if(right._value < 1  ||  right._value > 8)
        {
            Cpu::reportError(Cpu::OprError, stderr, "Operator::LSL() : '%s:%d' : right hand side operand must be a literal between 1 and 8 : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
            left._isValid = false;
            return left;
        }

        Compiler::getNextTempVar();

        if(right._value == 8)
        {
            if(Compiler::getCodeRomType() >= Cpu::ROMvX0  &&  Compiler::getCodeRomType() < Cpu::SDCARD)
            {
                switch(left._varType)
                {
                    // Temporary variable address
                    case Expression::TmpVar:
                    {
                        Compiler::emitVcpuAsm("MOVB", Expression::byteToHexString(uint8_t(std::lround(left._value))) + ", giga_vAC + 1", false);
                        Compiler::emitVcpuAsm("MOVQB", "giga_vAC, 0", false);
                    }
                    break;

                    // User variable name
                    case Expression::IntVar16:
                    {
                        int varIndex = Compiler::findVar(left._name);
                        if(varIndex == -1)
                        {
                            Cpu::reportError(Cpu::OprError, stderr, "Operator::LSL() : '%s:%d' : couldn't find variable name '%s' : %s\n", moduleName.c_str(), codeLineStart, left._name.c_str(), codeLineText.c_str());
                            left._isValid = false;
                            return left;
                        }

                        Compiler::emitVcpuAsm("MOVB", "_" + left._name + ", giga_vAC + 1", false);
                        Compiler::emitVcpuAsm("MOVQB", "giga_vAC, 0", false);
                    }
                    break;

                    default: break;
                }
            }
            else
            {
                switch(left._varType)
                {
                    // Temporary variable address
                    case Expression::TmpVar:
                    {
                        Compiler::emitVcpuAsm("LD", Expression::byteToHexString(uint8_t(std::lround(left._value))), false);
                    }
                    break;

                    // User variable name
                    case Expression::IntVar16:
                    {
                        int varIndex = Compiler::findVar(left._name);
                        if(varIndex == -1)
                        {
                            Cpu::reportError(Cpu::OprError, stderr, "Operator::LSL() : '%s:%d' : couldn't find variable name '%s' : %s\n", moduleName.c_str(), codeLineStart, left._name.c_str(), codeLineText.c_str());
                            left._isValid = false;
                            return left;
                        }

                        Compiler::emitVcpuAsm("LD", "_" + left._name, false);
                    }
                    break;

                    default: break;
                }

                // Move low byte to high byte
                Compiler::emitVcpuAsm("ST", "giga_vAC + 1", false);

                // Clear low byte
                Compiler::emitVcpuAsm("ORI", "0xFF", false);
                Compiler::emitVcpuAsm("XORI", "0xFF", false);
            }

            changeToTmpVar(left);
        }
        else
        {
            handleLogicalOp("LSLW", left);

            int n = int(std::lround(right._value));
            if(Compiler::getCodeRomType() >= Cpu::ROMvX0  &&  Compiler::getCodeRomType() < Cpu::SDCARD  &&  n > 4)
            {
                Compiler::emitVcpuAsm("LSLN", std::to_string(n), false);
            }
            else
            {
                for(int i=0; i<n; i++)
                {
                    Compiler::emitVcpuAsm("LSLW", "", false);
                }
            }
        }

        Compiler::emitVcpuAsm("STW", Expression::byteToHexString(uint8_t(Compiler::getTempVarStart())), false);

        return left;
    }

    Expression::Numeric LSR(Expression::Numeric& left, Expression::Numeric& right, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        UNREFERENCED_PARAM(moduleName);
        UNREFERENCED_PARAM(codeLineText);
        UNREFERENCED_PARAM(codeLineStart);

        if(right._varType != Expression::Number)
        {
            Cpu::reportError(Cpu::OprError, stderr, "Operator::LSR() : '%s:%d' : right hand side operand must be a literal : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
            left._isValid = false;
            return left;
        }
        if(left._varType != Expression::TmpVar  &&  left._varType != Expression::IntVar16)
        {
            Cpu::reportError(Cpu::OprError, stderr, "Operator::LSR() : '%s:%d' : left hand side operand must be an int16 var or int16 expression : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
            left._isValid = false;
            return left;
        }

        // LHS and RHS are both literals
        if(left._varType == Expression::Number)
        {
            left._value = int16_t(std::lround(left._value)) >> int16_t(std::lround(right._value));
            return left;
        }

        if(right._value < 1  ||  right._value > 8)
        {
            Cpu::reportError(Cpu::OprError, stderr, "Operator::LSR() : '%s:%d' : right hand side operand must be a literal between 1 and 8 : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
            left._isValid = false;
            return left;
        }

        Compiler::getNextTempVar();

        // Optimised high byte read
        if(right._value == 8)
        {
            switch(left._varType)
            {
                // Temporary variable address
                case Expression::TmpVar:
                {
                    Compiler::emitVcpuAsm("LD", Expression::byteToHexString(uint8_t(std::lround(left._value))) + " + 1", false);
                }
                break;

                // User variable name
                case Expression::IntVar16:
                {
                    int varIndex = Compiler::findVar(left._name);
                    if(varIndex == -1)
                    {
                        Cpu::reportError(Cpu::OprError, stderr, "Operator::LSR() : '%s:%d' : couldn't find variable name '%s' : %s\n", moduleName.c_str(), codeLineStart, left._name.c_str(), codeLineText.c_str());
                        left._isValid = false;
                        return left;
                    }

                    Compiler::emitVcpuAsm("LD", "_" + left._name + " + 1", false);
                }
                break;

                default: break;
            }

            changeToTmpVar(left);
        }
        else
        {
            // LSR sys call
            if(left._int16Byte == Expression::Int16Both  ||  right._value > 4  ||  Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD)
            {
                std::string opcode;
                switch(int16_t(std::lround(right._value)))
                {
                    case 1:
                    {
                        if(Compiler::getCodeRomType() >= Cpu::ROMvX0  &&  Compiler::getCodeRomType() < Cpu::SDCARD)
                        {
                            opcode = "LSRV";
                        }
                        else
                        {
                            opcode = "%Lsr1bit";
                        }
                    }
                    break;

                    case 2:
                    {
                        if(Compiler::getCodeRomType() >= Cpu::ROMvX0  &&  Compiler::getCodeRomType() < Cpu::SDCARD)
                        {
                            opcode = "LSRV";
                        }
                        else
                        {
                            opcode = "%Lsr2bit";
                        }
                    }
                    break;

                    case 3: opcode = "%Lsr3bit"; break;
                    case 4: opcode = "%Lsr4bit"; break;
                    case 5: opcode = "%Lsr5bit"; break;
                    case 6: opcode = "%Lsr6bit"; break;
                    case 7: opcode = "%Lsr7bit"; break;

                    default: break;
                }

                handleLogicalOp(opcode, left);
                if(Compiler::getCodeRomType() >= Cpu::ROMvX0  &&  Compiler::getCodeRomType() < Cpu::SDCARD)
                {
                    if(int16_t(std::lround(right._value)) == 2) Compiler::emitVcpuAsm(opcode, "giga_vAC", false);
                    Compiler::emitVcpuAsm(opcode, "giga_vAC", false);
                }
                else
                {
                    Compiler::emitVcpuAsm(opcode, "", false);
                }
            }
            // LSRB optimised for 8 bit
            else
            {
                handleLogicalOp("LSRB", left);
                for(int i=0; i<int(std::lround(right._value)); i++)
                {
                    Compiler::emitVcpuAsm("LSRB", "giga_vAC", false);
                }
            }
        }

        Compiler::emitVcpuAsm("STW", Expression::byteToHexString(uint8_t(Compiler::getTempVarStart())), false);

        return left;
    }

    Expression::Numeric ASR(Expression::Numeric& left, Expression::Numeric& right, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        UNREFERENCED_PARAM(moduleName);
        UNREFERENCED_PARAM(codeLineText);
        UNREFERENCED_PARAM(codeLineStart);

        if(right._varType != Expression::Number)
        {
            Cpu::reportError(Cpu::OprError, stderr, "Operator::ASR() : '%s:%d' : right hand side operand must be a literal : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
            left._isValid = false;
            return left;
        }
        if(left._varType != Expression::TmpVar  &&  left._varType != Expression::IntVar16)
        {
            Cpu::reportError(Cpu::OprError, stderr, "Operator::ASR() : '%s:%d' : left hand side operand must be an int16 var or int16 expression : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
            left._isValid = false;
            return left;
        }

        // LHS and RHS are literals
        if(left._varType == Expression::Number)
        {
            left._value /= (1 << int16_t(std::lround(right._value))) & 0x0000FFFF;
            return left;
        }

        if(right._value < 1  ||  right._value > 8)
        {
            Cpu::reportError(Cpu::OprError, stderr, "Operator::ASR() : '%s:%d' : right hand side operand must be a literal between 1 and 8 : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
            left._isValid = false;
            return left;
        }

        Compiler::getNextTempVar();

        std::string opcode;
        switch(int16_t(std::lround(right._value)))
        {
            case 1: opcode = "%Asr1bit"; break;
            case 2: opcode = "%Asr2bit"; break;
            case 3: opcode = "%Asr3bit"; break;
            case 4: opcode = "%Asr4bit"; break;
            case 5: opcode = "%Asr5bit"; break;
            case 6: opcode = "%Asr6bit"; break;
            case 7: opcode = "%Asr7bit"; break;
            case 8: opcode = "%Asr8bit"; break;

            default: break;
        }

        handleLogicalOp(opcode, left);
        Compiler::emitVcpuAsm(opcode, "", false);
        Compiler::emitVcpuAsm("STW", Expression::byteToHexString(uint8_t(Compiler::getTempVarStart())), false);

        return left;
    }


    // ********************************************************************************************
    // Conditional Operators
    // ********************************************************************************************
    Expression::Numeric EQ(Expression::Numeric& left, Expression::Numeric& right, Expression::CCType ccType, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        UNREFERENCED_PARAM(moduleName);
        UNREFERENCED_PARAM(codeLineText);
        UNREFERENCED_PARAM(codeLineStart);

        if(handleStringCcOP(left, right, Expression::EqOP)) return left;

        if(left._varType == Expression::Number  &&  right._varType == Expression::Number)
        {
            left._value = (left._value == right._value);
            return left;
        }

        bool swappedLogic = false;
        bool invertedLogic = false;
        left._isValid = handleConditionOp(left, right, ccType, swappedLogic, invertedLogic, "XOR");

        // Swap does nothing to EQ, invert changes EQ to NE
        std::string cc = (invertedLogic) ? "Ne" : "Eq";
        emitCcType(ccType, cc);

        return left;
    }

    Expression::Numeric NE(Expression::Numeric& left, Expression::Numeric& right, Expression::CCType ccType, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        UNREFERENCED_PARAM(moduleName);
        UNREFERENCED_PARAM(codeLineText);
        UNREFERENCED_PARAM(codeLineStart);

        if(handleStringCcOP(left, right, Expression::NeOP)) return left;

        if(left._varType == Expression::Number  &&  right._varType == Expression::Number)
        {
            left._value = left._value != right._value;
            return left;
        }

        bool swappedLogic = false;
        bool invertedLogic = false;
        left._isValid = handleConditionOp(left, right, ccType, swappedLogic, invertedLogic, "XOR");

        // Swap does nothing to EQ, invert changes EQ to NE
        std::string cc = (invertedLogic) ? "Eq" : "Ne";
        emitCcType(ccType, cc);

        return left;
    }

    Expression::Numeric LE(Expression::Numeric& left, Expression::Numeric& right, Expression::CCType ccType, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        UNREFERENCED_PARAM(moduleName);
        UNREFERENCED_PARAM(codeLineText);
        UNREFERENCED_PARAM(codeLineStart);

        if(handleStringCcOP(left, right, Expression::LeOP)) return left;

        if(left._varType == Expression::Number  &&  right._varType == Expression::Number)
        {
            left._value = left._value <= right._value;
            return left;
        }

        bool swappedLogic = false;
        bool invertedLogic = false;
        left._isValid = handleConditionOp(left, right, ccType, swappedLogic, invertedLogic);
        
        // Swap LE to GE
        std::string cc = (swappedLogic) ? "Ge" : "Le";

        // Invert LE or GE
        if(invertedLogic)
        {
            if(cc == "Le") cc = "Gt";
            if(cc == "Ge") cc = "Lt";
        }
        emitCcType(ccType, cc);

        return left;
    }

    Expression::Numeric GE(Expression::Numeric& left, Expression::Numeric& right, Expression::CCType ccType, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        UNREFERENCED_PARAM(moduleName);
        UNREFERENCED_PARAM(codeLineText);
        UNREFERENCED_PARAM(codeLineStart);

        if(handleStringCcOP(left, right, Expression::GeOP)) return left;

        if(left._varType == Expression::Number  &&  right._varType == Expression::Number)
        {
            left._value = left._value >= right._value;
            return left;
        }

        bool swappedLogic = false;
        bool invertedLogic = false;
        left._isValid = handleConditionOp(left, right, ccType, swappedLogic, invertedLogic);

        // Swap GE to LE
        std::string cc = (swappedLogic) ? "le" : "Ge";

        // Invert GE or LE
        if(invertedLogic)
        {
            if(cc == "Ge") cc = "Lt";
            if(cc == "Le") cc = "Gt";
        }
        emitCcType(ccType, cc);

        return left;
    }

    Expression::Numeric LT(Expression::Numeric& left, Expression::Numeric& right, Expression::CCType ccType, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        UNREFERENCED_PARAM(moduleName);
        UNREFERENCED_PARAM(codeLineText);
        UNREFERENCED_PARAM(codeLineStart);

        if(handleStringCcOP(left, right, Expression::LtOP)) return left;

        if(left._varType == Expression::Number  &&  right._varType == Expression::Number)
        {
            left._value = (left._value < right._value);
            return left;
        }

        bool swappedLogic = false;
        bool invertedLogic = false;
        left._isValid = handleConditionOp(left, right, ccType, swappedLogic, invertedLogic);

        // Swap LT to GT
        std::string cc = (swappedLogic) ? "Gt" : "Lt";

        // Invert LT or GT
        if(invertedLogic)
        {
            if(cc == "Lt") cc = "Ge";
            if(cc == "Gt") cc = "Le";
        }
        emitCcType(ccType, cc);

        return left;
    }

    Expression::Numeric GT(Expression::Numeric& left, Expression::Numeric& right, Expression::CCType ccType, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        UNREFERENCED_PARAM(moduleName);
        UNREFERENCED_PARAM(codeLineText);
        UNREFERENCED_PARAM(codeLineStart);

        if(handleStringCcOP(left, right, Expression::GtOP)) return left;

        if(left._varType == Expression::Number  &&  right._varType == Expression::Number)
        {
            left._value = (left._value > right._value);
            return left;
        }

        bool swappedLogic = false;
        bool invertedLogic = false;
        left._isValid = handleConditionOp(left, right, ccType, swappedLogic, invertedLogic);

        // Swap GT to LT
        std::string cc = (swappedLogic) ? "Lt" : "Gt";

        // Invert GT or LT
        if(invertedLogic)
        {
            if(cc == "Gt") cc = "Le";
            if(cc == "Lt") cc = "Ge";
        }
        emitCcType(ccType, cc);

        return left;
    }


    // ********************************************************************************************
    // Math Operators
    // ********************************************************************************************
    Expression::Numeric POW(Expression::Numeric& left, Expression::Numeric& right, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        UNREFERENCED_PARAM(moduleName);
        UNREFERENCED_PARAM(codeLineText);
        UNREFERENCED_PARAM(codeLineStart);

        if(left._varType == Expression::Number  &&  right._varType == Expression::Number)
        {
            left._value = pow(double(left._value), double(right._value));
            return left;
        }

        // Optimise base = 0
        if(left._varType == Expression::Number  &&  left._value == 0)
        {
            return Expression::Numeric(0, -1, true, false, false, Expression::Number, Expression::BooleanCC, Expression::Int16Both, std::string(""), std::string(""));
        }
        // Optimise base = 1
        else if(left._varType == Expression::Number  &&  left._value == 1)
        {
            return Expression::Numeric(1, -1, true, false, false, Expression::Number, Expression::BooleanCC, Expression::Int16Both, std::string(""), std::string(""));
        }
        // Optimise exponent = 0
        else if(right._varType == Expression::Number  &&  right._value == 0)
        {
            return Expression::Numeric(1, -1, true, false, false, Expression::Number, Expression::BooleanCC, Expression::Int16Both, std::string(""), std::string(""));
        }

        left._isValid = (Compiler::getCodeRomType() >= Cpu::ROMv5a) ? handleMathOp("CALLI", "power16bit", left, right) : handleMathOp("CALL", "power16bit", left, right);

        return left;
    }

    Expression::Numeric MUL(Expression::Numeric& left, Expression::Numeric& right, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        UNREFERENCED_PARAM(moduleName);
        UNREFERENCED_PARAM(codeLineText);
        UNREFERENCED_PARAM(codeLineStart);

        if(left._varType == Expression::Number  &&  right._varType == Expression::Number)
        {
            left._value *= right._value;
            return left;
        }

        // Optimise multiply with 0
        if((left._varType == Expression::Number  &&  left._value == 0)  ||  (right._varType == Expression::Number  &&  right._value == 0))
        {
            return Expression::Numeric(0, -1, true, false, false, Expression::Number, Expression::BooleanCC, Expression::Int16Both, std::string(""), std::string(""));
        }

        left._isValid = (Compiler::getCodeRomType() >= Cpu::ROMv5a) ? handleMathOp("CALLI", "multiply16bit", left, right) : handleMathOp("CALL", "multiply16bit", left, right);

        return left;
    }

    Expression::Numeric DIV(Expression::Numeric& left, Expression::Numeric& right, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        UNREFERENCED_PARAM(moduleName);
        UNREFERENCED_PARAM(codeLineText);
        UNREFERENCED_PARAM(codeLineStart);

        if(left._varType == Expression::Number  &&  right._varType == Expression::Number)
        {
            left._value = (right._value == 0) ? 0 : left._value / right._value;
            return left;
        }

        // Optimise divide with 0, term() never lets denominator = 0
        if((left._varType == Expression::Number  &&  left._value == 0)  ||  (right._varType == Expression::Number  &&  right._value == 0))
        {
            return Expression::Numeric(0, -1, true, false, false, Expression::Number, Expression::BooleanCC, Expression::Int16Both, std::string(""), std::string(""));
        }

        left._isValid = (Compiler::getCodeRomType() >= Cpu::ROMv5a) ? handleMathOp("CALLI", "divide16bit", left, right) : handleMathOp("CALL", "divide16bit", left, right);

        return left;
    }

    Expression::Numeric MOD(Expression::Numeric& left, Expression::Numeric& right, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        UNREFERENCED_PARAM(moduleName);
        UNREFERENCED_PARAM(codeLineText);
        UNREFERENCED_PARAM(codeLineStart);

        if(left._varType == Expression::Number  &&  right._varType == Expression::Number)
        {
            left._value = (int16_t(std::lround(right._value)) == 0) ? 0 : int16_t(std::lround(left._value)) % int16_t(std::lround(right._value));
            return left;
        }

        // Optimise divide with 0, term() never lets denominator = 0
        if((left._varType == Expression::Number  &&  left._value == 0)  ||  (right._varType == Expression::Number  &&  right._value == 0))
        {
            return Expression::Numeric(0, -1, true, false, false, Expression::Number, Expression::BooleanCC, Expression::Int16Both, std::string(""), std::string(""));
        }

        left._isValid = (Compiler::getCodeRomType() >= Cpu::ROMv5a) ? handleMathOp("CALLI", "divide16bit", left, right, true) : handleMathOp("CALL", "divide16bit", left, right, true);

        return left;
    }
}