#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include <inttypes.h>
#include <map>
#include <vector>
#include <string>
#include <algorithm>


#define MAJOR_VERSION "1.2"
#define MINOR_VERSION "1R"
#define VERSION_STR "gtemuAT67 v" MAJOR_VERSION "." MINOR_VERSION
#define RUNTIME_VERSION 121 // this must match RUNTIME_VERSION in runtime/util.i
 
#define ROM_INST 0
#define ROM_DATA 1

#define NUM_INT_ROMS 5

#define ROM_TITLE_ADDRESS 0xFEB1
#define MAX_TITLE_CHARS   25

#define BOOT_COUNT 0x0004
#define BOOT_CHECK 0x0005

#define STACK_POINTER 0x001C

#define VIDEO_MODE_D  0x000D
#define VIDEO_MODE_B  0x001F
#define VIDEO_MODE_C  0x0020
#define VIDEO_VRETURN 0x001E

#define VBLANK_PROC 0x01F6
#define VIDEO_TOP   0x01F9

#define VCPU_SOFT_RESET0 0x00C9
#define VCPU_SOFT_RESET1 0x00EC
#define VCPU_SOFT_RESET2 0x01F0
#define VCPU_RESET_SIZE0 33
#define VCPU_RESET_SIZE1 18
#define VCPU_RESET_SIZE2 6

#define ROM_TYPE               0x0021
#define ROM_TYPE_MASK          0x00FC
#define ROM_VCPU_DISPATCH      0x0309
#define ROM_VCPU_DISPATCH_P3   0x2209
#define ROM_VCPU_DISPATCH_P2   0x2309
#define ROM_VCPU_DISPATCH_P1   0x2409
#define ROM_ROMvX0_INTERPRETER 0x003B
#define ROM_ACTIVE_INTERPRETER 0x0047
#define ROM_OLD_INTERPRETER    0x004F
#define ROM_EXT_SIZE           4


#if defined(_WIN32)
#define _EXIT_(f)              \
    do                         \
    {                          \
        (void)!system("pause");\
        exit(f);               \
    }                          \
    while(0)
#else
#define _EXIT_(f)                                                           \
    do                                                                      \
    {                                                                       \
        (void)!system("echo \"Press ENTER to continue . . .\"; read input");\
        exit(f);                                                            \
    }                                                                       \
    while(0)
#endif

#if defined(_WIN32)
#define _PAUSE_ (void)!system("pause")
#else
#define _PAUSE_ (void)!system("echo \"Press ENTER to continue . . .\"; read input")
#endif

// At least on Windows, _X is a constant defined somewhere before here
#if defined(_X)
#undef _X
#endif

#define UNREFERENCED_PARAM(P) ((void)P)


namespace Cpu
{
    enum RomType {ROMERR=0x00, ROMv1=0x1c, ROMv2=0x20, ROMv3=0x28, ROMv4=0x38, ROMv5a=0x40, ROMvX0=0x80, SDCARD=0xF0, DEVROM=0xF8};
    enum RomTypeAddr {ROMADDRERR=0x0000, ROMADDRv1=0x009A, ROMADDRv2=0x0098, ROMADDRv3=0x0098, ROMADDRv4=0x007E, ROMADDRv5a=0x005E, ROMADDRvX0=0x0000, SDCARDADDR=0x005E, DEVROMADDR=0x005E};
    enum ScanlineMode {Normal=0, VideoB, VideoC, VideoBC, NumScanlineModes};
    enum InternalGt1Id {SnakeGt1=0, RacerGt1=1, MandelbrotGt1=2, PicturesGt1=3, CreditsGt1=4, LoaderGt1=5, NumInternalGt1s};
    enum Endianness {LittleEndian=0x03020100ul, BigEndian=0x00010203ul};
    enum AudioMode {Audio4bit=0, Audio6bit, Audio8bit, NumAudioModes};
    enum ErrorType
    {
        NoError=0, WarnError, CpuError, RomError, RamError, MemError, XprError, OprError, FncError, KwdError, EmuError, SdlError, 
        ImgError, GfxError, AudError, MidError, ModError, AsmError, ComError, LnkError, ValError, OptError, PrgError, TrmError, LodError, UtlError, XxxError
    }; 

