#pragma once

#include "mmu.h"
#include "ppu.h"
#include "sm83.h"

class GB {
public:
    GB(Cartridge cartridge);

    void Run();
private:
    MMU mmu;
    PPU ppu;
    SM83 sm83;

    u64 cycles;
};