#pragma once

#include <filesystem>
#include <vector>
#include "common/types.h"

class Cartridge {
public:
    Cartridge(std::filesystem::path& cartridge_path);

    void PrintMetadata();
    std::string GetGameTitle() const;
    u8 GetMBCType() const;
    const char* GetMBCTypeString() const;
    const char* GetROMSizeString() const;
    const char* GetRAMSizeString() const;
    bool CheckNintendoLogo() const;
    u8 CalculateHeaderChecksum() const;
    u16 CalculateROMChecksum() const;

    u8 Read(u32 addr) const;
private:
    void LoadCartridge(std::filesystem::path& cartridge_path);
    std::vector<u8> rom;
    u32 rom_size = 0; // ROMs range from 32KB to 8MB
};
