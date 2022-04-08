#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <fstream>
#include <algorithm>

#include "memory.h"
#include "cpu.h"
#include "loader.h"
#include "image.h"
#include "assembler.h"
#include "load.h"
#include "linker.h"
#include "midi.h"


namespace Load
{
    void loadUsage(int msgType, Compiler::CodeLine& codeLine, int codeLineStart)
    {
        switch(msgType)
        {
            case LoadType:    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::LOAD() : '%s:%d' : syntax error, use 'LOAD <TYPE>, <filename>, where <TYPE> = 'IMAGE', 'BLIT', 'FONT', 'SPRITE', 'MIDI', 'WAVE', 'RAW' : %s\n",
                                                                      codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str()); break;
            case LoadRaw:     Cpu::reportError(Cpu::KwdError, stderr, "Keywords::LOAD() : '%s:%d' : syntax error, use 'LOAD RAW, <filename>, <optional const output address>/<optional input address>, <optional const output size>' : %s\n",
                                                                      codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str()); break;
            case LoadWave:    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::LOAD() : '%s:%d' : syntax error, use 'LOAD WAVE, <filename>, <optional address>, <optional address offset>' : %s\n",
                                                                      codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str()); break;
            case LoadMidi:    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::LOAD() : '%s:%d' : syntax error, use 'LOAD MIDI, <filename>, <id>, <optional loop count 1<->255, 0=forever>' : %s\n",
                                                                      codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str()); break;
            case LoadImage:   Cpu::reportError(Cpu::KwdError, stderr, "Keywords::LOAD() : '%s:%d' : syntax error, use 'LOAD IMAGE, <filename>, <optional address>' : %s\n",
                                                                      codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str()); break;
            case LoadBlit:    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::LOAD() : '%s:%d' : syntax error, use 'LOAD BLIT, <filename>, <id>, <optional flip>, <optional overlap>' : %s\n",
                                                                      codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str()); break;
            case LoadFont:    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::LOAD() : '%s:%d' : syntax error, use 'LOAD FONT, <filename>, <id>, <optional 16 bit bg:fg colours>' : %s\n",
                                                                      codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str()); break;
            case LoadSprite:  Cpu::reportError(Cpu::KwdError, stderr, "Keywords::LOAD() : '%s:%d' : syntax error, use 'LOAD SPRITE, <filename>, <id>' : %s\n",
                                                                      codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str()); break;
            case LoadPattern: Cpu::reportError(Cpu::KwdError, stderr, "Keywords::LOAD() : '%s:%d' : syntax error, use 'LOAD PATTERN, <filename>, <id>' : %s\n",
                                                                      codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str()); break;
            default: break;
        }
    }

    bool loadRaw(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, const std::vector<std::string>& tokens)
    {
        if(tokens.size() < 2  ||  tokens.size() > 4)
        {
            loadUsage(LoadRaw, codeLine, codeLineStart);
            return false;
        }

        std::string filename = tokens[1];
        Expression::stripWhitespace(filename);
        std::string ext = filename;
        Expression::strToUpper(ext);
        if(ext.find(".RAW") == std::string::npos  &&  ext.find(".BIN") == std::string::npos)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::loadRaw() : '%s:%d' : file extension error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, filename.c_str(), codeLine._text.c_str());
            return false;
        }

        std::string filepath = Loader::getFilePath();
        size_t slash = filepath.find_last_of("\\/");
        filepath = (slash != std::string::npos) ? filepath.substr(0, slash) : ".";
        filename = filepath + "/" + filename;

        // Parse optional consts or address
        int constAddrIndex = -1;
        int constSizeIndex = -1;
        uint16_t address = 0x0000;
        if(tokens.size() >= 3)
        {
            // Optional constant size
            if(tokens.size() == 4)
            {
                std::string constSizeToken = tokens[3];
                Expression::stripWhitespace(constSizeToken);
                if(Expression::isVarNameValid(constSizeToken) == Expression::Variable)
                {
                    constSizeIndex = Compiler::findConst(constSizeToken);
                    if(constSizeIndex == -1)
                    {
                        constSizeIndex = int(Compiler::getConstants().size());
                        Compiler::getConstants().push_back({2, 0, 0x0000, "", constSizeToken, "_" + constSizeToken, Compiler::ConstInt16});
                    }
                }
            }

            // Optional constant address
            std::string constAddrToken = tokens[2];
            Expression::stripWhitespace(constAddrToken);
            if(Expression::isVarNameValid(constAddrToken) == Expression::Variable)
            {
                constAddrIndex = Compiler::findConst(constAddrToken);
                if(constAddrIndex == -1)
                {
                    constAddrIndex = int(Compiler::getConstants().size());
                    Compiler::getConstants().push_back({2, 0, 0x0000, "", constAddrToken, "_" + constAddrToken, Compiler::ConstInt16});
                }
            }
            // Address
            else
            {
                std::string addrToken = tokens[2];
                Expression::Numeric addrNumeric;
                std::string addrOperand;
                if(Compiler::parseStaticExpression(codeLineIndex, addrToken, addrOperand, addrNumeric) == Compiler::OperandInvalid)
                {
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::loadRaw() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, addrToken.c_str(), codeLine._text.c_str());
                    return false;
                }
                address = uint16_t(std::lround(addrNumeric._value));
                if(address < DEFAULT_EXEC_ADDRESS)
                {
                    loadUsage(LoadRaw, codeLine, codeLineStart);
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::loadRaw() : '%s:%d' : address field must be above &h%04x, found %s : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                                                    DEFAULT_EXEC_ADDRESS, addrToken.c_str(), codeLine._text.c_str());
                    return false;
                }
            }
        }

        // Open RAW file
        std::ifstream infile(filename, std::ios::binary | std::ios::in);
        if(!infile.is_open())
        {
            loadUsage(LoadRaw, codeLine, codeLineStart);
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::loadRaw() : '%s:%d' : failed to open file '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, filename.c_str(), codeLine._text.c_str());
            return false;
        }

        // Size of RAW file
        infile.seekg(0, infile.end);
        int fileSize = int(infile.tellg());
        infile.seekg(0, infile.beg);
        if(fileSize >= RAW_MAX_FILE_SIZE)
        {
            loadUsage(LoadRaw, codeLine, codeLineStart);
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::loadRaw() : '%s:%d' : file '%s' is too big : %s\n", codeLine._moduleName.c_str(), codeLineStart, filename.c_str(), codeLine._text.c_str());
            return false;
        }
        if(fileSize == 0)
        {
            loadUsage(LoadRaw, codeLine, codeLineStart);
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::loadRaw() : '%s:%d' : file '%s' is zero length : %s\n", codeLine._moduleName.c_str(), codeLineStart, filename.c_str(), codeLine._text.c_str());
            return false;
        }

        // Read RAW file
        std::vector<uint8_t> dataBytes(fileSize);
        infile.read((char *)&dataBytes[0], fileSize);
        if(infile.eof()  ||  infile.bad() || infile.fail())
        {
            loadUsage(LoadRaw, codeLine, codeLineStart);
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::loadRaw() : '%s:%d' : failed to read file '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, filename.c_str(), codeLine._text.c_str());
            return false;
        }

        std::vector<uint8_t> data;
        // TODO: add command option to control this
        // Convert 8bit unsigned RAW data to 6bit unsigned RAW data
        //for(int i=0; i<int(dataBytes.size()); i++) dataBytes[i] >>= 4;
        //data = dataBytes;

        // TODO: add command option to control this
        // Convert 8bit unsigned RAW data to 4bit unsigned RAW data and halve the data size
        for(int i=0; i<fileSize; i+=2)
        {
            uint8_t dataByte2 = (i + 1 >= fileSize) ? 0 : dataBytes[i + 1];
            data.push_back(((dataBytes[i] >>4) <<4) | (dataByte2 >>4));
        }

        // Allocate gigatron memory
        if(address)
        {
            if(!Memory::takeFreeRAM(address, int(data.size()), false))
            {
                loadUsage(LoadRaw, codeLine, codeLineStart);
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::loadRaw() : '%s:%d' : allocating RAM at '0x%04x' of size '%d' failed : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                                                 address, int(data.size()), codeLine._text.c_str());
                return false;
            }
        }
        else
        {
            if(!Memory::getFreePageRAM(Memory::FitDescending, int(data.size()), USER_CODE_START, Compiler::getRuntimeStart(), address, false))
            {
                loadUsage(LoadRaw, codeLine, codeLineStart);
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::loadRaw() : '%s:%d' : allocating RAM of size '%d' failed : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                                     int(data.size()), codeLine._text.c_str());
                return false;
            }
        }

        // Optional const addr exists so update it with address
        if(constAddrIndex >= 0) Compiler::getConstants()[constAddrIndex]._data = address;

        // Optional const size exists so update it with size
        if(constSizeIndex >= 0) Compiler::getConstants()[constSizeIndex]._data = int16_t(data.size());

        Compiler::getDefDataBytes().push_back({address, 0, data});

        return true;
    }

    bool loadWave(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, const std::vector<std::string>& tokens)
    {
        if(tokens.size() < 2  ||  tokens.size() > 4)
        {
            loadUsage(LoadWave, codeLine, codeLineStart);
            return false;
        }

        std::string filename = tokens[1];
        Expression::stripWhitespace(filename);
        std::string ext = filename;
        Expression::strToUpper(ext);
        if(ext.find(".GTWAV") == std::string::npos)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::loadWave() : '%s:%d' : file extension error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, filename.c_str(), codeLine._text.c_str());
            return false;
        }

        std::string filepath = Loader::getFilePath();
        size_t slash = filepath.find_last_of("\\/");
        filepath = (slash != std::string::npos) ? filepath.substr(0, slash) : ".";
        filename = filepath + "/" + filename;

        // Parse optional address
        uint16_t address = RAM_AUDIO_START;
        if(tokens.size() >= 3)
        {
            std::string addrToken = tokens[2];
            Expression::Numeric addrNumeric;
            std::string addrOperand;
            if(Compiler::parseStaticExpression(codeLineIndex, addrToken, addrOperand, addrNumeric) == Compiler::OperandInvalid)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::loadWave() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, addrToken.c_str(), codeLine._text.c_str());
                return false;
            }
            address = uint16_t(std::lround(addrNumeric._value));
            if(address < DEFAULT_EXEC_ADDRESS)
            {
                loadUsage(LoadWave, codeLine, codeLineStart);
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::loadWave() : '%s:%d' : address field must be above &h%04x, found %s : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                                                DEFAULT_EXEC_ADDRESS, addrToken.c_str(), codeLine._text.c_str());
                return false;
            }
        }

        // Parse optional address offset
        uint16_t addrOffset = 0;
        if(tokens.size() == 4)
        {
            std::string offsetToken = tokens[3];
            Expression::Numeric offsetNumeric;
            std::string offsetOperand;
            if(Compiler::parseStaticExpression(codeLineIndex, offsetToken, offsetOperand, offsetNumeric) == Compiler::OperandInvalid)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::loadWave() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, offsetToken.c_str(), codeLine._text.c_str());
                return false;
            }
            addrOffset = uint16_t(std::lround(offsetNumeric._value));
        }

        // Load wave file
        std::vector<uint8_t> dataBytes(64);
        std::ifstream infile(filename, std::ios::binary | std::ios::in);
        if(!infile.is_open())
        {
            loadUsage(LoadWave, codeLine, codeLineStart);
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::loadWave() : '%s:%d' : failed to open file '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, filename.c_str(), codeLine._text.c_str());
            return false;
        }
        infile.read((char *)&dataBytes[0], 64);
        if(infile.eof() || infile.bad() || infile.fail())
        {
            loadUsage(LoadWave, codeLine, codeLineStart);
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::loadWave() : '%s:%d' : failed to read file '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, filename.c_str(), codeLine._text.c_str());
            return false;
        }

        uint16_t addr = address;
        for(int i=0; i<int(dataBytes.size()); i++)
        {
            if(addrOffset != 0)
            {
                if(addr < RAM_AUDIO_START  ||  addr > RAM_AUDIO_END)
                {
                    if(!Memory::takeFreeRAM(addr, 1, false))
                    {
                        loadUsage(LoadWave, codeLine, codeLineStart);
                        Cpu::reportError(Cpu::KwdError, stderr, "Keywords::loadWave() : '%s:%d' : allocating RAM at '0x%04x' of size '%d' failed : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                                                          addr, 1, codeLine._text.c_str());
                        return false;
                    }
                    addr += addrOffset;
                }
            }
        }
        if(addrOffset == 0)
        {
            if(!Memory::takeFreeRAM(address, int(dataBytes.size()), false))
            {
                loadUsage(LoadWave, codeLine, codeLineStart);
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::loadWave() : '%s:%d' : allocating RAM at '0x%04x' of size '%d' failed : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                                                  address, int(dataBytes.size()), codeLine._text.c_str());
                return false;
            }
        }
        Compiler::getDefDataBytes().push_back({address, addrOffset, dataBytes});

        return true;
    }

    bool loadMidi(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, const std::vector<std::string>& tokens)
    {
        if(tokens.size() < 3  ||  tokens.size() > 4)
        {
            loadUsage(LoadMidi, codeLine, codeLineStart);
            return false;
        }

        std::string filename = tokens[1];
        Expression::stripWhitespace(filename);
        std::string ext = filename;
        Expression::strToUpper(ext);
        if(ext.find(".GTMID") == std::string::npos)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::loadMidi() : '%s:%d' : file extension error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, filename.c_str(), codeLine._text.c_str());
            return false;
        }

        std::string filepath = Loader::getFilePath();
        size_t slash = filepath.find_last_of("\\/");
        filepath = (slash != std::string::npos) ? filepath.substr(0, slash) : ".";
        filename = filepath + "/" + filename;

        // Unique midi ID
        std::string idToken = tokens[2];
        Expression::Numeric idNumeric;
        std::string idOperand;
        if(Compiler::parseStaticExpression(codeLineIndex, idToken, idOperand, idNumeric) == Compiler::OperandInvalid)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::loadMidi() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, idToken.c_str(), codeLine._text.c_str());
            return false;
        }
        int midiId = int(std::lround(idNumeric._value));
        if(Compiler::getDefDataMidis().find(midiId) != Compiler::getDefDataMidis().end())
        {
            loadUsage(LoadMidi, codeLine, codeLineStart);
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::loadMidi() : '%s:%d' : MIDI id %d not unique : %s\n", codeLine._moduleName.c_str(), codeLineStart, midiId, codeLine._text.c_str());
            return false;
        }

        // Parse optional loop count
        uint16_t loops = 0;
        if(tokens.size() == 4)
        {
            std::string loopsToken = tokens[2];
            Expression::Numeric loopsNumeric;
            std::string loopsOperand;
            if(Compiler::parseStaticExpression(codeLineIndex, loopsToken, loopsOperand, loopsNumeric) == Compiler::OperandInvalid)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::loadMidi() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, loopsToken.c_str(), codeLine._text.c_str());
                return false;
            }
            loops = uint16_t(std::lround(loopsNumeric._value));
            if(loops > 255)
            {
                loadUsage(LoadMidi, codeLine, codeLineStart);
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::loadMidi() : '%s:%d' : loops field must be between 0 and 255, found %s : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                                                    loopsToken.c_str(), codeLine._text.c_str());
                return false;
            }
        }

        // Load gtMID file
        int midiSize = 0;
        GtMidiHdr gtMidiHdr;
        std::vector<uint8_t> midiBuffer(MIDI_MAX_BUFFER_SIZE);
        if(!Midi::loadFile(filename, midiBuffer.data(), midiSize, &gtMidiHdr))
        {
            loadUsage(LoadMidi, codeLine, codeLineStart);
            return false;
        }
        midiBuffer.resize(midiSize);

        // Allocate memory for midi segments
        int byteIndex = 0;
        int segmentCount = 0;
        std::vector<uint16_t> segSizes;
        std::vector<uint16_t> segAddrs;
        bool hasVolume = bool(gtMidiHdr._hasVolume);
        while(midiSize)
        {
            uint16_t size = 0;
            uint16_t address = 0;
            if(!Memory::getFreeChunkRAM(Memory::FitDescending, USER_CODE_START, Compiler::getRuntimeStart(), MIDI_MIN_SEGMENT_SIZE, address, uint16_t(midiSize + MIDI_CMD_JMP_SEG_SIZE), size, false))
            {
                loadUsage(LoadMidi, codeLine, codeLineStart);
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::loadMidi() : '%s:%d' : getting MIDI memory for segment %d failed : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                                                segmentCount, codeLine._text.c_str());
                return false;
            }

            byteIndex += size;

            // Pad for jump segment command and adjust for start notes, start note can be either 3 bytes, (0x90, nn, vv) or 2 bytes, (0x90, nn), depending on hasVolume
            if(hasVolume  &&  (midiBuffer[byteIndex - 5] & 0xF0) == MIDI_CMD_START_NOTE)
            {
                // Landed on volume, (vv)
                byteIndex -= 2; size -= 2; 
                Memory::giveFreeRAM(address + size, 2);
            }
            else if((midiBuffer[byteIndex - 4] & 0xF0) == MIDI_CMD_START_NOTE)
            {
                // Landed on note, (nn)
                byteIndex -= 1; size -= 1;
                Memory::giveFreeRAM(address + size, 1);
            }

            // Jump segment, (0xD0, lo, hi)
            byteIndex -= 3; size -= 3;

            midiSize -= size;
            segSizes.push_back(size);
            segAddrs.push_back(address);
            segmentCount++;
        }

        Compiler::getDefDataMidis()[midiId] = {midiId, hasVolume, uint8_t(loops), midiBuffer, segSizes, segAddrs};

        return true;
    }

    bool loadImageChunk(Compiler::CodeLine& codeLine, int codeLineStart, const std::vector<uint8_t>& data, int row, uint16_t width, uint16_t address, uint8_t chunkSize, uint16_t& chunkOffset, uint16_t& chunkAddr)
    {
        if(!Memory::getFreePageRAM(Memory::FitDescending, chunkSize, USER_CODE_START, Compiler::getRuntimeStart(), chunkAddr, true))
        {
            loadUsage(LoadImage, codeLine, codeLineStart);
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::loadImageChunk() : '%s:%d' : allocating RAM for offscreen pixel chunk on row %d failed : %s\n",
                                                    codeLine._moduleName.c_str(), codeLineStart, row, codeLine._text.c_str());
            return false;
        }

        std::vector<uint8_t> chunkData;
        for(int j=0; j<chunkSize; j++) chunkData.push_back(data[row*width + chunkOffset + j]);

        // Output loader image chunks copy code
        if(Compiler::getDefDataLoaderImageChunks().size() == 0)
        {
            if(Compiler::getCodeRomType() < Cpu::ROMv5a)
            {
                Compiler::emitVcpuAsm("LDWI", "copyLoaderImages", false);
                Compiler::emitVcpuAsm("CALL", "giga_vAC", false);
            }
            else
            {
                Compiler::emitVcpuAsm("CALLI", "copyLoaderImages", false);
            }
        }

        Compiler::DefDataLoaderImageChunk defDataLoaderImageChunk = {{chunkAddr, uint16_t(address + chunkOffset), chunkSize}, chunkData};
        Compiler::getDefDataLoaderImageChunks().push_back(defDataLoaderImageChunk);

        chunkOffset += chunkSize;

        return true;
    }

    bool loadImage(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, const std::vector<std::string>& tokens, const std::string& filename, const Image::TgaFile& tgaFile, Image::GtRgbFile& gtRgbFile)
    {
        const uint16_t stride = 256;

        if(tokens.size() < 2  ||  tokens.size() > 3)
        {
            loadUsage(LoadImage, codeLine, codeLineStart);
            return false;
        }

        switch(tgaFile._header._bitsPerPixel)
        {
            case 24: Image::convertRGB8toRGB2(tgaFile._data, gtRgbFile._data, tgaFile._header._width, tgaFile._header._height, tgaFile._imageOrigin);   break;
            case 32: Image::convertARGB8toARGB2(tgaFile._data, gtRgbFile._data, tgaFile._header._width, tgaFile._header._height, tgaFile._imageOrigin); break;

            default:
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::loadImage() : '%s:%d' : wrong bit depth, 'LOAD IMAGE' requires 24 or 32 bit image data : %s\n",
                                                        codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
                return false;
            }
            break;
        }

        // Parse optional address
        uint16_t address = RAM_VIDEO_START;
        if(tokens.size() == 3)
        {
            std::string addrToken = tokens[2];
            Expression::Numeric addrNumeric;
            std::string addrOperand;
            if(Compiler::parseStaticExpression(codeLineIndex, addrToken, addrOperand, addrNumeric) == Compiler::OperandInvalid)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::loadImage() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, addrToken.c_str(), codeLine._text.c_str());
                return false;
            }
            address = uint16_t(std::lround(addrNumeric._value));
            if(address < DEFAULT_EXEC_ADDRESS)
            {
                loadUsage(LoadImage, codeLine, codeLineStart);
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::loadImage() : '%s:%d' : address field must be above &h%04x, found %s : %s\n", codeLine._moduleName.c_str(), codeLineStart, DEFAULT_EXEC_ADDRESS,
                                                                                                                                                 addrToken.c_str(), codeLine._text.c_str());
                return false;
            }
        }

        if(gtRgbFile._header._width > stride  ||  gtRgbFile._header._width + (address & 0x00FF) > stride)
        {
            loadUsage(LoadImage, codeLine, codeLineStart);
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::loadImage() : '%s:%d' : image width %d + starting address 0x%04x overflow, for %s : %s\n",
                                                    codeLine._moduleName.c_str(), codeLineStart, gtRgbFile._header._width, address, filename.c_str(), codeLine._text.c_str());
            return false;
        }

        Compiler::DefDataImage defDataImage = {address, tgaFile._header._width, tgaFile._header._height, stride, gtRgbFile._data};
        Compiler::getDefDataImages().push_back(defDataImage);

        int size = gtRgbFile._header._width;
        for(int y=0; y<gtRgbFile._header._height; y++)
        {
            // Take offscreen memory from compiler for images wider than visible screen resolution, or images in offscreen memory
            if(address >= RAM_VIDEO_START  &&  address <= RUN_TIME_START)
            {
                size = gtRgbFile._header._width + (address & 0x00FF) - RAM_SCANLINE_SIZE;
                if(size > 0)
                {
                    if(!Memory::takeFreeRAM((address & 0xFF00) + RAM_SCANLINE_SIZE, size, false))
                    {
                        loadUsage(LoadImage, codeLine, codeLineStart);
                        Cpu::reportError(Cpu::KwdError, stderr, "Keywords::loadImage() : '%s:%d' : allocating RAM at '0x%04x' of size '%d' failed : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                                                           address, size, codeLine._text.c_str());
                        return false;
                    }
                }
            }

            // No need to do this for ROMvX0 as it has a new loader that does not require screen memory
            if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD)
            {
                // 'Loader.gcl' is resident at these addresses when loading *.gt1 files, therefore you can overwrite these locations only AFTER the loading process has finished
                // Split up image scanlines into offscreen chunks and load the offscreen chunks into the correct onscreen memory locations after 'Loader.gcl' is done
                if((address >= LOADER_SCANLINE0_START  &&  address<= LOADER_SCANLINE0_END)  ||  (address >= LOADER_SCANLINE1_START  &&  address<= LOADER_SCANLINE1_END)  ||  
                    (address >= LOADER_SCANLINE2_START  &&  address<= LOADER_SCANLINE2_END))
                {
                    uint16_t chunkOffset = 0x0000, chunkAddr = 0x0000;
                    for(int i=0; i<gtRgbFile._header._width / RAM_SEGMENTS_SIZE; i++)
                    {
                        loadImageChunk(codeLine, codeLineStart, gtRgbFile._data, y, gtRgbFile._header._width, address, RAM_SEGMENTS_SIZE, chunkOffset, chunkAddr);
                    }
                    loadImageChunk(codeLine, codeLineStart, gtRgbFile._data, y, gtRgbFile._header._width, address, gtRgbFile._header._width % RAM_SEGMENTS_SIZE, chunkOffset, chunkAddr);
                }
            }

            // Next destination row
            address += stride; 
        }

        return true;
    }

    bool loadBlit(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, const std::vector<std::string>& tokens, const std::string& filename, const Image::TgaFile& tgaFile, Image::GtRgbFile& gtRgbFile)
    {
        if(Compiler::getCodeRomType() < Cpu::ROMv3)
        {
            std::string romTypeStr;
            getRomTypeStr(Compiler::getCodeRomType(), romTypeStr);
            loadUsage(LoadBlit, codeLine, codeLineStart);
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::loadBlit() : '%s:%d' : version error, 'LOAD BLIT' requires ROMv3 or higher, you are trying to link against '%s' : %s\n",
                                                    codeLine._moduleName.c_str(), codeLineStart, romTypeStr.c_str(), codeLine._text.c_str());
            return false;
        }
        if(tokens.size() < 3  ||  tokens.size() > 5)
        {
            loadUsage(LoadBlit, codeLine, codeLineStart);
            return false;
        }
        if(gtRgbFile._header._width % BLIT_CHUNK_SIZE != 0)
        {
            loadUsage(LoadBlit, codeLine, codeLineStart);
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::loadBlit() : '%s:%d' : blit width not a multiple of %d, (%d x %d), for %s : %s\n",
                                                    codeLine._moduleName.c_str(), codeLineStart, BLIT_CHUNK_SIZE, gtRgbFile._header._width, gtRgbFile._header._height, filename.c_str(), codeLine._text.c_str());
            return false;
        }

        switch(tgaFile._header._bitsPerPixel)
        {
            case 24: Image::convertRGB8toRGB2(tgaFile._data, gtRgbFile._data, tgaFile._header._width, tgaFile._header._height, tgaFile._imageOrigin);   break;
            case 32: Image::convertARGB8toARGB2(tgaFile._data, gtRgbFile._data, tgaFile._header._width, tgaFile._header._height, tgaFile._imageOrigin); break;

            default:
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::loadBlit() : '%s:%d' : wrong bit depth, 'LOAD BLIT' requires 24 or 32 bit image data : %s\n", codeLine._moduleName.c_str(),
                                                                                                                                                                 codeLineStart, codeLine._text.c_str());
                return false;
            }
            break;
        }

        // Unique blit ID
        std::string idToken = tokens[2];
        Expression::Numeric idNumeric;
        std::string idOperand;
        if(Compiler::parseStaticExpression(codeLineIndex, idToken, idOperand, idNumeric) == Compiler::OperandInvalid)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::loadBlit() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, idToken.c_str(), codeLine._text.c_str());
            return false;
        }
        int blitId = int(std::lround(idNumeric._value));
        if(Compiler::getDefDataBlits().find(blitId) != Compiler::getDefDataBlits().end())
        {
            loadUsage(LoadBlit, codeLine, codeLineStart);
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::loadBlit() : '%s:%d' : blit id %d not unique : %s\n", codeLine._moduleName.c_str(), codeLineStart, blitId, codeLine._text.c_str());
            return false;
        }

        // Parse optional blit flip
        Compiler::BlitFlipType flipType = Compiler::NoFlip;
        if(tokens.size() >= 4)
        {
            std::string flipToken = tokens[3];
            Expression::stripWhitespace(flipToken);
            Expression::strToUpper(flipToken);
            if(flipToken == "NOFLIP")      flipType = Compiler::NoFlip;
            else if(flipToken == "FLIPX")  flipType = Compiler::FlipX;
            else if(flipToken == "FLIPY")  flipType = Compiler::FlipY;
            else if(flipToken == "FLIPXY") flipType = Compiler::FlipXY;
            else
            {
                loadUsage(LoadBlit, codeLine, codeLineStart);
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::loadBlit() : '%s:%d' : unknown blit flip type, %s : %s\n", codeLine._moduleName.c_str(), codeLineStart, flipToken.c_str(), codeLine._text.c_str());
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::loadBlit() : '%s:%d' : must use one of 'NOFLIP', 'FLIPX', 'FLIPY', 'FLIPXY' : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
                return false;
            }
        }

        // Parse optional blit overlap, (for last column)
        uint16_t overlap = 0;
        if(tokens.size() == 5)
        {
            std::string overlapToken = tokens[4];
            Expression::Numeric overlapNumeric;
            std::string overlapOperand;
            if(Compiler::parseStaticExpression(codeLineIndex, overlapToken, overlapOperand, overlapNumeric) == Compiler::OperandInvalid)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::loadBlit() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, overlapToken.c_str(), codeLine._text.c_str());
                return false;
            }
            overlap = uint16_t(std::lround(overlapNumeric._value));
        }

        // Build blit data from image data
        uint16_t numColumns = gtRgbFile._header._width / BLIT_CHUNK_SIZE;
        uint16_t remStripeChunks = gtRgbFile._header._height % Compiler::getBlitStripeChunks();
        uint16_t numStripesPerCol = gtRgbFile._header._height / Compiler::getBlitStripeChunks() + int(remStripeChunks > 0);
        uint16_t numStripeChunks = (numStripesPerCol == 1) ? gtRgbFile._header._height : Compiler::getBlitStripeChunks();
        std::vector<uint16_t> stripeAddrs;
        std::vector<uint8_t> blitData;

        if(numColumns == 1  &&  overlap)
        {
            loadUsage(LoadBlit, codeLine, codeLineStart);
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::loadBlit() : '%s:%d' : can't have a non zero overlap with a single column blit : %s\n", codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
            return false;
        }

        // Search blit image for instancing
        int parentInstance = 0;
        bool isInstanced = false;
        for(auto it=Compiler::getDefDataBlits().begin(); it!=Compiler::getDefDataBlits().end(); ++it)
        {
            if(it->second._filename == filename)
            {
                blitData = it->second._data;
                parentInstance = it->first;
                isInstanced = true;
                break;
            }
        }

        // Allocate blit memory
        int addrIndex = 0;
        uint16_t address = 0x0000;
        for(int i=0; i<numColumns; i++)
        {
            // One stripe per column
            if(numStripesPerCol == 1)
            {
                if(isInstanced)
                {
                    address = Compiler::getDefDataBlits()[parentInstance]._stripeAddrs[addrIndex];
                    addrIndex += 2;
                }
                else
                {
                    if(!Memory::getFreePageRAM(Compiler::getBlitStripeFitType(), numStripeChunks*BLIT_CHUNK_SIZE + 1, Compiler::getBlitStripeMinAddress(), Compiler::getBlitStripeMaxAddress(), address, true))
                    {
                        loadUsage(LoadBlit, codeLine, codeLineStart);
                        Cpu::reportError(Cpu::KwdError, stderr, "Keywords::loadBlit() : '%s:%d' : getting blit memory for stripe %d failed : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                                                    int(stripeAddrs.size()/2 + 1), codeLine._text.c_str());
                        return false;
                    }
                }
                stripeAddrs.push_back(address);

                // Destination offsets
                switch(flipType)
                {
                    case Compiler::NoFlip: stripeAddrs.push_back(uint16_t(0 + i*6));
                                           if(i == numColumns - 1) stripeAddrs.back() -= overlap;  // push last column closer to all other columns
                    break;

                    case Compiler::FlipX: stripeAddrs.push_back(uint16_t(0 + (numColumns-1-i)*6));
                                          if(i != numColumns - 1) stripeAddrs.back() -= overlap;  // push all other columns closer to last column
                    break;

                    case Compiler::FlipY: stripeAddrs.push_back(uint16_t((numStripeChunks-1)*256 + i*6));
                                          if(i == numColumns - 1) stripeAddrs.back() -= overlap;  // push last column closer to all other columns
                    break;

                    case Compiler::FlipXY: stripeAddrs.push_back(uint16_t((numStripeChunks-1)*256 + (numColumns-1-i)*6));
                                           if(i != numColumns - 1) stripeAddrs.back() -= overlap;  // push all other columns closer to last column
                    break;

                    default: break;
                }

                // Copy blit data and delimiter
                for(int j=0; j<numStripeChunks; j++)
                {
                    for(int k=0; k<BLIT_CHUNK_SIZE; k++)
                    {
                        blitData.push_back(gtRgbFile._data[i*BLIT_CHUNK_SIZE + j*BLIT_CHUNK_SIZE*numColumns + k] & 0x3F);
                    }
                }
                blitData.push_back(uint8_t(-gtRgbFile._header._height));
            }
            // Multiple stripes per column
            else
            {
                // MAX_BLIT_CHUNKS_PER_STRIPE stripes
                for(int j=0; j<numStripesPerCol-1; j++)
                {
                    if(isInstanced)
                    {
                        address = Compiler::getDefDataBlits()[parentInstance]._stripeAddrs[addrIndex];
                        addrIndex += 2;
                    }
                    else
                    {
                        if(!Memory::getFreePageRAM(Compiler::getBlitStripeFitType(), numStripeChunks*BLIT_CHUNK_SIZE + 1, Compiler::getBlitStripeMinAddress(), Compiler::getBlitStripeMaxAddress(), address, true))
                        {
                            loadUsage(LoadBlit, codeLine, codeLineStart);
                            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::loadBlit() : '%s:%d' : getting blit memory failed for stripe %d : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                                                        int(stripeAddrs.size()/2 + 1), codeLine._text.c_str());
                            return false;
                        }
                    }
                    stripeAddrs.push_back(address);

                    // Destination offsets
                    switch(flipType)
                    {
                        case Compiler::NoFlip: stripeAddrs.push_back(uint16_t(j*numStripeChunks*256 + i*6));
                                               if(i == numColumns - 1) stripeAddrs.back() -= overlap;  // push last column closer to all other columns
                        break;

                        case Compiler::FlipX: stripeAddrs.push_back(uint16_t(j*numStripeChunks*256 + (numColumns-1-i)*6));
                                              if(i != numColumns - 1) stripeAddrs.back() -= overlap;  // push all other columns closer to last column
                        break;

                        case Compiler::FlipY: stripeAddrs.push_back(uint16_t(((numStripesPerCol-1-j)*numStripeChunks+remStripeChunks-1)*256 + i*6));
                                              if(i == numColumns - 1) stripeAddrs.back() -= overlap;  // push last column closer to all other columns
                        break;

                        case Compiler::FlipXY: stripeAddrs.push_back(uint16_t(((numStripesPerCol-1-j)*numStripeChunks+remStripeChunks-1)*256 + (numColumns-1-i)*6));
                                               if(i != numColumns - 1) stripeAddrs.back() -= overlap;  // push all other columns closer to last column
                        break;

                        default: break;
                    }

                    // Copy blit data and delimiter
                    for(int k=0; k<numStripeChunks; k++)
                    {
                        for(int l=0; l<BLIT_CHUNK_SIZE; l++)
                        {
                            blitData.push_back(gtRgbFile._data[i*BLIT_CHUNK_SIZE + j*numStripeChunks*BLIT_CHUNK_SIZE*numColumns + k*BLIT_CHUNK_SIZE*numColumns + l] & 0x3F);
                        }
                    }
                    blitData.push_back(255);
                }

                // Remainder stripe
                if(isInstanced)
                {
                    address = Compiler::getDefDataBlits()[parentInstance]._stripeAddrs[addrIndex];
                    addrIndex += 2;
                }
                else
                {
                    if(!Memory::getFreePageRAM(Compiler::getBlitStripeFitType(), remStripeChunks*BLIT_CHUNK_SIZE + 1, Compiler::getBlitStripeMinAddress(), Compiler::getBlitStripeMaxAddress(), address, true))
                    {
                        loadUsage(LoadBlit, codeLine, codeLineStart);
                        Cpu::reportError(Cpu::KwdError, stderr, "Keywords::loadBlit() : '%s:%d' : getting blit memory failed for stripe %d : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                                                    int(stripeAddrs.size()/2 + 1), codeLine._text.c_str());
                        return false;
                    }
                }
                stripeAddrs.push_back(address);

                // Destination offsets
                switch(flipType)
                {
                    case Compiler::NoFlip: stripeAddrs.push_back(uint16_t((numStripesPerCol-1)*numStripeChunks*256 + i*6));
                                           if(i == numColumns - 1) stripeAddrs.back() -= overlap;  // push last column closer to all other columns
                    break;

                    case Compiler::FlipX: stripeAddrs.push_back(uint16_t((numStripesPerCol-1)*numStripeChunks*256 + (numColumns-1-i)*6));
                                          if(i != numColumns - 1) stripeAddrs.back() -= overlap;  // push all other columns closer to last column
                    break;

                    case Compiler::FlipY: stripeAddrs.push_back(uint16_t((remStripeChunks-1)*256 + i*6));
                                          if(i == numColumns - 1) stripeAddrs.back() -= overlap;  // push last column closer to all other columns
                    break;

                    case Compiler::FlipXY: stripeAddrs.push_back(uint16_t((remStripeChunks-1)*256 + (numColumns-1-i)*6));
                                           if(i != numColumns - 1) stripeAddrs.back() -= overlap;  // push all other columns closer to last column
                    break;

                    default: break;
                }

                // Copy blit data and delimiter
                for(int j=0; j<remStripeChunks; j++)
                {
                    for(int k=0; k<BLIT_CHUNK_SIZE; k++)
                    {
                        blitData.push_back(gtRgbFile._data[i*BLIT_CHUNK_SIZE + (numStripesPerCol-1)*numStripeChunks*BLIT_CHUNK_SIZE*numColumns + j*BLIT_CHUNK_SIZE*numColumns + k] & 0x3F);
                    }
                }
                blitData.push_back(255);
            }
        }

        Compiler::getDefDataBlits()[blitId] = {blitId, filename, tgaFile._header._width, tgaFile._header._height, numColumns, numStripesPerCol, numStripeChunks, remStripeChunks, stripeAddrs, blitData, flipType, isInstanced};

        return true;
    }

    bool loadSprite(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, const std::vector<std::string>& tokens, const std::string& filename, const Image::TgaFile& tgaFile, Image::GtRgbFile& gtRgbFile)
    {
        UNREFERENCED_PARAM(tgaFile);

        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD)
        {
            std::string romTypeStr;
            getRomTypeStr(Compiler::getCodeRomType(), romTypeStr);
            loadUsage(LoadSprite, codeLine, codeLineStart);
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::loadSprite() : '%s:%d' : version error, 'LOAD SPRITE' requires ROMvX0, you are trying to link against '%s' : %s\n",
                                                    codeLine._moduleName.c_str(), codeLineStart, romTypeStr.c_str(), codeLine._text.c_str());
            return false;
        }
        if(tokens.size() != 3)
        {
            loadUsage(LoadSprite, codeLine, codeLineStart);
            return false;
        }
        if(gtRgbFile._header._width != SPRITE_WIDTH)
        {
            loadUsage(LoadSprite, codeLine, codeLineStart);
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::loadSprite() : '%s:%d' : sprite width must be %d, (%d x %d), for %s : %s\n",
                                                    codeLine._moduleName.c_str(), codeLineStart, SPRITE_WIDTH, gtRgbFile._header._width, gtRgbFile._header._height, filename.c_str(), codeLine._text.c_str());
            return false;
        }
        if(gtRgbFile._header._height > MAX_SPRITE_HEIGHT  ||  gtRgbFile._header._height == 0  ||  (gtRgbFile._header._height & 1))
        {
            loadUsage(LoadSprite, codeLine, codeLineStart);
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::loadSprite() : '%s:%d' : sprite height must be an even number between 2 and %d, (%d x %d), for %s : %s\n",
                                                    codeLine._moduleName.c_str(), codeLineStart, MAX_SPRITE_HEIGHT, gtRgbFile._header._width, gtRgbFile._header._height, filename.c_str(), codeLine._text.c_str());
            return false;
        }

        switch(tgaFile._header._bitsPerPixel)
        {
            case 24: Image::convertRGB8toRGB2(tgaFile._data, gtRgbFile._data, tgaFile._header._width, tgaFile._header._height, tgaFile._imageOrigin);   break;
            case 32: Image::convertARGB8toARGB2(tgaFile._data, gtRgbFile._data, tgaFile._header._width, tgaFile._header._height, tgaFile._imageOrigin); break;

            default:
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::loadSprite() : '%s:%d' : wrong bit depth, 'LOAD SPRITE' requires 24 or 32 bit image data : %s\n",
                                                        codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
                return false;
            }
            break;
        }

        // Unique sprite ID
        std::string idToken = tokens[2];
        Expression::Numeric idNumeric;
        std::string idOperand;
        if(Compiler::parseStaticExpression(codeLineIndex, idToken, idOperand, idNumeric) == Compiler::OperandInvalid)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::loadSprite() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, idToken.c_str(), codeLine._text.c_str());
            return false;
        }
        int spriteId = int(std::lround(idNumeric._value));
        if(Compiler::getDefDataSprites().find(spriteId) != Compiler::getDefDataSprites().end())
        {
            loadUsage(LoadSprite, codeLine, codeLineStart);
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::loadSprite() : '%s:%d' : sprite id %d not unique : %s\n", codeLine._moduleName.c_str(), codeLineStart, spriteId, codeLine._text.c_str());
            return false;
        }

        // Build sprite data from image data
        std::vector<uint8_t> spriteData;
        for(int i=0; i<int(gtRgbFile._data.size()); i++)
        {
            uint8_t pixel;

            switch(tgaFile._header._bitsPerPixel)
            {
                case 24:
                {
                    pixel = gtRgbFile._data[i] & 0x3F;
                }
                break;

                case 32:
                {
                    pixel = gtRgbFile._data[i];
                    pixel = ((pixel & 0x80)  &&  (pixel & 0x3F) == 0x00) ? 0x90 : pixel;        // black to darkest blue
                    pixel = ((pixel & 0x80) == 0x00)                     ? 0x00 : pixel & 0x3F; // alpha < 0.5 = black
                }
                break;

                default: break;
            }

            spriteData.push_back(pixel);
        }

        // Search for free sprite memory
        uint16_t width = gtRgbFile._header._width;
        uint16_t height = gtRgbFile._header._height;
        uint16_t address = Compiler::getRuntimeStart() & 0xFF00 | 0x00A0;

