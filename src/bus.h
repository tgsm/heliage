#pragma once

#include <array>
#include "bootrom.h"
#include "cartridge.h"
#include "joypad.h"
#include "ppu.h"
#include "timer.h"
#include "types.h"

class Bus {
public:
    Bus(BootROM& bootrom, Cartridge& cartridge, Joypad& joypad, PPU& ppu, Timer& timer);

    u8 Read8(u16 addr);
    void Write8(u16 addr, u8 value);
    void WriteMBC(u8 mbc_type, u16 addr, u8 value);

    u16 Read16(u16 addr);
    void Write16(u16 addr, u16 value);

    u8 ReadIO(u16 addr);
    void WriteIO(u16 addr, u8 value);

    void DumpMemoryToFile();

    Joypad* GetJoypad();
private:
    bool boot_rom_enabled;

    std::array<u8, 0x10000> memory;

    bool mbc_ram_enabled = false;
    u8 mbc1_rom_bank_lo = 0x01;
    u8 mbc1_rom_bank_hi = 0x00;
    u8 mbc1_bank_mode = 1;
    u8 mbc1_ram_bank = 0;

    u8 mbc3_rom_bank = 0x01;

    BootROM bootrom;
    Cartridge cartridge;
    Joypad& joypad;
    PPU& ppu;
    Timer& timer;
};