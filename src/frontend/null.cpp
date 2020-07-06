#include <filesystem>
#include "../bootrom.h"
#include "../cartridge.h"
#include "../logging.h"
#include "null.h"

void HandleEvents([[maybe_unused]] Joypad* joypad) {
}

void DrawFramebuffer([[maybe_unused]] std::array<PPU::Color, 160 * 144>& framebuffer) {
}

int main_null(char* argv[]) {
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

    GB gb(bootrom, cartridge);

    while (true) {
        gb.Run();
    }

    return 0;
}
