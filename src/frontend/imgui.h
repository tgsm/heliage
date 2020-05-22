#pragma once

#include <array>
#include <thread>
#include "../joypad.h"
#include "../ppu.h"
#include "../types.h"

void DrawFramebuffer(std::array<PPU::Color, 160 * 144>& framebuffer);
void HandleEvents(Joypad* joypad);
int main_imgui(char* argv[]);
