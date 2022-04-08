#include <cmath>

#include "memory.h"
#include "cpu.h"
#include "loader.h"
#include "expression.h"
#include "assembler.h"
#include "compiler.h"
#include "pragmas.h"


namespace Pragmas
{
    std::map<std::string, Pragma> _pragmas;


    std::map<std::string, Pragma>& getPragmas(void) {return _pragmas;}


    bool initialise(void)
    {
        // Pragmas
        _pragmas["_codeRomType_"]        = {"_codeRomType_",        CODEROMTYPE       };
        _pragmas["_runtimePath_"]        = {"_runtimePath_",        RUNTIMEPATH       };
        _pragmas["_runtimeStart_"]       = {"_runtimeStart_",       RUNTIMESTART      };
        _pragmas["_userCodeStart_"]      = {"_userCodeStart_",      USERCODESTART     };
        _pragmas["_arraysStart_"]        = {"_arraysStart_",        ARRAYSSTART       };
        _pragmas["_stringsStart_"]       = {"_stringsStart_",       STRINGSSTART      };
        _pragmas["_stringWorkArea_"]     = {"_stringWorkArea_",     STRINGWORKAREA    };
        _pragmas["_stringMaxSize_"]      = {"_stringMaxSize_",      STRINGMAXSIZE     };
        _pragmas["_tempVarSize_"]        = {"_tempVarSize_",        TEMPVARSIZE       };
        _pragmas["_codeOptimiseType_"]   = {"_codeOptimiseType_",   CODEOPTIMISETYPE  };
        _pragmas["_arrayIndiciesOne_"]   = {"_arrayIndiciesOne_",   ARRAYINDICIESONE  };
        _pragmas["_maxNumBlits_"]        = {"_maxNumBlits_",        MAXNUMBLITS       };
        _pragmas["_blitStripeChunks_"]   = {"_blitStripeChunks_",   BLITSTRIPECHUNKS  };
        _pragmas["_maxNumPatterns_"]     = {"_maxNumPatterns_",     MAXNUMPATTERNS    };
        _pragmas["_enable6BitAudioEmu_"] = {"_enable6BitAudioEmu_", ENABLE6BITAUDIOEMU};
        _pragmas["_enable8BitAudioEmu_"] = {"_enable8BitAudioEmu_", ENABLE8BITAUDIOEMU};

        return true;
    }


    bool findPragma(std::string code, const std::string& pragma, size_t& foundPos)
    {
        foundPos = code.find(pragma);
        if(foundPos != std::string::npos)
        {
            foundPos += pragma.size();
            return true;
        }
        return false;
    }

    PragmaResult handlePragmas(std::string& input, int codeLineIndex)
    {
        std::vector<std::string> tokens = Expression::tokenise(input, ' ', false);
        if(tokens.size() >= 1)
        {
            std::string token = tokens[0];
            Expression::stripNonStringWhitespace(token);

            if(_pragmas.find(token) == _pragmas.end()) return PragmaNotFound;

            // Handle pragma in input
            size_t foundPos;
            if(findPragma(token, _pragmas[token]._name, foundPos)  &&  _pragmas[token]._func)
            {
                bool success = _pragmas[token]._func(input, codeLineIndex, foundPos);
                if(success) return PragmaFound;
                
                return PragmaError;
            }
        }

        return PragmaNotFound;
    }


    // ********************************************************************************************
    // Pragmas
    // ********************************************************************************************
    bool CODEROMTYPE(const std::string& input, int codeLineIndex, size_t foundPos)
    {
        // Get rom type
        std::string pragma = input.substr(foundPos);
        Expression::stripNonStringWhitespace(pragma);
        Expression::strToUpper(pragma);
        if(Cpu::getRomTypeMap().find(pragma) == Cpu::getRomTypeMap().end())
        {
            Cpu::reportError(Cpu::PrgError, stderr, "Pragmas::CODEROMTYPE() : 'Main:%d' : syntax error, use _codeRomType_ <ROM TYPE> : %s\n", codeLineIndex + 1, input.c_str());
            return false;
        }

        Compiler::setCodeRomType(Cpu::getRomTypeMap()[pragma]);

        return true;
    }

