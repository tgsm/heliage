#include "gb.h"
#include "logging.h"
#include "sm83.h"
#include "ppu.h"

GB::GB(Cartridge cartridge)
    : bus(cartridge, ppu), ppu(bus), sm83(bus) {
    LINFO("powering on...");
    cycles = 0;
}

void GB::Run() {
    for (u64 i = 0; i < 250000000; i++) {
    // while (true) {
        u8 c = sm83.Tick();
        cycles += c;
        ppu.Tick(c);
    }

    bus.DumpMemoryToFile();
}