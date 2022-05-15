#ifndef LOAD_H
#define LOAD_H


#include <string>
#include <vector>

#include "compiler.h"


#define RAW_MAX_FILE_SIZE 0x10000

#define FONT_WIDTH   6
#define FONT_HEIGHT  8
#define MAPPING_SIZE 96


namespace Load
{
    enum LoadUsage {LoadType=0, LoadRaw, LoadWave, LoadMidi, LoadImage, LoadBlit, LoadFont, LoadSprite, LoadPattern};


    void loadUsage(int msgType, Compiler::CodeLine& codeLine, int codeLineStart);

    bool loadRaw(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, const std::vector<std::string>& tokens);

    bool loadWave(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, const std::vector<std::string>& tokens);
    bool loadMidi(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, const std::vector<std::string>& tokens);

    bool loadImageChunk(Compiler::CodeLine& codeLine, int codeLineStart, const std::vector<uint8_t>& data, int row, uint16_t width, uint16_t address, uint8_t chunkSize, uint16_t& chunkOffset, uint16_t& chunkAddr);
    bool loadImage(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, const std::vector<std::string>& tokens, const std::string& filename, const Image::TgaFile& tgaFile, Image::GtRgbFile& gtRgbFile);

    bool loadBlit(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, const std::vector<std::string>& tokens, const std::string& filename, const Image::TgaFile& tgaFile, Image::GtRgbFile& gtRgbFile);
    bool loadSprite(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, const std::vector<std::string>& tokens, const std::string& filename, const Image::TgaFile& tgaFile, Image::GtRgbFile& gtRgbFile);
    bool loadPattern(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, const std::vector<std::string>& tokens, const std::string& filename, const Image::TgaFile& tgaFile, Image::GtRgbFile& gtRgbFile);

    bool loadFont(Compiler::CodeLine& codeLine, int codeLineIndex, int codeLineStart, const std::vector<std::string>& tokens, const std::string& filename, const Image::TgaFile& tgaFile, Image::GtRgbFile& gtRgbFile);
}

#endif