#pragma once

#include "bus.h"
#include "ppu.h"
#include "sm83.h"

class GB {
public:
    GB(Cartridge cartridge);

    void Run();
private:
    Bus bus;
    PPU ppu;
    SM83 sm83;

    u64 cycles;
};