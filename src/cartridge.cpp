#include "cartridge.h"
#include "logging.h"

Cartridge::Cartridge(const std::string filename) {
    std::vector<char> data = ReadFileBytes(filename);
    LINFO("loaded %u bytes (%u KB)", data.size(), data.size() / 1024);

    rom = data;

    LINFO("some metadata:");
    LINFO("  title: %s", GetGameTitle(rom).c_str());
    LINFO("  manufacturer code: 0x%02X%02X%02X%02X", rom[0x13F], rom[0x140], rom[0x141], rom[0x142]);
    LINFO("  GBC compatibility: 0x%02X", rom[0x143] & 0xFF);
    LINFO("  new licensee code: 0x%02X%02X", rom[0x144], rom[0x145]);
    LINFO("  SGB compatibility: 0x%02X", rom[0x146]);
    LINFO("  MBC type: 0x%02X", rom[0x147]);
    LINFO("  ROM size: 0x%02X", rom[0x148]);
    LINFO("  RAM size: 0x%02X", rom[0x149]);
    LINFO("  destination code: 0x%02X", rom[0x14A]);
    LINFO("  old licensee code: 0x%02X", rom[0x14B]);
    LINFO("  mask ROM version: 0x%02X", rom[0x14C]);
    LINFO("  complement checksum: 0x%02X", rom[0x14D] & 0xFF);
    LINFO("  checksum: 0x%02X%02X", rom[0x14E] & 0xFF, rom[0x14F] & 0xFF);
}

std::vector<char> Cartridge::ReadFileBytes(const std::string filename) {
    std::ifstream stream(filename.c_str(), std::ios::ate | std::ios::binary);
    std::ifstream::pos_type position = stream.tellg();
    std::vector<char> file(position);

    stream.seekg(0, std::ios::beg);
    stream.read(&file[0], position);

    return file;
}

std::string Cartridge::GetGameTitle(std::vector<char> rom) {
    char title[0x10] = {};

    for (int i = 0; i < 0x10; i++) {
        title[i] = rom[0x134 + i];
    }

    return std::string(title);
}

u8 Cartridge::Read8(u16 addr) {
    return rom[addr];
}