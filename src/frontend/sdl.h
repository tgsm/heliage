#pragma once

#include "../gb.h"
#include "../ppu.h"
#include "../types.h"

void HandleSDLEvents(Joypad* joypad);
u32 GetARGBColor(PPU::Color pixel);
void DrawFramebuffer(PPU::Color* framebuffer);
void Shutdown();
int main_SDL(char* argv[]);
