#pragma once

#include <array>
#include "cartridge.h"
#include "ppu.h"
#include "types.h"

class Bus {
public:
    Bus(Cartridge& cartridge, PPU& ppu);

    u8 Read8(u16 addr);
    void Write8(u16 addr, u8 value);

    u16 Read16(u16 addr);
    void Write16(u16 addr, u16 value);

    u8 ReadIO(u16 addr);
    void WriteIO(u16 addr, u8 value);

    bool IsBootROMActive();

    void DumpMemoryToFile();
private:
    std::array<u8, 0x10000> memory;

    Cartridge& cartridge;
    PPU& ppu;
};