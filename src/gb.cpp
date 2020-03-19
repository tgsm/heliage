#include "gb.h"
#include "logging.h"
#include "sm83.h"
#include "ppu.h"

GB::GB(BootROM bootrom, Cartridge cartridge)
    : bus(bootrom, cartridge, ppu), ppu(bus), sm83(bus) {
    LINFO("powering on...");
    cycles = 0;
}

void GB::Run() {
    u8 c = sm83.Tick();
    cycles += c;
    ppu.Tick(c);
}

Bus GB::GetBus() {
    return bus;
}