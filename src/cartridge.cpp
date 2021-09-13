#include <fstream>
#include "cartridge.h"
#include "logging.h"

Cartridge::Cartridge(std::filesystem::path& cartridge_path) {
    LoadCartridge(cartridge_path);
    // PrintMetadata();

    bool logo_check = CheckNintendoLogo();
    if (!logo_check) {
        LERROR("Nintendo logo is wrong, this game will not make it past the bootrom");
    }

    u8 header_checksum = CalculateHeaderChecksum();
    if (header_checksum != rom.at(0x14D) ) {
        LERROR("header checksum is wrong, this game will not make it past the bootrom (expected 0x{:02X}, got 0x{:02X})", header_checksum, rom.at(0x14D));
    }

    u16 rom_checksum = CalculateROMChecksum();
    if (((rom_checksum >> 8) & 0xFF) != rom.at(0x14E) && (rom_checksum & 0xFF) != rom.at(0x14F)) {
        LWARN("ROM checksum is wrong, however a real gameboy does not check this (expected 0x{:04X}, got 0x{:02X})", rom_checksum, rom.at(0x14E), rom.at(0x14F));
    }
}

void Cartridge::LoadCartridge(std::filesystem::path& cartridge_path) {
    ASSERT_MSG(std::filesystem::is_regular_file(cartridge_path), "ROM is not a regular file");

    rom_size = std::filesystem::file_size(cartridge_path);
    ASSERT_MSG(rom_size <= 8 * 1024 * 1024, "ROM is too big");

    std::ifstream stream(cartridge_path.string().c_str(), std::ios::binary);
    ASSERT_MSG(stream.is_open(), "could not open ROM: {}", cartridge_path.string().c_str());

    rom.resize(rom_size);
    stream.read(reinterpret_cast<char*>(rom.data()), rom.size());

    LINFO("cartridge: loaded {} bytes ({} KB)", rom_size, rom_size / 1024);
}

void Cartridge::PrintMetadata() {
    LINFO("some metadata:");
    LINFO("  title: {}", GetGameTitle());
    LINFO("  manufacturer code: 0x{:02X}", rom.at(0x13F), rom.at(0x140), rom.at(0x141), rom.at(0x142));
    LINFO("  GBC compatibility: 0x{:02X}", rom.at(0x143));
    LINFO("  new licensee code: {:c}{:c}", rom.at(0x144), rom.at(0x145));
    LINFO("  SGB compatibility: 0x{:02X}", rom.at(0x146));
    LINFO("  MBC type: 0x{:02X} ({})", rom.at(0x147), GetMBCTypeString());
    LINFO("  ROM size: 0x{:02X} ({})", rom.at(0x148), GetROMSizeString());
    LINFO("  RAM size: 0x{:02X} ({})", rom.at(0x149), GetRAMSizeString());
    LINFO("  destination code: 0x{:02X}", rom.at(0x14A));
    LINFO("  old licensee code: 0x{:02X}", rom.at(0x14B));
    LINFO("  mask ROM version: 0x{:02X}", rom.at(0x14C));
    LINFO("  header checksum: 0x{:02X}", rom.at(0x14D));
    LINFO("  ROM checksum: 0x{:04X}", rom.at(0x14E), rom.at(0x14F));
}

std::string Cartridge::GetGameTitle() {
    std::array<char, 0x10> title;

    for (int i = 0; i < 0x10; i++) {
        title[i] = rom.at(0x134 + i);
    }

    return std::string(title.data());
}

u8 Cartridge::GetMBCType() {
    return rom.at(0x147);
}

const char* Cartridge::GetMBCTypeString() {
    u8 value = GetMBCType();

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

const char* Cartridge::GetROMSizeString() {
    u8 value = rom.at(0x148);

    switch (value) {
#define ROM(value, size) case value: return size
        ROM(0x00, "32KB, 2 banks");
        ROM(0x01, "64KB, 4 banks");
        ROM(0x02, "128KB, 8 banks");
        ROM(0x03, "256KB, 16 banks");
        ROM(0x04, "512KB, 32 banks");
        ROM(0x05, "1MB, 64 banks");
        ROM(0x06, "2MB, 128 banks");
        ROM(0x07, "4MB, 256 banks");
        ROM(0x08, "8MB, 512 banks");
        ROM(0x52, "1.1MB, 72 banks");
        ROM(0x53, "1.2MB, 80 banks");
        ROM(0x54, "1.5MB, 96 banks");
#undef ROM
        default:
            return "Unknown";
    }
}

const char* Cartridge::GetRAMSizeString() {
    u8 value = rom.at(0x149);

    switch (value) {
#define RAM(value, size) case value: return size
        RAM(0x00, "None");
        RAM(0x01, "2KB");
        RAM(0x02, "8KB");
        RAM(0x03, "32KB, 4 banks");
        RAM(0x04, "128KB, 16 banks");
        RAM(0x05, "64KB, 8 banks");
#undef RAM
        default:
            return "Unknown";
    }
}

bool Cartridge::CheckNintendoLogo() {
    const std::array<u8, 0x30> nintendo_logo = {
        0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B,
        0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D,
        0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E,
        0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99,
        0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC,
        0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E,
    };

    for (u8 i = 0x00; i < 0x30; i++) {
        if (rom[0x0104 + i] != nintendo_logo.at(i)) {
            return false;
        }
    }

    return true;
}

u8 Cartridge::CalculateHeaderChecksum() {
    u8 result = 0x00;
    for (u16 i = 0x0134; i <= 0x014C; i++) {
        result -= rom.at(i);
        result--;
    }

    return result;
}

u16 Cartridge::CalculateROMChecksum() {
    u16 result = 0x0000;

    for (u32 i = 0x00000000; i < rom_size; i++) {
        if (i == 0x014E || i == 0x014F) {
            continue;
        }

        result += rom.at(i);
    }

    return result;
}

u8 Cartridge::Read(u32 addr) {
    return rom.at(addr);
}
