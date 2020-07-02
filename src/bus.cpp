#include "bus.h"
#include "logging.h"

Bus::Bus(BootROM& bootrom, Cartridge& cartridge, Joypad& joypad, PPU& ppu, Timer& timer)
    : bootrom(bootrom), cartridge(cartridge), joypad(joypad), ppu(ppu), timer(timer) {
    boot_rom_enabled = true;
    LoadInitialValues();
}

void Bus::LoadInitialValues() {
    std::fill(vram.begin(), vram.end(), 0xFF);
    std::fill(wram.begin(), wram.end(), 0xFF);
    std::fill(oam.begin(), oam.end(), 0xFF);
    std::fill(io.begin(), io.end(), 0xFF);
    std::fill(hram.begin(), hram.end(), 0xFF);
    std::fill(cartridge_ram.begin(), cartridge_ram.end(), 0xFF);

    // Some addresses need to start at a different value than 0xFF.
    // For example, the interrupt registers need to be zero at startup,
    // otherwise they would be able to execute any interrupt once
    // interrupts are enabled if these registers weren't zeroed out before.
    io[0x0F] = 0xE0;
    ie = 0x00;
}

u8 Bus::Read8(u16 addr, bool affect_timer) {
    if (affect_timer) {
        timer.AdvanceCycles(4);
    }

    if (addr <= 0x7FFF) {
        if (addr < 0x0100 && boot_rom_enabled) {
            return bootrom.Read(addr);
        }

#define CART_IS_MBC1() (mbc_type >= 0x01 && mbc_type <= 0x03)
#define CART_IS_MBC3() (mbc_type >= 0x0F && mbc_type <= 0x13)

        if (addr >= 0x4000) {
            u8 mbc_type = cartridge.GetMBCType();
            u16 rom_bank = 0x001;
            if (CART_IS_MBC1()) {
                rom_bank = ((mbc1_bank2 & 3) << 5) | (mbc1_bank1 & 0x1F);
            } else if (CART_IS_MBC3()) {
                rom_bank = mbc3_rom_bank;
            }
            return cartridge.Read((addr & 0x3FFF) + rom_bank * 0x4000);
        }

        return cartridge.Read(addr);
    }

    // VRAM
    if (addr >= 0x8000 && addr < 0xA000) {
        // LDEBUG("bus: reading 0x%02X from 0x%04X (VRAM)", vram[addr - 0x8000], addr);
        return vram[addr - 0x8000];
    }

    if (addr >= 0xA000 && addr < 0xC000) {
        if (mbc_ram_enabled) {
            LDEBUG("bus: reading 0x%02X from 0x%04X (Cartridge RAM)", cartridge_ram[addr - 0xA000], addr);
            return cartridge_ram[addr - 0xA000];
        } else {
            // LWARN("bus: attempted to read from cartridge RAM while it is disabled (from 0x%04X)", addr);
            return 0xFF;
        }
    }

    if (addr >= 0xC000 && addr < 0xE000) {
        // LDEBUG("bus: reading 0x%02X from 0x%04X (WRAM)", wram[addr - 0xC000], addr);
        return wram[addr - 0xC000];
    }    

    if (addr >= 0xE000 && addr < 0xFE00) {
        // LWARN("bus: reading from echo RAM (0x%02X from 0x%04X)", wram[addr - 0xE000], addr);
        return wram[addr - 0xE000];
    }

    if (addr >= 0xFE00 && addr < 0xFEA0) {
        // LDEBUG("bus: reading 0x%02X to 0x%04X (OAM / Sprite Attribute Table)", oam[0xFE00], addr);
        return oam[addr - 0xFE00];
    }

    if (addr >= 0xFEA0 && addr < 0xFF00) {
        // LWARN("bus: attempted to read from unusable memory (0x%04X)", addr);
        return 0x00;
    }

    if (addr >= 0xFF00 && addr < 0xFF80) {
        return ReadIO(addr & 0xFF);
    }

    // Zero Page
    if (addr >= 0xFF80 && addr < 0xFFFF) {
        // LDEBUG("bus: reading 0x%02X from 0x%04X (Zero Page)", hram[addr - 0xFF80], addr);
        return hram[addr - 0xFF80];
    }

    if (addr == 0xFFFF) {
        // Interrupt enable
        return ie;
    }

    LERROR("bus: unrecognized read8 from 0x%04X", addr);
    return 0xFF;
}

