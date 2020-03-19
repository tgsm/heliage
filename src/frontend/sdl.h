#pragma once

#include "../ppu.h"
#include "../types.h"

void HandleSDLEvents();
u32 GetARGBColor(PPU::Color pixel);
void DrawFramebuffer(PPU::Color* framebuffer);
void Shutdown();
int main_SDL(char* argv[]);
