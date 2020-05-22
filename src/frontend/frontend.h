#pragma once

#if defined(HELIAGE_FRONTEND_SDL)
#include "sdl.h"
#else

#include <array>
#include "../joypad.h"
#include "../ppu.h"

void DrawFramebuffer(std::array<PPU::Color, 160 * 144>& framebuffer) {
    (void)framebuffer;
}

void HandleEvents(Joypad* joypad) {
    (void)joypad;
}

#endif
