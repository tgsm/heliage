#pragma once

#include "bootrom.h"
#include "bus.h"
#include "cartridge.h"
#include "joypad.h"
#include "ppu.h"
#include "sm83.h"
#include "timer.h"

class GB {
public:
    GB(BootROM bootrom, Cartridge cartridge);

    void Run();

    Bus* GetBus();
    Joypad* GetJoypad();
private:
    Bus bus;
    Joypad joypad;
    PPU ppu;
    SM83 sm83;
    Timer timer;

    u64 cycles;
};