    bool RUNTIMEPATH(const std::string& input, int codeLineIndex, size_t foundPos)
    {
        std::string pragma = input.substr(foundPos);
        Expression::stripNonStringWhitespace(pragma);
        if(pragma.size() < 3  ||  !Expression::isStringValid(pragma))
        {
            Cpu::reportError(Cpu::PrgError, stderr, "Pragmas::RUNTIMEPATH() : 'Main:%d' : syntax error, use _runtimePath_ <\"Path to runtime\"> : %s\n", codeLineIndex + 1, input.c_str());
            return false;
        }

        // Strip quotes
        std::string runtimePath = pragma;
        runtimePath.erase(0, 1);
        runtimePath.erase(runtimePath.size() - 1, 1);

        // Set build path
        Compiler::setBuildPath(runtimePath, Loader::getFilePath());

        return true;
    }

    bool RUNTIMESTART(const std::string& input, int codeLineIndex, size_t foundPos)
    {
        std::string pragma = input.substr(foundPos);
        Expression::stripNonStringWhitespace(pragma);
        std::vector<std::string> tokens = Expression::tokenise(pragma, ',', false);
        if(tokens.size() != 1)
        {
            Cpu::reportError(Cpu::PrgError, stderr, "Pragmas::RUNTIMESTART() : 'Main:%d' : syntax error, use _runtimeStart_ <address> : %s\n", codeLineIndex + 1, input.c_str());
            return false;
        }

        Expression::Numeric addrNumeric;
        std::string addrOperand;
        if(Compiler::parseStaticExpression(codeLineIndex, tokens[0], addrOperand, addrNumeric) == Compiler::OperandInvalid)
        {
            Cpu::reportError(Cpu::PrgError, stderr, "Pragmas::RUNTIMESTART() : 'Main:%d' : syntax error in address field %s : %s\n", codeLineIndex + 1, tokens[0].c_str(), input.c_str());
            return false;
        }
        uint16_t address = uint16_t(std::lround(addrNumeric._value));
        if(address < DEFAULT_EXEC_ADDRESS)
        {
            Cpu::reportError(Cpu::PrgError, stderr, "Pragmas::RUNTIMESTART() : 'Main:%d' : address field must be above &h%04x, found %s : %s\n", codeLineIndex + 1, DEFAULT_EXEC_ADDRESS, tokens[0].c_str(), input.c_str());
            return false;
        }

        Compiler::setRuntimeStart(address);

        // Re-initialise memory manager for 64K
        if(address >= RAM_UPPER_START  &&  Memory::getSizeRAM() != RAM_SIZE_HI)
        {
            Cpu::setMemoryModel(RAM_SIZE_HI);
        }

        // String work area needs to be updated, (return old work areas and get new ones)
        for(int i=0; i<NUM_STR_WORK_AREAS; i++)
        {
            uint16_t strWorkArea;
            Memory::giveFreeRAM(Compiler::getStrWorkArea(i), USER_STR_SIZE + 2);
            if(!Memory::getFreePageRAM(Memory::FitDescending, USER_STR_SIZE + 2, USER_CODE_START, address, strWorkArea, true))
            {
                Cpu::reportError(Cpu::PrgError, stderr, "Pragmas::RUNTIMESTART() : 'Main:%d' : setting new String Work Area failed : %s\n", codeLineIndex + 1, input.c_str());
                return false;
            }
            Compiler::setStrWorkArea(strWorkArea, i);
        }

        return true;
    }

    bool USERCODESTART(const std::string& input, int codeLineIndex, size_t foundPos)
    {
        std::string pragma = input.substr(foundPos);
        Expression::stripNonStringWhitespace(pragma);
        std::vector<std::string> tokens = Expression::tokenise(pragma, ',', false);
        if(tokens.size() != 1)
        {
            Cpu::reportError(Cpu::PrgError, stderr, "Pragmas::USERCODESTART() : 'Main:%d' : syntax error, use _userCodeStart_ <address> : %s\n", codeLineIndex + 1, input.c_str());
            return false;
        }

        Expression::Numeric addrNumeric;
        std::string addrOperand;
        if(Compiler::parseStaticExpression(codeLineIndex, tokens[0], addrOperand, addrNumeric) == Compiler::OperandInvalid)
        {
            Cpu::reportError(Cpu::PrgError, stderr, "Pragmas::USERCODESTART() : 'Main:%d' : syntax error in address field %s : %s\n", codeLineIndex + 1, tokens[0].c_str(), input.c_str());
            return false;
        }
        uint16_t address = uint16_t(std::lround(addrNumeric._value));
        if(address < DEFAULT_EXEC_ADDRESS)
        {
            Cpu::reportError(Cpu::PrgError, stderr, "Pragmas::USERCODESTART() : 'Main:%d' : address field must be above &h%04x, found %s : %s\n", codeLineIndex + 1, DEFAULT_EXEC_ADDRESS, tokens[0].c_str(), input.c_str());
            return false;
        }

        // Programmer wants to use video memory as code space
        if(address >= 0x0800  &&  address < 0x7FA0  &&  (address & 0x00FF) < 0x00A0)
        {
            Memory::freeVideoRAM();
        }

        Compiler::setUserCodeStart(address);

        return true;
    }

