#pragma once

#include <array>
#include "../gb.h"
#include "../joypad.h"
#include "../ppu.h"

// Unused
void HandleEvents([[maybe_unused]] Joypad* joypad);
void DrawFramebuffer([[maybe_unused]] std::array<PPU::Color, 160 * 144>& framebuffer);

int main_null(char* argv[]);
