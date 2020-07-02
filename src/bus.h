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

    u8 Read8(u16 addr, bool affect_timer = true);
    void Write8(u16 addr, u8 value, bool affect_timer = true);
    void WriteMBC(u8 mbc_type, u16 addr, u8 value);

    u8 ReadIO(u8 addr);
    void WriteIO(u8 addr, u8 value);

    void DumpMemoryToFile();

    Joypad* GetJoypad();
private:
    void LoadInitialValues();

    bool boot_rom_enabled;

    std::array<u8, 0x2000> vram;
    // TODO: make this a std::vector
    std::array<u8, 0x2000> cartridge_ram;
    std::array<u8, 0x2000> wram;
    std::array<u8, 0xA0> oam;
    std::array<u8, 0x80> io;
    std::array<u8, 0x7F> hram;
    u8 ie;

    bool mbc_ram_enabled = false;
    u8 mbc1_bank1 = 0x01;
    u8 mbc1_bank2 = 0x00;
    u8 mbc1_mode = 1;
    u8 mbc1_ram_bank = 0;

    u8 mbc3_rom_bank = 0x01;

    BootROM bootrom;
    Cartridge cartridge;
    Joypad& joypad;
    PPU& ppu;
    Timer& timer;
};