    bool ARRAYSSTART(const std::string& input, int codeLineIndex, size_t foundPos)
    {
        std::string pragma = input.substr(foundPos);
        Expression::stripNonStringWhitespace(pragma);
        std::vector<std::string> tokens = Expression::tokenise(pragma, ',', false);
        if(tokens.size() != 1)
        {
            Cpu::reportError(Cpu::PrgError, stderr, "Pragmas::ARRAYSSTART() : 'Main:%d' : syntax error, use _arraysStart_ <address> : %s\n", codeLineIndex + 1, input.c_str());
            return false;
        }

        Expression::Numeric addrNumeric;
        std::string addrOperand;
        if(Compiler::parseStaticExpression(codeLineIndex, tokens[0], addrOperand, addrNumeric) == Compiler::OperandInvalid)
        {
            Cpu::reportError(Cpu::PrgError, stderr, "Pragmas::ARRAYSSTART() : 'Main:%d' : syntax error in address field %s : %s\n", codeLineIndex + 1, tokens[0].c_str(), input.c_str());
            return false;
        }
        uint16_t address = uint16_t(std::lround(addrNumeric._value));
        if(address < DEFAULT_EXEC_ADDRESS)
        {
            Cpu::reportError(Cpu::PrgError, stderr, "Pragmas::ARRAYSSTART() : 'Main:%d' : address field must be above &h%04x, found %s : %s\n", codeLineIndex + 1, DEFAULT_EXEC_ADDRESS, tokens[0].c_str(), input.c_str());
            return false;
        }

        Compiler::setArraysStart(address);

        // Re-initialise memory manager for 64K
        if(address >= RAM_UPPER_START  &&  Memory::getSizeRAM() != RAM_SIZE_HI)
        {
            Cpu::setMemoryModel(RAM_SIZE_HI);
        }

        return true;
    }

    bool STRINGSSTART(const std::string& input, int codeLineIndex, size_t foundPos)
    {
        std::string pragma = input.substr(foundPos);
        Expression::stripNonStringWhitespace(pragma);
        std::vector<std::string> tokens = Expression::tokenise(pragma, ',', false);
        if(tokens.size() != 1)
        {
            Cpu::reportError(Cpu::PrgError, stderr, "Pragmas::STRINGSSTART() : 'Main:%d' : syntax error, use _stringsStart_ <address> : %s\n", codeLineIndex + 1, input.c_str());
            return false;
        }

        Expression::Numeric addrNumeric;
        std::string addrOperand;
        if(Compiler::parseStaticExpression(codeLineIndex, tokens[0], addrOperand, addrNumeric) == Compiler::OperandInvalid)
        {
            Cpu::reportError(Cpu::PrgError, stderr, "Pragmas::STRINGSSTART() : 'Main:%d' : syntax error in address field %s : %s\n", codeLineIndex + 1, tokens[0].c_str(), input.c_str());
            return false;
        }
        uint16_t address = uint16_t(std::lround(addrNumeric._value));
        if(address < DEFAULT_EXEC_ADDRESS)
        {
            Cpu::reportError(Cpu::PrgError, stderr, "Pragmas::STRINGSSTART() : 'Main:%d' : address field must be above &h%04x, found %s : %s\n", codeLineIndex + 1, DEFAULT_EXEC_ADDRESS, tokens[0].c_str(), input.c_str());
            return false;
        }

        Compiler::setStringsStart(address);

        // Re-initialise memory manager for 64K
        if(address >= RAM_UPPER_START  &&  Memory::getSizeRAM() != RAM_SIZE_HI)
        {
            Cpu::setMemoryModel(RAM_SIZE_HI);
        }

        return true;
    }

