#include <cstdio>
#include "bus.h"
#include "logging.h"

Bus::Bus(BootROM& bootrom, Cartridge& cartridge, Joypad& joypad, PPU& ppu, Timer& timer)
    : bootrom(bootrom), cartridge(cartridge), joypad(joypad), ppu(ppu), timer(timer) {
    boot_rom_enabled = true;
    memory = std::array<u8, 0x10000>();
    std::fill(memory.begin(), memory.end(), 0xFF);
}

u8 Bus::Read8(u16 addr) {
    if (addr <= 0x7FFF) {
        if (addr < 0x0100 && boot_rom_enabled) {
            memory[addr] = bootrom.Read(addr);
            return memory[addr];
        }

#define CART_IS_MBC1() (mbc_type >= 0x01 && mbc_type <= 0x03)
#define CART_IS_MBC3() (mbc_type >= 0x0F && mbc_type <= 0x13)

        if (addr >= 0x4000) {
            u8 mbc_type = cartridge.GetMBCType();
            u16 rom_bank = 0x001;
            if (CART_IS_MBC1()) {
                rom_bank = ((mbc1_rom_bank_hi & 3) << 5) | (mbc1_rom_bank_lo & 0x1F);
            } else if (CART_IS_MBC3()) {
                rom_bank = mbc3_rom_bank;
            }
            memory[addr] = cartridge.Read((addr & 0x3FFF) + rom_bank * 0x4000);
            return memory[addr];
        }

        memory[addr] = cartridge.Read(addr);
        return memory[addr];
    }

    // VRAM
    if (addr >= 0x8000 && addr < 0xA000) {
        // LDEBUG("reading 0x%02X from 0x%04X (VRAM)", memory[addr], addr);
        return memory[addr];
    }

    if (addr >= 0xA000 && addr < 0xC000) {
        if (mbc_ram_enabled) {
            LDEBUG("reading 0x%02X from 0x%04X (Cartridge RAM)", memory[addr], addr);
            return memory[addr];
        } else {
            // LWARN("attempted to read from cartridge RAM while it is disabled (from 0x%04X)", addr);
            return 0xFF;
        }
    }

    if (addr >= 0xC000 && addr < 0xE000) {
        // LDEBUG("reading 0x%02X from 0x%04X (WRAM)", memory[addr], addr);
        return memory[addr];
    }    

    if (addr >= 0xE000 && addr < 0xFE00) {
        // LWARN("reading from echo RAM (0x%02X from 0x%04X)", memory[addr - 0x2000], addr);
        return memory[addr - 0x2000];
    }

    if (addr >= 0xFE00 && addr < 0xFEA0) {
        // LDEBUG("reading 0x%02X to 0x%04X (OAM / Sprite Attribute Table)", memory[addr], addr);
        return memory[addr];
    }

    if (addr >= 0xFEA0 && addr < 0xFF00) {
        // LWARN("attempted to read from unusable memory (0x%04X)", addr);
        return 0x00;
    }

    if (addr >= 0xFF00 && addr < 0xFF80) {
        return ReadIO(addr);
    }

    // Zero Page
    if (addr >= 0xFF80 && addr < 0xFFFF) {
        // LDEBUG("reading 0x%02X from 0x%04X (Zero Page)", memory[addr], addr);
        return memory[addr];
    }

    if (addr == 0xFFFF) {
        // Interrupt enable
        return memory[0xFFFF];
    }

    LERROR("unrecognized read8 from 0x%04X", addr);
    return 0xFF;
}

void Bus::Write8(u16 addr, u8 value) {
    if (addr <= 0x7FFF) {
        u8 mbc_type = cartridge.GetMBCType();
        if (mbc_type) {
            WriteMBC(mbc_type, addr, value);
        }

        return;
    }

    if (addr >= 0x8000 && addr < 0xA000) {
        memory[addr] = value;
        ppu.UpdateTile(addr);
        return;
    }

    if (addr >= 0xA000 && addr < 0xC000) {
        if (mbc_ram_enabled) {
            LDEBUG("writing 0x%02X to 0x%04X (Cartridge RAM)", value, addr);
            memory[addr] = value;
        } else {
            // LWARN("attempted to write to cartridge RAM while it is disabled (0x%02X to 0x%04X)", value, addr);
        }
        return;
    }

    if (addr >= 0xC000 && addr < 0xE000) {
        // LDEBUG("writing 0x%02X to 0x%04X (WRAM)", value, addr);
        memory[addr] = value;

        if (addr < 0xDE00) {
            memory[addr + 0x2000] = value;
        }

        return;
    }

    if (addr >= 0xE000 && addr < 0xFE00) {
        // LWARN("writing to echo RAM (0x%02X to 0x%04X)", value, addr);
        memory[addr] = value;
        memory[addr - 0x2000] = value;
        return;
    }

    if (addr >= 0xFE00 && addr < 0xFEA0) {
        // LDEBUG("writing 0x%02X to 0x%04X (OAM / Sprite Attribute Table)", value, addr);
        memory[addr] = value;
        return;
    }

    if (addr >= 0xFEA0 && addr < 0xFF00) {
        // LWARN("attempted to write to unusable memory (0x%02X to 0x%04X)", value, addr);
        return;
    }

    if (addr >= 0xFF00 && addr < 0xFF80) {
        WriteIO(addr, value);
        return;
    }

    // Zero Page
    if (addr >= 0xFF80 && addr < 0xFFFF) {
        // LDEBUG("writing 0x%02X to 0x%04X (Zero Page)", value, addr);
        memory[addr] = value;
        return;
    }

    if (addr == 0xFFFF) {
        // Interrupt enable
        memory[0xFFFF] = value;
        return;
    }

    LERROR("unrecognized write8 0x%02X to 0x%04X", value, addr);
    return;
}

