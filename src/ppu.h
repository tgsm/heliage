#pragma once

#include "types.h"

class MMU;

class PPU {
public:
    PPU(MMU& mmu);

    void Tick(u8 cycles);
    void DrawBackground();

    u8 GetLY();
private:
    MMU& mmu;
    u64 vcycles;
    u8 ly;
};