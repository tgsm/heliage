#include <fstream>
#include "bootrom.h"
#include "logging.h"
#include "types.h"

BootROM::BootROM(std::filesystem::path& bootrom_path) {
    bootrom.fill(0xFF);
    LoadBootROM(bootrom_path);
}

bool BootROM::CheckBootROM(std::filesystem::path& bootrom_path) {
    if (!std::filesystem::is_regular_file(bootrom_path)) {
        return false;
    }

    if (std::filesystem::file_size(bootrom_path) != BOOTROM_SIZE) {
        return false;
    }

    return true;
}

void BootROM::LoadBootROM(std::filesystem::path& bootrom_path) {
    std::ifstream stream(bootrom_path.string().c_str(), std::ios::binary);
    ASSERT_MSG(stream.is_open(), "could not open bootROM: {}", bootrom_path.string().c_str());

    stream.read(reinterpret_cast<char*>(bootrom.data()), bootrom.size());
    LINFO("bootrom: loaded {} bytes", std::filesystem::file_size(bootrom_path));
}

u8 BootROM::Read(u16 addr) {
    return bootrom.at(addr);
}