    bool STRINGWORKAREA(const std::string& input, int codeLineIndex, size_t foundPos)
    {
        std::string pragma = input.substr(foundPos);
        Expression::stripNonStringWhitespace(pragma);
        std::vector<std::string> tokens = Expression::tokenise(pragma, ',', false);
        if(tokens.size() != 1)
        {
            Cpu::reportError(Cpu::PrgError, stderr, "Pragmas::STRINGWORKAREA() : 'Main:%d' : syntax error, use _stringWorkArea_ <address> : %s\n", codeLineIndex + 1, input.c_str());
            return false;
        }

        Expression::Numeric addrNumeric;
        std::string addrOperand;
        if(Compiler::parseStaticExpression(codeLineIndex, tokens[0], addrOperand, addrNumeric) == Compiler::OperandInvalid)
        {
            Cpu::reportError(Cpu::PrgError, stderr, "Pragmas::STRINGWORKAREA() : 'Main:%d' : syntax error in address field %s : %s\n", codeLineIndex + 1, tokens[0].c_str(), input.c_str());
            return false;
        }
        uint16_t strWorkArea = uint16_t(std::lround(addrNumeric._value));
        if(strWorkArea < DEFAULT_EXEC_ADDRESS)
        {
            Cpu::reportError(Cpu::PrgError, stderr, "Pragmas::STRINGWORKAREA() : 'Main:%d' : address field must be above &h%04x, found %s : %s\n", codeLineIndex + 1, DEFAULT_EXEC_ADDRESS, tokens[0].c_str(), input.c_str());
            return false;
        }

        // String work area needs to be updated, (return old work areas and get new ones)
        Memory::giveFreeRAM(Compiler::getStrWorkArea(0), USER_STR_SIZE + 2);
        if(!Memory::takeFreeRAM(strWorkArea, USER_STR_SIZE + 2, false))
        {
            Cpu::reportError(Cpu::PrgError, stderr, "Pragmas::STRINGWORKAREA() : 'Main:%d' : setting new string work area failed : %s\n", codeLineIndex + 1, input.c_str());
            return false;
        }
        Compiler::setStrWorkArea(strWorkArea, 0);
        Memory::giveFreeRAM(Compiler::getStrWorkArea(1), USER_STR_SIZE + 2);
        if(!Memory::getFreePageRAM(Memory::FitDescending, USER_STR_SIZE + 2, USER_CODE_START, strWorkArea, strWorkArea, true))
        {
            Cpu::reportError(Cpu::PrgError, stderr, "Pragmas::STRINGWORKAREA() : 'Main:%d' : setting new string work area failed : %s\n", codeLineIndex + 1, input.c_str());
            return false;
        }
        Compiler::setStrWorkArea(strWorkArea, 1);

        return true;
    }

    bool STRINGMAXSIZE(const std::string& input, int codeLineIndex, size_t foundPos)
    {
        std::string pragma = input.substr(foundPos);
        Expression::stripNonStringWhitespace(pragma);
        std::vector<std::string> tokens = Expression::tokenise(pragma, ',', false);
        if(tokens.size() != 1)
        {
            Cpu::reportError(Cpu::PrgError, stderr, "Pragmas::STRINGMAXSIZE() : 'Main:%d' : syntax error, use _stringMaxSize_ <size> : %s\n", codeLineIndex + 1, input.c_str());
            return false;
        }

        Expression::Numeric sizeNumeric;
        std::string sizeOperand;
        if(Compiler::parseStaticExpression(codeLineIndex, tokens[0], sizeOperand, sizeNumeric) == Compiler::OperandInvalid)
        {
            Cpu::reportError(Cpu::PrgError, stderr, "Pragmas::STRINGMAXSIZE() : 'Main:%d' : syntax error in size field %s : %s\n", codeLineIndex + 1, tokens[0].c_str(), input.c_str());
            return false;
        }
        uint8_t strMaxSize = uint8_t(std::lround(sizeNumeric._value));
        if(strMaxSize < 1)
        {
            Cpu::reportError(Cpu::PrgError, stderr, "Pragmas::STRINGMAXSIZE() : 'Main:%d' : size field must be greater than 1, found %s : %s\n", codeLineIndex + 1, tokens[0].c_str(), input.c_str());
            return false;
        }

        return true;
    }

