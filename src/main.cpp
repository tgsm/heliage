#include <cstdio>
#include "frontend/frontend.h"

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("usage: %s <bootrom> <cartridge>\n", argv[0]);
        return 1;
    }

#if defined(HELIAGE_FRONTEND_SDL)
    return main_SDL(argv);
#elif defined(HELIAGE_FRONTEND_IMGUI)
    return main_imgui(argv);
#else
    return main_null(argv);
#endif
}
