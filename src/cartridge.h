#pragma once

#include <filesystem>
#include <vector>
#include "common/types.h"

class Cartridge {
public:
    Cartridge(std::filesystem::path& cartridge_path);

    void PrintMetadata();
    std::string GetGameTitle();
    u8 GetMBCType();
    const char* GetMBCTypeString();
    const char* GetROMSizeString();
    const char* GetRAMSizeString();
    bool CheckNintendoLogo();
    u8 CalculateHeaderChecksum();
    u16 CalculateROMChecksum();

    u8 Read(u32 addr);
private:
    void LoadCartridge(std::filesystem::path& cartridge_path);
    std::vector<u8> rom;
    u32 rom_size = 0; // ROMs range from 32KB to 8MB
};
