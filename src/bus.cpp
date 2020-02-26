#include <cstdio>
#include "dmgboot.h"
#include "logging.h"
#include "bus.h"

Bus::Bus(Cartridge& cartridge, PPU& ppu)
    : cartridge(cartridge), ppu(ppu) {
    memory = std::array<u8, 0x10000>();
}

u8 Bus::Read8(u16 addr) {
    if (addr <= 0x7FFF) {
        if (addr < 0x0100 && IsBootROMActive()) {
            return dmgboot[addr];
        }

        return cartridge.Read8(addr);
    }

    // VRAM
    if (addr >= 0x8000 && addr < 0xA000) {
        LDEBUG("reading 0x%02X from 0x%04X (VRAM)", memory[addr], addr);
        return memory[addr];
    }

    if (addr >= 0xA000 && addr < 0xC000) {
        LDEBUG("reading 0x%02X from 0x%04X (External RAM)", memory[addr], addr);
        return memory[addr];
    }

    if (addr >= 0xC000 && addr < 0xE000) {
        LDEBUG("reading 0x%02X from 0x%04X (WRAM)", memory[addr], addr);
        return memory[addr];
    }    

    if (addr >= 0xE000 && addr < 0xFE00) {
        LWARN("reading from echo RAM (0x%02X from 0x%04X)", memory[addr - 0x2000], addr);
        return memory[addr - 0x2000];
    }

    if (addr >= 0xFE00 && addr < 0xFEA0) {
        LDEBUG("reading 0x%02X to 0x%04X (OAM / Sprite Attribute Table)", memory[addr], addr);
        return memory[addr];
    }

    if (addr >= 0xFEA0 && addr < 0xFF00) {
        LWARN("attempted to read from unusable memory (0x%04X)", addr);
        return 0x00;
    }

    if (addr >= 0xFF00 && addr < 0xFF80) {
        return ReadIO(addr);
    }

    // Zero Page
    if (addr >= 0xFF80 && addr < 0xFFFF) {
        LDEBUG("reading 0x%02X from 0x%04X (Zero Page)", memory[addr], addr);
        return memory[addr];
    }

    if (addr == 0xFFFF) {
        LDEBUG("reading 0x%02X from 0xFFFF (Interrupt Enable Flag)", memory[0xFFFF]);
        return memory[0xFFFF];
    }

    LERROR("unrecognized read8 from 0x%04X", addr);
    return 0xFF;
}

void Bus::Write8(u16 addr, u8 value) {
    // VRAM
    if (addr >= 0x8000 && addr < 0xA000) {
        LDEBUG("writing 0x%02X to 0x%04X (VRAM)", value, addr);
        memory[addr] = value;
        return;
    }

    if (addr >= 0xA000 && addr < 0xC000) {
        LDEBUG("writing 0x%02X to 0x%04X (Cartridge RAM)", value, addr);
        memory[addr] = value;
        return;
    }

    if (addr >= 0xC000 && addr < 0xE000) {
        LDEBUG("writing 0x%02X to 0x%04X (WRAM)", value, addr);
        memory[addr] = value;

        if (addr < 0xDE00) {
            memory[addr + 0x2000] = value;
        }

        return;
    }

    if (addr >= 0xE000 && addr < 0xFE00) {
        LWARN("writing to echo RAM (0x%02X to 0x%04X)", value, addr);
        memory[addr] = value;
        memory[addr - 0x2000] = value;
        return;
    }

    if (addr >= 0xFE00 && addr < 0xFEA0) {
        LDEBUG("writing 0x%02X to 0x%04X (OAM / Sprite Attribute Table)", value, addr);
        memory[addr] = value;
        return;
    }

    if (addr >= 0xFEA0 && addr < 0xFF00) {
        LWARN("attempted to write to unusable memory (0x%02X to 0x%04X)", value, addr);
        return;
    }

    if (addr >= 0xFF00 && addr < 0xFF80) {
        WriteIO(addr, value);
        return;
    }

    // Zero Page
    if (addr >= 0xFF80 && addr < 0xFFFF) {
        LDEBUG("writing 0x%02X to 0x%04X (Zero Page)", value, addr);
        memory[addr] = value;
        return;
    }

    if (addr == 0xFFFF) {
        LDEBUG("writing 0x%02X to 0xFFFF (Interrupt Enable)", value);
        memory[0xFFFF] = value;
        return;
    }

    LERROR("unrecognized write8 0x%02X to 0x%04X", value, addr);
    return;
}

u16 Bus::Read16(u16 addr) {
    LERROR("unrecognized read16 from 0x%04X", addr);
    return 0xFFFF;
}

void Bus::Write16(u16 addr, u16 value) {
    u8 high = static_cast<u8>((value >> 4) & 0xFF);
    u8 low = static_cast<u8>(value & 0xFF);

    memory[addr] = low;
    memory[addr + 1] = high;
}

u8 Bus::ReadIO(u16 addr) {
    switch (addr) {
        case 0xFF00:
            LDEBUG("reading 0x%02X from 0xFF00 (Joypad)", 0xFF);
            // LWARN("(hack) all reads from 0xFF00 are currently stubbed to 0xFF");
            return 0xFF;
        case 0xFF0F:
            LDEBUG("reading 0x%02X from 0xFF0F (Interrupt Fetch)", memory[0xFF0F]);
            return memory[0xFF0F];
        case 0xFF44:
            LDEBUG("reading 0x%02X from 0xFF44 (LCD Control Y)", memory[0xFF44]);
            return memory[0xFF44];
        case 0xFF50:
            // boot ROM switch
            // LDEBUG("reading 0x%02X from 0xFF50 (Boot ROM switch)", memory[addr]);
            return memory[0xFF50];
        case 0xFFFF:
            LDEBUG("reading 0x%02X from 0xFFFF (Interrupt Enable)", memory[0xFFFF]);
            return memory[0xFFFF];
        default:
            LDEBUG("reading 0x%02X from 0x%04X (IO)", memory[addr], addr);
            return memory[addr];
    }
}

void Bus::WriteIO(u16 addr, u8 value) {
    switch (addr) {
        case 0xFF01:
            // used by blargg tests
            printf("%c", value);
            return;
        case 0xFF0F:
            LDEBUG("writing 0x%02X to 0xFF0F (Interrupt Fetch)", value);
            memory[0xFF0F] = value;
            break;
        case 0xFF40:
            LDEBUG("writing 0x%02X to 0xFF40 (LCD Control)", value);
            memory[addr] = value;
            return;
        case 0xFF42:
            LDEBUG("writing 0x%02X to 0xFF42 (Scroll Y)", value);
            memory[addr] = value;
            return;
        case 0xFF47:
            LDEBUG("writing 0x%02X to 0xFF47 (Background palette data)", value);
            memory[addr] = value;
            return;
        case 0xFF50:
            if (value == 1) {
                LINFO("disabling boot ROM");
            }

            memory[addr] = value;
            return;
        default:
            LDEBUG("writing 0x%02X to 0x%04X (IO)", value, addr);
            memory[addr] = value;
            return;
    }
}

bool Bus::IsBootROMActive() {
    // boot ROM sets 0xFF50 to 1 at the end
    return Read8(0xFF50) != 0x1;
}

void Bus::DumpMemoryToFile() {
    FILE* file = fopen("memdump.log", "w");

    for (u32 i = 0x0000; i < 0x10000; i += 0x10) {
        fprintf(file, "%04X ", i);
        for (u16 j = 0x0; j < 0x10; j++) {
            fprintf(file, " %02X", Read8(static_cast<u16>(i + j)));
        }
        fprintf(file, "\n");
    }

    fclose(file);
}