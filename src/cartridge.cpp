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
    LINFO("  MBC type: %s", GetMBCType(rom[0x147]));
    LINFO("  ROM size: 0x%02X", rom[0x148]);
    LINFO("  RAM size: 0x%02X", rom[0x149]);
    LINFO("  destination code: 0x%02X", rom[0x14A]);
    LINFO("  old licensee code: 0x%02X", rom[0x14B]);
    LINFO("  mask ROM version: 0x%02X", rom[0x14C]);
    LINFO("  complement checksum: 0x%02X", rom[0x14D] & 0xFF);
    LINFO("  checksum: 0x%02X%02X", rom[0x14E] & 0xFF, rom[0x14F] & 0xFF);

    if (rom[0x147] != 0x00) {
        LWARN("this cartridge uses an unimplemented MBC type: %s (0x%02X)", GetMBCType(rom[0x147]), rom[0x147]);
    }
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

const char* Cartridge::GetMBCType(u8 value) {

    switch (value) {
#define MBC(value, mbc) case value: return mbc
        MBC(0x00, "ROM only");
        MBC(0x01, "MBC1");
        MBC(0x02, "MBC1 + RAM");
        MBC(0x03, "MBC1 + RAM + Battery");
        MBC(0x05, "MBC2");
        MBC(0x06, "MBC2 + Battery");
        MBC(0x08, "ROM + RAM");
        MBC(0x09, "ROM + RAM + Battery");
        MBC(0x0B, "MMM01");
        MBC(0x0C, "MMM01 + RAM");
        MBC(0x0D, "MMM01 + RAM + Battery");
        MBC(0x0F, "MBC3 + Timer + Battery");
        MBC(0x10, "MBC3 + Timer + RAM + Battery");
        MBC(0x11, "MBC3");
        MBC(0x12, "MBC3 + RAM");
        MBC(0x13, "MBC3 + RAM + Battery");
        MBC(0x19, "MBC5");
        MBC(0x1A, "MBC5 + RAM");
        MBC(0x1B, "MBC5 + RAM + Battery");
        MBC(0x1C, "MBC5 + Rumble");
        MBC(0x1D, "MBC5 + Rumble + RAM");
        MBC(0x1E, "MBC5 + Rumble + RAM + Battery");
        MBC(0x20, "MBC6");
        MBC(0x22, "MBC7 + Sensor + Rumble + RAM + Battery");
        MBC(0xFC, "Pocket Camera");
        MBC(0xFD, "Bandai TAMA5");
        MBC(0xFE, "HuC3");
        MBC(0xFF, "HuC1 + RAM + Battery");
#undef MBC
        default:
            return "Unknown";
    }
}

u8 Cartridge::Read8(u16 addr) {
    return rom[addr];
}