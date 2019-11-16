#include "gb.h"
#include "logging.h"
#include "sm83.h"

GB::GB(Cartridge cartridge)
    : mmu(cartridge), sm83(mmu) {
    LINFO("powering on...");
}

void GB::Run() {
    // for (int i = 0; i < 1000000; i++) {
    while (true) {
        sm83.Tick();
    }

    mmu.DumpMemoryToFile();
}