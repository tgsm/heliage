#include "gb.h"
#include "logging.h"
#include "sm83.h"
#include "ppu.h"

GB::GB(BootROM bootrom, Cartridge cartridge)
    : bus(bootrom, cartridge, joypad, ppu, timer), joypad(bus), ppu(bus), sm83(bus), timer(bus) {
    LINFO("powering on...");
    cycles = 0;
}

void GB::Run() {
    u8 c = sm83.Tick();
    cycles += c;
    ppu.Tick(c);
    timer.Tick(c);
}

Bus* GB::GetBus() {
    return &bus;
}

Joypad* GB::GetJoypad() {
    return &joypad;
}
