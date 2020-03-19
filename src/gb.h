#pragma once

#include "bootrom.h"
#include "cartridge.h"
#include "bus.h"
#include "ppu.h"
#include "sm83.h"

class GB {
public:
    GB(BootROM bootrom, Cartridge cartridge);

    void Run();

    Bus GetBus();
private:
    Bus bus;
    PPU ppu;
    SM83 sm83;

    u64 cycles;
};