SPRITE_ALLOC_RESTART:;
        for(int i=0; i<height; i++)
        {
            bool memoryTaken = false;
            if(!Memory::isFreeRAM(address, SPRITE_WIDTH*2 + 2)) memoryTaken = true;

            address -= 0x0100;
            if(address < 0x08A0)
            {
                loadUsage(LoadSprite, codeLine, codeLineStart);
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::loadSprite() : '%s:%d' : couldn't allocate memory for sprite %d : %s\n", codeLine._moduleName.c_str(), codeLineStart, spriteId, codeLine._text.c_str());
                return false;
            }

            if(memoryTaken) goto SPRITE_ALLOC_RESTART;
        }

        // Alloc sprite memory
        for(int i=0; i<height; i++)
        {
            if(!Memory::takeFreeRAM(address + uint16_t(i)*0x0100, SPRITE_WIDTH*2 + 2), false)
            {
                loadUsage(LoadSprite, codeLine, codeLineStart);
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::loadSprite() : '%s:%d' : couldn't allocate memory for sprite %d : %s\n", codeLine._moduleName.c_str(), codeLineStart, spriteId, codeLine._text.c_str());
                return false;
            }
        }

        Compiler::getDefDataSprites()[spriteId] = {spriteId, filename, width, height, address, spriteData};

        return true;
    }

    bool loadPattern(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, const std::vector<std::string>& tokens, const std::string& filename, const Image::TgaFile& tgaFile, Image::GtRgbFile& gtRgbFile)
    {
        UNREFERENCED_PARAM(tgaFile);

        if(Compiler::getCodeRomType() < Cpu::ROMvX0  ||  Compiler::getCodeRomType() >= Cpu::SDCARD)
        {
            std::string romTypeStr;
            getRomTypeStr(Compiler::getCodeRomType(), romTypeStr);
            loadUsage(LoadPattern, codeLine, codeLineStart);
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::loadPattern() : '%s:%d' : version error, 'LOAD PATTERN' requires ROMvX0, you are trying to link against '%s' : %s\n",
                                                    codeLine._moduleName.c_str(), codeLineStart, romTypeStr.c_str(), codeLine._text.c_str());
            return false;
        }
        if(tokens.size() != 3)
        {
            loadUsage(LoadPattern, codeLine, codeLineStart);
            return false;
        }
        if(gtRgbFile._header._width != SPRITE_WIDTH)
        {
            loadUsage(LoadPattern, codeLine, codeLineStart);
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::loadPattern() : '%s:%d' : pattern width must be %d, (%d x %d), for %s : %s\n", codeLine._moduleName.c_str(), codeLineStart, SPRITE_WIDTH, 
                                                                                                                                              gtRgbFile._header._width, gtRgbFile._header._height,
                                                                                                                                              filename.c_str(), codeLine._text.c_str());
            return false;
        }
        if(gtRgbFile._header._height > MAX_SPRITE_HEIGHT  ||  gtRgbFile._header._height == 0  ||  (gtRgbFile._header._height & 1))
        {
            loadUsage(LoadPattern, codeLine, codeLineStart);
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::loadPattern() : '%s:%d' : pattern height must be an even number between 2 and %d, (%d x %d), for %s : %s\n",
                                                    codeLine._moduleName.c_str(), codeLineStart, MAX_SPRITE_HEIGHT, gtRgbFile._header._width, gtRgbFile._header._height, filename.c_str(), codeLine._text.c_str());
            return false;
        }

        switch(tgaFile._header._bitsPerPixel)
        {
            case 24: Image::convertRGB8toRGB2(tgaFile._data, gtRgbFile._data, tgaFile._header._width, tgaFile._header._height, tgaFile._imageOrigin);   break;
            case 32: Image::convertARGB8toARGB2(tgaFile._data, gtRgbFile._data, tgaFile._header._width, tgaFile._header._height, tgaFile._imageOrigin); break;

            default:
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::loadPattern() : '%s:%d' : wrong bit depth, 'LOAD PATTERN' requires 24 or 32 bit image data : %s\n",
                                                        codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
                return false;
            }
            break;
        }

        // Unique pattern ID
        std::string idToken = tokens[2];
        Expression::Numeric idNumeric;
        std::string idOperand;
        if(Compiler::parseStaticExpression(codeLineIndex, idToken, idOperand, idNumeric) == Compiler::OperandInvalid)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::loadPattern() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, idToken.c_str(), codeLine._text.c_str());
            return false;
        }
        int patternId = int(std::lround(idNumeric._value));
        if(Compiler::getDefDataPatterns().find(patternId) != Compiler::getDefDataPatterns().end())
        {
            loadUsage(LoadPattern, codeLine, codeLineStart);
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::loadPattern() : '%s:%d' : pattern id %d not unique : %s\n", codeLine._moduleName.c_str(), codeLineStart, patternId, codeLine._text.c_str());
            return false;
        }

        // Build pattern data from image data
        std::vector<uint8_t> patternData;
        for(int i=0; i<int(gtRgbFile._data.size()); i++)
        {
            uint8_t pixel;

            switch(tgaFile._header._bitsPerPixel)
            {
                case 24:
                {
                    pixel = gtRgbFile._data[i] & 0x3F;
                }
                break;

                case 32:
                {
                    pixel = gtRgbFile._data[i];
                    pixel = ((pixel & 0x80)  &&  (pixel & 0x3F) == 0x00) ? 0x90 : pixel;        // black to darkest blue
                    pixel = ((pixel & 0x80) == 0x00)                     ? 0x00 : pixel & 0x3F; // alpha < 0.5 = black
                }
                break;

                default: break;
            }

            patternData.push_back(pixel);
        }

        // Alloc pattern memory
        uint16_t address = 0x0000;
        std::vector<uint16_t> patternAddrs;
        uint16_t width = gtRgbFile._header._width;
        uint16_t height = gtRgbFile._header._height;
        for(int i=0; i<height/2; i++)
        {
            if(!Memory::getFreePageRAM(Memory::FitDescending, SPRITE_WIDTH*2, USER_CODE_START, Compiler::getRuntimeStart(), address, true))
            {
                loadUsage(LoadPattern, codeLine, codeLineStart);
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::loadPattern() : '%s:%d' : couldn't allocate memory for pattern %d : %s\n", codeLine._moduleName.c_str(), codeLineStart, patternId, codeLine._text.c_str());
                return false;
            }

            patternAddrs.push_back(address);
        }

        Compiler::getDefDataPatterns()[patternId] = {patternId, filename, width, height, patternAddrs, patternData};

        return true;
    }

    bool loadFont(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, const std::vector<std::string>& tokens, const std::string& filename, const Image::TgaFile& tgaFile, Image::GtRgbFile& gtRgbFile)
    {
        if(Compiler::getCodeRomType() < Cpu::ROMv3)
        {
            std::string romTypeStr;
            getRomTypeStr(Compiler::getCodeRomType(), romTypeStr);
            loadUsage(LoadFont, codeLine, codeLineStart);
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::loadFont() : '%s:%d' : version error, 'LOAD FONT' requires ROMv3 or higher, you are trying to link against '%s' : %s\n",
                                                    codeLine._moduleName.c_str(), codeLineStart, romTypeStr.c_str(), codeLine._text.c_str());
            return false;
        }
        if(tokens.size() < 3  ||  tokens.size() > 4)
        {
            loadUsage(LoadFont, codeLine, codeLineStart);
            return false;
        }

        switch(tgaFile._header._bitsPerPixel)
        {
            case 24: Image::convertRGB8toRGB2(tgaFile._data, gtRgbFile._data, tgaFile._header._width, tgaFile._header._height, tgaFile._imageOrigin);   break;
            case 32: Image::convertARGB8toARGB2(tgaFile._data, gtRgbFile._data, tgaFile._header._width, tgaFile._header._height, tgaFile._imageOrigin); break;

            default:
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::loadFont() : '%s:%d' : wrong bit depth, 'LOAD FONT' requires 24 or 32 bit image data : %s\n",
                                                        codeLine._moduleName.c_str(), codeLineStart, codeLine._text.c_str());
                return false;
            }
            break;
        }

        // Unique font ID
        std::string idToken = tokens[2];
        Expression::Numeric idNumeric;
        std::string idOperand;
        if(Compiler::parseStaticExpression(codeLineIndex, idToken, idOperand, idNumeric) == Compiler::OperandInvalid)
        {
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::loadFont() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, idToken.c_str(), codeLine._text.c_str());
            return false;
        }
        int fontId = int(std::lround(idNumeric._value));
        if(Compiler::getDefDataFonts().find(fontId) != Compiler::getDefDataFonts().end())
        {
            loadUsage(LoadFont, codeLine, codeLineStart);
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::loadFont() : '%s:%d' : font id %d not unique : %s\n", codeLine._moduleName.c_str(), codeLineStart, fontId, codeLine._text.c_str());
            return false;
        }

        // Foreground/background colours
        uint16_t fgbgColour = 0x0000;
        if(tokens.size() == 4)
        {
            std::string fgbgToken = tokens[3];
            Expression::Numeric fgbgNumeric;
            std::string fgbgOperand;
            if(Compiler::parseStaticExpression(codeLineIndex, fgbgToken, fgbgOperand, fgbgNumeric) == Compiler::OperandInvalid)
            {
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::loadFont() : '%s:%d' : syntax error in : '%s' : %s\n", codeLine._moduleName.c_str(), codeLineStart, fgbgToken.c_str(), codeLine._text.c_str());
                return false;
            }
            fgbgColour = uint16_t(std::lround(fgbgNumeric._value));
        }

        // Width
        if(gtRgbFile._header._width % FONT_WIDTH != 0)
        {
            loadUsage(LoadFont, codeLine, codeLineStart);
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::loadFont() : '%s:%d' : font width %d is not a multiple of %d : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                                     gtRgbFile._header._width, FONT_WIDTH, codeLine._text.c_str());
            return false;
        }

        // Height
        if(gtRgbFile._header._height % FONT_HEIGHT != 0)
        {
            loadUsage(LoadFont, codeLine, codeLineStart);
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::loadFont() : '%s:%d' : font height %d is not a multiple of %d : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                                      gtRgbFile._header._height, FONT_HEIGHT, codeLine._text.c_str());
            return false;
        }

        // Load font mapping file
        bool foundMapFile = true;
        size_t nameSuffix = filename.find_last_of(".");
        std::string filenameMap = filename.substr(0, nameSuffix) + ".map";
        std::ifstream infile(filenameMap, std::ios::in);
        if(!infile.is_open())
        {
            foundMapFile = false;
        }

        // Parse font mapping file
        int maxIndex = -1;
        uint16_t mapAddr = 0x0000;
        std::vector<uint8_t> mapping(MAPPING_SIZE);
        if(foundMapFile)
        {
            int ascii, index, line = 0;
            while(!infile.eof())
            {
                infile >> ascii >> index;
                if(index > maxIndex) maxIndex = index;
                if(!infile.good() && !infile.eof())
                {
                    loadUsage(LoadFont, codeLine, codeLineStart);
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::loadFont() : '%s:%d' : error in mapping file %s on line %d : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                                           filenameMap.c_str(), line + 1, codeLine._text.c_str());
                    return false;
                }

                if(line >= MAPPING_SIZE) break;
                mapping[line++] = uint8_t(index);
            }

            if(line != MAPPING_SIZE)
            {
                loadUsage(LoadFont, codeLine, codeLineStart);
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::loadFont() : '%s:%d' : error, found an incorrect number of map entries %d for file %s, should be %d : %s\n",
                                                        codeLine._moduleName.c_str(), codeLineStart, line - 1, filenameMap.c_str(), MAPPING_SIZE, codeLine._text.c_str());
                return false;
            }

            if(!Memory::getFreePageRAM(Memory::FitDescending, MAPPING_SIZE, USER_CODE_START, Compiler::getRuntimeStart(), mapAddr, true))
            {
                loadUsage(LoadFont, codeLine, codeLineStart);
                Cpu::reportError(Cpu::KwdError, stderr, "Keywords::loadFont() : '%s:%d' : getting mapping memory for map size of %d failed : %s\n",
                                                        codeLine._moduleName.c_str(), codeLineStart, MAPPING_SIZE, codeLine._text.c_str());
                return false;
            }
        }

        // 8th line is implemented as a separate blit call, to save memory and allow for more efficient memory packing
        const int kCharHeight = FONT_HEIGHT-1;

        // Copy font data and create delimiter
        std::vector<uint8_t> charData;
        std::vector<uint16_t> charAddrs;
        std::vector<std::vector<uint8_t>> fontData;
        for(int j=0; j<tgaFile._header._height; j+=FONT_HEIGHT)
        {
            for(int i=0; i<tgaFile._header._width; i+=FONT_WIDTH)
            {
                for(int l=0; l<kCharHeight; l++)
                {
                    for(int k=0; k<FONT_WIDTH; k++)
                    {
                        uint8_t pixel = gtRgbFile._data[j*tgaFile._header._width + i + l*tgaFile._header._width + k] & 0x3F;
                        if(fgbgColour)
                        {
                            if(pixel == 0x00) pixel = fgbgColour & 0x3F;
                            if(pixel == 0x3F) pixel = (fgbgColour >> 8) & 0x3F ;
                        }
                        charData.push_back(pixel);
                    }
                }
                charData.push_back(uint8_t(-(kCharHeight)));
                fontData.push_back(charData);
                charData.clear();

                uint16_t address = 0x0000;
                if(!Memory::getFreePageRAM(Memory::FitDescending, (kCharHeight)*FONT_WIDTH + 1, USER_CODE_START, Compiler::getRuntimeStart(), address, true))
                {
                    loadUsage(LoadFont, codeLine, codeLineStart);
                    Cpu::reportError(Cpu::KwdError, stderr, "Keywords::loadFont() : '%s:%d' : getting font memory for char %d failed : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                                              int(fontData.size() - 1), codeLine._text.c_str());
                    return false;
                }

                charAddrs.push_back(address);
            }
        }

        if(foundMapFile  &&  maxIndex + 1 != int(fontData.size()))
        {
            loadUsage(LoadFont, codeLine, codeLineStart);
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::loadFont() : '%s:%d' : font mapping table does not match font data, found a mapping count of %d and a chars count of %d : %s\n",
                                                    codeLine._moduleName.c_str(), codeLineStart, maxIndex + 1, int(fontData.size()), codeLine._text.c_str());
            return false;
        }

        // Create baseline for all chars in each font
        uint16_t baseAddr = 0x0000;
        if(!Memory::getFreePageRAM(Memory::FitDescending, FONT_WIDTH + 1, USER_CODE_START, Compiler::getRuntimeStart(), baseAddr, true))
        {
            loadUsage(LoadFont, codeLine, codeLineStart);
            Cpu::reportError(Cpu::KwdError, stderr, "Keywords::loadFont() : '%s:%d' : getting font memory for char %d failed : %s\n", codeLine._moduleName.c_str(), codeLineStart,
                                                                                                                                      int(fontData.size() - 1), codeLine._text.c_str());
            return false;
        }

        Compiler::getDefDataFonts()[fontId] = {fontId, filenameMap, tgaFile._header._width, tgaFile._header._height, charAddrs, fontData, mapAddr, mapping, baseAddr, fgbgColour};

        Linker::enableFontLinking();

        return true;
    }
}