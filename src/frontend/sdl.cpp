#include <SDL2/SDL.h>
#include <filesystem>
#include <string>
#include "../bootrom.h"
#include "../cartridge.h"
#include "../logging.h"
#include "sdl.h"

SDL_Window* window;
SDL_Renderer* renderer;
SDL_Texture* framebuffer_output;
SDL_Event event;

bool running = false;

void HandleEvents(Joypad* joypad) {
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_KEYDOWN:
#define KEYDOWN(k, button) if (event.key.keysym.sym == k) joypad->PressButton(Joypad::Button::button)
                KEYDOWN(SDLK_UP, Up);
                KEYDOWN(SDLK_DOWN, Down);
                KEYDOWN(SDLK_LEFT, Left);
                KEYDOWN(SDLK_RIGHT, Right);
                KEYDOWN(SDLK_a, A);
                KEYDOWN(SDLK_s, B);
                KEYDOWN(SDLK_BACKSPACE, Select);
                KEYDOWN(SDLK_RETURN, Start);
#undef KEYDOWN
                break;
            case SDL_KEYUP:
#define KEYUP(k, button) if (event.key.keysym.sym == k) joypad->ReleaseButton(Joypad::Button::button)
                KEYUP(SDLK_UP, Up);
                KEYUP(SDLK_DOWN, Down);
                KEYUP(SDLK_LEFT, Left);
                KEYUP(SDLK_RIGHT, Right);
                KEYUP(SDLK_a, A);
                KEYUP(SDLK_s, B);
                KEYUP(SDLK_BACKSPACE, Select);
                KEYUP(SDLK_RETURN, Start);
#undef KEYUP
                break;
            case SDL_QUIT:
                running = false;
                break;
        }
    }    
}

u32 GetARGBColor(PPU::Color pixel) {
    u8 color = ~(static_cast<u8>(pixel) * 0x55);
    return 0xFF << 24 | color << 16 | color << 8 | color;
}

void DrawFramebuffer(std::array<PPU::Color, 160 * 144>& framebuffer) {
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

    std::string title = "heliage";
    std::string game_title = cartridge.GetGameTitle();
    if (!game_title.empty()) {
        title = "heliage - " + game_title;
    }
    SDL_SetWindowTitle(window, title.c_str());

    running = true;
    while (running) {
        gb.Run();
    }

    gb.GetBus()->DumpMemoryToFile();

    Shutdown();
    return 0;
}
