#include "gb.h"
#include "logging.h"
#include "sm83.h"
#include "ppu.h"

GB::GB(Cartridge cartridge)
    : mmu(cartridge, ppu), ppu(mmu), sm83(mmu) {
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

    mmu.DumpMemoryToFile();
}