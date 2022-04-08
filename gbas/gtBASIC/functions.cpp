#include <ctime>
#include <random>
#include <numeric>
#include <algorithm>

#include "memory.h"
#include "cpu.h"
#include "functions.h"
#include "operators.h"


namespace Functions
{
    int _nestedCount = -1;

    double _umin = 0.0;
    double _umax = 0.0;
    double _ulen = 0.0;
    double _ustp = 1.0;
    uint16_t _uidx = 0;
    std::vector<int16_t> _uvalues;

    std::mt19937_64 _randGenerator;

    std::map<std::string, std::string> _functions;
    std::map<std::string, std::string> _stringFunctions;


    std::map<std::string, std::string>& getFunctions(void)       {return _functions;      }
    std::map<std::string, std::string>& getStringFunctions(void) {return _stringFunctions;}


    void restart(void)
    {
        _nestedCount = -1;

        _umin = _umax = _ulen = 0.0;
        _ustp = 1.0;
        _uidx = 0;
        _uvalues.clear();
    }

    bool initialise(void)
    {
        restart();

        // Functions
        _functions["IARR"   ] = "IARR";
        _functions["SARR"   ] = "SARR";
        _functions["PEEK"   ] = "PEEK";
        _functions["DEEK"   ] = "DEEK";
        _functions["USR"    ] = "USR";
        _functions["RND"    ] = "RND";
        _functions["URND"   ] = "URND";
        _functions["LEN"    ] = "LEN";
        _functions["GET"    ] = "GET";
        _functions["ABS"    ] = "ABS";
        _functions["SGN"    ] = "SGN";
        _functions["ASC"    ] = "ASC";
        _functions["STRCMP" ] = "STRCMP";
        _functions["BCDCMP" ] = "BCDCMP";
        _functions["VAL"    ] = "VAL";
        _functions["LUP"    ] = "LUP";
        _functions["ADDR"   ] = "ADDR";
        _functions["ARRB"   ] = "ARRB";
        _functions["ARRW"   ] = "ARRW";
        _functions["POINT"  ] = "POINT";
        _functions["MIN"    ] = "MIN";
        _functions["MAX"    ] = "MAX";
        _functions["CLAMP"  ] = "CLAMP";
        _functions["FMUL"   ] = "FMUL";
        _functions["FDIV"   ] = "FDIV";
        _functions["FMOD"   ] = "FMOD";
        _functions["FDIVMOD"] = "FDIVMOD";
        _functions["SQRT"   ] = "SQRT";

        // String functions
        _stringFunctions["CHR$"   ] = "CHR$";
        _stringFunctions["SPC$"   ] = "SPC$";
        _stringFunctions["STR$"   ] = "STR$";
        _stringFunctions["STRING$"] = "STRING$";
        _stringFunctions["TIME$"  ] = "TIME$";
        _stringFunctions["HEX$"   ] = "HEX$";
        _stringFunctions["LEFT$"  ] = "LEFT$";
        _stringFunctions["RIGHT$" ] = "RIGHT$";
        _stringFunctions["MID$"   ] = "MID$";
        _stringFunctions["LCASE$" ] = "LCASE$";
        _stringFunctions["UCASE$" ] = "UCASE$";
        _stringFunctions["STRCAT$"] = "STRCAT$";

        uint64_t timeSeed = time(NULL);
        std::seed_seq seedSequence{uint32_t(timeSeed & 0xffffffff), uint32_t(timeSeed>>32)};
        _randGenerator.seed(seedSequence);

        return true;
    }


    void handleConstantString(const Expression::Numeric& numeric, Compiler::ConstStrType constStrType, std::string& name, int& index)
    {
        switch(constStrType)
        {
            case Compiler::StrLeft:
            case Compiler::StrRight:
            {
                uint8_t length = uint8_t(std::lround(numeric._params[0]._value));
                Compiler::getOrCreateConstString(constStrType, numeric._text, length, 0, index);
            }
            break;

            case Compiler::StrMid:
            {
                uint8_t offset = uint8_t(std::lround(numeric._params[0]._value));
                uint8_t length = uint8_t(std::lround(numeric._params[1]._value));
                Compiler::getOrCreateConstString(constStrType, numeric._text, length, offset, index);
            }
            break;

            case Compiler::StrLower:
            case Compiler::StrUpper:
            {
                Compiler::getOrCreateConstString(constStrType, numeric._text, 0, 0, index);
            }
            break;

            default: break;
        }

        name = Compiler::getStringVars()[index]._name;
        uint16_t srcAddr = Compiler::getStringVars()[index]._address;

        if(Expression::getEnableOptimisedPrint()  &&  Expression::getOutputNumeric()._nestedCount == 0)
        {
            Compiler::emitVcpuAsm("LDWI", Expression::wordToHexString(srcAddr), false);
            Compiler::emitVcpuAsm("%PrintAcString", "", false);
        }
        else
        {
            uint16_t dstAddr = Compiler::getStringVars()[Expression::getOutputNumeric()._index]._address;
            Compiler::emitVcpuAsm("LDWI", Expression::wordToHexString(srcAddr), false);
            Compiler::emitVcpuAsm("STW", "strSrcAddr", false);
            Compiler::emitVcpuAsm("LDWI", Expression::wordToHexString(dstAddr), false);
            Compiler::emitVcpuAsm("%StringCopy", "", false);
        }
    }

    void handleStringParameter(Expression::Numeric& param)
    {
        // Literals
        if(param._varType == Expression::Number)
        {
            Compiler::emitVcpuAsmForLiteral(param, false);
            return;
        }

        Operators::handleSingleOp("LDW", param);
    }

    // Does a function contain nested functions as parameters
    bool isFuncNested(void)
    {
        if(Expression::getOutputNumeric()._nestedCount == _nestedCount) return false;

        bool codeInit = (_nestedCount == -1);
        _nestedCount = Expression::getOutputNumeric()._nestedCount;
        if(codeInit) return false;

        return true;
    }


    // ********************************************************************************************
    // Functions
    // ********************************************************************************************
    void opcodeARR(Expression::Numeric& param)
    {
        // Can't call Operators::handleSingleOp() here, so special case it
        switch(param._varType)
        {
            // Temporary variable address
            case Expression::TmpVar:
            {
                Compiler::emitVcpuAsm("LDW", Expression::byteToHexString(uint8_t(std::lround(param._value))), false);
            }
            break;

            // User variable
            case Expression::IntVar16:
            {
                Compiler::emitVcpuAsmUserVar("LDW", param, false);
            }
            break;

            // Literal or constant
            case Expression::Number:
            {
                Compiler::emitVcpuAsm("LDI", std::to_string(uint8_t(std::lround(param._value))), false);
            }
            break;

            default: break;
        }
    }
    Expression::Numeric IARR(Expression::Numeric& numeric, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        if(Expression::getOutputNumeric()._staticInit)
        {
            Cpu::reportError(Cpu::FncError, stderr, "Functions::IARR() : '%s:%d' : %s cannot be used in static initialisation : %s\n", moduleName.c_str(), codeLineStart, numeric._name.c_str(), codeLineText.c_str());
            numeric._isValid = false;
            return numeric;
        }

        Compiler::getNextTempVar();

        int intSize = Compiler::getIntegerVars()[numeric._index]._intSize;
        uint16_t arrayPtr = Compiler::getIntegerVars()[numeric._index]._address;

        // Literal array index, (only optimise for 1d arrays)
        if(numeric._varType == Expression::Arr1Var8  &&  numeric._params.size() == 1  &&  numeric._params[0]._varType == Expression::Number)
        {
            std::string operand = Expression::wordToHexString(arrayPtr + uint16_t(numeric._params[0]._value*intSize));
            Compiler::emitVcpuAsm("LDWI", operand, false); 
            Compiler::emitVcpuAsm("PEEK", "",      false);
        }
        else if(numeric._varType == Expression::Arr1Var16  &&  numeric._params.size() == 1  &&  numeric._params[0]._varType == Expression::Number)
        {
            std::string operand = Expression::wordToHexString(arrayPtr + uint16_t(numeric._params[0]._value*intSize));

            // Handle .LO and .HI
            switch(numeric._int16Byte)
            {
                case Expression::Int16Low:  Compiler::emitVcpuAsm("LDWI", operand,          false); Compiler::emitVcpuAsm("PEEK", "", false); break;
                case Expression::Int16High: Compiler::emitVcpuAsm("LDWI", operand + " + 1", false); Compiler::emitVcpuAsm("PEEK", "", false); break;
                case Expression::Int16Both: Compiler::emitVcpuAsm("LDWI", operand,          false); Compiler::emitVcpuAsm("DEEK", "", false); break;

                default: break;
            }
        }
        // Variable array index or 2d/3d array
        else
        {
            size_t numDims = 0;
            if(numeric._varType >= Expression::Arr1Var8  &&  numeric._varType <= Expression::Arr3Var8)
            {
                numDims = numeric._varType - Expression::Arr1Var8 + 1;
            }
            else if(numeric._varType >= Expression::Arr1Var16  &&  numeric._varType <= Expression::Arr3Var16)
            {
                numDims = numeric._varType - Expression::Arr1Var16 + 1;
            }
            if(numDims != numeric._params.size())
            {
                Cpu::reportError(Cpu::FncError, stderr, "Functions::IARR() : '%s:%d' : %s() expects %d dimension/s, found %d : %s\n", moduleName.c_str(), codeLineStart, numeric._name.c_str(), int(numDims),
                                                                                                                                      int(numeric._params.size()), codeLineText.c_str());
                numeric._isValid = false;
                return numeric;
            }

            // Generate array indices
            for(size_t i=0; i<numeric._params.size(); i++)
            {
                Expression::Numeric param = numeric._params[i];
                opcodeARR(param);
                Compiler::emitVcpuAsm("STW", "memIndex" + std::to_string(i), false);
            }

            // Handle 1d/2d/3d arrays
            switch(numeric._varType)
            {
                case Expression::Arr1Var8:
                {
                    Compiler::emitVcpuAsm("LDWI", Expression::wordToHexString(arrayPtr), false);
                    Compiler::emitVcpuAsm("ADDW", "memIndex0", false);
                }
                break;

                case Expression::Arr2Var8:
                {
                    Compiler::emitVcpuAsm("LDWI", Expression::wordToHexString(arrayPtr), false);
                    (Compiler::getCodeRomType() >= Cpu::ROMv5a) ? Compiler::emitVcpuAsm("CALLI", "convert8Arr2d", false) : Compiler::emitVcpuAsm("CALL", "convert8Arr2dAddr", false);
                }
                break;

                case Expression::Arr3Var8:
                {
                    Compiler::emitVcpuAsm("LDWI", Expression::wordToHexString(arrayPtr), false);
                    (Compiler::getCodeRomType() >= Cpu::ROMv5a) ? Compiler::emitVcpuAsm("CALLI", "convert8Arr3d", false) : Compiler::emitVcpuAsm("CALL", "convert8Arr3dAddr", false);
                }
                break;

                case Expression::Arr1Var16:
                {
                    Compiler::emitVcpuAsm("LDWI", Expression::wordToHexString(arrayPtr), false);
                    Compiler::emitVcpuAsm("ADDW", "memIndex0", false);
                    Compiler::emitVcpuAsm("ADDW", "memIndex0", false);
                }
                break;

                case Expression::Arr2Var16:
                {
                    Compiler::emitVcpuAsm("LDWI", Expression::wordToHexString(arrayPtr), false);
                    (Compiler::getCodeRomType() >= Cpu::ROMv5a) ? Compiler::emitVcpuAsm("CALLI", "convert16Arr2d", false) : Compiler::emitVcpuAsm("CALL", "convert16Arr2dAddr", false);
                }
                break;

                case Expression::Arr3Var16:
                {
                    Compiler::emitVcpuAsm("LDWI", Expression::wordToHexString(arrayPtr), false);
                    (Compiler::getCodeRomType() >= Cpu::ROMv5a) ? Compiler::emitVcpuAsm("CALLI", "convert16Arr3d", false) : Compiler::emitVcpuAsm("CALL", "convert16Arr3dAddr", false);
                }
                break;

                default: break;
            }

            // Functions like LEN() and ADDR() require IARR() to return the address rather than the value
            if(!numeric._returnAddress)
            {
                // Bytes
                if(numeric._varType >= Expression::Arr1Var8  &&  numeric._varType <= Expression::Arr3Var8)
                {
                    Compiler::emitVcpuAsm("PEEK", "", false);
                }
                // Words, handle .LO and .HI
                else
                {
                    switch(numeric._int16Byte)
                    {
                        case Expression::Int16Low:  Compiler::emitVcpuAsm("PEEK", "",  false);                                           break;
                        case Expression::Int16High: Compiler::emitVcpuAsm("ADDI", "1", false); Compiler::emitVcpuAsm("PEEK", "", false); break;
                        case Expression::Int16Both: Compiler::emitVcpuAsm("DEEK", "",  false);                                           break;

                        default: break;
                    }
                }
            }
        }

        Operators::changeToTmpVar(numeric);
        Compiler::emitVcpuAsm("STW", Expression::byteToHexString(uint8_t(Compiler::getTempVarStart())), false);
        numeric._params.clear();
        return numeric;
    }

    Expression::Numeric SARR(Expression::Numeric& numeric, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        if(Expression::getOutputNumeric()._staticInit)
        {
            Cpu::reportError(Cpu::FncError, stderr, "Functions::SARR() : '%s:%d' : %s cannot be used in static initialisation : %s\n", moduleName.c_str(), codeLineStart, numeric._name.c_str(), codeLineText.c_str());
            numeric._isValid = false;
            return numeric;
        }
        if(numeric._params.size() != 1)
        {
            Cpu::reportError(Cpu::FncError, stderr, "Functions::SARR() : '%s:%d' : %s() expects 1 dimension, found %d : %s\n", moduleName.c_str(), codeLineStart, numeric._name.c_str(),
                                                                                                                               int(numeric._params.size()), codeLineText.c_str());
            numeric._isValid = false;
            return numeric;
        }

        // String addresses cannot be combined using boolean expressions, so no need to increment temporary var
        //Compiler::getNextTempVar();

        uint16_t arrayPtr = Compiler::getStringVars()[numeric._index]._address;

        // Literal array index
        if(numeric._params[0]._varType == Expression::Number)
        {
            std::string operand = Expression::wordToHexString(arrayPtr + uint16_t(numeric._params[0]._value)*2);
            Compiler::emitVcpuAsm("LDWI", operand, false);
            Compiler::emitVcpuAsm("DEEK", "",      false);
        }
        // Variable array index
        else
        {
            Expression::Numeric param = numeric._params[0];
            opcodeARR(param);
            Compiler::emitVcpuAsm("STW", "memIndex0", false);
            Compiler::emitVcpuAsm("LDWI", Expression::wordToHexString(arrayPtr), false);
            Compiler::emitVcpuAsm("ADDW", "memIndex0", false);
            Compiler::emitVcpuAsm("ADDW", "memIndex0", false);
            Compiler::emitVcpuAsm("DEEK", "", false);
        }

        Operators::changeToTmpVar(numeric);
        Compiler::emitVcpuAsm("STW", Expression::byteToHexString(uint8_t(Compiler::getTempVarStart())), false);

        numeric._varType = Expression::Str2Var;
        numeric._params.clear();
        return numeric;
    }