    struct State
    {
        uint16_t _PC;
        uint8_t _IR, _D, _AC, _X, _Y, _OUT, _undef;
    };

    struct InternalGt1
    {
        uint16_t _start;
        uint16_t _patch;
        uint16_t _string;
        uint8_t _length; // string length
    };


    int getNumRoms(void);
    int getRomIndex(void);

    ErrorType getCriticalError(void);
    void clearCriticalError(void);

    uint8_t* getPtrToROM(int& romSize);
    RomType getRomType(void);
    std::map<std::string, RomType>& getRomTypeMap(void);
    bool getRomTypeStr(RomType romType, std::string& romTypeStr);

#ifdef _WIN32
    void enableWin32ConsoleSaveFile(bool consoleSaveFile);
#endif
    
    Endianness getHostEndianness(void);
    void swapEndianness(uint16_t& value);
    void swapEndianness(uint32_t& value);
    void swapEndianness(uint64_t& value);

    int reportError(ErrorType error, FILE* file, const char* format, ...);

    void initialiseInternalGt1s(void);

    void patchSYS_Exec_88(void);
    void patchScanlineModeVideoB(void);
    void patchScanlineModeVideoC(void);
    void patchTitleIntoRom(const std::string& title);
    bool patchSplitGt1IntoRom(const std::string& splitGt1path, const std::string& splitGt1name, uint16_t startAddress, InternalGt1Id gt1Id);

    void setMemoryModel(int sizeRAM);

#ifndef STAND_ALONE
    using vCpuPc = std::pair<uint16_t, uint16_t>;

    bool getColdBoot(void);
    bool getIsInReset(void);
    State& getStateS(void);
    State& getStateT(void);
    int64_t getClock(void);
    int64_t getVcpuStall(void);
    uint8_t getIN(void);
    uint8_t getXOUT(void);
    uint16_t getCTRL(void); // extension ctrl register
    uint8_t getXIN(void);   // extension input
    uint16_t getVPC(void);
    uint16_t getOldVPC(void);
    uint8_t getRAM(uint16_t address);
    uint8_t getXRAM(uint32_t address);
    uint8_t getROM(uint16_t address, int page);
    uint16_t getRAM16(uint16_t address);
    uint16_t getXRAM16(uint32_t address);
    uint16_t getROM16(uint16_t address, int page);
    float getvCpuUtilisation(void);

    void setColdBoot(bool coldBoot);
    void setIsInReset(bool isInReset);
    void setClock(int64_t clock);
    void setvCpuStall(int64_t vCpuStall);
    void setIN(uint8_t in);
    void setCTRL(uint16_t ctrl);
    void setXIN(uint8_t xin);
    void setRAM(uint16_t address, uint8_t data);
    void setXRAM(uint32_t address, uint8_t data);
    void setROM(uint16_t base, uint16_t address, uint8_t data);
    void setRAM16(uint16_t address, uint16_t data);
    void setXRAM16(uint32_t address, uint16_t data);
    void setROM16(uint16_t base, uint16_t address, uint16_t data);
    void setSizeRAM(int size);

    bool atVpcDispatchAddress(const State& S);
    std::vector<std::string>& getVcpuTrace(void);
    void saveVcpuTrace(void);

    void initRomType(void);
    void initInterpreter(void);

    void saveScanlineModes(void);
    void restoreScanlineModes(void);
    void swapScanlineMode(void);

    void loadRom(int index);
    void swapRom(void);

    void initialise(void);
    void shutdown(void);
    void cycle(const State& S, State& T);
    void reset(bool coldBoot=false);
    void softReset(void);
    void swapMemoryModel(void);
    void resetVcpuStall(void);
    bool process(bool disableInput=false, bool disableOutput=false);

#ifdef _WIN32
    void restoreWin32Console(void);
    void saveWin32Console(void);
#endif

    // Experimental, (emulation only, for now)
    void enableAudioMode(RomType romType, AudioMode audioMode);
#endif
}

#endif