    bool TEMPVARSIZE(const std::string& input, int codeLineIndex, size_t foundPos)
    {
        std::string pragma = input.substr(foundPos);
        Expression::stripNonStringWhitespace(pragma);
        std::vector<std::string> tokens = Expression::tokenise(pragma, ',', false);
        if(tokens.size() != 1)
        {
            Cpu::reportError(Cpu::PrgError, stderr, "Pragmas::TEMPVARSIZE() : 'Main:%d' : syntax error, use '_tempVarSize_ <size>' : %s\n", codeLineIndex + 1, input.c_str());
            return false;
        }

        Expression::Numeric sizeNumeric;
        std::string sizeOperand;
        if(Compiler::parseStaticExpression(codeLineIndex, tokens[0], sizeOperand, sizeNumeric) == Compiler::OperandInvalid)
        {
            Cpu::reportError(Cpu::PrgError, stderr, "Pragmas::TEMPVARSIZE() : 'Main:%d' : syntax error in size field %s : %s\n", codeLineIndex + 1, tokens[0].c_str(), input.c_str());
            return false;
        }
        uint8_t size = uint8_t(std::lround(sizeNumeric._value));
        if(size < 2  ||  size > 16)
        {
            Cpu::reportError(Cpu::PrgError, stderr, "Pragmas::TEMPVARSIZE() : 'Main:%d' : size field must be in the range 2 to 16, found %s : %s\n", codeLineIndex + 1, tokens[0].c_str(), input.c_str());
            return false;
        }

        Compiler::setTempVarSize(size);

        return true;
    }

    // TODO: not implemented yet
    bool CODEOPTIMISETYPE(const std::string& input, int codeLineIndex, size_t foundPos)
    {
        std::string pragma = input.substr(foundPos);
        Expression::stripNonStringWhitespace(pragma);
        std::vector<std::string> tokens = Expression::tokenise(pragma, ',', false);
        if(tokens.size() != 1)
        {
            Cpu::reportError(Cpu::PrgError, stderr, "Pragmas::CODEOPTIMISETYPE() : 'Main:%d' : syntax error, use _codeOptimiseType_ <size/speed> : %s\n", codeLineIndex + 1, input.c_str());
            return false;
        }

        if(tokens[0] == "SIZE")
        {
            Compiler::setCodeOptimiseType(Compiler::CodeSize);
            return true;
        }
        else if(tokens[0] == "SPEED")
        {
            Compiler::setCodeOptimiseType(Compiler::CodeSpeed);
            return true;
        }

        Cpu::reportError(Cpu::PrgError, stderr, "Pragmas::CODEOPTIMISETYPE() : 'Main:%d' : syntax error, _use codeOptimiseType_ <'size'/'speed'> : %s\n", codeLineIndex + 1, input.c_str());

        return false;
    }

    // TODO: not implemented yet
    bool ARRAYINDICIESONE(const std::string& input, int codeLineIndex, size_t foundPos)
    {
        UNREFERENCED_PARAM(input);
        UNREFERENCED_PARAM(foundPos);
        UNREFERENCED_PARAM(codeLineIndex);

        Compiler::setArrayIndiciesOne(true);

        return true;
    }

    bool MAXNUMBLITS(const std::string& input, int codeLineIndex, size_t foundPos)
    {
        std::string pragma = input.substr(foundPos);
        Expression::stripNonStringWhitespace(pragma);
        std::vector<std::string> tokens = Expression::tokenise(pragma, ',', false, true);
        if(tokens.size() != 1)
        {
            Cpu::reportError(Cpu::PrgError, stderr, "Pragmas::MAXNUMBLITS() : 'Main:%d' : syntax error, use _maxNumBlits_ <max num blits> : %s\n", codeLineIndex + 1, input.c_str());
            return false;
        }

        // Max num blits
        Expression::Numeric maxNumNumeric;
        std::string maxNumOperand;
        if(Compiler::parseStaticExpression(codeLineIndex, tokens[0], maxNumOperand, maxNumNumeric) == Compiler::OperandInvalid)
        {
            Cpu::reportError(Cpu::PrgError, stderr, "Pragmas::MAXNUMBLITS() : 'Main:%d' : syntax error in max num field %s : %s\n", codeLineIndex + 1, tokens[0].c_str(), input.c_str());
            return false;
        }
        uint16_t maxNumBlits = uint16_t(std::lround(maxNumNumeric._value));
        if(Compiler::getRuntimeStart() < RAM_UPPER_START  &&  maxNumBlits > MAX_NUM_BLITS_LO)
        {
            Cpu::reportError(Cpu::PrgError, stderr, "Pragmas::MAXNUMBLITS() : 'Main:%d' : maximum number of blits for 32K RAM is limited to %d, found %s : %s\n",
                                                    codeLineIndex + 1, MAX_NUM_BLITS_LO, tokens[0].c_str(), input.c_str());
            return false;
        }

        if(Compiler::getRuntimeStart() >= RAM_UPPER_START  &&  maxNumBlits > MAX_NUM_BLITS_HI)
        {
            Cpu::reportError(Cpu::PrgError, stderr, "Pragmas::MAXNUMBLITS() : 'Main:%d' : maximum number of blits for 64K RAM is limited to %d, found %s : %s\n",
                                                    codeLineIndex + 1, MAX_NUM_BLITS_HI, tokens[0].c_str(), input.c_str());
            return false;
        }

        // Allocate RAM for blit's LUT now, otherwise more than 48 blits in a 32K RAM system can fail
        uint16_t lutAddress;
        int lutSize = int(maxNumBlits) * 2;
        if(!Memory::getFreePageRAM(Memory::FitDescending, lutSize, USER_CODE_START, Compiler::getRuntimeStart(), lutAddress, true))
        {
            Cpu::reportError(Cpu::PrgError, stderr, "Pragmas::MAXNUMBLITS() : 'Main:%d' : not enough RAM for blits LUT of size %d : %s\n", codeLineIndex + 1, lutSize, input.c_str());
            return false;
        }
        Compiler::setBlitsAddrLutAddress(lutAddress);

        return true;
    }

