#include "gb.h"
#include "logging.h"
#include "sm83.h"
#include "ppu.h"

GB::GB(BootROM bootrom, Cartridge cartridge)
    : bus(bootrom, cartridge, joypad, ppu, timer), ppu(bus), sm83(bus, timer), timer(bus, ppu) {
    LINFO("powering on...");
}

void GB::Run() {
    sm83.Tick();
}

Bus* GB::GetBus() {
    return &bus;
}

Joypad* GB::GetJoypad() {
    return &joypad;
}

PPU* GB::GetPPU() {
    return &ppu;
}
