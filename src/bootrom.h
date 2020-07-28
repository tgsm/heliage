#pragma once

#include <array>
#include <filesystem>
#include "types.h"

// TODO: CGB?
constexpr u32 BOOTROM_SIZE = 256; // 256 bytes

class BootROM {
public:
    BootROM(std::filesystem::path& bootrom_path);
    bool CheckBootROM(std::filesystem::path& bootrom_path);
    void LoadBootROM(std::filesystem::path& bootrom_path);

    u8 Read(u16 addr);
private:
    std::array<u8, BOOTROM_SIZE> bootrom;
};