void Bus::WriteMBC(u8 mbc_type, u16 addr, u8 value) {
    switch (mbc_type) {
        case 0x00:
            LWARN("attempted to write to cartridge ROM (0x%02X to 0x%04X)", value, addr);
            return;
        case 0x01:
        case 0x02:
        case 0x03:
            switch (addr & 0xF000) {
                case 0x0000:
                case 0x1000:
                    mbc_ram_enabled = (value == 0x0A);
                    break;
                case 0x2000:
                case 0x3000:
                    if (value == 0x00 || value == 0x20 || value == 0x40 || value == 0x60) {
                        mbc1_rom_bank_lo = value + 1;
                    } else {
                        mbc1_rom_bank_lo = value & 0x1F;
                    }

                    break;
                case 0x4000:
                case 0x5000:
                    if (mbc1_bank_mode == 0) {
                        mbc1_rom_bank_hi = value & 3;
                        LFATAL("hi=%02X", mbc1_rom_bank_hi);
                    } else if (mbc1_bank_mode == 1) {
                        mbc1_ram_bank = value & 3;
                        LFATAL("ram=%02X", mbc1_ram_bank);
                    }

                    break;
                case 0x6000:
                case 0x7000:
                    // TODO: 00h = ROM Banking Mode (up to 8KByte RAM, 2MByte ROM) (default)
                    //       01h = RAM Banking Mode (up to 32KByte RAM, 512KByte ROM)
                    mbc1_bank_mode = value;
                    LINFO("set MBC1 bank mode (%u)", value);
                    break;
                default:
                    LERROR("unimplemented MBC1 write (0x%02X to 0x%04X)", value, addr);
                    break;
            }
            return;
        case 0x10:
        case 0x13:
            switch (addr & 0xF000) {
                case 0x0000:
                case 0x1000:
                    mbc_ram_enabled = (value == 0x0A);
                    break;
                case 0x2000:
                case 0x3000:
                    if (value == 0x00) {
                        mbc3_rom_bank = 0x01;
                    } else {
                        mbc3_rom_bank = value & 0x7F;
                    }

                    break;
                default:
                    LERROR("unimplemented MBC3 write (0x%02X to 0x%04X)", value, addr);
                    break;
            }
            return;
        default:
            LERROR("unimplemented MBC (type 0x%02X) write (0x%02X to 0x%04X)", mbc_type, value, addr);
            return;
    }
}

u16 Bus::Read16(u16 addr) {
    LERROR("unrecognized read16 from 0x%04X", addr);
    return 0xFFFF;
}

void Bus::Write16(u16 addr, u16 value) {
    LDEBUG("writing 0x%04X to 0x%04X", value, addr);

    u8 high = static_cast<u8>(value >> 8);
    u8 low = static_cast<u8>(value & 0xFF);

    memory[addr] = low;
    memory[addr + 1] = high;
}

