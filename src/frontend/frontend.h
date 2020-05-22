#pragma once

#if defined(HELIAGE_FRONTEND_SDL)
#include "sdl.h"
#elif defined(HELIAGE_FRONTEND_IMGUI)
#include "imgui.h"
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
