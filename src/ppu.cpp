#include <cmath>
#include "bus.h"
#include "logging.h"
#include "ppu.h"

PPU::PPU(Bus& bus)
    : bus(bus) {
    vcycles = 0;
    ly = 0x00;
}

void PPU::Tick(u8 cycles) {
    if (cycles == 0) {
        LWARN("The executed instruction advanced zero cycles");
    }
    vcycles += cycles;

    ly = floor(vcycles / 456);
    bus.Write8(0xFF44, ly);

    if (vcycles >= 70220) {
        LTRACE("redraw!");
        vcycles = 0;
    }
}

void PPU::DrawBackground() {
    for (u16 i = 0x8000; i < 0xA000; i += 2) {
        u8 line1 = bus.Read8(i);
        u8 line2 = bus.Read8(i + 1);
        for (u8 bit = 7; bit > 0; bit--) {

        }
    }
}

u8 PPU::GetLY() {
    return ly;
}