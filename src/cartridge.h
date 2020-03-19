#pragma once

#include <filesystem>
#include <vector>
#include "types.h"

class Cartridge {
public:
    Cartridge(std::filesystem::path cartridge_path);

    void PrintMetadata();
    std::string GetGameTitle();
    u8 GetMBCType();
    const char* GetMBCTypeString();
    const char* GetROMSizeString(u8 value);
    const char* GetRAMSizeString(u8 value);
    bool CheckNintendoLogo();
    u8 CalculateHeaderChecksum();
    u16 CalculateROMChecksum();

    u8 Read(u32 addr);
private:
    void LoadCartridge(std::filesystem::path cartridge_path);
    u8* rom = {};
    u32 rom_size = 0; // ROMs range from 32KB to 8MB
};
