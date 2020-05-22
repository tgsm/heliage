#include <cstdio>
#include "frontend/frontend.h"

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("usage: %s <bootrom> <cartridge>\n", argv[0]);
        return 1;
    }

    // TODO: make an imgui frontend
#if defined(HELIAGE_FRONTEND_SDL)
    return main_SDL(argv);
#else
    printf("you didn't #define a frontend! :(\n");
    return 0;
#endif
}