void Bus::Write8(u16 addr, u8 value, bool affect_timer) {
    if (affect_timer) {
        timer.AdvanceCycles(4);
    }

    if (addr <= 0x7FFF) {
        u8 mbc_type = cartridge.GetMBCType();
        if (mbc_type) {
            WriteMBC(mbc_type, addr, value);
        }

        return;
    }

    if (addr >= 0x8000 && addr < 0xA000) {
        vram[addr - 0x8000] = value;
        ppu.UpdateTile(addr);
        return;
    }

    if (addr >= 0xA000 && addr < 0xC000) {
        if (mbc_ram_enabled) {
            LDEBUG("bus: writing 0x%02X to 0x%04X (Cartridge RAM)", value, addr);
            cartridge_ram[addr - 0xA000] = value;
        } else {
            // LWARN("bus: attempted to write to cartridge RAM while it is disabled (0x%02X to 0x%04X)", value, addr);
        }
        return;
    }

    if (addr >= 0xC000 && addr < 0xE000) {
        // LDEBUG("bus: writing 0x%02X to 0x%04X (WRAM)", value, addr);
        wram[addr - 0xC000] = value;
        return;
    }

    if (addr >= 0xE000 && addr < 0xFE00) {
        // LWARN("bus: writing to echo RAM (0x%02X to 0x%04X)", value, addr);
        wram[addr - 0xE000] = value;
        return;
    }

    if (addr >= 0xFE00 && addr < 0xFEA0) {
        // LDEBUG("bus: writing 0x%02X to 0x%04X (OAM / Sprite Attribute Table)", value, addr);
        oam[addr - 0xFE00] = value;
        return;
    }

    if (addr >= 0xFEA0 && addr < 0xFF00) {
        // LWARN("bus: attempted to write to unusable memory (0x%02X to 0x%04X)", value, addr);
        return;
    }

    if (addr >= 0xFF00 && addr < 0xFF80) {
        WriteIO(addr & 0xFF, value);
        return;
    }

    // Zero Page
    if (addr >= 0xFF80 && addr < 0xFFFF) {
        // LDEBUG("bus: writing 0x%02X to 0x%04X (Zero Page)", value, addr);
        hram[addr - 0xFF80] = value;
        return;
    }

    if (addr == 0xFFFF) {
        // Interrupt enable

        // The highest 3 bits are always set
        value |= 0xE0;

        ie = value;
        return;
    }

    LERROR("bus: unrecognized write8 0x%02X to 0x%04X", value, addr);
    return;
}

void Bus::WriteMBC(u8 mbc_type, u16 addr, u8 value) {
    switch (mbc_type) {
        case 0x00:
            LWARN("bus: attempted to write to cartridge ROM (0x%02X to 0x%04X)", value, addr);
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
                        mbc1_bank1 = value + 1;
                    } else {
                        mbc1_bank1 = value & 0x1F;
                    }

                    break;
                case 0x4000:
                case 0x5000:
                    if (mbc1_mode == 0) {
                        mbc1_bank2 = value & 3;
                        LFATAL("MBC1: bank2=%02X", mbc1_bank2);
                    } else if (mbc1_mode == 1) {
                        mbc1_ram_bank = value & 3;
                        LFATAL("MBC1: ram bank=%02X", mbc1_ram_bank);
                    }

                    break;
                case 0x6000:
                case 0x7000:
                    // TODO: 00h = ROM Banking Mode (up to 8KByte RAM, 2MByte ROM) (default)
                    //       01h = RAM Banking Mode (up to 32KByte RAM, 512KByte ROM)
                    mbc1_mode = value;
                    LINFO("MBC1: set mode=%u", value);
                    break;
                default:
                    LERROR("MBC1: unimplemented write (0x%02X to 0x%04X)", value, addr);
                    break;
            }
            return;
        case 0x10:
        case 0x11:
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
                    LERROR("MBC3: unimplemented write (0x%02X to 0x%04X)", value, addr);
                    break;
            }
            return;
        default:
            LERROR("bus: unimplemented MBC (type 0x%02X) write (0x%02X to 0x%04X)", mbc_type, value, addr);
            return;
    }
}

u8 Bus::ReadIO(u8 addr) {
    switch (addr) {
        case 0x00:
        {
            u8 buttons = joypad.Read();
            LDEBUG("bus: reading 0x%02X from Joypad (0xFF00)", buttons);
            return buttons;
        }
        case 0x04:
        {
            u8 div = timer.GetDivider();
            LDEBUG("bus: reading 0x%02X from DIV (0xFF04)", div);
            return div;
        }
        case 0x05:
        {
            u8 tima = timer.GetTIMA();
            LDEBUG("bus: reading 0x%02X from TIMA (0xFF05)", tima);
            return tima;
        }
        case 0x06:
        {
            u8 tma = timer.GetTMA();
            LDEBUG("bus: reading 0x%02X from TMA (0xFF06)", tma);
            return tma;
        }
        case 0x07:
        {
            u8 tac = timer.GetTAC();
            LDEBUG("bus: reading 0x%02X from TAC (0xFF07)", tac);
            return tac;
        }
        case 0x0F:
            // Interrupt fetch
            return io[0x0F];
        case 0x40:
        {
            u8 lcdc = ppu.GetLCDC();
            LDEBUG("bus: reading 0x%02X from LCDC (0xFF40)", lcdc);
            return lcdc;
        }
        case 0x41:
        {
            u8 status = ppu.GetSTAT();
            LDEBUG("bus: reading 0x%02X from STAT (0xFF41)", status);
            return status;
        }
        case 0x42:
            LDEBUG("bus: reading 0x%02X from SCY (0xFF42)", io[0x42]);
            return io[0x42];
        case 0x43:
            LDEBUG("bus: reading 0x%02X from SCX (0xFF43)", io[0x43]);
            return io[0x43];
        case 0x44:
        {
            u8 ly = ppu.GetLY();
            // LDEBUG("bus: reading 0x%02X from LY (0xFF44)", ly);
            return ly;
        }
        case 0x50:
            // boot ROM switch
            return io[0x50];

        case 0x4D:
        case 0x56:
        case 0x6C:
        case 0x70:
        case 0x72:
        case 0x73:
        case 0x74:
        case 0x75:
        case 0x76:
        case 0x77:
            // These are CGB registers.
            return 0xFF;
        default:
            LDEBUG("bus: reading 0x%02X from 0xFF%02X (IO)", io[addr], addr);
            return io[addr];
    }
}