u8 Bus::ReadIO(u16 addr) {
    switch (addr) {
        case 0xFF00:
        {
            u8 buttons = joypad.Read();
            LDEBUG("reading 0x%02X from Joypad (0xFF00)", buttons);
            return buttons;
        }
        case 0xFF04:
        {
            u8 div = timer.GetDivider();
            LDEBUG("reading 0x%02X from DIV (0xFF04)", div);
            return div;
        }
        case 0xFF05:
        {
            u8 tima = timer.GetTIMA();
            LDEBUG("reading 0x%02X from TIMA (0xFF05)", tima);
            return tima;
        }
        case 0xFF06:
        {
            u8 tma = timer.GetTMA();
            LDEBUG("reading 0x%02X from TMA (0xFF06)", tma);
            return tma;
        }
        case 0xFF07:
        {
            u8 tac = timer.GetTAC();
            LDEBUG("reading 0x%02X from TAC (0xFF07)", tac);
            return tac;
        }
        case 0xFF0F:
            // Interrupt fetch
            return memory[0xFF0F];
        case 0xFF40:
        {
            u8 lcdc = ppu.GetLCDC();
            LDEBUG("reading 0x%02X from LCDC (0xFF40)", lcdc);
            return lcdc;
        }
        case 0xFF41:
        {
            u8 status = ppu.GetSTAT();
            LDEBUG("reading 0x%02X from STAT (0xFF41)", status);
            return status;
        }
        case 0xFF42:
            LDEBUG("reading 0x%02X from SCY (0xFF42)", memory[0xFF42]);
            return memory[0xFF42];
        case 0xFF43:
            LDEBUG("reading 0x%02X from SCX (0xFF43)", memory[0xFF43]);
            return memory[0xFF43];
        case 0xFF44:
        {
            u8 ly = ppu.GetLY();
            // LDEBUG("reading 0x%02X from LY (0xFF44)", ly);
            return ly;
        }
        case 0xFF50:
            // boot ROM switch
            return memory[0xFF50];

        case 0xFF4D:
        case 0xFF56:
        case 0xFF6C:
        case 0xFF70:
        case 0xFF72:
        case 0xFF73:
        case 0xFF74:
        case 0xFF75:
        case 0xFF76:
        case 0xFF77:
            // These are CGB registers.
            return 0xFF;
        default:
            LDEBUG("reading 0x%02X from 0x%04X (IO)", memory[addr], addr);
            return memory[addr];
    }
}

void Bus::WriteIO(u16 addr, u8 value) {
    switch (addr) {
        case 0xFF00:
            joypad.Write(value);
            return;
        case 0xFF01:
            // used by blargg tests
            // putchar(value);
            LDEBUG("writing 0x%02X to Serial data (0xFF01)", value);
            return;
        case 0xFF04:
            // Timer divider
            // All writes to this set it to 0.
            timer.ResetDivider();
            memory[0xFF04] = 0x00;
            return;
        case 0xFF05:
            // Timer counter
            LDEBUG("writing 0x%02X to TIMA (0xFF05)", value);
            timer.SetTIMA(value);
            memory[0xFF05] = value;
            return;
        case 0xFF06:
            // Timer modulo
            LDEBUG("writing 0x%02X to TMA (0xFF06)", value);
            timer.SetTMA(value);
            memory[0xFF06] = value;
            return;
        case 0xFF07:
            // Timer control
            LDEBUG("writing 0x%02X to TAC (0xFF07)", value);
            timer.SetTAC(value);
            memory[0xFF07] = value;
            return;
        case 0xFF0F:
            // Interrupt fetch
            memory[0xFF0F] = value;
            return;
        case 0xFF40:
            LDEBUG("writing 0x%02X to LCDC (0xFF40)", value);
            ppu.SetLCDC(value);
            memory[addr] = value;
            return;
        case 0xFF41:
            LDEBUG("writing 0x%02X to STAT (0xFF41)", value);
            ppu.SetSTAT(value);
            memory[addr] = value;
            return;
        case 0xFF42:
            LDEBUG("writing 0x%02X to SCY (0xFF42)", value);
            memory[addr] = value;
            return;
        case 0xFF43:
            LDEBUG("writing 0x%02X to SCX (0xFF43)", value);
            memory[addr] = value;
            return;
        case 0xFF46: {
            LDEBUG("writing 0x%02X to DMA transfer location (0xFF46)", value);
            memory[addr] = value;

            // copies 160 bytes from somewhere to OAM
            // source is determined by (value << 8)
            // for example, if `value` was 0xC3, 160 bytes would be copied
            // from 0xC300 - 0xC39F to OAM.
            u16 source = (value << 8);
            for (u8 i = 0; i < 0xA0; i++) {
                Write8(0xFE00 + i, Read8(source + i));
            }
        }
            return;
        case 0xFF47:
            LDEBUG("writing 0x%02X to Background palette data (0xFF47)", value);
            memory[addr] = value;
            return;
        case 0xFF50:
            if (value & 0b1) {
                LINFO("disabling bootrom");
                boot_rom_enabled = false;
            }

            memory[addr] = value;
            return;
        default:
            LDEBUG("writing 0x%02X to 0x%04X (unknown IO)", value, addr);
            memory[addr] = value;
            return;
    }
}

void Bus::DumpMemoryToFile() {
    FILE* file = fopen("memory.log", "wt");

    for (u32 i = 0x0000; i < 0x10000; i += 0x10) {
        fprintf(file, "%04X ", i);
        for (u16 j = 0x0; j < 0x10; j++) {
            u8 byte = Read8(static_cast<u16>(i + j));
            fprintf(file, " %02X", byte);
        }
        fprintf(file, "\n");
    }

    // for (u16 i = 0x9800; i < 0xA000; i += 0x10) {
    //     // fprintf(file, "%04X ", i);
    //     for (u16 j = 0x0; j < 0x10; j++) {
    //         u8 byte = Read8(static_cast<u16>(i + j));
    //         printf("%c", byte);
    //         // fprintf(file, " %02X", byte);
    //     }
    //     // fprintf(file, "\n");
    // }

    fclose(file);
}
