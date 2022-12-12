#include <stdio.h>
#include <stdlib.h>
#include <sstream>

#include "../../memory.h"
#include "../../loader.h"
#include "../../cpu.h"
#include "../../expression.h"
#include "../../assembler.h"
#include "../../compiler.h"
#include "../../operators.h"
#include "../../functions.h"
#include "../../keywords.h"
#include "../../pragmas.h"
#include "../../optimiser.h"
#include "../../validater.h"
#include "../../linker.h"


#define GTBASIC_VERSION_STR "gtbasic v" MAJOR_VERSION "." MINOR_VERSION


int main(int argc, char* argv[])
{
    Cpu::reportError(Cpu::NoError, stderr, "\n%s\n", GTBASIC_VERSION_STR);
    if(argc != 2  &&  argc != 3)
    {
        Cpu::reportError(Cpu::UtlError, stderr, "Usage:   gtbasic <input filename> <optional include path>\n");
        return 1;
    }

    std::string name = Expression::strUpper(std::string(argv[1]));
    if(name.find(".GTBAS") == name.npos  &&  name.find(".GBAS") == name.npos  &&  name.find(".BAS") == name.npos)
    {
        Cpu::reportError(Cpu::UtlError, stderr, "Wrong file extension in %s : must be one of : '.gbas' or '.bas'\n", name.c_str());
        return 1;
    }

    Memory::initialise();
    Loader::initialise();
    Expression::initialise();
    Assembler::initialise();
    Compiler::initialise();
    Operators::initialise();
    Functions::initialise();
    Keywords::initialise();
    Pragmas::initialise();
    Optimiser::initialise();
    Validater::initialise();
    Linker::initialise();

    // Choose memory model
    if(name.find("64K") != std::string::npos)
    {
        Cpu::setMemoryModel(RAM_SIZE_HI);
    }

    // Optional include path
    std::string includepath = (argc == 3) ? std::string(argv[2]) : ".";

    // Output file
    name = std::string(argv[1]);
    size_t nameSuffix = name.find_last_of(".");
    std::string output = name.substr(0, nameSuffix) + ".gasm";

    // Path and name
    size_t slash = name.find_last_of("\\/");
    std::string path = (slash != std::string::npos) ? name.substr(0, slash) : ".";
    Expression::replaceText(path, "\\", "/");
    name = (slash != std::string::npos) ? name.substr(slash + 1) : name;
    std::string filename = path + "/" + name;
    Loader::setFilePath(filename);

    // Set build path, (set from command line, can be overriden by "_runtimePath_" in source code)
    Compiler::setBuildPath(includepath, Loader::getFilePath());

#ifdef _WIN32
    Cpu::enableWin32ConsoleSaveFile(false);
#endif

    if(!Compiler::compile(filename, output)) return 1;
    uint16_t execAddress = Compiler::getUserCodeStart();
    if(!Assembler::assemble(output, execAddress, true)) return 1;
    if(!Validater::checkRuntimeVersion()) return 1;

    // Create gt1 format
    Loader::Gt1File gt1File;
    gt1File._loStart = LO_BYTE(execAddress);
    gt1File._hiStart = HI_BYTE(execAddress);
    Loader::Gt1Segment gt1Segment;
    gt1Segment._loAddress = LO_BYTE(execAddress);
    gt1Segment._hiAddress = HI_BYTE(execAddress);

    Assembler::ByteCode byteCode;
    while(!Assembler::getNextAssembledByte(byteCode))
    {
        if(byteCode._isRomAddress)
        {
            Cpu::reportError(Cpu::UtlError, stderr, "Couldn't assemble %s : contains Native code\n", name.c_str());
            return 1;
        }

        // Custom address
        if(byteCode._isCustomAddress)
        {
            if(gt1Segment._dataBytes.size())
            {
                // Previous segment
                gt1Segment._segmentSize = uint8_t(gt1Segment._dataBytes.size());
                gt1File._segments.push_back(gt1Segment);
                gt1Segment._dataBytes.clear();
            }

            gt1Segment._isRomAddress = byteCode._isRomAddress;
            gt1Segment._loAddress = LO_BYTE(byteCode._address);
            gt1Segment._hiAddress = HI_BYTE(byteCode._address);
        }

        gt1Segment._dataBytes.push_back(byteCode._data);
    }

    // Last segment
    if(gt1Segment._dataBytes.size())
    {
        gt1Segment._segmentSize = uint8_t(gt1Segment._dataBytes.size());
        gt1File._segments.push_back(gt1Segment);
    }

    std::string gt1FileName;
    if(!saveGt1File(filename, gt1File, gt1FileName))
    {
        Cpu::reportError(Cpu::UtlError, stderr, "Couldn't save %s : file system error\n", gt1FileName.c_str());
        return 1;
    }

    Loader::printGt1Stats(gt1FileName, gt1File, true);

    return 0;
}