void Bus::WriteIO(u8 addr, u8 value) {
    switch (addr) {
        case 0x00:
            joypad.Write(value);
            return;
        case 0x01:
            // used by blargg tests
            // putchar(value);
            LDEBUG("bus: writing 0x%02X to Serial data (0xFF01)", value);
            io[0x01] = value;
            return;
        case 0x04:
            // Timer divider
            // All writes to this set it to 0.
            timer.ResetDivider();
            io[0x04] = 0x00;
            return;
        case 0x05:
            // Timer counter
            LDEBUG("bus: writing 0x%02X to TIMA (0xFF05)", value);
            timer.SetTIMA(value);
            io[0x05] = value;
            return;
        case 0x06:
            // Timer modulo
            LDEBUG("bus: writing 0x%02X to TMA (0xFF06)", value);
            timer.SetTMA(value);
            io[0x06] = value;
            return;
        case 0x07:
            // Timer control
            LDEBUG("bus: writing 0x%02X to TAC (0xFF07)", value);
            timer.SetTAC(value);
            io[0x07] = value;
            return;
        case 0x0F:
            // Interrupt fetch

            // The highest 3 bits are always set
            value |= 0xE0;

            io[0x0F] = value;
            return;
        case 0x40:
            LDEBUG("bus: writing 0x%02X to LCDC (0xFF40)", value);
            ppu.SetLCDC(value);
            io[0x40] = value;
            return;
        case 0x41:
            LDEBUG("bus: writing 0x%02X to STAT (0xFF41)", value);
            // Bit 7 is always set
            value |= 0x80;

            // The lowest 3 bits are read-only
            value &= ~0x7;
            value |= (ppu.GetSTAT() & 0x7);

            ppu.SetSTAT(value);
            io[0x41] = value;
            return;
        case 0x42:
            LDEBUG("bus: writing 0x%02X to SCY (0xFF42)", value);
            ppu.SetSCY(value);
            io[0x42] = value;
            return;
        case 0x43:
            LDEBUG("bus: writing 0x%02X to SCX (0xFF43)", value);
            ppu.SetSCX(value);
            io[0x43] = value;
            return;
        case 0x45:
            LDEBUG("bus: writing 0x%02X to LYC (0xFF45)", value);
            ppu.SetLYC(value);
            io[0x45] = value;
            return;
        case 0x46: {
            LDEBUG("bus: writing 0x%02X to DMA transfer location (0xFF46)", value);
            io[0x46] = value;

            // copies 160 bytes from somewhere to OAM
            // source is determined by (value << 8)
            // for example, if `value` was 0xC3, 160 bytes would be copied
            // from 0xC300 - 0xC39F to OAM.
            u16 source = (value << 8);
            for (u8 i = 0; i < 0xA0; i++) {
                Write8(0xFE00 + i, Read8(source + i, false));
            }
        }
            return;
        case 0x47:
            LDEBUG("bus: writing 0x%02X to Background palette data (0xFF47)", value);
            ppu.SetBGWindowPalette(value);
            io[0x47] = value;
            return;
        case 0x50:
            if (value & 0b1) {
                LINFO("bus: disabling bootrom");
                boot_rom_enabled = false;
            }

            io[0x50] = value;
            return;
        default:
            LDEBUG("bus: writing 0x%02X to 0xFF%02X (unknown IO)", value, addr);
            io[addr] = value;
            return;
    }
}

void Bus::DumpMemoryToFile() {
    FILE* file = fopen("memory.log", "wt");

    for (u32 i = 0x0000; i < 0x10000; i += 0x10) {
        fprintf(file, "%04X ", i);
        for (u16 j = 0x0; j < 0x10; j++) {
            u8 byte = Read8(static_cast<u16>(i + j), false);
            fprintf(file, " %02X", byte);
        }
        fprintf(file, "\n");
    }

    // for (u16 i = 0x9800; i < 0xA000; i += 0x10) {
    //     // fprintf(file, "%04X ", i);
    //     for (u16 j = 0x0; j < 0x10; j++) {
    //         u8 byte = Read8(static_cast<u16>(i + j), false);
    //         printf("%c", byte);
    //         // fprintf(file, " %02X", byte);
    //     }
    //     // fprintf(file, "\n");
    // }

    fclose(file);
}

Joypad* Bus::GetJoypad() {
    return &joypad;
}