    bool BLITSTRIPECHUNKS(const std::string& input, int codeLineIndex, size_t foundPos)
    {
        std::string pragma = input.substr(foundPos);
        Expression::stripNonStringWhitespace(pragma);
        std::vector<std::string> tokens = Expression::tokenise(pragma, ',', false, true);
        if(tokens.size() < 1  ||  tokens.size() > 4)
        {
            Cpu::reportError(Cpu::PrgError, stderr, "Pragmas::BLITSTRIPECHUNKS() : 'Main:%d' : syntax error, use _blitStripeChunks_ <num chunks>, <optional minimum address>, \
                                                     <optional maximum address>, <optional ascending/descending> : %s\n", codeLineIndex + 1, input.c_str());
            return false;
        }

        // Number of chunks
        Expression::Numeric chunksNumeric;
        std::string chunksOperand;
        if(Compiler::parseStaticExpression(codeLineIndex, tokens[0], chunksOperand, chunksNumeric) == Compiler::OperandInvalid)
        {
            Cpu::reportError(Cpu::PrgError, stderr, "Pragmas::BLITSTRIPECHUNKS() : 'Main:%d' : syntax error in num chunks field %s : %s\n", codeLineIndex + 1, tokens[0].c_str(), input.c_str());
            return false;
        }
        uint16_t blitStripeChunks = uint16_t(std::lround(chunksNumeric._value));
        if(blitStripeChunks > BLIT_STRIPE_CHUNKS_HI)
        {
            Cpu::reportError(Cpu::PrgError, stderr, "Pragmas::BLITSTRIPECHUNKS() : 'Main:%d' : num chunks field can not be larger than %d, found %s : %s\n",
                                                    codeLineIndex + 1, BLIT_STRIPE_CHUNKS_HI, tokens[0].c_str(), input.c_str());
            return false;
        }

        Compiler::setBlitStripeChunks(blitStripeChunks);

        // RAM minimum address
        if(tokens.size() >= 2)
        {
            Expression::Numeric addrNumeric;
            std::string addrOperand;
            if(Compiler::parseStaticExpression(codeLineIndex, tokens[1], addrOperand, addrNumeric) == Compiler::OperandInvalid)
            {
                Cpu::reportError(Cpu::PrgError, stderr, "Pragmas::BLITSTRIPECHUNKS() : 'Main:%d' : syntax error in min address field %s : %s\n", codeLineIndex + 1, tokens[1].c_str(), input.c_str());
                return false;
            }
            uint16_t address = uint16_t(std::lround(addrNumeric._value));
            if(address < DEFAULT_EXEC_ADDRESS  ||  address > Compiler::getRuntimeStart())
            {
                Cpu::reportError(Cpu::PrgError, stderr, "Pragmas::BLITSTRIPECHUNKS() : 'Main:%d' : min address field must be in the range &h%04x to &h%04x, found %s : %s\n",
                                                        codeLineIndex + 1, DEFAULT_EXEC_ADDRESS, Compiler::getRuntimeStart(), tokens[1].c_str(), input.c_str());
                return false;
            }

            Compiler::setBlitStripeMinAddress(address);
            if(tokens.size() == 2) return true;
        }

        // RAM maximum address
        if(tokens.size() >= 3)
        {
            Expression::Numeric addrNumeric;
            std::string addrOperand;
            if(Compiler::parseStaticExpression(codeLineIndex, tokens[2], addrOperand, addrNumeric) == Compiler::OperandInvalid)
            {
                Cpu::reportError(Cpu::PrgError, stderr, "Pragmas::BLITSTRIPECHUNKS() : 'Main:%d' : syntax error in max address field %s : %s\n", codeLineIndex + 1, tokens[2].c_str(), input.c_str());
                return false;
            }
            uint16_t address = uint16_t(std::lround(addrNumeric._value));
            if(address < DEFAULT_EXEC_ADDRESS  ||  address > Compiler::getRuntimeStart())
            {
                Cpu::reportError(Cpu::PrgError, stderr, "Pragmas::BLITSTRIPECHUNKS() : 'Main:%d' : max address field must be in the range &h%04x to &h%04x, found %s : %s\n",
                                                        codeLineIndex + 1, DEFAULT_EXEC_ADDRESS, Compiler::getRuntimeStart(), tokens[2].c_str(), input.c_str());
                return false;
            }

            Compiler::setBlitStripeMaxAddress(address);
            if(tokens.size() == 3) return true;
        }

        // RAM fit type
        if(tokens.size() == 4)
        {
            if(tokens[3] == "ASCENDING")
            {
                Compiler::setBlitStripeFitType(Memory::FitAscending);
                return true;
            }
            else if(tokens[3] == "DESCENDING")
            {
                Compiler::setBlitStripeFitType(Memory::FitDescending);
                return true;
            }
            else
            {
                Cpu::reportError(Cpu::PrgError, stderr, "Pragmas::BLITSTRIPECHUNKS() : 'Main:%d' : search direction field must be 'ascending or descending', found '%s' : %s\n",
                                                        codeLineIndex + 1, tokens[3].c_str(), input.c_str());
                return false;
            }
        }

        return true;
    }