    Expression::Numeric PEEK(Expression::Numeric& numeric, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        if(Expression::getOutputNumeric()._staticInit)
        {
            Cpu::reportError(Cpu::FncError, stderr, "Functions::PEEK() : '%s:%d' : PEEK() cannot be used in static initialisation : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
            numeric._isValid = false;
            return numeric;
        }

        if(numeric._varType == Expression::Number)
        {
            if(numeric._lazyUpdate)
            {
                Compiler::emitVcpuAsm("LDWI", "_" + numeric._name, false);
            }
            // Optimise for page 0
            else if(numeric._value >= 0  &&  numeric._value <= 255)
            {
                Compiler::emitVcpuAsm("LD",  Expression::byteToHexString(uint8_t(std::lround(numeric._value))), false);
                Compiler::emitVcpuAsm("STW", Expression::byteToHexString(uint8_t(Compiler::getTempVarStart())), false);
                Operators::changeToTmpVar(numeric);
                return numeric;
            }
            else
            {
                Compiler::emitVcpuAsm("LDWI", Expression::wordToHexString(int16_t(std::lround(numeric._value))), false);
            }
        }

        Compiler::getNextTempVar();
        Operators::handleSingleOp("LDW", numeric);
        Compiler::emitVcpuAsm("PEEK", "", false);
        Compiler::emitVcpuAsm("STW", Expression::byteToHexString(uint8_t(Compiler::getTempVarStart())), false);

        return numeric;
    }

    Expression::Numeric DEEK(Expression::Numeric& numeric, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        if(Expression::getOutputNumeric()._staticInit)
        {
            Cpu::reportError(Cpu::FncError, stderr, "Functions::PEEK() : '%s:%d' : PEEK() cannot be used in static initialisation : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
            numeric._isValid = false;
            return numeric;
        }

        if(numeric._varType == Expression::Number)
        {
            if(numeric._lazyUpdate)
            {
                Compiler::emitVcpuAsm("LDWI", "_" + numeric._name, false);
            }
            // Optimise for page 0
            else if(numeric._value >= 0  &&  numeric._value <= 255)
            {
                Compiler::emitVcpuAsm("LDW", Expression::byteToHexString(uint8_t(std::lround(numeric._value))), false);
                Compiler::emitVcpuAsm("STW", Expression::byteToHexString(uint8_t(Compiler::getTempVarStart())), false);
                Operators::changeToTmpVar(numeric);
                return numeric;
            }
            else
            {
                Compiler::emitVcpuAsm("LDWI", Expression::wordToHexString(int16_t(std::lround(numeric._value))), false);
            }
        }

        Compiler::getNextTempVar();
        Operators::handleSingleOp("LDW", numeric);
        Compiler::emitVcpuAsm("DEEK", "", false);
        Compiler::emitVcpuAsm("STW", Expression::byteToHexString(uint8_t(Compiler::getTempVarStart())), false);

        return numeric;
    }

    Expression::Numeric USR(Expression::Numeric& numeric, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        if(Expression::getOutputNumeric()._staticInit)
        {
            Cpu::reportError(Cpu::FncError, stderr, "Functions::USR() : '%s:%d' : USR() cannot be used in static initialisation : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
            numeric._isValid = false;
            return numeric;
        }

        if(numeric._varType == Expression::Number)
        {
            Compiler::emitVcpuAsmForLiteral(numeric, true);
        }

        Compiler::getNextTempVar();

        if(Compiler::getCodeRomType() >= Cpu::ROMv5a)
        {
            Operators::handleSingleOp("CALLI", numeric);
        }
        else
        {
            Operators::handleSingleOp("LDW", numeric);
            Compiler::emitVcpuAsm("CALL", "giga_vAC", false);
        }
        Compiler::emitVcpuAsm("STW", Expression::byteToHexString(uint8_t(Compiler::getTempVarStart())), false);

        return numeric;
    }

    Expression::Numeric RND(Expression::Numeric& numeric, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        UNREFERENCED_PARAM(moduleName);
        UNREFERENCED_PARAM(codeLineText);
        UNREFERENCED_PARAM(codeLineStart);

        bool useMod = true;
        bool useFastMod= false;
        if(numeric._varType == Expression::Number)
        {
            // No code needed for static initialisation
            if(Expression::getOutputNumeric()._staticInit)
            {
                if(numeric._value == 0)
                {
                    std::uniform_int_distribution<uint16_t> distribution(0, 0xFFFF);
                    return Expression::Numeric(distribution(_randGenerator), -1, true, false, false, Expression::Number, Expression::BooleanCC, Expression::Int16Both, std::string(""), std::string(""));
                }

                std::uniform_int_distribution<uint16_t> distribution(0, uint16_t(numeric._value));
                return Expression::Numeric(distribution(_randGenerator), -1, true, false, false, Expression::Number, Expression::BooleanCC, Expression::Int16Both, std::string(""), std::string(""));
            }

            // RND(0) skips the MOD call and allows you to filter the output manually
            if(numeric._value == 0)
            {
                useMod = false;
            }
            else
            {
                int16_t value = Compiler::emitVcpuAsmForLiteral(numeric, true);
                if(value >= 0  &&  value <= 255) useFastMod = true;
            }
        }

        Compiler::getNextTempVar();
        if(useMod)
        {
            Operators::handleSingleOp("LDW", numeric);
            if(Compiler::getCodeRomType() >= Cpu::ROMvX0  &&  Compiler::getCodeRomType() < Cpu::SDCARD  &&  useFastMod)
            {
                Compiler::emitVcpuAsm("%RandFastMod", "", false);
            }
            else
            {
                Compiler::emitVcpuAsm("%RandMod", "", false);
            }
        }
        else
        {
            Operators::changeToTmpVar(numeric);
            Compiler::emitVcpuAsm("%Rand", "", false);
        }
        Compiler::emitVcpuAsm("STW", Expression::byteToHexString(uint8_t(Compiler::getTempVarStart())), false);

        return numeric;
    }

    Expression::Numeric URND(Expression::Numeric& numeric, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        UNREFERENCED_PARAM(codeLineStart);

        if(!Expression::getOutputNumeric()._staticInit)
        {
            Cpu::reportError(Cpu::FncError, stderr, "Functions::URND() : '%s:%d' : URND only works in static initialisation : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
            numeric._isValid = false;
            return numeric;
        }
        if(numeric._params.size() != 3)
        {
            Cpu::reportError(Cpu::FncError, stderr, "Functions::URND() : '%s:%d' : URND expects 4 parameters, found %d : %s\n", moduleName.c_str(), codeLineStart, int(numeric._params.size()), codeLineText.c_str());
            numeric._isValid = false;
            return numeric;
        }
        if(numeric._varType != Expression::Number  ||  numeric._params[0]._varType != Expression::Number  ||  numeric._params[1]._varType != Expression::Number  ||  numeric._params[2]._varType != Expression::Number)
        {
            Cpu::reportError(Cpu::FncError, stderr, "Functions::URND() : '%s:%d' : URND expects 4 literal parameters, 'URND(<min>, <max>, <len>, <step>) : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
            numeric._isValid = false;
            return numeric;
        }

        // Initialise unique random number generator
        if(numeric._value != _umin  ||  numeric._params[0]._value != _umax  ||  numeric._params[1]._value != _ulen  ||  numeric._params[2]._value != _ustp)
        {
            _umin = _umax = _ulen = 0.0;
            _ustp = 1.0;

            if(abs(numeric._params[0]._value - numeric._value) < numeric._params[1]._value)
            {
                Cpu::reportError(Cpu::FncError, stderr, "Functions::URND() : '%s:%d' : range is smaller than length : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
                numeric._isValid = false;
                return numeric;
            }
            if(numeric._params[0]._value <= numeric._value)
            {
                Cpu::reportError(Cpu::FncError, stderr, "Functions::URND() : '%s:%d' : maximum must be greater than minimum : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
                numeric._isValid = false;
                return numeric;
            }
            if(numeric._params[1]._value <= 0.0  ||  std::lround(numeric._params[1]._value) > 0xFFFF)
            {
                Cpu::reportError(Cpu::FncError, stderr, "Functions::URND() : '%s:%d' : 0x0000 < length < 0x10000 : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
                numeric._isValid = false;
                return numeric;
            }
            if(numeric._params[2]._value == 0.0)
            {
                Cpu::reportError(Cpu::FncError, stderr, "Functions::URND() : '%s:%d' : step must not be equal to zero : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
                numeric._isValid = false;
                return numeric;
            }

            _umin = numeric._value;
            _umax = numeric._params[0]._value;
            _ulen = numeric._params[1]._value;
            _ustp = numeric._params[2]._value;
            _uidx = 0;

            uint16_t range = uint16_t((abs(std::lround(_umax) - std::lround(_umin))) / std::lround(abs(_ustp))) + 1;
            if(range == 0)
            {
                Cpu::reportError(Cpu::FncError, stderr, "Functions::URND() : '%s:%d' : step size is too large for range : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
                numeric._isValid = false;
                return numeric;
            }

            _uvalues.resize(range);

            for(int i=0; i<range; i++) _uvalues[i] = int16_t(std::lround(_umin)) + int16_t(i*abs(_ustp));
            //std::iota(_uvalues.begin(), _uvalues.end(), int16_t(std::lround(_umin)));
            std::shuffle(_uvalues.begin(), _uvalues.end(), _randGenerator);
        }

        if(_uidx >= uint16_t(_uvalues.size()))
        {
            Cpu::reportError(Cpu::FncError, stderr, "Functions::URND() : '%s:%d' : length is greater than range : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
            numeric._isValid = false;
            return numeric;
        }

        numeric._value = _uvalues[_uidx++];

        return numeric;
    }

    Expression::Numeric LEN(Expression::Numeric& numeric, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        if(numeric._varType == Expression::Number)
        {
            Cpu::reportError(Cpu::FncError, stderr, "Functions::LEN() : '%s:%d' : parameter can't be a literal : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
            numeric._isValid = false;
            return numeric;
        }
        if(numeric._params.size() != 0)
        {
            Cpu::reportError(Cpu::FncError, stderr, "Functions::LEN() : '%s:%d' : LEN expects 1 parameter, found %d : %s\n", moduleName.c_str(), codeLineStart, int(numeric._params.size()), codeLineText.c_str());
            numeric._isValid = false;
            return numeric;
        }

        // Handle non variables
        if(numeric._index == -1)
        {
            switch(numeric._varType)
            {
                // Get or create constant string
                case Expression::String:
                {
                    int index;
                    Compiler::getOrCreateConstString(numeric._text, index);
                    numeric._index = int16_t(index);
                    numeric._varType = Expression::StrVar;
                }
                break;

                // Needs to pass through
                case Expression::TmpStrVar:
                {
                }
                break;

                default:
                {
                    Cpu::reportError(Cpu::FncError, stderr, "Functions::LEN() : '%s:%d' : couldn't find variable name '%s' : %s\n", moduleName.c_str(), codeLineStart, numeric._name.c_str(), codeLineText.c_str());
                    numeric._isValid = false;
                    return numeric;
                }
            }
        }

        int length = 0;
        switch(numeric._varType)
        {
            case Expression::IntVar16:
            case Expression::Arr1Var8:
            case Expression::Arr2Var8:
            case Expression::Arr3Var8:
            case Expression::Arr1Var16:
            case Expression::Arr2Var16:
            case Expression::TmpVar:
            case Expression::Arr3Var16: length = Compiler::getIntegerVars()[numeric._index]._intSize; break;
            case Expression::Constant:  length = Compiler::getConstants()[numeric._index]._size;      break;
            case Expression::StrVar:    length = Compiler::getStringVars()[numeric._index]._size;     break;

            default: break;
        }

        // No code needed for static initialisation
        if(Expression::getOutputNumeric()._staticInit)
        {
            return Expression::Numeric(length, -1, true, false, false, Expression::Number, Expression::BooleanCC, Expression::Int16Both, std::string(""), std::string(""));
        }

        // String vars
        if(numeric._varType == Expression::StrVar  &&  !Compiler::getStringVars()[numeric._index]._constant)
        {
            Compiler::emitVcpuAsm("LDWI", Expression::wordToHexString(Compiler::getStringVars()[numeric._index]._address), false);
            Compiler::emitVcpuAsm("PEEK", "", false);
        }
        // String arrays
        else if(numeric._varType == Expression::Str2Var)
        {
            // Optimiser does a better job
            //if(Compiler::getCodeRomType() >= Cpu::ROMvX0  &&  Compiler::getCodeRomType() < Cpu::SDCARD)
            //{
            //    Compiler::emitVcpuAsm("PEEKV", Expression::byteToHexString(uint8_t(Compiler::getTempVarStart())), false);
            //}
            //else
            {
                Compiler::emitVcpuAsm("LDW", Expression::byteToHexString(uint8_t(Compiler::getTempVarStart())), false);
                Compiler::emitVcpuAsm("PEEK", "", false);
            }
        }
        // Temp string vars
        else if(numeric._varType == Expression::TmpStrVar)
        {
            Compiler::emitVcpuAsm("LDWI", Expression::wordToHexString(Compiler::getStrWorkArea()), false);
            Compiler::emitVcpuAsm("PEEK", "", false);
        }
        // Ints, int arrays and constants
        else
        {
            // Generate code to save result into a tmp var
            Expression::Numeric numericTmp = Expression::Numeric(int16_t(length), -1, true, false, false, Expression::Number, Expression::BooleanCC, Expression::Int16Both, std::string(""), std::string(""));
            Compiler::emitVcpuAsmForLiteral(numericTmp, false);
        }

        Compiler::getNextTempVar();
        Operators::changeToTmpVar(numeric);
        Compiler::emitVcpuAsm("STW", Expression::byteToHexString(uint8_t(Compiler::getTempVarStart())), false);

        return numeric;
    }

    Expression::Numeric GET(Expression::Numeric& numeric, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        if(numeric._varType == Expression::String)
        {
            std::string sysVarName = numeric._text;
            Expression::strToUpper(sysVarName);

            if(sysVarName != "SPRITE_LUTS")
            {
                if(Expression::getOutputNumeric()._staticInit)
                {
                    Cpu::reportError(Cpu::FncError, stderr, "Functions::GET() : '%s:%d' : GET() cannot be used in static initialisation : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
                    numeric._isValid = false;
                    return numeric;
                }
            }

            if(sysVarName == "ROM_READ_DIR"  &&  numeric._params.size() == 1)
            {
                // Literal constant
                if(numeric._params[0]._varType == Expression::Number)
                {
                    Compiler::emitVcpuAsm("LDI", Expression::byteToHexString(uint8_t(std::lround(numeric._params[0]._value))), false);
                }

                // handleSingleOp LDW is skipped if above was a constant literal
                Compiler::getNextTempVar();
                Operators::handleSingleOp("LDW", numeric._params[0]);
                Compiler::emitVcpuAsm("%RomRead", "", false);
                Compiler::emitVcpuAsm("STW", Expression::byteToHexString(uint8_t(Compiler::getTempVarStart())), false);
                numeric._params[0]._params.clear();
                return numeric._params[0];
            }
            else if(sysVarName == "BLIT_LUT"  &&  numeric._params.size() == 1)
            {
                if(Compiler::getCodeRomType() < Cpu::ROMv3)
                {
                    std::string romTypeStr;
                    getRomTypeStr(Compiler::getCodeRomType(), romTypeStr);
                    Cpu::reportError(Cpu::FncError, stderr, "Functions::GET() : '%s:%d' : version error, 'GET(\"BLIT_LUT\")' requires ROMv3 or higher, you are trying to link against '%s' : %s\n",
                                                            moduleName.c_str(), codeLineStart, romTypeStr.c_str(), codeLineText.c_str());
                    numeric._isValid = false;
                    return numeric;
                }

                // Literal constant
                if(numeric._params[0]._varType == Expression::Number)
                {
                    Compiler::emitVcpuAsm("LDI", Expression::byteToHexString(uint8_t(std::lround(numeric._params[0]._value))), false);
                }

                // Look up blit lut from blits lut using a blit index, (handleSingleOp LDW is skipped if above was a constant literal)
                Compiler::getNextTempVar();
                Operators::handleSingleOp("LDW", numeric._params[0]);
                Compiler::emitVcpuAsm("STW", "blitId", false);
                Compiler::emitVcpuAsm("%GetBlitLUT", "", false);
                Compiler::emitVcpuAsm("STW", Expression::byteToHexString(uint8_t(Compiler::getTempVarStart())), false);
                numeric._params[0]._params.clear();
                return numeric._params[0];
            }
            else if(sysVarName == "SPRITE_LUTS"  &&  numeric._params.size() == 1)
            {
                if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD)
                {
                    std::string romTypeStr;
                    getRomTypeStr(Compiler::getCodeRomType(), romTypeStr);
                    Cpu::reportError(Cpu::FncError, stderr, "Functions::GET() : '%s:%d' : version error, 'GET(\"SPRITE_LUTS\")' requires ROMvX0, you are trying to link against '%s' : %s\n",
                                                            moduleName.c_str(), codeLineStart, romTypeStr.c_str(), codeLineText.c_str());
                    numeric._isValid = false;
                    return numeric;
                }

                // Literal constant
                if(numeric._params[0]._varType != Expression::Number)
                {
                    Cpu::reportError(Cpu::FncError, stderr, "Functions::GET() : '%s:%d' : syntax error, 'GET(\"SPRITE_LUTS\")' requires a literal parameter : %s\n", moduleName.c_str(), codeLineStart,
                                                                                                                                                                     codeLineText.c_str());
                    numeric._isValid = false;
                    return numeric;
                }

                if(Expression::getOutputNumeric()._staticInit)
                {
                    switch(uint8_t(std::lround(numeric._params[0]._value)))
                    {
                        case 0:
                        {
                            numeric = Expression::Numeric(0x0000, -1, true, false, false, Expression::Number, Expression::BooleanCC, Expression::Int16Both, std::string(""), std::string(""));
                            numeric._lazyUpdate = (int16_t *)&Compiler::getSpritesAddrLut()._address;
                            return numeric;
                        }
                        break;

                        case 1:
                        {
                            numeric = Expression::Numeric(0x0000, -1, true, false, false, Expression::Number, Expression::BooleanCC, Expression::Int16Both, std::string(""), std::string(""));
                            numeric._lazyUpdate = (int16_t *)&Compiler::getSpritesAddrLut()._tmpAddr;
                            return numeric;
                        }
                        break;

                        case 2:
                        {
                            numeric = Expression::Numeric(0x0000, -1, true, false, false, Expression::Number, Expression::BooleanCC, Expression::Int16Both, std::string(""), std::string(""));
                            numeric._lazyUpdate = (int16_t *)&Compiler::getSpritesAddrLut()._indicesAddr;
                            return numeric;
                        }
                        break;

                        case 3:
                        {
                            numeric = Expression::Numeric(0x0000, -1, true, false, false, Expression::Number, Expression::BooleanCC, Expression::Int16Both, std::string(""), std::string(""));
                            numeric._lazyUpdate = (int16_t *)&Compiler::getSpritesAddrLut()._xPosAddr; 
                            return numeric;
                        }
                        break;

                        case 4:
                        {
                            numeric = Expression::Numeric(0x0000, -1, true, false, false, Expression::Number, Expression::BooleanCC, Expression::Int16Both, std::string(""), std::string(""));
                            numeric._lazyUpdate = (int16_t *)&Compiler::getSpritesAddrLut()._xPosTmpAddr;
                            return numeric;
                        }
                        break;

                        case 5:
                        {
                            numeric = Expression::Numeric(0x0000, -1, true, false, false, Expression::Number, Expression::BooleanCC, Expression::Int16Both, std::string(""), std::string(""));
                            numeric._lazyUpdate = (int16_t *)&Compiler::getSpritesAddrLut()._yPosAddr;
                            return numeric;
                        }
                        break;

                        case 6:
                        {
                            numeric = Expression::Numeric(0x0000, -1, true, false, false, Expression::Number, Expression::BooleanCC, Expression::Int16Both, std::string(""), std::string(""));
                            numeric._lazyUpdate = (int16_t *)&Compiler::getSpritesAddrLut()._yPosTmpAddr;
                            return numeric;
                        }
                        break;

                        default:
                        {
                            Cpu::reportError(Cpu::FncError, stderr, "Functions::GET() : '%s:%d' : syntax error, 'GET(\"SPRITE_LUTS\")' literal must be >= 0 and <= 3 : %s\n", moduleName.c_str(), codeLineStart,
                                                                                                                                                                              codeLineText.c_str());
                            numeric._isValid = false;
                            return numeric;
                        }
                        break;
                    }
                }

                switch(uint8_t(std::lround(numeric._params[0]._value)))
                {
                    case 0: Compiler::emitVcpuAsm("LDWI", "_spritesLut_",           false); break;
                    case 1: Compiler::emitVcpuAsm("LDWI", "_spritesTmpLut_",        false); break;
                    case 2: Compiler::emitVcpuAsm("LDWI", "_spritesIndicesLut_",    false); break;
                    case 3: Compiler::emitVcpuAsm("LDWI", "_spritesXposLut_",       false); break;
                    case 4: Compiler::emitVcpuAsm("LDWI", "_spritesXposTmpLut_",    false); break;
                    case 5: Compiler::emitVcpuAsm("LDWI", "_spritesYposLut_",       false); break;
                    case 6: Compiler::emitVcpuAsm("LDWI", "_spritesYposTmpLut_",    false); break;

                    default:
                    {
                        Cpu::reportError(Cpu::FncError, stderr, "Functions::GET() : '%s:%d' : syntax error, 'GET(\"SPRITE_LUTS\")' literal must be >= 0 and <= 3 : %s\n", moduleName.c_str(), codeLineStart,
                                                                                                                                                                          codeLineText.c_str());
                        numeric._isValid = false;
                        return numeric;
                    }
                    break;
                }

                Compiler::getNextTempVar();
                Operators::changeToTmpVar(numeric._params[0]);
                Compiler::emitVcpuAsm("STW", Expression::byteToHexString(uint8_t(Compiler::getTempVarStart())), false);
                numeric._params[0]._params.clear();
                return numeric._params[0];
            }
            else if(sysVarName == "MIDI_NOTE"  &&  numeric._params.size() == 1)
            {
                // Literal constant
                if(numeric._params[0]._varType == Expression::Number)
                {
                    Compiler::emitVcpuAsm("LDI", Expression::byteToHexString(uint8_t(std::lround(numeric._params[0]._value))), false);
                }

                // Look up a ROM note using a midi index, (handleSingleOp LDW is skipped if above was a constant literal)
                Compiler::getNextTempVar();
                Operators::handleSingleOp("LDW", numeric._params[0]);
                if(Compiler::getCodeRomType() >= Cpu::ROMvX0  &&  Compiler::getCodeRomType() < Cpu::SDCARD)
                {
                    Compiler::emitVcpuAsm("MIDI", "", false);
                }
                else
                {
                    Compiler::emitVcpuAsm("STW", "musicNote", false);
                    Compiler::emitVcpuAsm("%GetMidiNote", "", false);
                }
                Compiler::emitVcpuAsm("STW", Expression::byteToHexString(uint8_t(Compiler::getTempVarStart())), false);
                numeric._params[0]._params.clear();
                return numeric._params[0];
            }
            else if(sysVarName == "MUSIC_NOTE"  &&  numeric._params.size() == 1)
            {
                // Literal constant
                if(numeric._params[0]._varType == Expression::Number)
                {
                    Compiler::emitVcpuAsm("LDI", Expression::byteToHexString(uint8_t(std::lround(numeric._params[0]._value))), false);
                }

                // Look up a ROM note using a note index, (handleSingleOp LDW is skipped if above was a constant literal)
                Compiler::getNextTempVar();
                Operators::handleSingleOp("LDW", numeric._params[0]);
                if(Compiler::getCodeRomType() >= Cpu::ROMvX0  &&  Compiler::getCodeRomType() < Cpu::SDCARD)
                {
                    Compiler::emitVcpuAsm("NOTE", "", false);
                }
                else
                {
                    Compiler::emitVcpuAsm("STW", "musicNote", false);
                    Compiler::emitVcpuAsm("%GetMusicNote", "", false);
                }
                Compiler::emitVcpuAsm("STW", Expression::byteToHexString(uint8_t(Compiler::getTempVarStart())), false);
                numeric._params[0]._params.clear();
                return numeric._params[0];
            }
            else if(numeric._params.size() == 0)
            {
                Compiler::getNextTempVar();
                Operators::changeToTmpVar(numeric);

                if(sysVarName == "TIME_MODE")
                {
                    Compiler::emitVcpuAsm("LDWI", "handleT_mode + 1", false);
                    Compiler::emitVcpuAsm("PEEK", "", false);
                }
                else if(sysVarName == "TIME_EPOCH")
                {
                    Compiler::emitVcpuAsm("LDWI", "handleT_epoch + 1", false);
                    Compiler::emitVcpuAsm("PEEK", "", false);
                }
                else if(sysVarName == "TIME_S")
                {
                    Compiler::emitVcpuAsm("LDWI", "_timeArray_ + 0", false);
                    Compiler::emitVcpuAsm("PEEK", "", false);
                }
                else if(sysVarName == "TIME_M")
                {
                    Compiler::emitVcpuAsm("LDWI", "_timeArray_ + 1", false);
                    Compiler::emitVcpuAsm("PEEK", "", false);
                }
                else if(sysVarName == "TIME_H")
                {
                    Compiler::emitVcpuAsm("LDWI", "_timeArray_ + 2", false);
                    Compiler::emitVcpuAsm("PEEK", "", false);
                }
                else if(sysVarName == "TIMER")
                {
                    Compiler::emitVcpuAsm("LDW", "timerTick", false);
                }
                else if(sysVarName == "TIMER_PREV")
                {
                    Compiler::emitVcpuAsm("LDW", "timerPrev", false);
                }
                else if(sysVarName == "VBLANK")
                {
                    //std::string internalLabel = Expression::wordToHexString(Compiler::getVasmPC());
                    //Compiler::emitVcpuAsm("%GetVBlank", "_getVBlank_" + internalLabel, false);
                    Compiler::emitVcpuAsm("%GetVBlank", "", false);
                }
                else if(sysVarName == "VBLANK_PROC")
                {
                    if(Compiler::getCodeRomType() < Cpu::ROMv5a)
                    {
                        std::string romTypeStr;
                        getRomTypeStr(Compiler::getCodeRomType(), romTypeStr);
                        Cpu::reportError(Cpu::FncError, stderr, "Functions::GET() : '%s:%d' : version error, 'GET(\"VBLANK_PROC\")' requires ROMv5a or higher, you are trying to link against '%s' : %s\n",
                                                                moduleName.c_str(), codeLineStart, romTypeStr.c_str(), codeLineText.c_str());
                        numeric._isValid = false;
                        return numeric;
                    }
                    else
                    {
                        Compiler::emitVcpuAsm("LDWI", Expression::wordToHexString(VBLANK_PROC), false);
                        Compiler::emitVcpuAsm("DEEK", "", false);
                    }
                }
                else if(sysVarName == "VBLANK_FREQ")
                {
                    if(Compiler::getCodeRomType() < Cpu::ROMv5a)
                    {
                        std::string romTypeStr;
                        getRomTypeStr(Compiler::getCodeRomType(), romTypeStr);
                        Cpu::reportError(Cpu::FncError, stderr, "Functions::GET() : '%s:%d' : version error, 'GET(\"VBLANK_FREQ\")' requires ROMv5a or higher, you are trying to link against '%s' : %s\n",
                                                                moduleName.c_str(), codeLineStart, romTypeStr.c_str(), codeLineText.c_str());
                        numeric._isValid = false;
                        return numeric;
                    }
                    // (256 - n) = vblank interrupt frequency, where n = 1 to 255
                    else
                    {
                        Compiler::emitVcpuAsm("LDWI", "realTS_rti + 2", false);
                        Compiler::emitVcpuAsm("PEEK", "", false);
                    }
                }
                else if(sysVarName == "CURSOR_X")
                {
                    Compiler::emitVcpuAsm("LD", "cursorXY", false);
                }
                else if(sysVarName == "CURSOR_Y")
                {
                    Compiler::emitVcpuAsm("LD", "cursorXY + 1", false);
                }
                else if(sysVarName == "CURSOR_XY")
                {
                    Compiler::emitVcpuAsm("LDW", "cursorXY", false);
                }
                else if(sysVarName == "FG_COLOUR")
                {
                    Compiler::emitVcpuAsm("LD", "fgbgColour + 1", false);
                }
                else if(sysVarName == "BG_COLOUR")
                {
                    Compiler::emitVcpuAsm("LD", "fgbgColour", false);
                }
                else if(sysVarName == "FGBG_COLOUR")
                {
                    Compiler::emitVcpuAsm("LDW", "fgbgColour", false);
                }
                else if(sysVarName == "MIDI_STREAM")
                {
                    Compiler::emitVcpuAsm("LDW", "midiStream", false);
                }
                else if(sysVarName == "LED_TEMPO")
                {
                    Compiler::emitVcpuAsm("LD", "giga_ledTempo", false);
                }
                else if(sysVarName == "LED_STATE")
                {
                    Compiler::emitVcpuAsm("LD", "giga_ledState", false);
                }
                else if(sysVarName == "SOUND_TIMER")
                {
                    Compiler::emitVcpuAsm("LD", "giga_soundTimer", false);
                }
                else if(sysVarName == "CHANNEL_MASK")
                {
                    Compiler::emitVcpuAsm("LD", "giga_channelMask", false);
                    Compiler::emitVcpuAsm("ANDI", "0x03", false);
                }
                else if(sysVarName == "ROM_TYPE")
                {
                    Compiler::emitVcpuAsm("LD", "giga_romType", false);
                    Compiler::emitVcpuAsm("ANDI", "0xFC", false);
                }
                else if(sysVarName == "VSP")
                {
                    Compiler::emitVcpuAsm("LD", "giga_vSP", false);
                }
                else if(sysVarName == "VLR")
                {
                    Compiler::emitVcpuAsm("LDW", "giga_vLR", false);
                }
                else if(sysVarName == "VAC")
                {
                    Compiler::emitVcpuAsm("LDW", "giga_vAC", false);
                }
                else if(sysVarName == "VPC")
                {
                    Compiler::emitVcpuAsm("LDW", "giga_vPC", false);
                }
                else if(sysVarName == "XOUT_MASK")
                {
                    Compiler::emitVcpuAsm("LD", "giga_xoutMask", false);
                }
                else if(sysVarName == "BUTTON_STATE")
                {
                    Compiler::emitVcpuAsm("LD", "giga_buttonState", false);
                }
                else if(sysVarName == "SERIAL_RAW")
                {
                    Compiler::emitVcpuAsm("LD", "giga_serialRaw", false);
                }
                else if(sysVarName == "PS2_KEYBD") // alias for SERIAL_RAW
                {
                    Compiler::emitVcpuAsm("LD", "giga_serialRaw", false);
                }
                else if(sysVarName == "FRAME_COUNT")
                {
                    Compiler::emitVcpuAsm("LD", "giga_frameCount", false);
                }
                else if(sysVarName == "VIDEO_Y")
                {
                    Compiler::emitVcpuAsm("LD", "giga_videoY", false);
                }
                else if(sysVarName == "RAND2")
                {
                    Compiler::emitVcpuAsm("LD", "giga_rand2", false);
                }
                else if(sysVarName == "RAND1")
                {
                    Compiler::emitVcpuAsm("LD", "giga_rand1", false);
                }
                else if(sysVarName == "RAND0")
                {
                    Compiler::emitVcpuAsm("LD", "giga_rand0", false);
                }
                else if(sysVarName == "MEM_SIZE")
                {
                    Compiler::emitVcpuAsm("LD", "giga_memSize", false);
                }
                else if(sysVarName == "Y_RES")
                {
                    Compiler::emitVcpuAsm("LDI", "giga_yres", false);
                }
                else if(sysVarName == "X_RES")
                {
                    Compiler::emitVcpuAsm("LDI", "giga_xres", false);
                }
                else if(sysVarName == "SND_CHN4")
                {
                    Compiler::emitVcpuAsm("LDWI", "giga_soundChan4", false);
                }
                else if(sysVarName == "SND_CHN3")
                {
                    Compiler::emitVcpuAsm("LDWI", "giga_soundChan3", false);
                }
                else if(sysVarName == "SND_CHN2")
                {
                    Compiler::emitVcpuAsm("LDWI", "giga_soundChan2", false);
                }
                else if(sysVarName == "SND_CHN1")
                {
                    Compiler::emitVcpuAsm("LDWI", "giga_soundChan1", false);
                }
                else if(sysVarName == "V_TOP")
                {
                    Compiler::emitVcpuAsm("LDWI", "giga_videoTop", false);
                }
                else if(sysVarName == "V_TABLE")
                {
                    Compiler::emitVcpuAsm("LDWI", "giga_videoTable", false);
                }
                else if(sysVarName == "V_RAM")
                {
                    Compiler::emitVcpuAsm("LDWI", "giga_vram", false);
                }
                else if(sysVarName == "ROM_NOTES")
                {
                    Compiler::emitVcpuAsm("LDWI", "giga_notesTable", false);
                }
                else if(sysVarName == "ROM_TEXT82")
                {
                    Compiler::emitVcpuAsm("LDWI", "giga_text82", false);
                }
                else if(sysVarName == "ROM_TEXT32")
                {
                    Compiler::emitVcpuAsm("LDWI", "giga_text32", false);
                }
                else
                {
                    Cpu::reportError(Cpu::FncError, stderr, "Functions::GET() : '%s:%d' : system variable name '%s' does not exist : %s\n", moduleName.c_str(), codeLineStart, numeric._text.c_str(), codeLineText.c_str());
                    numeric._isValid = false;
                    return numeric;
                }

                Compiler::emitVcpuAsm("STW", Expression::byteToHexString(uint8_t(Compiler::getTempVarStart())), false);
            }
            else
            {
                Cpu::reportError(Cpu::FncError, stderr, "Functions::GET() : '%s:%d' : system variable name '%s' does not exist : %s\n", moduleName.c_str(), codeLineStart, numeric._text.c_str(), codeLineText.c_str());
                numeric._isValid = false;
                return numeric;
            }
        }

        return numeric;
    }

    Expression::Numeric ABS(Expression::Numeric& numeric, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        if(Expression::getOutputNumeric()._staticInit)
        {
            Cpu::reportError(Cpu::FncError, stderr, "Functions::ABS() : '%s:%d' : ABS() cannot be used in static initialisation : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
            numeric._isValid = false;
            return numeric;
        }

        if(numeric._varType != Expression::String  &&  numeric._varType != Expression::StrVar &&  numeric._varType != Expression::TmpStrVar)
        {
            Compiler::getNextTempVar();

            if(numeric._varType == Expression::Number)
            {
                numeric._value = abs(numeric._value);
                Compiler::emitVcpuAsmForLiteral(numeric, false);
                Operators::changeToTmpVar(numeric);
                Compiler::emitVcpuAsm("STW", Expression::byteToHexString(uint8_t(Compiler::getTempVarStart())), false);
            }
            else
            {
                Operators::handleSingleOp("LDW", numeric);

                if(Compiler::getCodeRomType() >= Cpu::ROMvX0  &&  Compiler::getCodeRomType() < Cpu::SDCARD)
                {
                    std::string internalLabel = Expression::wordToHexString(Compiler::getVasmPC());
                    Compiler::emitVcpuAsm("%Absolute", "_abs_" + internalLabel, false);
                }
                else
                {
                    Compiler::emitVcpuAsm("%Absolute", "", false);
                }
                Compiler::emitVcpuAsm("STW", Expression::byteToHexString(uint8_t(Compiler::getTempVarStart())), false);
            }
        }

        return numeric;
    }

    Expression::Numeric SGN(Expression::Numeric& numeric, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        if(Expression::getOutputNumeric()._staticInit)
        {
            Cpu::reportError(Cpu::FncError, stderr, "Functions::SGN() : '%s:%d' : SGN() cannot be used in static initialisation : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
            numeric._isValid = false;
            return numeric;
        }

        if(numeric._varType != Expression::String  &&  numeric._varType != Expression::StrVar &&  numeric._varType != Expression::TmpStrVar)
        {
            Compiler::getNextTempVar();

            if(numeric._varType == Expression::Number)
            {
                numeric._value = Expression::sgn(numeric._value);
                Compiler::emitVcpuAsmForLiteral(numeric, false);
                Operators::changeToTmpVar(numeric);
                Compiler::emitVcpuAsm("STW", Expression::byteToHexString(uint8_t(Compiler::getTempVarStart())), false);
            }
            else
            {
                Operators::handleSingleOp("LDW", numeric);
                Compiler::emitVcpuAsm("%Sign", "", false);
                Compiler::emitVcpuAsm("STW", Expression::byteToHexString(uint8_t(Compiler::getTempVarStart())), false);
            }
        }

        return numeric;
    }

    Expression::Numeric ASC(Expression::Numeric& numeric, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        if(numeric._varType == Expression::Number)
        {
            Cpu::reportError(Cpu::FncError, stderr, "Functions::ASC() : '%s:%d' : parameter can't be a literal : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
            numeric._isValid = false;
            return numeric;
        }

        Compiler::getNextTempVar();

        // Handle non variables
        if(numeric._index == -1)
        {
            switch(numeric._varType)
            {
                // Get or create constant string
                case Expression::String:
                {
                    int index;
                    Compiler::getOrCreateConstString(numeric._text, index);
                    numeric._index = int16_t(index);
                    numeric._varType = Expression::StrVar;
                }
                break;

                case Expression::TmpStrVar:
                {
                }
                break;

                default:
                {
                    Cpu::reportError(Cpu::FncError, stderr, "Functions::ASC() : '%s:%d' : couldn't find variable name '%s' : %s\n", moduleName.c_str(), codeLineStart, numeric._name.c_str(), codeLineText.c_str());
                    numeric._isValid = false;
                    return numeric;
                }
            }
        }

        uint8_t ascii = 0;
        switch(numeric._varType)
        {
            case Expression::StrVar:   ascii = Compiler::getStringVars()[numeric._index]._text[0]; break;
            case Expression::Constant: ascii = Compiler::getConstants()[numeric._index]._text[0];  break;

            default: break;
        }

        // No code needed for static initialisation
        if(Expression::getOutputNumeric()._staticInit)
        {
            return Expression::Numeric(ascii, -1, true, false, false, Expression::Number, Expression::BooleanCC, Expression::Int16Both, std::string(""), std::string(""));
        }

        // Variables
        if(numeric._varType == Expression::StrVar  &&  !Compiler::getStringVars()[numeric._index]._constant)
        {
            Compiler::emitVcpuAsm("LDWI", Expression::wordToHexString(Compiler::getStringVars()[numeric._index]._address) + " + 1", false);
            Compiler::emitVcpuAsm("PEEK", "", false);
        }
        else if(numeric._varType == Expression::TmpStrVar)
        {
            Compiler::emitVcpuAsm("LDWI", Expression::wordToHexString(Compiler::getStrWorkArea()) + " + 1", false);
            Compiler::emitVcpuAsm("PEEK", "", false);
        }
        // Constants
        else
        {
            // Generate code to save result into a tmp var
            Compiler::emitVcpuAsm("LDI", std::to_string(ascii), false);
        }

        Operators::changeToTmpVar(numeric);
        Compiler::emitVcpuAsm("STW", Expression::byteToHexString(uint8_t(Compiler::getTempVarStart())), false);

        return numeric;
    }

    Expression::Numeric STRCMP(Expression::Numeric& numeric, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        if(numeric._params.size() != 1)
        {
            Cpu::reportError(Cpu::FncError, stderr, "Functions::STRCMP() : '%s:%d' : STRCMP() requires two string parameters : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
            numeric._isValid = false;
            return numeric;
        }

        // Literal strings, (optimised case)
        if(numeric._varType == Expression::String  &&  numeric._params[0]._varType == Expression::String)
        {
            // No code needed for static initialisation
            if(Expression::getOutputNumeric()._staticInit)
            {
                uint8_t value = uint8_t(numeric._text == numeric._params[0]._text) - 1;
                return Expression::Numeric(value, -1, true, false, false, Expression::Number, Expression::BooleanCC, Expression::Int16Both, std::string(""), std::string(""));
            }
            // Generate code to save result into a tmp var
            else
            {
                int result = int(numeric._text == numeric._params[0]._text) - 1;
                Expression::Numeric numericTmp = Expression::Numeric(int16_t(result), -1, true, false, false, Expression::Number, Expression::BooleanCC, Expression::Int16Both, std::string(""), std::string(""));
                Compiler::emitVcpuAsmForLiteral(numericTmp, false);
            }
        }
        else
        {
            // Get addresses of strings to be compared
            uint16_t srcAddr0 = 0x0000, srcAddr1 = 0x0000;
            if(numeric._varType == Expression::TmpStrVar) srcAddr0 = Compiler::getStrWorkArea(0);
            if(numeric._params[0]._varType == Expression::TmpStrVar) srcAddr1 = Compiler::getStrWorkArea(0);
            if(numeric._varType == Expression::TmpStrVar  &&  numeric._params[0]._varType == Expression::TmpStrVar)
            {
                // If both params are temp strs then swap them so they compare correctly
                srcAddr1 = Compiler::getStrWorkArea(1);
                std::swap(srcAddr0, srcAddr1);
            }

            if(!srcAddr0)
            {
                std::string name;
                int index = int(numeric._index);
                Compiler::getOrCreateString(numeric, name, srcAddr0, index);
            }
            if(!srcAddr1)
            {
                std::string name;
                int index = int(numeric._params[0]._index);
                Compiler::getOrCreateString(numeric._params[0], name, srcAddr1, index);
            }

            // By definition this must be a match
            if(srcAddr0 == srcAddr1)
            {
                Compiler::emitVcpuAsm("LDI", "1", false);
            }
            // Compare strings, -1, 0, 1, (smaller, equal, bigger)
            else
            {
                Compiler::emitVcpuAsm("LDWI", Expression::wordToHexString(srcAddr0), false);
                Compiler::emitVcpuAsm("STW",  "strSrcAddr", false);
                Compiler::emitVcpuAsm("LDWI", Expression::wordToHexString(srcAddr1), false);
                Compiler::emitVcpuAsm("%StringCmp", "", false);
                Compiler::emitVcpuAsm("SUBI", "1", false); // convert 0, 1, 2 to -1, 0, 1
            }
        }

        Compiler::getNextTempVar();
        Operators::changeToTmpVar(numeric);
        Compiler::emitVcpuAsm("STW", Expression::byteToHexString(uint8_t(Compiler::getTempVarStart())), false);
        numeric._params.clear();
        return numeric;
    }

    Expression::Numeric BCDCMP(Expression::Numeric& numeric, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        if(Expression::getOutputNumeric()._staticInit)
        {
            Cpu::reportError(Cpu::FncError, stderr, "Functions::BCDCMP() : '%s:%d' : BCDCMP() cannot be used in static initialisation : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
            numeric._isValid = false;
            return numeric;
        }
        if(numeric._params.size() != 2)
        {
            Cpu::reportError(Cpu::FncError, stderr, "Functions::BCDCMP() : '%s:%d' : BCDCMP() requires three parameters : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
            numeric._isValid = false;
            return numeric;
        }

        // Get addresses and length of bcd values to be compared
        uint16_t srcAddr0 = uint16_t(numeric._value);
        uint16_t srcAddr1 = uint16_t(numeric._params[0]._value);
        uint16_t length = uint16_t(numeric._params[1]._value);

        // Compare bcd values, (addresses MUST point to msd)
        Compiler::emitVcpuAsm("LDWI", Expression::wordToHexString(srcAddr0), false);
        Compiler::emitVcpuAsm("STW",  "bcdSrcAddr", false);
        Compiler::emitVcpuAsm("LDWI", Expression::wordToHexString(srcAddr1), false);
        Compiler::emitVcpuAsm("STW",  "bcdDstAddr", false);
        Compiler::emitVcpuAsm("LDI",  std::to_string(length), false);
        Compiler::emitVcpuAsm("%BcdCmp", "", false);

        Compiler::getNextTempVar();
        Operators::changeToTmpVar(numeric);
        Compiler::emitVcpuAsm("STW", Expression::byteToHexString(uint8_t(Compiler::getTempVarStart())), false);
        numeric._params.clear();
        return numeric;
    }

    Expression::Numeric VAL(Expression::Numeric& numeric, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        if(numeric._params.size() != 0)
        {
            Cpu::reportError(Cpu::FncError, stderr, "Functions::VAL() : '%s:%d' : VAL() requires only one string parameter : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
            numeric._isValid = false;
            return numeric;
        }

        // Literal strings, (optimised case)
        if(numeric._varType == Expression::String)
        {
            int16_t val = 0;
            Expression::stringToI16(numeric._text, val);

            // No code needed for static initialisation
            if(Expression::getOutputNumeric()._staticInit)
            {
                return Expression::Numeric(val, -1, true, false, false, Expression::Number, Expression::BooleanCC, Expression::Int16Both, std::string(""), std::string(""));
            }
            // Generate code to save result into a tmp var
            else
            {
                Expression::Numeric numericTmp = Expression::Numeric(val, -1, true, false, false, Expression::Number, Expression::BooleanCC, Expression::Int16Both, std::string(""), std::string(""));
                Compiler::emitVcpuAsmForLiteral(numericTmp, false);
            }
        }
        else
        {
            // Get addresses of src string
            std::string name;
            uint16_t srcAddr;
            int index = int(numeric._index);
            Compiler::getOrCreateString(numeric, name, srcAddr, index);

            // StringVal expects srcAddr to point past the string's length byte
            Compiler::emitVcpuAsm("LDWI", Expression::wordToHexString(srcAddr + 1), false);
            Compiler::emitVcpuAsm("%IntegerStr", "", false);
        }

        Compiler::getNextTempVar();
        Operators::changeToTmpVar(numeric);
        Compiler::emitVcpuAsm("STW", Expression::byteToHexString(uint8_t(Compiler::getTempVarStart())), false);

        return numeric;
    }

    Expression::Numeric LUP(Expression::Numeric& numeric, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        if(Expression::getOutputNumeric()._staticInit)
        {
            Cpu::reportError(Cpu::FncError, stderr, "Functions::LUP() : '%s:%d' : LUP(<address>, <offset>) cannot be used in static initialisation : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
            numeric._isValid = false;
            return numeric;
        }
        if(numeric._params.size() != 1) 
        {
            Cpu::reportError(Cpu::FncError, stderr, "Functions::LUP() : '%s:%d' : LUP(<address>, <offset>) missing offset : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
            numeric._isValid = false;
            return numeric;
        }

        if(numeric._params[0]._varType != Expression::Number)
        {
            Cpu::reportError(Cpu::FncError, stderr, "Functions::LUP() : '%s:%d' : LUP(<address>, <offset>) offset is not a constant literal : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
            numeric._isValid = false;
            return numeric;
        }

        std::string offset = Expression::byteToHexString(uint8_t(std::lround(numeric._params[0]._value)));

        if(numeric._varType == Expression::Number)
        {
            Compiler::emitVcpuAsm("LDWI", Expression::wordToHexString(uint16_t(std::lround(numeric._value))), false);
        }
        else
        {
            Operators::createSingleOp("LDW", numeric);
        }

        Compiler::getNextTempVar();
        Operators::changeToTmpVar(numeric);
        Compiler::emitVcpuAsm("LUP", offset, false);
        Compiler::emitVcpuAsm("STW", Expression::byteToHexString(uint8_t(Compiler::getTempVarStart())), false);
        numeric._params.clear();
        return numeric;
    }

    Expression::Numeric ADDR(Expression::Numeric& numeric, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        if(numeric._varType == Expression::Number)
        {
            Cpu::reportError(Cpu::FncError, stderr, "Functions::ADDR() : '%s:%d' : parameter can't be a literal : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
            numeric._isValid = false;
            return numeric;
        }
        if(numeric._params.size() != 0)
        {
            Cpu::reportError(Cpu::FncError, stderr, "Functions::ADDR() : '%s:%d' : expects 1 parameter, found %d : %s\n", moduleName.c_str(), codeLineStart, int(numeric._params.size()), codeLineText.c_str());
            numeric._isValid = false;
            return numeric;
        }

        // Handle non variables
        if(numeric._index == -1)
        {
            switch(numeric._varType)
            {
                // Get or create constant string
                case Expression::String:
                {
                    int index;
                    Compiler::getOrCreateConstString(numeric._text, index);
                    numeric._index = int16_t(index);
                    numeric._varType = Expression::StrVar;
                }
                break;

                // Needs to pass through
                case Expression::TmpStrVar:
                {
                }
                break;

                default:
                {
                    Cpu::reportError(Cpu::FncError, stderr, "Functions::ADDR() : '%s:%d' : couldn't find variable name '%s' : %s\n", moduleName.c_str(), codeLineStart, numeric._name.c_str(), codeLineText.c_str());
                    numeric._isValid = false;
                    return numeric;
                }
            }
        }

        uint16_t address = 0x0000;
        switch(numeric._varType)
        {
            case Expression::IntVar16: 
            case Expression::Arr1Var8:
            case Expression::Arr1Var16: address = Compiler::getIntegerVars()[numeric._index]._address; break;
            case Expression::Constant:  address = Compiler::getConstants()[numeric._index]._address;   break;
            case Expression::StrVar:    address = Compiler::getStringVars()[numeric._index]._address;  break;

            default: break;
        }

        // No code needed for static initialisation
        if(Expression::getOutputNumeric()._staticInit)
        {
            switch(numeric._varType)
            {
                case Expression::TmpVar:
                case Expression::Str2Var:
                {
                    Cpu::reportError(Cpu::FncError, stderr, "Functions::ADDR() : '%s:%d' : can't statically initialise from multi-dimensional array '%s' : %s\n",
                                                            moduleName.c_str(), codeLineStart, numeric._name.c_str(), codeLineText.c_str());
                    numeric._isValid = false;
                    return numeric;
                }

                default: break;
            }
            
            return Expression::Numeric(address, -1, true, false, false, Expression::Number, Expression::BooleanCC, Expression::Int16Both, std::string(""), std::string(""));
        }

        // String vars
        if(numeric._varType == Expression::StrVar  &&  !Compiler::getStringVars()[numeric._index]._constant)
        {
            Compiler::emitVcpuAsm("LDWI", Expression::wordToHexString(Compiler::getStringVars()[numeric._index]._address), false);
        }
        // Multi-dimensional arrays, (array of strings, Str2Var, is treated as a 2D array of bytes)
        else if(numeric._varType == Expression::TmpVar  ||  numeric._varType == Expression::Str2Var)
        {
            Compiler::emitVcpuAsm("LDW", Expression::byteToHexString(uint8_t(Compiler::getTempVarStart())), false);
        }
        // Temp string vars
        else if(numeric._varType == Expression::TmpStrVar)
        {
            Compiler::emitVcpuAsm("LDWI", Expression::wordToHexString(Compiler::getStrWorkArea()), false);
        }
        // Ints, int arrays and constants
        else
        {
            Compiler::emitVcpuAsm("LDWI", Expression::wordToHexString(address), false);
        }

        Compiler::getNextTempVar();
        Operators::changeToTmpVar(numeric);
        Compiler::emitVcpuAsm("STW", Expression::byteToHexString(uint8_t(Compiler::getTempVarStart())), false);

        return numeric;
    }

    // TODO: not finished
    Expression::Numeric ARRB(Expression::Numeric& numeric, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        if(numeric._params.size() != 1)
        {
            Cpu::reportError(Cpu::FncError, stderr, "Functions::ARRB() : '%s:%d' : expects 2 parameter, found %d : %s\n", moduleName.c_str(), codeLineStart, int(numeric._params.size()), codeLineText.c_str());
            numeric._isValid = false;
            return numeric;
        }

        return numeric;
    }

    // TODO: not finished
    Expression::Numeric ARRW(Expression::Numeric& numeric, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        if(numeric._params.size() != 1)
        {
            Cpu::reportError(Cpu::FncError, stderr, "Functions::ARRW() : '%s:%d' : expects 2 parameter, found %d : %s\n", moduleName.c_str(), codeLineStart, int(numeric._params.size()), codeLineText.c_str());
            numeric._isValid = false;
            return numeric;
        }

        return numeric;
    }

    Expression::Numeric POINT(Expression::Numeric& numeric, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        if(Expression::getOutputNumeric()._staticInit)
        {
            Cpu::reportError(Cpu::FncError, stderr, "Functions::POINT() : '%s:%d' : POINT() cannot be used in static initialisation : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
            numeric._isValid = false;
            return numeric;
        }
        if(numeric._params.size() != 1)
        {
            Cpu::reportError(Cpu::FncError, stderr, "Functions::POINT() : '%s:%d' : syntax error, 'POINT(x, y)' requires two parameters : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
            numeric._isValid = false;
            return numeric;
        }

        if(Compiler::getCodeRomType() >= Cpu::ROMvX0  &&  Compiler::getCodeRomType() < Cpu::SDCARD)
        {
            if(numeric._varType == Expression::Number  &&  numeric._params[0]._varType == Expression::Number)
            {
                Compiler::emitVcpuAsm("LDWI", Expression::wordToHexString(uint16_t(std::lround(numeric._params[0]._value))*256 + uint16_t(std::lround(numeric._value))), false);
            }
            else
            {
                if(numeric._varType == Expression::Number)
                {
                    Compiler::emitVcpuAsm("MOVQB", "giga_vAC, " + Expression::byteToHexString(uint8_t(std::lround(numeric._value))), false);
                }
                else
                {
                    Operators::createSingleOp("MOVB", numeric, ", giga_vAC");
                }
                
                if(numeric._params[0]._varType == Expression::Number)
                {
                    Compiler::emitVcpuAsm("MOVQB", "giga_vAC + 1, " + Expression::byteToHexString(uint8_t(std::lround(numeric._params[0]._value))), false);
                }
                else
                {
                    Operators::createSingleOp("MOVB", numeric._params[0], ", giga_vAC + 1");
                }
            }

            Compiler::getNextTempVar();
            Operators::changeToTmpVar(numeric);
            Compiler::emitVcpuAsm("LDPX", "giga_vAC, giga_vAC", false);
            Compiler::emitVcpuAsm("ST", Expression::byteToHexString(uint8_t(Compiler::getTempVarStart())), false);
        }
        else
        {
            if(numeric._varType == Expression::Number  &&  numeric._params[0]._varType == Expression::Number)
            {
                Compiler::emitVcpuAsm("LDWI", Expression::wordToHexString(uint16_t(std::lround(numeric._params[0]._value))*256 + uint16_t(std::lround(numeric._value))), false);
                Compiler::emitVcpuAsm("STW", "readPixel_xy", false);
            }
            else
            {
                if(numeric._varType == Expression::Number)
                {
                    Compiler::emitVcpuAsm("LDI", Expression::byteToHexString(uint8_t(std::lround(numeric._value))), false);
                    Compiler::emitVcpuAsm("ST", "readPixel_xy", false);
                }
                else
                {
                    Operators::createSingleOp("LDW", numeric);
                    Compiler::emitVcpuAsm("ST", "readPixel_xy", false);
                }

                if(numeric._params[0]._varType == Expression::Number)
                {
                    Compiler::emitVcpuAsm("LDI", Expression::byteToHexString(uint8_t(std::lround(numeric._params[0]._value))), false);
                    Compiler::emitVcpuAsm("ST", "readPixel_xy + 1", false);
                }
                else
                {
                    Operators::createSingleOp("LDW", numeric._params[0]);
                    Compiler::emitVcpuAsm("ST", "readPixel_xy + 1", false);
                }
            }

            Compiler::getNextTempVar();
            Operators::changeToTmpVar(numeric);
            Compiler::emitVcpuAsm("%ReadPixel", "", false);
            Compiler::emitVcpuAsm("STW", Expression::byteToHexString(uint8_t(Compiler::getTempVarStart())), false);
        }

        numeric._params.clear();
        return numeric;
    }

    Expression::Numeric MIN(Expression::Numeric& numeric, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        if(numeric._params.size() != 1)
        {
            Cpu::reportError(Cpu::FncError, stderr, "Functions::MIN() : '%s:%d' : syntax error, 'MIN(x, y)' requires two parameters : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
            numeric._isValid = false;
            return numeric;
        }
        if(numeric._varType == Expression::Number  &&  numeric._params[0]._varType == Expression::Number)
        {
            numeric._value = std::min(numeric._value, numeric._params[0]._value);
            numeric._params.clear();
            return numeric;
        }

        if(numeric._varType == Expression::Number)
        {
            Compiler::emitVcpuAsmForLiteral(numeric, "intSrcA", false);
        }
        else
        {
            Operators::createSingleOp("LDW", numeric);
            Compiler::emitVcpuAsm("STW", "intSrcA", false);
        }

        if(numeric._params[0]._varType == Expression::Number)
        {
            Compiler::emitVcpuAsmForLiteral(numeric._params[0], false);
        }
        else
        {
            Operators::createSingleOp("LDW", numeric._params[0]);
        }

        Compiler::getNextTempVar();
        Operators::changeToTmpVar(numeric);
        Compiler::emitVcpuAsm("%IntMin", "", false);
        Compiler::emitVcpuAsm("STW", Expression::byteToHexString(uint8_t(Compiler::getTempVarStart())), false);
        numeric._params.clear();
        return numeric;
    }

    Expression::Numeric MAX(Expression::Numeric& numeric, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        if(numeric._params.size() != 1)
        {
            Cpu::reportError(Cpu::FncError, stderr, "Functions::MAX() : '%s:%d' : syntax error, 'MAX(x, y)' requires two parameters : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
            numeric._isValid = false;
            return numeric;
        }
        if(numeric._varType == Expression::Number  &&  numeric._params[0]._varType == Expression::Number)
        {
            numeric._value = std::max(numeric._value, numeric._params[0]._value);
            numeric._params.clear();
            return numeric;
        }

        if(numeric._varType == Expression::Number)
        {
            Compiler::emitVcpuAsmForLiteral(numeric, "intSrcA", false);
        }
        else
        {
            Operators::createSingleOp("LDW", numeric);
            Compiler::emitVcpuAsm("STW", "intSrcA", false);
        }

        if(numeric._params[0]._varType == Expression::Number)
        {
            Compiler::emitVcpuAsmForLiteral(numeric._params[0], false);
        }
        else
        {
            Operators::createSingleOp("LDW", numeric._params[0]);
        }

        Compiler::getNextTempVar();
        Operators::changeToTmpVar(numeric);
        Compiler::emitVcpuAsm("%IntMax", "", false);
        Compiler::emitVcpuAsm("STW", Expression::byteToHexString(uint8_t(Compiler::getTempVarStart())), false);
        numeric._params.clear();
        return numeric;
    }

    Expression::Numeric CLAMP(Expression::Numeric& numeric, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        if(numeric._params.size() != 2)
        {
            Cpu::reportError(Cpu::FncError, stderr, "Functions::CLAMP() : '%s:%d' : syntax error, 'CLAMP(x, a, b)' requires three parameters : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
            numeric._isValid = false;
            return numeric;
        }
        if(numeric._varType == Expression::Number  &&  numeric._params[0]._varType == Expression::Number  &&  numeric._params[1]._varType == Expression::Number)
        {
            numeric._value = std::min(std::max(numeric._value, numeric._params[0]._value), numeric._params[1]._value);
            numeric._params.clear();
            return numeric;
        }

        if(numeric._varType == Expression::Number)
        {
            Compiler::emitVcpuAsmForLiteral(numeric, "intSrcX", false);
        }
        else
        {
            Operators::createSingleOp("LDW", numeric);
            Compiler::emitVcpuAsm("STW", "intSrcX", false);
        }

        if(numeric._params[0]._varType == Expression::Number)
        {
            Compiler::emitVcpuAsmForLiteral(numeric._params[0], "intSrcA", false);
        }
        else
        {
            Operators::createSingleOp("LDW", numeric._params[0]);
            Compiler::emitVcpuAsm("STW", "intSrcA", false);
        }

        if(numeric._params[1]._varType == Expression::Number)
        {
            Compiler::emitVcpuAsmForLiteral(numeric._params[1], false);
        }
        else
        {
            Operators::createSingleOp("LDW", numeric._params[1]);
        }

        Compiler::getNextTempVar();
        Operators::changeToTmpVar(numeric);
        Compiler::emitVcpuAsm("%IntClamp", "", false);
        Compiler::emitVcpuAsm("STW", Expression::byteToHexString(uint8_t(Compiler::getTempVarStart())), false);
        numeric._params.clear();
        return numeric;
    }

    Expression::Numeric COND(Expression::Numeric& numeric, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        if(numeric._params.size() != 2)
        {
            Cpu::reportError(Cpu::FncError, stderr, "Functions::COND() : '%s:%d' : syntax error, 'COND(x, a, b)' requires three parameters : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
            numeric._isValid = false;
            return numeric;
        }
        if(numeric._varType == Expression::Number  &&  numeric._params[0]._varType == Expression::Number  &&  numeric._params[1]._varType == Expression::Number)
        {
            numeric._value = (numeric._value) ? numeric._params[0]._value : numeric._params[1]._value;
            numeric._params.clear();
            return numeric;
        }

        // Specific case for ROMvX0 and both outputs are immediates 0 <-> 255
        if(Compiler::getCodeRomType() >= Cpu::ROMvX0  &&  Compiler::getCodeRomType() < Cpu::SDCARD)
        {
            if(numeric._params[0]._varType == Expression::Number  &&  numeric._params[1]._varType == Expression::Number)
            {
                if(numeric._params[0]._value >= 0  &&  numeric._params[0]._value <= 255  &&  numeric._params[1]._value >= 0  &&  numeric._params[1]._value <= 255)
                {
                    uint8_t imm0 = int8_t(std::lround(numeric._params[0]._value));
                    uint8_t imm1 = int8_t(std::lround(numeric._params[1]._value));
                    Operators::createSingleOp("LDW", numeric);
                    Compiler::emitVcpuAsm("CONDI", std::to_string(imm0) + ", " + std::to_string(imm1), false);

                    Compiler::getNextTempVar();
                    Operators::changeToTmpVar(numeric);
                    Compiler::emitVcpuAsm("STW", Expression::byteToHexString(uint8_t(Compiler::getTempVarStart())), false);
                    numeric._params.clear();
                    return numeric;
                }
            }
        }

        // Condition to check against 0
        if(numeric._varType == Expression::Number)
        {
            Compiler::emitVcpuAsmForLiteral(numeric, false);
        }
        else
        {
            Operators::createSingleOp("LDW", numeric);
        }

        std::string internalLabel = Expression::wordToHexString(Compiler::getVasmPC());

        // Both outputs are immediates
        if(numeric._params[0]._varType == Expression::Number  &&  numeric._params[1]._varType == Expression::Number)
        {
            std::string imm0 = std::to_string(int16_t(std::lround(numeric._params[0]._value)));
            std::string imm1 = std::to_string(int16_t(std::lround(numeric._params[1]._value)));
            Compiler::emitVcpuAsm("%IntCondii", "_cond_" + internalLabel + " " + imm0 + " " + imm1, false);
        }
        // Var output + immediate output
        else if(numeric._params[0]._varType != Expression::Number  &&  numeric._params[1]._varType == Expression::Number)
        {
            std::string var = Operators::getVarAsOperand(numeric._params[0]);
            std::string imm = std::to_string(int16_t(std::lround(numeric._params[1]._value)));
            Compiler::emitVcpuAsm("%IntCondvi", "_cond_" + internalLabel + " " + var + " " + imm, false);
        }
        // Immediate output + var output
        else if(numeric._params[0]._varType == Expression::Number  &&  numeric._params[1]._varType != Expression::Number)
        {
            std::string imm = std::to_string(int16_t(std::lround(numeric._params[0]._value)));
            std::string var = Operators::getVarAsOperand(numeric._params[1]);
            Compiler::emitVcpuAsm("%IntCondiv", "_cond_" + internalLabel + " " + imm + " " + var, false);
        }
        else
        {
            // Var output + var output
            std::string var0 = Operators::getVarAsOperand(numeric._params[0]);
            std::string var1 = Operators::getVarAsOperand(numeric._params[1]);
            Compiler::emitVcpuAsm("%IntCondvv", "_cond_" + internalLabel + " " + var0 + " " + var1, false);
        }

        Compiler::getNextTempVar();
        Operators::changeToTmpVar(numeric);
        Compiler::emitVcpuAsm("STW", Expression::byteToHexString(uint8_t(Compiler::getTempVarStart())), false);
        numeric._params.clear();
        return numeric;
    }

    Expression::Numeric FMUL(Expression::Numeric& numeric, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD)
        {
            Cpu::reportError(Cpu::FncError, stderr, "Functions::FMUL() : '%s:%d' : version error, 'FMUL(x, y)' requires ROMvX0 : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
            numeric._isValid = false;
            return numeric;
        }
        if(numeric._params.size() != 1)
        {
            Cpu::reportError(Cpu::FncError, stderr, "Functions::FMUL() : '%s:%d' : syntax error, 'FMUL(x, y)' requires two parameters : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
            numeric._isValid = false;
            return numeric;
        }
        if(numeric._varType == Expression::Number  &&  numeric._params[0]._varType == Expression::Number)
        {
            numeric._value = int16_t(std::lround(numeric._value)) * int16_t(std::lround(numeric._params[0]._value));
            numeric._params.clear();
            return numeric;
        }

        if(numeric._varType == Expression::Number)
        {
            uint8_t val = uint8_t(std::lround(numeric._value));
            Compiler::emitVcpuAsm("MOVQB", "giga_sysArg2, " + std::to_string(val), false);
        }
        else
        {
            Operators::createSingleOp("LDW", numeric);
            Compiler::emitVcpuAsm("ST", "giga_sysArg2", false);
        }

        if(numeric._params[0]._varType == Expression::Number)
        {
            uint8_t val = uint8_t(std::lround(numeric._params[0]._value));
            Compiler::emitVcpuAsm("MOVQB", "giga_sysArg3, " + std::to_string(val), false);
        }
        else
        {
            Operators::createSingleOp("LDW", numeric._params[0]);
            Compiler::emitVcpuAsm("ST", "giga_sysArg3", false);
        }

        Compiler::getNextTempVar();
        Operators::changeToTmpVar(numeric);
        Compiler::emitVcpuAsm("%FMul", "", false);
        Compiler::emitVcpuAsm("STW", Expression::byteToHexString(uint8_t(Compiler::getTempVarStart())), false);
        numeric._params.clear();
        return numeric;
    }

    Expression::Numeric FDIV(Expression::Numeric& numeric, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD)
        {
            Cpu::reportError(Cpu::FncError, stderr, "Functions::FDIV() : '%s:%d' : version error, 'FDIV(x, y)' requires ROMvX0 : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
            numeric._isValid = false;
            return numeric;
        }
        if(numeric._params.size() != 1)
        {
            Cpu::reportError(Cpu::FncError, stderr, "Functions::FDIV() : '%s:%d' : syntax error, 'FDIV(x, y)' requires two parameters : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
            numeric._isValid = false;
            return numeric;
        }
        if(numeric._varType == Expression::Number  &&  numeric._params[0]._varType == Expression::Number)
        {
            numeric._value = numeric._value / numeric._params[0]._value;
            numeric._params.clear();
            return numeric;
        }

        if(numeric._varType == Expression::Number)
        {
            Compiler::emitVcpuAsmForLiteral(numeric, "giga_sysArg0", false);
        }
        else
        {
            Operators::createSingleOp("LDW", numeric);
            Compiler::emitVcpuAsm("STW", "giga_sysArg0", false);
        }

        if(numeric._params[0]._varType == Expression::Number)
        {
            Compiler::emitVcpuAsmForLiteral(numeric._params[0], "giga_sysArg2", false);
        }
        else
        {
            Operators::createSingleOp("LDW", numeric._params[0]);
            Compiler::emitVcpuAsm("STW", "giga_sysArg2", false);
        }

        Compiler::getNextTempVar();
        Operators::changeToTmpVar(numeric);
        Compiler::emitVcpuAsm("%FDiv", "", false);
        Compiler::emitVcpuAsm("STW", Expression::byteToHexString(uint8_t(Compiler::getTempVarStart())), false);
        numeric._params.clear();
        return numeric;
    }

    Expression::Numeric FMOD(Expression::Numeric& numeric, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD)
        {
            Cpu::reportError(Cpu::FncError, stderr, "Functions::FMOD() : '%s:%d' : version error, 'FMOD(x, y)' requires ROMvX0 : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
            numeric._isValid = false;
            return numeric;
        }
        if(numeric._params.size() != 1)
        {
            Cpu::reportError(Cpu::FncError, stderr, "Functions::FMOD() : '%s:%d' : syntax error, 'FMOD(x, y)' requires two parameters : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
            numeric._isValid = false;
            return numeric;
        }
        if(numeric._varType == Expression::Number  &&  numeric._params[0]._varType == Expression::Number)
        {
            numeric._value = int16_t(std::lround(numeric._value)) % int16_t(std::lround(numeric._params[0]._value));
            numeric._params.clear();
            return numeric;
        }

        if(numeric._varType == Expression::Number)
        {
            Compiler::emitVcpuAsmForLiteral(numeric, "giga_sysArg0", false);
        }
        else
        {
            Operators::createSingleOp("LDW", numeric);
            Compiler::emitVcpuAsm("STW", "giga_sysArg0", false);
        }

        if(numeric._params[0]._varType == Expression::Number)
        {
            uint8_t val = uint8_t(std::lround(numeric._params[0]._value));
            Compiler::emitVcpuAsm("MOVQB", "giga_sysArg2, " + std::to_string(val), false);
        }
        else
        {
            Operators::createSingleOp("LDW", numeric._params[0]);
            Compiler::emitVcpuAsm("ST", "giga_sysArg2", false);
        }

        Compiler::getNextTempVar();
        Operators::changeToTmpVar(numeric);
        Compiler::emitVcpuAsm("%FMod", "", false);
        Compiler::emitVcpuAsm("STW", Expression::byteToHexString(uint8_t(Compiler::getTempVarStart())), false);
        numeric._params.clear();
        return numeric;
    }

    Expression::Numeric FDIVMOD(Expression::Numeric& numeric, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD)
        {
            Cpu::reportError(Cpu::FncError, stderr, "Functions::FDIVMOD() : '%s:%d' : version error, 'FDIVMOD(x, y)' requires ROMvX0 : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
            numeric._isValid = false;
            return numeric;
        }
        if(numeric._params.size() != 1)
        {
            Cpu::reportError(Cpu::FncError, stderr, "Functions::FDIVMOD() : '%s:%d' : syntax error, 'FDIVMOD(x, y)' requires two parameters : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
            numeric._isValid = false;
            return numeric;
        }
        if(numeric._varType == Expression::Number  &&  numeric._params[0]._varType == Expression::Number)
        {
            uint8_t res = uint8_t(uint16_t(std::lround(numeric._value)) / uint8_t(std::lround(numeric._params[0]._value)));
            uint8_t rem = uint8_t(uint16_t(std::lround(numeric._value)) % uint8_t(std::lround(numeric._params[0]._value)));
            numeric._value = rem*256 + res;
            numeric._params.clear();
            return numeric;
        }

        if(numeric._varType == Expression::Number)
        {
            Compiler::emitVcpuAsmForLiteral(numeric, "giga_sysArg0", false);
        }
        else
        {
            Operators::createSingleOp("LDW", numeric);
            Compiler::emitVcpuAsm("STW", "giga_sysArg0", false);
        }

        if(numeric._params[0]._varType == Expression::Number)
        {
            uint8_t val = uint8_t(std::lround(numeric._params[0]._value));
            Compiler::emitVcpuAsm("MOVQB", "giga_sysArg2, " + std::to_string(val), false);
        }
        else
        {
            Operators::createSingleOp("LDW", numeric._params[0]);
            Compiler::emitVcpuAsm("ST", "giga_sysArg2", false);
        }

        Compiler::getNextTempVar();
        Operators::changeToTmpVar(numeric);
        Compiler::emitVcpuAsm("%FDivMod", "", false);
        Compiler::emitVcpuAsm("STW", Expression::byteToHexString(uint8_t(Compiler::getTempVarStart())), false);
        numeric._params.clear();
        return numeric;
    }

    Expression::Numeric SQRT(Expression::Numeric& numeric, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        if(Expression::getOutputNumeric()._staticInit)
        {
            Cpu::reportError(Cpu::FncError, stderr, "Functions::SQRT() : '%s:%d' : SQRT() cannot be used in static initialisation : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
            numeric._isValid = false;
            return numeric;
        }

        if(numeric._varType != Expression::String  &&  numeric._varType != Expression::StrVar &&  numeric._varType != Expression::TmpStrVar)
        {
            Compiler::getNextTempVar();

            if(numeric._varType == Expression::Number)
            {
                numeric._value = sqrt(numeric._value);
                Compiler::emitVcpuAsmForLiteral(numeric, false);
                Operators::changeToTmpVar(numeric);
                Compiler::emitVcpuAsm("STW", Expression::byteToHexString(uint8_t(Compiler::getTempVarStart())), false);
            }
            else
            {
                Operators::handleSingleOp("LDW", numeric);
                Compiler::emitVcpuAsm("%Sqrt", "", false);
                Compiler::emitVcpuAsm("STW", Expression::byteToHexString(uint8_t(Compiler::getTempVarStart())), false);
            }
        }

        return numeric;
    }

    Expression::Numeric CHR$(Expression::Numeric& numeric, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        if(Expression::getOutputNumeric()._staticInit)
        {
            Cpu::reportError(Cpu::FncError, stderr, "Functions::CHR$() : '%s:%d' : CHR$() cannot be used in static initialisation : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
            numeric._isValid = false;
            return numeric;
        }

        // Create a new temporary string
        if(!isFuncNested()) Compiler::nextStrWorkArea();
        uint16_t dstAddr = Compiler::getStrWorkArea();

        if(numeric._varType == Expression::Number)
        {
            // Print CHR string, (without wasting memory)
            if(Expression::getEnableOptimisedPrint()  &&  Expression::getOutputNumeric()._nestedCount == 0)
            {
                Compiler::emitVcpuAsm("LDI", std::to_string(int16_t(std::lround(numeric._value))), false);
                Compiler::emitVcpuAsm("%PrintAcChr", "", false);
                return numeric;
            }

            // Create CHR string
            Compiler::emitVcpuAsm("LDI", std::to_string(int16_t(std::lround(numeric._value))), false);
            Compiler::emitVcpuAsm("STW", "strChr", false);
            Compiler::emitVcpuAsm("LDWI", Expression::wordToHexString(dstAddr), false);
            Compiler::emitVcpuAsm("%StringChr", "", false);

            return Expression::Numeric(0, uint16_t(-1), true, false, false, Expression::TmpStrVar, Expression::BooleanCC, Expression::Int16Both, std::string(""), std::string(""));
        }

        Compiler::getNextTempVar();
        Operators::handleSingleOp("LDW", numeric);
        if(Expression::getEnableOptimisedPrint()  &&  Expression::getOutputNumeric()._nestedCount == 0)
        {
            Compiler::emitVcpuAsm("%PrintAcChr", "", false);
            return numeric;
        }

        // Create CHR string
        Compiler::emitVcpuAsm("STW", "strChr", false);
        Compiler::emitVcpuAsm("LDWI", Expression::wordToHexString(dstAddr), false);
        Compiler::emitVcpuAsm("%StringChr", "", false);

        return Expression::Numeric(0, uint16_t(-1), true, false, false, Expression::TmpStrVar, Expression::BooleanCC, Expression::Int16Both, std::string(""), std::string(""));
    }

    Expression::Numeric SPC$(Expression::Numeric& numeric, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        if(Expression::getOutputNumeric()._staticInit)
        {
            Cpu::reportError(Cpu::FncError, stderr, "Functions::SPC$() : '%s:%d' : SPC$() cannot be used in static initialisation : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
            numeric._isValid = false;
            return numeric;
        }

        // Create a new temporary string
        if(!isFuncNested()) Compiler::nextStrWorkArea();
        uint16_t dstAddr = Compiler::getStrWorkArea();

        if(numeric._varType == Expression::Number)
        {
            uint8_t len = uint8_t(std::lround(numeric._value));
            if(len < 1  ||  len > 94)
            {
                Cpu::reportError(Cpu::FncError, stderr, "Functions::SPC$() : '%s:%d' : syntax error, 'SPC$(n)', if 'n' is a literal, it MUST be <1-94> : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
                numeric._isValid = false;
                return numeric;
            }

            if(Expression::getEnableOptimisedPrint()  &&  Expression::getOutputNumeric()._nestedCount == 0)
            {
                Compiler::emitVcpuAsm("LDI", std::to_string(len), false);
                Compiler::emitVcpuAsm("%PrintSpc", "", false);
                return numeric;
            }

            // Create SPC string
            Compiler::emitVcpuAsm("LDI", std::to_string(len), false);
            Compiler::emitVcpuAsm("STW", "strLen", false);
            Compiler::emitVcpuAsm("LDWI", Expression::wordToHexString(dstAddr), false);
            Compiler::emitVcpuAsm("%StringSpc", "", false);

            return Expression::Numeric(0, uint16_t(-1), true, false, false, Expression::TmpStrVar, Expression::BooleanCC, Expression::Int16Both, std::string(""), std::string(""));
        }

        Compiler::getNextTempVar();
        Operators::handleSingleOp("LDW", numeric);
        if(Expression::getEnableOptimisedPrint()  &&  Expression::getOutputNumeric()._nestedCount == 0)
        {
            Compiler::emitVcpuAsm("%PrintSpc", "", false);
            return numeric;
        }

        // Create SPC string
        Compiler::emitVcpuAsm("STW", "strLen", false);
        Compiler::emitVcpuAsm("LDWI", Expression::wordToHexString(dstAddr), false);
        Compiler::emitVcpuAsm("%StringSpc", "", false);

        return Expression::Numeric(0, uint16_t(-1), true, false, false, Expression::TmpStrVar, Expression::BooleanCC, Expression::Int16Both, std::string(""), std::string(""));
    }

    Expression::Numeric STR$(Expression::Numeric& numeric, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        if(Expression::getOutputNumeric()._staticInit)
        {
            Cpu::reportError(Cpu::FncError, stderr, "Functions::STR$() : '%s:%d' : STR$() cannot be used in static initialisation : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
            numeric._isValid = false;
            return numeric;
        }

        // Create a new temporary string
        if(!isFuncNested()) Compiler::nextStrWorkArea();
        uint16_t dstAddr = Compiler::getStrWorkArea();

        if(numeric._varType == Expression::Number)
        {
            // Print STR string, (without wasting memory)
            if(Expression::getEnableOptimisedPrint()  &&  Expression::getOutputNumeric()._nestedCount == 0)
            {
                Compiler::emitVcpuAsm("LDWI", Expression::wordToHexString(int16_t(std::lround(numeric._value))), false);
                Compiler::emitVcpuAsm("%PrintAcInt16", "", false);
                return numeric;
            }

            // Create STR string
            Compiler::emitVcpuAsm("LDWI", Expression::wordToHexString(int16_t(std::lround(numeric._value))), false);
            Compiler::emitVcpuAsm("STW", "strInteger", false);
            Compiler::emitVcpuAsm("LDWI", Expression::wordToHexString(dstAddr), false);
            Compiler::emitVcpuAsm("%StringInt", "", false);

            return Expression::Numeric(0, uint16_t(-1), true, false, false, Expression::TmpStrVar, Expression::BooleanCC, Expression::Int16Both, std::string(""), std::string(""));
        }

        Compiler::getNextTempVar();
        Operators::handleSingleOp("LDW", numeric);
        if(Expression::getEnableOptimisedPrint()  &&  Expression::getOutputNumeric()._nestedCount == 0)
        {
            Compiler::emitVcpuAsm("%PrintAcInt16", "", false);
            return numeric;
        }

        // Create STR string
        Compiler::emitVcpuAsm("STW", "strInteger", false);
        Compiler::emitVcpuAsm("LDWI", Expression::wordToHexString(dstAddr), false);
        Compiler::emitVcpuAsm("%StringInt", "", false);

        return Expression::Numeric(0, uint16_t(-1), true, false, false, Expression::TmpStrVar, Expression::BooleanCC, Expression::Int16Both, std::string(""), std::string(""));
    }

    Expression::Numeric STRING$(Expression::Numeric& numeric, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        if(Expression::getOutputNumeric()._staticInit)
        {
            Cpu::reportError(Cpu::FncError, stderr, "Functions::STRING$() : '%s:%d' : STRING$() cannot be used in static initialisation : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
            numeric._isValid = false;
            return numeric;
        }

        if(numeric._varType == Expression::Number)
        {
            // Print STRING string, (without wasting memory)
            if(Expression::getEnableOptimisedPrint()  &&  Expression::getOutputNumeric()._nestedCount == 0)
            {
                Compiler::emitVcpuAsm("LDWI", Expression::wordToHexString(uint16_t(std::lround(numeric._value))), false);
                Compiler::emitVcpuAsm("%PrintAcString", "", false);
                return numeric;
            }

            // Point to STRING address
            return Expression::Numeric(numeric._value, uint16_t(-1), true, false, false, Expression::StrAddr, Expression::BooleanCC, Expression::Int16Both, std::string(""), std::string(""));
        }

        Compiler::getNextTempVar();
        Operators::handleSingleOp("LDW", numeric);
        if(Expression::getEnableOptimisedPrint()  &&  Expression::getOutputNumeric()._nestedCount == 0)
        {
            Compiler::emitVcpuAsm("%PrintAcString", "", false);
            return numeric;
        }

        Operators::changeToTmpVar(numeric);
        Compiler::emitVcpuAsm("STW", Expression::byteToHexString(uint8_t(Compiler::getTempVarStart())), false);
        numeric._varType = Expression::TmpStrAddr;

        return numeric;
    }

    Expression::Numeric TIME$(Expression::Numeric& numeric, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        if(Expression::getOutputNumeric()._staticInit)
        {
            Cpu::reportError(Cpu::FncError, stderr, "Functions::TIME$() : '%s:%d' : TIME$() cannot be used in static initialisation : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
            numeric._isValid = false;
            return numeric;
        }

        // Function with no parameters, so isValid needs to be explicitly set
        numeric._isValid = true;

        // Generate new time string
        Compiler::emitVcpuAsm("%TimeString", "", false);

        // Print it directly if able
        if(Expression::getEnableOptimisedPrint()  &&  Expression::getOutputNumeric()._nestedCount == 0)
        {
            Compiler::emitVcpuAsm("%PrintString", "_timeString_", false);
            return numeric;
        }

        // Create a new temporary string
        if(!isFuncNested()) Compiler::nextStrWorkArea();
        uint16_t dstAddr = Compiler::getStrWorkArea();

        Compiler::emitVcpuAsm("LDWI", "_timeString_", false);
        Compiler::emitVcpuAsm("STW", "strSrcAddr", false);
        Compiler::emitVcpuAsm("LDWI", Expression::wordToHexString(dstAddr), false);
        Compiler::emitVcpuAsm("%StringCopy", "", false);

        return Expression::Numeric(0, uint16_t(-1), true, false, false, Expression::TmpStrVar, Expression::BooleanCC, Expression::Int16Both, std::string(""), std::string(""));
    }

    Expression::Numeric HEX$(Expression::Numeric& numeric, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        if(Expression::getOutputNumeric()._staticInit)
        {
            Cpu::reportError(Cpu::FncError, stderr, "Functions::HEX$() : '%s:%d' : HEX$() cannot be used in static initialisation : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
            numeric._isValid = false;
            return numeric;
        }
        if(numeric._params.size() != 1)
        {
            Cpu::reportError(Cpu::FncError, stderr, "Functions::HEX$() : '%s:%d' : syntax error, 'HEX$(x, n)' requires two parameters : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
            numeric._isValid = false;
            return numeric;
        }

        if(numeric._params[0]._varType == Expression::Number)
        {
            int16_t val = int16_t(std::lround(numeric._params[0]._value));
            if(val < 1  ||  val > 4)
            {
                Cpu::reportError(Cpu::FncError, stderr, "Functions::HEX$() : '%s:%d' : syntax error, 'HEX$(x, n)', if 'n' is a literal, it MUST be <1-4> : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
                numeric._isValid = false;
                return numeric;
            }

            Compiler::emitVcpuAsm("LDI", std::to_string(val), false);
        }
        else
        {
            Operators::createSingleOp("LDW", numeric._params[0]);
        }
        Compiler::emitVcpuAsm("ST", "textLen", false);

        // Create a new temporary string
        if(!isFuncNested()) Compiler::nextStrWorkArea();
        uint16_t dstAddr = Compiler::getStrWorkArea();

        if(numeric._varType == Expression::Number)
        {
            Compiler::emitVcpuAsm("LDWI", Expression::wordToHexString(uint16_t(std::lround(numeric._value))), false);
            Compiler::emitVcpuAsm("STW", "textHex", false);

            // Print HEX string, (without wasting memory)
            if(Expression::getEnableOptimisedPrint()  &&  Expression::getOutputNumeric()._nestedCount == 0)
            {
                Compiler::emitVcpuAsm("%PrintHex", "", false);
                numeric._params.clear();
                return numeric;
            }

            // Create HEX string
            Compiler::emitVcpuAsm("LDWI", Expression::wordToHexString(dstAddr), false);
            Compiler::emitVcpuAsm("%StringHex", "", false);
            return Expression::Numeric(0, uint16_t(-1), true, false, false, Expression::TmpStrVar, Expression::BooleanCC, Expression::Int16Both, std::string(""), std::string(""));
        }

        Compiler::getNextTempVar();
        Operators::handleSingleOp("LDW", numeric);
        if(Expression::getEnableOptimisedPrint()  &&  Expression::getOutputNumeric()._nestedCount == 0)
        {
            Compiler::emitVcpuAsm("STW", "textHex", false);
            Compiler::emitVcpuAsm("%PrintHex", "", false);
            numeric._params.clear();
            return numeric;
        }

        // Create HEX string
        Compiler::emitVcpuAsm("STW", "strHex", false);
        Compiler::emitVcpuAsm("LDWI", Expression::wordToHexString(dstAddr), false);
        Compiler::emitVcpuAsm("%StringHex", "", false);
        return Expression::Numeric(0, uint16_t(-1), true, false, false, Expression::TmpStrVar, Expression::BooleanCC, Expression::Int16Both, std::string(""), std::string(""));
    }

    Expression::Numeric LEFT$(Expression::Numeric& numeric, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        if(Expression::getOutputNumeric()._staticInit)
        {
            Cpu::reportError(Cpu::FncError, stderr, "Functions::LEFT$() : '%s:%d' : LEFT$() cannot be used in static initialisation : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
            numeric._isValid = false;
            return numeric;
        }
        if(numeric._params.size() != 1)
        {
            Cpu::reportError(Cpu::FncError, stderr, "Functions::LEFT$() : '%s:%d' : syntax error, 'LEFT$(s$, n)' requires two parameters : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
            numeric._isValid = false;
            return numeric;
        }

        // Literal string and parameter, (optimised case)
        if(numeric._varType == Expression::String  &&  numeric._params[0]._varType == Expression::Number)
        {
            int index;
            std::string name;
            handleConstantString(numeric, Compiler::StrLeft, name, index);
            return Expression::Numeric(0, uint16_t(index), true, false, false, Expression::StrVar, Expression::BooleanCC, Expression::Int16Both, name, std::string(""));
        }

        // Non optimised case
        std::string name;
        uint16_t srcAddr;
        int index = int(numeric._index);

        // String input can be literal, const, var and temp
        if(numeric._varType == Expression::TmpStrVar)
        {
            // Second parameter can never be a temp string
            srcAddr = Compiler::getStrWorkArea();
        }
        else
        {
            Compiler::getOrCreateString(numeric, name, srcAddr, index);
        }

        if(Expression::getEnableOptimisedPrint()  &&  Expression::getOutputNumeric()._nestedCount == 0)
        {
            Compiler::emitStringAddress(numeric, srcAddr);
            Compiler::emitVcpuAsm("STW", "textStr", false);
            handleStringParameter(numeric._params[0]);
            Compiler::emitVcpuAsm("STW", "textLen", false);
            Compiler::emitVcpuAsm("%PrintAcLeft", "", false);
        }
        else
        {
            // Create a new temporary string
            if(!isFuncNested()) Compiler::nextStrWorkArea();
            uint16_t dstAddr = Compiler::getStrWorkArea();

            // Optimise STW/LDW
            if(numeric._params[0]._varType == Expression::TmpVar)
            {
                handleStringParameter(numeric._params[0]);
                Compiler::emitVcpuAsm("STW", "strDstLen", false);
                Compiler::emitStringAddress(numeric, srcAddr);
                Compiler::emitVcpuAsm("STW", "strSrcAddr", false);
            }
            else
            {
                Compiler::emitStringAddress(numeric, srcAddr);
                Compiler::emitVcpuAsm("STW", "strSrcAddr", false);
                handleStringParameter(numeric._params[0]);
                Compiler::emitVcpuAsm("STW", "strDstLen", false);
            }
            Compiler::emitVcpuAsm("LDWI", Expression::wordToHexString(dstAddr), false);
            Compiler::emitVcpuAsm("%StringLeft", "", false);
        }

        return Expression::Numeric(0, uint16_t(-1), true, false, false, Expression::TmpStrVar, Expression::BooleanCC, Expression::Int16Both, name, std::string(""));
    }

    Expression::Numeric RIGHT$(Expression::Numeric& numeric, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        if(Expression::getOutputNumeric()._staticInit)
        {
            Cpu::reportError(Cpu::FncError, stderr, "Functions::RIGHT$() : '%s:%d' : RIGHT$() cannot be used in static initialisation : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
            numeric._isValid = false;
            return numeric;
        }
        if(numeric._params.size() != 1)
        {
            Cpu::reportError(Cpu::FncError, stderr, "Functions::RIGHT$() : '%s:%d' : syntax error, 'RIGHT$(s$, n)' requires two parameters : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
            numeric._isValid = false;
            return numeric;
        }

        // Literal string and parameter, (optimised case)
        if(numeric._varType == Expression::String  &&  numeric._params[0]._varType == Expression::Number)
        {
            int index;
            std::string name;
            handleConstantString(numeric, Compiler::StrRight, name, index);
            return Expression::Numeric(0, uint16_t(index), true, false, false, Expression::StrVar, Expression::BooleanCC, Expression::Int16Both, name, std::string(""));
        }

        // Non optimised case
        std::string name;
        uint16_t srcAddr;
        int index = int(numeric._index);

        // String input can be literal, const, var and temp
        if(numeric._varType == Expression::TmpStrVar)
        {
            srcAddr = Compiler::getStrWorkArea();
        }
        else
        {
            Compiler::getOrCreateString(numeric, name, srcAddr, index);
        }

        if(Expression::getEnableOptimisedPrint()  &&  Expression::getOutputNumeric()._nestedCount == 0)
        {
            Compiler::emitStringAddress(numeric, srcAddr);
            Compiler::emitVcpuAsm("STW", "textStr", false);
            handleStringParameter(numeric._params[0]);
            Compiler::emitVcpuAsm("STW", "textLen", false);
            Compiler::emitVcpuAsm("%PrintAcRight", "", false);
        }
        else
        {
            // Create a new temporary string
            if(!isFuncNested()) Compiler::nextStrWorkArea();
            uint16_t dstAddr = Compiler::getStrWorkArea();

            // Optimise STW/LDW
            if(numeric._params[0]._varType == Expression::TmpVar)
            {
                handleStringParameter(numeric._params[0]);
                Compiler::emitVcpuAsm("STW", "strDstLen", false);
                Compiler::emitStringAddress(numeric, srcAddr);
                Compiler::emitVcpuAsm("STW", "strSrcAddr", false);
            }
            else
            {
                Compiler::emitStringAddress(numeric, srcAddr);
                Compiler::emitVcpuAsm("STW", "strSrcAddr", false);
                handleStringParameter(numeric._params[0]);
                Compiler::emitVcpuAsm("STW", "strDstLen", false);
            }
            Compiler::emitVcpuAsm("LDWI", Expression::wordToHexString(dstAddr), false);
            Compiler::emitVcpuAsm("%StringRight", "", false);
        }

        return Expression::Numeric(0, uint16_t(-1), true, false, false, Expression::TmpStrVar, Expression::BooleanCC, Expression::Int16Both, name, std::string(""));
    }

    Expression::Numeric MID$(Expression::Numeric& numeric, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        if(Expression::getOutputNumeric()._staticInit)
        {
            Cpu::reportError(Cpu::FncError, stderr, "Functions::MID$() : '%s:%d' : MID$() cannot be used in static initialisation : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
            numeric._isValid = false;
            return numeric;
        }
        if(numeric._params.size() != 2)
        {
            Cpu::reportError(Cpu::FncError, stderr, "Functions::MID$() : '%s:%d' : syntax error, 'MID$(s$, i, n)' requires three parameters : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
            numeric._isValid = false;
            return numeric;
        }

        // Literal string and parameters, (optimised case)
        if(numeric._varType == Expression::String  &&  numeric._params[0]._varType == Expression::Number  &&  numeric._params[1]._varType == Expression::Number)
        {
            int index;
            std::string name;
            handleConstantString(numeric, Compiler::StrMid, name, index);
            return Expression::Numeric(0, uint16_t(index), true, false, false, Expression::StrVar, Expression::BooleanCC, Expression::Int16Both, name, std::string(""));
        }

        // Non optimised case
        std::string name;
        uint16_t srcAddr;
        int index = int(numeric._index);

        // String input can be literal, const, var and temp
        if(numeric._varType == Expression::TmpStrVar)
        {
            srcAddr = Compiler::getStrWorkArea();
        }
        else
        {
            Compiler::getOrCreateString(numeric, name, srcAddr, index);
        }

        if(Expression::getEnableOptimisedPrint()  &&  Expression::getOutputNumeric()._nestedCount == 0)
        {
            Compiler::emitStringAddress(numeric, srcAddr);
            Compiler::emitVcpuAsm("STW", "textStr", false);
            handleStringParameter(numeric._params[1]);
            Compiler::emitVcpuAsm("STW", "textLen", false);
            handleStringParameter(numeric._params[0]);
            Compiler::emitVcpuAsm("STW", "textOfs", false);
            Compiler::emitVcpuAsm("%PrintAcMid", "", false);
        }
        else
        {
            // Create a new temporary string
            if(!isFuncNested()) Compiler::nextStrWorkArea();
            uint16_t dstAddr = Compiler::getStrWorkArea();

            Compiler::emitStringAddress(numeric, srcAddr);
            Compiler::emitVcpuAsm("STW", "strSrcAddr", false);
            handleStringParameter(numeric._params[1]);
            Compiler::emitVcpuAsm("STW", "strDstLen", false);
            handleStringParameter(numeric._params[0]);
            Compiler::emitVcpuAsm("STW", "strOffset", false);
            Compiler::emitVcpuAsm("LDWI", Expression::wordToHexString(dstAddr), false);
            Compiler::emitVcpuAsm("%StringMid", "", false);
        }

        return Expression::Numeric(0, uint16_t(-1), true, false, false, Expression::TmpStrVar, Expression::BooleanCC, Expression::Int16Both, name, std::string(""));
    }

    Expression::Numeric LCASE$(Expression::Numeric& numeric, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        if(Expression::getOutputNumeric()._staticInit)
        {
            Cpu::reportError(Cpu::FncError, stderr, "Functions::LCASE$() : '%s:%d' : 'LCASE$()' cannot be used in static initialisation : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
            numeric._isValid = false;
            return numeric;
        }
        if(numeric._params.size() != 0)
        {
            Cpu::reportError(Cpu::FncError, stderr, "Functions::LCASE$() : '%s:%d' : syntax error, 'LCASE$()' requires one string parameter : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
            numeric._isValid = false;
            return numeric;
        }

        // Literal string and parameter, (optimised case)
        if(numeric._varType == Expression::String)
        {
            int index;
            std::string name;
            handleConstantString(numeric, Compiler::StrLower, name, index);
            return Expression::Numeric(0, uint16_t(index), true, false, false, Expression::StrVar, Expression::BooleanCC, Expression::Int16Both, name, std::string(""));
        }

        // Non optimised case
        std::string name;
        uint16_t srcAddr;
        int index = int(numeric._index);

        // String input can be literal, const, var and temp
        if(numeric._varType == Expression::TmpStrVar)
        {
            srcAddr = Compiler::getStrWorkArea();
        }
        else
        {
            Compiler::getOrCreateString(numeric, name, srcAddr, index);
        }

        if(Expression::getEnableOptimisedPrint()  &&  Expression::getOutputNumeric()._nestedCount == 0)
        {
            Compiler::emitStringAddress(numeric, srcAddr);
            Compiler::emitVcpuAsm("STW", "textStr", false);
            Compiler::emitVcpuAsm("%PrintAcLower", "", false);
        }
        else
        {
            // Create a new temporary string
            if(!isFuncNested()) Compiler::nextStrWorkArea();
            uint16_t dstAddr = Compiler::getStrWorkArea();

            Compiler::emitStringAddress(numeric, srcAddr);
            Compiler::emitVcpuAsm("STW", "strSrcAddr", false);
            Compiler::emitVcpuAsm("LDWI", Expression::wordToHexString(dstAddr), false);
            Compiler::emitVcpuAsm("%StringLower", "", false);
        }

        return Expression::Numeric(0, uint16_t(-1), true, false, false, Expression::TmpStrVar, Expression::BooleanCC, Expression::Int16Both, name, std::string(""));
    }

    Expression::Numeric UCASE$(Expression::Numeric& numeric, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        if(Expression::getOutputNumeric()._staticInit)
        {
            Cpu::reportError(Cpu::FncError, stderr, "Functions::UCASE$() : '%s:%d' : 'UCASE$()' cannot be used in static initialisation : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
            numeric._isValid = false;
            return numeric;
        }
        if(numeric._params.size() != 0)
        {
            Cpu::reportError(Cpu::FncError, stderr, "Functions::UCASE$() : '%s:%d' : syntax error, 'UCASE$()' requires one string parameter : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
            numeric._isValid = false;
            return numeric;
        }

        // Literal string and parameter, (optimised case)
        if(numeric._varType == Expression::String)
        {
            int index;
            std::string name;
            handleConstantString(numeric, Compiler::StrUpper, name, index);
            return Expression::Numeric(0, uint16_t(index), true, false, false, Expression::StrVar, Expression::BooleanCC, Expression::Int16Both, name, std::string(""));
        }

        // Non optimised case
        std::string name;
        uint16_t srcAddr;
        int index = int(numeric._index);

        // String input can be literal, const, var and temp
        if(numeric._varType == Expression::TmpStrVar)
        {
            srcAddr = Compiler::getStrWorkArea();
        }
        else
        {
            Compiler::getOrCreateString(numeric, name, srcAddr, index);
        }

        if(Expression::getEnableOptimisedPrint()  &&  Expression::getOutputNumeric()._nestedCount == 0)
        {
            Compiler::emitStringAddress(numeric, srcAddr);
            Compiler::emitVcpuAsm("STW", "textStr", false);
            Compiler::emitVcpuAsm("%PrintAcUpper", "", false);
        }
        else
        {
            // Create a new temporary string
            if(!isFuncNested()) Compiler::nextStrWorkArea();
            uint16_t dstAddr = Compiler::getStrWorkArea();

            Compiler::emitStringAddress(numeric, srcAddr);
            Compiler::emitVcpuAsm("STW", "strSrcAddr", false);
            Compiler::emitVcpuAsm("LDWI", Expression::wordToHexString(dstAddr), false);
            Compiler::emitVcpuAsm("%StringUpper", "", false);
        }

        return Expression::Numeric(0, uint16_t(-1), true, false, false, Expression::TmpStrVar, Expression::BooleanCC, Expression::Int16Both, name, std::string(""));
    }

    Expression::Numeric STRCAT$(Expression::Numeric& numeric, const std::string& moduleName, const std::string& codeLineText, int codeLineStart)
    {
        if(Expression::getOutputNumeric()._staticInit)
        {
            Cpu::reportError(Cpu::FncError, stderr, "Functions::STRCAT$() : '%s:%d' : STRCAT$() cannot be used in static initialisation : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
            numeric._isValid = false;
            return numeric;
        }
        if(Expression::getEnableOptimisedPrint())
        {
            Cpu::reportError(Cpu::FncError, stderr, "Functions::STRCAT$() : '%s:%d' : syntax error, STRCAT$() cannot be used in PRINT statements : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
            numeric._isValid = false;
            return numeric;
        }
        if(numeric._params.size() == 0)
        {
            Cpu::reportError(Cpu::FncError, stderr, "Functions::STRCAT$() : '%s:%d' : syntax error, STRCAT$() requires at least two string parameters : %s\n", moduleName.c_str(), codeLineStart, codeLineText.c_str());
            numeric._isValid = false;
            return numeric;
        }
        if(numeric._varType == Expression::TmpStrVar)
        {
            Cpu::reportError(Cpu::FncError, stderr, "Functions::STRCAT$() : '%s:%d' : syntax error, STRCAT$() requires string literals or string variables as ALL parameters : %s\n",
                                                    moduleName.c_str(), codeLineStart, codeLineText.c_str());
            numeric._isValid = false;
            return numeric;
        }
        for(int i=0; i<int(numeric._params.size()); i++)
        {
            if(numeric._params[i]._varType == Expression::TmpStrVar)
            {
                Cpu::reportError(Cpu::FncError, stderr, "Functions::STRCAT$() : '%s:%d' : syntax error, STRCAT$() requires string literals or string variables as ALL parameters : %s\n",
                                                        moduleName.c_str(), codeLineStart, codeLineText.c_str());
                numeric._isValid = false;
                return numeric;
            }
        }

        // Source string addresses, (extra 0x0000 delimiter used by VASM runtime)
        std::string name;
        int index = int(numeric._index);
        std::vector<uint16_t> strAddrs(numeric._params.size() + 2, 0x0000);
        Compiler::getOrCreateString(numeric, name, strAddrs[0], index);
        for(int i=0; i<int(numeric._params.size()); i++)
        {
            index = int(numeric._params[i]._index);
            Compiler::getOrCreateString(numeric._params[i], name, strAddrs[i + 1], index);
        }

        // Source string addresses LUT
        uint16_t lutAddress;
        if(!Memory::getFreePageRAM(Memory::FitDescending, int(strAddrs.size()*2), USER_CODE_START, Compiler::getStringsStart(), lutAddress, true))
        {
            Cpu::reportError(Cpu::FncError, stderr, "Functions::STRCAT$() : '%s:%d' : not enough RAM for string concatenation LUT of size %d : %s\n", moduleName.c_str(), codeLineStart,
                                                                                                                                                      int(strAddrs.size()), codeLineText.c_str());
            return false;
        }
        Compiler::getCodeLines()[Compiler::getCurrentCodeLineIndex()]._strConcatLut = {lutAddress, strAddrs};

        // Concatenate multiple source strings to string work area
        Compiler::emitVcpuAsm("LDWI", Expression::wordToHexString(lutAddress), false);
        Compiler::emitVcpuAsm("STW", "strLutAddr", false);
        Compiler::emitVcpuAsm("LDWI", Expression::wordToHexString(Compiler::getStrWorkArea()), false);
        Compiler::emitVcpuAsm("%StringConcatLut", "", false);

        return Expression::Numeric(0, uint16_t(-1), true, false, false, Expression::TmpStrVar, Expression::BooleanCC, Expression::Int16Both, name, std::string(""));
    }
}