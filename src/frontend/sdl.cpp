#include <SDL2/SDL.h>
#include <cstdio>
#include <filesystem>
#include <string>
#include "../bootrom.h"
#include "../cartridge.h"
#include "../gb.h"
#include "../logging.h"
#include "sdl.h"

SDL_Window* window;
SDL_Renderer* renderer;
SDL_Texture* framebuffer_output;
SDL_Event event;

bool running = false;

void HandleSDLEvents() {
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            // TODO: handle input
            case SDL_QUIT:
                running = false;
                break;
        }
    }    
}

u32 GetARGBColor(PPU::Color pixel) {
    u8 color = ~(static_cast<u8>(pixel) * 0x55);
    // switch (pixel) {
    //     case PPU::Color::White: color = 0xFF; break;
    //     case PPU::Color::LightGray: color = 0xAA; break;
    //     case PPU::Color::DarkGray: color = 0x55; break;
    //     case PPU::Color::Black: color = 0x00; break;
    // }

    return 0xFF << 24 | color << 16 | color << 8 | color;
}

void DrawFramebuffer(PPU::Color* framebuffer) {
    SDL_RenderClear(renderer);

    void* pixels;

    // we don't use this, but SDL just *needs* us to have it
    int pitch;

    SDL_LockTexture(framebuffer_output, nullptr, &pixels, &pitch);

    u32* pixel_array = static_cast<u32*>(pixels);

    for (unsigned int i = 0; i < 160 * 144; i++) {
        pixel_array[i] = GetARGBColor(framebuffer[i]);
    }

    SDL_UnlockTexture(framebuffer_output);

    SDL_RenderCopy(renderer, framebuffer_output, nullptr, nullptr);
    SDL_RenderPresent(renderer);
}

void Shutdown() {
    LINFO("shutting down SDL");
    SDL_DestroyTexture(framebuffer_output);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    std::exit(0);
}

int main_SDL(char* argv[]) {
    std::filesystem::path bootrom_path = argv[1];
    std::filesystem::path cart_path = argv[2];

    LINFO("loading bootrom: %s", bootrom_path.c_str());
    LINFO("loading cartridge: %s", cart_path.c_str());

    BootROM bootrom(bootrom_path);
    if (!bootrom.CheckBootROM(bootrom_path)) {
        LFATAL("invalid bootrom");
        return 1;
    }

    Cartridge cartridge(cart_path);

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        LFATAL("failed to initialize SDL: %s", SDL_GetError());
        return 1;
    }

    window = SDL_CreateWindow("heliage", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 160 * 2, 144 * 2, 0);
    if (!window) {
        LFATAL("failed to create SDL window: %s", SDL_GetError());
        return 1;
    }

    renderer = SDL_CreateRenderer(window, 0, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        LFATAL("failed to create SDL renderer: %s", SDL_GetError());
        return 1;
    }

    framebuffer_output = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 160, 144);
    if (!framebuffer_output) {
        LFATAL("failed to create framebuffer output texture: %s", SDL_GetError());
        return 1;
    }

    GB gb(bootrom, cartridge);

    running = true;
    while (running) {
        HandleSDLEvents();
        gb.Run();
    }

    gb.GetBus().DumpMemoryToFile();

    Shutdown();
    return 0;
}