    bool MAXNUMPATTERNS(const std::string& input, int codeLineIndex, size_t foundPos)
    {
        UNREFERENCED_PARAM(foundPos);
        UNREFERENCED_PARAM(codeLineIndex);
        UNREFERENCED_PARAM(input);

#if 0
        std::string pragma = input.substr(foundPos);
        Expression::stripNonStringWhitespace(pragma);
        std::vector<std::string> tokens = Expression::tokenise(pragma, ',', false, true);
        if(tokens.size() != 1)
        {
            Cpu::reportError(Cpu::PrgError, stderr, "Pragmas::MAXNUMPATTERNS() : 'Main:%d' : syntax error, use _maxNumPatterns_ <max num patterns> : %s\n", codeLineIndex + 1, input.c_str());
            return false;
        }

        // Max num patterns
        Expression::Numeric maxNumNumeric;
        std::string maxNumOperand;
        if(Compiler::parseStaticExpression(codeLineIndex, tokens[0], maxNumOperand, maxNumNumeric) == Compiler::OperandInvalid)
        {
            Cpu::reportError(Cpu::PrgError, stderr, "Pragmas::MAXNUMPATTERNS() : 'Main:%d' : syntax error in max num field %s : %s\n", codeLineIndex + 1, tokens[0].c_str(), input.c_str());
            return false;
        }
        uint16_t maxNumPatterns = uint16_t(std::lround(maxNumNumeric._value));
        if(Compiler::getRuntimeStart() < RAM_UPPER_START  &&  maxNumPatterns > MAX_NUM_BLITS_LO)
        {
            Cpu::reportError(Cpu::PrgError, stderr, "Pragmas::MAXNUMBLITS() : 'Main:%d' : maximum number of blits for 32K RAM is limited to %d, found %s : %s\n",
                                                    codeLineIndex + 1, MAX_NUM_BLITS_LO, tokens[0].c_str(), input.c_str());
            return false;
        }

        if(Compiler::getRuntimeStart() >= RAM_UPPER_START  &&  maxNumBlits > MAX_NUM_BLITS_HI)
        {
            Cpu::reportError(Cpu::PrgError, stderr, "Pragmas::MAXNUMBLITS() : 'Main:%d' : maximum number of blits for 64K RAM is limited to %d, found %s : %s\n",
                                                    codeLineIndex + 1, MAX_NUM_BLITS_HI, tokens[0].c_str(), input.c_str());
            return false;
        }

        // Allocate RAM for blit's LUT now, otherwise more than 48 blits in a 32K RAM system can fail
        uint16_t lutAddress;
        int lutSize = int(maxNumBlits) * 2;
        if(!Memory::getFreePageRAM(Memory::FitDescending, lutSize, USER_CODE_START, Compiler::getRuntimeStart(), lutAddress, true))
        {
            Cpu::reportError(Cpu::PrgError, stderr, "Pragmas::MAXNUMBLITS() : 'Main:%d' : not enough RAM for blits LUT of size %d : %s\n", codeLineIndex + 1, lutSize, input.c_str());
            return false;
        }
        Compiler::setBlitsAddrLutAddress(lutAddress);
#endif
        return true;
    }

