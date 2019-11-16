#include <cstdio>
#include <string>
#include "cartridge.h"
#include "gb.h"
#include "logging.h"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("usage: %s <cartridge>\n", argv[0]);
        return 1;
    }

    std::string cart_filename = argv[1];

    LINFO("loading cartridge: %s", cart_filename.c_str());
    Cartridge cartridge(cart_filename);

    GB gb(cartridge);
    gb.Run();

    return 0;
}