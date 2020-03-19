#include <fstream>
#include "bootrom.h"
#include "logging.h"
#include "types.h"

BootROM::BootROM(std::filesystem::path bootrom_path) {
    std::fill(bootrom.begin(), bootrom.end(), 0xFF);
    LoadBootROM(bootrom_path);
}

bool BootROM::CheckBootROM(std::filesystem::path bootrom_path) {
    if (!std::filesystem::exists(bootrom_path) || std::filesystem::is_directory(bootrom_path)) {
        return false;
    }

    if (std::filesystem::file_size(bootrom_path) != BOOTROM_SIZE) {
        return false;
    }

    return true;
}

void BootROM::LoadBootROM(std::filesystem::path bootrom_path) {
    std::ifstream stream(bootrom_path.c_str(), std::ios::binary);
    if (!stream.is_open()) {
        LFATAL("failed to load bootrom: %s", bootrom_path.c_str());
        std::exit(1);
    }

    stream.read(bootrom.data(), bootrom.size());

    LINFO("bootrom: loaded %ju bytes", std::filesystem::file_size(bootrom_path));
    stream.close();
}

u8 BootROM::Read(u16 addr) {
    return bootrom[addr];
}