    bool ENABLE6BITAUDIOEMU(const std::string& input, int codeLineIndex, size_t foundPos)
    {
#ifdef STAND_ALONE
        UNREFERENCED_PARAM(foundPos);
#endif

        if(Compiler::getCodeRomType() < Cpu::ROMv5a)
        {
            Cpu::reportError(Cpu::PrgError, stderr, "Pragmas::ENABLE6BITAUDIOEMU() : 'Main:%d' : version error, '_enable6BitAudioEmu_ <ON/OFF>' only works with ROMv5a or greater; \
                                                    use '_codeRomType_ ROMv5a' : %s\n", codeLineIndex + 1, input.c_str());
            return false;
        }

#ifndef STAND_ALONE
        std::string pragma = input.substr(foundPos);
        Expression::stripNonStringWhitespace(pragma);
        std::vector<std::string> tokens = Expression::tokenise(pragma, ',', false, true);
        if(tokens.size() != 1)
        {
            Cpu::reportError(Cpu::PrgError, stderr, "Pragmas::ENABLE6BITAUDIOEMU() : 'Main:%d' : syntax error, use '_enable6BitAudioEmu_ <ON/OFF>' : %s\n", codeLineIndex + 1, input.c_str());
            return false;
        }

        // ON/OFF
        bool enable = false;
        Expression::strToUpper(tokens[0]);
        if(tokens[0] == "ON")       enable = true;
        else if(tokens[0] == "OFF") enable = false;
        else
        {
            Cpu::reportError(Cpu::PrgError, stderr, "Pragmas::ENABLE6BITAUDIOEMU() : 'Main:%d' : syntax error, use '_enable6BitAudioEmu_ <ON/OFF>' : %s\n", codeLineIndex + 1, input.c_str());
            return false;
        }

        Cpu::enableAudioMode(Cpu::ROMv5a, Cpu::Audio6bit);

        return true;
#else
        Cpu::reportError(Cpu::PrgError, stderr, "Pragmas::ENABLE6BITAUDIOEMU() : 'Main:%d' : syntax error, '_enable6BitAudioEmu_ <ON/OFF>', only works in emulation mode : %s\n", codeLineIndex + 1, input.c_str());
        
        return false;
#endif
    }

    bool ENABLE8BITAUDIOEMU(const std::string& input, int codeLineIndex, size_t foundPos)
    {
#ifdef STAND_ALONE
        UNREFERENCED_PARAM(foundPos);
#endif

        if(Compiler::getCodeRomType() < Cpu::ROMv5a)
        {
            Cpu::reportError(Cpu::PrgError, stderr, "Pragmas::ENABLE8BITAUDIOEMU() : 'Main:%d' : version error, '_enable8BitAudioEmu_ <ON/OFF>' only works with ROMv5a or greater; \
                                                    use '_codeRomType_ ROMv5a' : %s\n", codeLineIndex + 1, input.c_str());
            return false;
        }

#ifndef STAND_ALONE
        std::string pragma = input.substr(foundPos);
        Expression::stripNonStringWhitespace(pragma);
        std::vector<std::string> tokens = Expression::tokenise(pragma, ',', false, true);
        if(tokens.size() != 1)
        {
            Cpu::reportError(Cpu::PrgError, stderr, "Pragmas::ENABLE8BITAUDIOEMU() : 'Main:%d' : syntax error, use '_enable8BitAudioEmu_ <ON/OFF>' : %s\n", codeLineIndex + 1, input.c_str());
            return false;
        }

        // ON/OFF
        bool enable = false;
        Expression::strToUpper(tokens[0]);
        if(tokens[0] == "ON")       enable = true;
        else if(tokens[0] == "OFF") enable = false;
        else
        {
            Cpu::reportError(Cpu::PrgError, stderr, "Pragmas::ENABLE8BITAUDIOEMU() : 'Main:%d' : syntax error, use '_enable8BitAudioEmu_ <ON/OFF>' : %s\n", codeLineIndex + 1, input.c_str());
            return false;
        }

        Cpu::enableAudioMode(Cpu::ROMv5a, Cpu::Audio8bit);

        return true;
#else
        Cpu::reportError(Cpu::PrgError, stderr, "Pragmas::ENABLE8BITAUDIOEMU() : 'Main:%d' : syntax error, '_enable8BitAudioEmu_ <ON/OFF>', only works in emulation mode : %s\n", codeLineIndex + 1, input.c_str());

        return false;
#endif
    }
}