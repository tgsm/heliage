#include <cstdio>
#include "dmgboot.h"
#include "logging.h"
#include "mmu.h"

MMU::MMU(Cartridge& cartridge)
    : cartridge(cartridge) {
    memory = std::array<u8, 0x10000>();
}

u8 MMU::Read8(u16 addr) {
    if (addr <= 0x7FFF) {
        if (addr < 0x0100 && IsBootROMActive()) {
            return dmgboot[addr];
        }

        return cartridge.Read8(addr);
    }

    if (addr >= 0xC000 && addr < 0xE000) {
        LDEBUG("reading 0x%02X from 0x%04X (WRAM)", memory[addr], addr);
        return memory[addr];
    }    

    // VRAM
    if (addr >= 0x8000 && addr < 0xA000) {
        LDEBUG("reading 0x%02X from 0x%04X (VRAM)", memory[addr], addr);
        return memory[addr];
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
        LDEBUG("reading %u from 0xFFFF (Interrupt Enable Flag)", memory[0xFFFF]);
        return memory[0xFFFF];
    }

    LERROR("unrecognized read8 from 0x%04X", addr);
    return 0xFF;
}

void MMU::Write8(u16 addr, u8 value) {
    // VRAM
    if (addr >= 0x8000 && addr < 0xA000) {
        LDEBUG("writing 0x%02X to 0x%04X (VRAM)", value, addr);
        memory[addr] = value;
        return;
    }

    if (addr >= 0xC000 && addr < 0xE000) {
        LDEBUG("writing 0x%02X to 0x%04X (WRAM)", value, addr);
        memory[addr] = value;
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
        LDEBUG("writing 0x%02X to 0xFFFF (Interrupt Enable Flag)", value);
        memory[0xFFFF] = value;
        return;
    }

    LERROR("unrecognized write8 0x%02X to 0x%04X", value, addr);
    return;
}

u16 MMU::Read16(u16 addr) {
    LERROR("unrecognized read16 from 0x%04X", addr);
    return 0xFFFF;
}

void MMU::Write16(u16 addr, u16 value) {
    u8 high = static_cast<u8>((value >> 4) & 0xFF);
    u8 low = static_cast<u8>(value & 0xFF);

    memory[addr] = low;
    memory[addr + 1] = high;
}

u8 MMU::ReadIO(u16 addr) {
    switch (addr) {
        case 0xFF44:
            // TODO
            LDEBUG("reading 0x%02X from 0xFF44 (LCD Control Y)", 0x90);
            LWARN("(hack) all reads from 0xFF44 are currently hardcoded to return 0x90");
            return 0x90;
        case 0xFF50:
            // boot ROM switch
            // LDEBUG("reading 0x%02X from 0xFF50 (Boot ROM switch)", memory[addr]);
            return memory[0xFF50];
        default:
            LDEBUG("reading 0x%02X from 0x%04X (IO)", memory[addr], addr);
            return memory[addr];
    }
}

void MMU::WriteIO(u16 addr, u8 value) {
    switch (addr) {
        case 0xFF01:
            // used by blargg tests
            printf("%c", value);
            return;
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

bool MMU::IsBootROMActive() {
    // boot ROM sets 0xFF50 to 1 at the end
    return Read8(0xFF50) != 0x1;
}

void MMU::DumpMemoryToFile() {
    FILE* file = fopen("memdump.log", "w+");

    for (u32 i = 0x0000; i < 0x10000; i += 0x10) {
        fprintf(file, "%04X  ", i);
        for (u16 j = 0x0; j < 0x10; j++) {
            fprintf(file, "%02X ", memory[i + j]);
        }
        fprintf(file, "\n");
    }

    fclose(file);
}