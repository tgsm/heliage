#pragma once

#include "types.h"

class Timer {
public:
    Timer(Bus& bus, PPU& ppu);

    void Tick();

    u8 GetDivider();
    void ResetDivider();
    u8 GetTIMA();
    void SetTIMA(u8 value);
    u8 GetTMA();
    void SetTMA(u8 value);
    u8 GetTAC();
    void SetTAC(u8 value);

    void AdvanceCycles(u64 cycles);

private:
    u32 GetTACFrequency();

    u16 cycle_count = 0x0000; // divider is pulled from the upper 8 bits of this
    u8 tima = 0x00; // timer counter
    u8 tma = 0x00; // timer modulo
    u8 tac = 0x00; // timer control

    bool timer_enable = false;

    u64 tima_cycles = 0;

    Bus& bus;
    PPU& ppu;
};
