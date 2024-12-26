#include "bus.h"
#include "logging.h"
#include "timer.h"

Timer::Timer(Bus& bus, PPU& ppu)
    : bus(bus), ppu(ppu) {
}

void Timer::Tick() {
    if (timer_enable) {
        while (tima_cycles >= GetTACFrequency()) {
            tima_cycles -= GetTACFrequency();
            tima++;
        }

        // overflow
        if (tima == 0) {
            tima = tma;
            // request a timer interrupt
            bus.Write8(0xFF0F, bus.Read8(0xFF0F, false) | 0x4, false);
        }
    }
}

u8 Timer::GetDivider() {
    // DIV is really just the upper 8 bits of the cycle counter.
    return (cycle_count >> 8) & 0xFF;
}

void Timer::ResetDivider() {
    cycle_count = 0x0000;
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
            UNREACHABLE_MSG("invalid TAC frequency {}", tac);
    }
}

void Timer::AdvanceCycles(u64 cycles) {
    cycle_count += cycles;

    if (timer_enable) {
        tima_cycles += cycles;
    }

    for (u64 i = 0; i < cycles / 4; i++) {
        if (bus.IsOAMDMAActive()) {
            bus.RunOAMDMATransferCycle();
        }
    }

    for (u64 i = 0; i < cycles; i++) {
        Tick();
    }

    ppu.AdvanceCycles(cycles);
}
