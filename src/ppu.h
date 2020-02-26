#pragma once

#include "types.h"

class Bus;

class PPU {
public:
    PPU(Bus& bus);

    void Tick(u8 cycles);
    void DrawBackground();

    u8 GetLY();
private:
    Bus& bus;
    u64 vcycles;
    u8 ly;
};