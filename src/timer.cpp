#include "bus.h"
#include "logging.h"
#include "timer.h"

Timer::Timer(Bus& bus)
    : bus(bus) {
    div = 0x00;
    tima = 0x00;
    tma = 0x00;
    tac = 0x00;
    timer_enable = false;
    div_cycles = 0;
    tima_cycles = 0;
}

void Timer::Tick(u64 cycles_to_add) {
    if (timer_enable) {
        tima_cycles += cycles_to_add;

        if (tima_cycles >= GetTACFrequency()) {
            tima_cycles -= GetTACFrequency();
            tima++;
            
            // overflow
            if (tima == 0) {
                tima = tma;
                // request a timer interrupt
                bus.Write8(0xFF0F, bus.Read8(0xFF0F) | 0x4);
                // LFATAL("Setting tima to 0x%02X", tma);
            }
        }
    }

    div_cycles += cycles_to_add;
    if (div_cycles >= 256) {
        div_cycles -= 256;
        div++;
    }
}

u8 Timer::GetDivider() {
    return div;
}

void Timer::ResetDivider() {
    div = 0x00;
}

u8 Timer::GetTIMA() {
    return tima;
}

void Timer::SetTIMA(u8 value) {
    tima = value;
}

u8 Timer::GetTMA() {
    return tma;
}

void Timer::SetTMA(u8 value) {
    tma = value;
}

u8 Timer::GetTAC() {
    return tac;
}

void Timer::SetTAC(u8 value) {
    tac = value;
    timer_enable = (tac & (1 << 2));
}

u32 Timer::GetTACFrequency() {
    switch (tac & 0b11) {
        case 0b00: return 1024;
        case 0b01: return 16;
        case 0b10: return 64;
        case 0b11: return 256;
        default:
            LERROR("unreachable TAC frequency");
            return 1;
    }
}