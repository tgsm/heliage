#include <imgui/imgui.h>
#include <imgui/examples/imgui_impl_sdl.h>
#include <imgui/examples/imgui_impl_opengl2.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <filesystem>
#include <thread>
#include "../cartridge.h"
#include "../gb.h"
#include "../joypad.h"
#include "../logging.h"
#include "../ppu.h"
#include "../types.h"

std::thread emu_thread;
std::array<u32, 160 * 144> fb;
bool done = false;
bool power = true;

bool debugger_draw_background = true;
bool debugger_draw_window = true;
bool debugger_draw_sprites = true;

void FramebufferToTexture(int* texture_width, int* texture_height, GLuint* framebuffer_texture) {
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 160, 144, 0, GL_RGBA, GL_UNSIGNED_BYTE, fb.data());

    *framebuffer_texture = texture;
    *texture_width = 160 * 2;
    *texture_height = 144 * 2;
}

u32 GetRGBAColor(u8 color) {
    u8 result = ~(color * 0x55);
    return 0xFF << 24 | (result << 16) | (result << 8) | result;
}

void DrawFramebuffer(std::array<PPU::Color, 160 * 144>& framebuffer) {
    for (u32 i = 0; i < framebuffer.size(); i++) {
        fb[i] = GetRGBAColor(static_cast<u8>(framebuffer[i]));
    }

    SDL_Delay(1000 / 60);
}

void HandleEvents(Joypad* joypad) {
#define KEYDOWN(k, button) if (ImGui::IsKeyPressed(k)) joypad->PressButton(Joypad::Button::button)
    KEYDOWN(SDL_SCANCODE_UP, Up);
    KEYDOWN(SDL_SCANCODE_DOWN, Down);
    KEYDOWN(SDL_SCANCODE_LEFT, Left);
    KEYDOWN(SDL_SCANCODE_RIGHT, Right);
    KEYDOWN(SDL_SCANCODE_A, A);
    KEYDOWN(SDL_SCANCODE_S, B);
    KEYDOWN(SDL_SCANCODE_BACKSPACE, Select);
    KEYDOWN(SDL_SCANCODE_RETURN, Start);
#undef KEYDOWN

#define KEYUP(k, button) if (ImGui::IsKeyReleased(k)) joypad->ReleaseButton(Joypad::Button::button)
    KEYUP(SDL_SCANCODE_UP, Up);
    KEYUP(SDL_SCANCODE_DOWN, Down);
    KEYUP(SDL_SCANCODE_LEFT, Left);
    KEYUP(SDL_SCANCODE_RIGHT, Right);
    KEYUP(SDL_SCANCODE_A, A);
    KEYUP(SDL_SCANCODE_S, B);
    KEYUP(SDL_SCANCODE_BACKSPACE, Select);
    KEYUP(SDL_SCANCODE_RETURN, Start);
#undef KEYUP
}

void Run(GB* gb) {
    while (!done && power) {
        gb->Run();
    }
}

int main_imgui(char* argv[]) {
    std::filesystem::path bootrom_path = argv[1];
    std::filesystem::path cartridge_path = argv[2];

    BootROM bootrom(bootrom_path);
    if (!bootrom.CheckBootROM(bootrom_path)) {
        LFATAL("invalid bootrom");
        return 1;
    }

    Cartridge cartridge(cartridge_path);
    GB gb(bootrom, cartridge);

    if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
    {
        printf("failed to initialize SDL: %s\n", SDL_GetError());
        return 1;
    }

    // Setup window
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window* window = SDL_CreateWindow("heliage", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer bindings
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL2_Init();

    // Our state
    bool show_demo_window = true;
    ImVec4 clear_color = ImVec4(0.25f, 0.00f, 0.06f, 1.00f);

    // Main loop
    while (!done)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL2_NewFrame();
        ImGui_ImplSDL2_NewFrame(window);
        ImGui::NewFrame();

        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                ImGui::MenuItem("Exit", "Alt+F4", nullptr);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("System")) {
                ImGui::MenuItem("Power", "", &power);
                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        // if (show_demo_window)
        //     ImGui::ShowDemoWindow(&show_demo_window);

        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
        {
            ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

            ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
            ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::End();
        }

        {
            ImGui::Begin("Screen");

            int texture_width = 0;
            int texture_height = 0;
            GLuint framebuffer_texture = 0;
            FramebufferToTexture(&texture_width, &texture_height, &framebuffer_texture);
            ImGui::Image((void*)(intptr_t)framebuffer_texture, ImVec2(texture_width, texture_height));

            ImGui::End();
        }

        {
            ImGui::Begin("Debugger");

            if (ImGui::Button("Step")) {
                gb.Run();
            }

            ImGui::Separator();

            if (ImGui::Checkbox("Draw background", &debugger_draw_background)) {
                gb.GetPPU()->SetBGDrawingEnabled(debugger_draw_background);
            }

            if (ImGui::Checkbox("Draw window", &debugger_draw_window)) {
                gb.GetPPU()->SetWindowDrawingEnabled(debugger_draw_window);
            }

            if (ImGui::Checkbox("Draw sprites", &debugger_draw_sprites)) {
                gb.GetPPU()->SetSpriteDrawingEnabled(debugger_draw_sprites);
            }

            ImGui::End();
        }

        {
            ImGui::Begin("Memory viewer");

            for (u32 i = 0x0000; i < 0x10000; i += 0x10) {
                ImGui::Text("%04X ", i);
                for (u16 j = 0x0; j < 0x10; j++) {
                    u8 byte = gb.GetBus()->Read8(static_cast<u16>(i + j), false);
                    ImGui::SameLine();
                    ImGui::Text("%02X", byte);
                }
            }

            ImGui::End();
        }

        if (power) {
            if (!emu_thread.joinable()) {
                emu_thread = std::thread(&Run, &gb);
            }
        }

        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        //glUseProgram(0); // You may want this if using this code in an OpenGL 3+ context where shaders may be bound
        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    if (emu_thread.joinable()) {
        emu_thread.join();
    }

    // Cleanup
    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
