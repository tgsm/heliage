#include <cmath>
#include "logging.h"
#include "mmu.h"
#include "ppu.h"

PPU::PPU(MMU& mmu)
    : mmu(mmu) {
    vcycles = 0;
    ly = 0x00;
}

void PPU::Tick(u8 cycles) {
    if (cycles == 0) {
        LWARN("The executed instruction advanced zero cycles");
    }
    vcycles += cycles;

    ly = floor(vcycles / 456);
    mmu.Write8(0xFF44, ly);

    if (vcycles >= 70220) {
        LTRACE("redraw!");
        vcycles = 0;
    }
}

void PPU::DrawBackground() {
    for (u16 i = 0x8000; i < 0xA000; i += 2) {
        u8 line1 = mmu.Read8(i);
        u8 line2 = mmu.Read8(i + 1);
        for (u8 bit = 7; bit > 0; bit--) {

        }
    }
}

u8 PPU::GetLY() {
    return ly;
}