#include <cstdio>
#include "frontend/sdl.h"

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("usage: %s <bootrom> <cartridge>\n", argv[0]);
        return 1;
    }

    // TODO: make an imgui frontend
    return main_SDL(argv);
}