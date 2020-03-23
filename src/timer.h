#pragma once

#include "types.h"

class Timer {
public:
    Timer(Bus& bus);

    void Tick(u64 cycles_to_add);

    u8 GetDivider();
    void ResetDivider();
    u8 GetTIMA();
    void SetTIMA(u8 value);
    u8 GetTMA();
    void SetTMA(u8 value);
    u8 GetTAC();
    void SetTAC(u8 value);

private:
    u32 GetTACFrequency();

    u8 div; // divider
    u8 tima; // timer counter
    u8 tma; // timer modulo
    u8 tac; // timer control

    bool timer_enable;

    u64 div_cycles;
    u64 tima_cycles;

    Bus& bus;
};