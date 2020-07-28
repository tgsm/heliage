#include "bus.h"
#include "logging.h"

Bus::Bus(BootROM& bootrom, Cartridge& cartridge, Joypad& joypad, PPU& ppu, Timer& timer)
    : bootrom(bootrom), cartridge(cartridge), joypad(joypad), ppu(ppu), timer(timer) {
    LoadInitialValues();
}

void Bus::LoadInitialValues() {
    vram.fill(0xFF);
    wram.fill(0xFF);
    oam.fill(0xFF);
    io.fill(0xFF);
    hram.fill(0xFF);
    cartridge_ram.fill(0xFF);

    // Some addresses need to start at a different value than 0xFF.
    // For example, the interrupt registers need to be zero at startup,
    // otherwise they would be able to execute any interrupt once
    // interrupts are enabled if these registers weren't zeroed out before.
    io[0x0F] = 0xE0;
}

u8 Bus::Read8(u16 addr, bool affect_timer) {
    if (affect_timer) {
        timer.AdvanceCycles(4);
    }

    switch (addr) {
        case 0x0000 ... 0x7FFF:
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

        case 0x8000 ... 0x9FFF:
            // LDEBUG("bus: reading 0x%02X from 0x%04X (VRAM)", vram[addr - 0x8000], addr);
            return vram[addr - 0x8000];

        case 0xA000 ... 0xBFFF:
            if (!mbc_ram_enabled) {
                // LWARN("bus: attempted to read from cartridge RAM while it is disabled (from 0x%04X)", addr);
                return 0xFF;
            }

            LDEBUG("bus: reading 0x%02X from 0x%04X (Cartridge RAM)", cartridge_ram[addr - 0xA000], addr);
            return cartridge_ram[addr - 0xA000];

        case 0xC000 ... 0xDFFF:
            // LDEBUG("bus: reading 0x%02X from 0x%04X (WRAM)", wram[addr - 0xC000], addr);
            return wram[addr - 0xC000];

        case 0xE000 ... 0xFDFF:
            // LWARN("bus: reading from echo RAM (0x%02X from 0x%04X)", wram[addr - 0xE000], addr);
            return wram[addr - 0xE000];

        case 0xFE00 ... 0xFE9F:
            // LDEBUG("bus: reading 0x%02X to 0x%04X (OAM / Sprite Attribute Table)", oam[0xFE00], addr);
            return oam[addr - 0xFE00];

        case 0xFEA0 ... 0xFEFF:
            // LWARN("bus: attempted to read from unusable memory (0x%04X)", addr);
            return 0x00;

        case 0xFF00 ... 0xFF7F:
            return ReadIO(addr & 0xFF);

        case 0xFF80 ... 0xFFFE:
            // LDEBUG("bus: reading 0x%02X from 0x%04X (Zero Page)", hram[addr - 0xFF80], addr);
            return hram[addr - 0xFF80];

        case 0xFFFF:
            // Interrupt enable
            return ie;

        default:
            UNREACHABLE();
    }
}

void Bus::Write8(u16 addr, u8 value, bool affect_timer) {
    if (affect_timer) {
        timer.AdvanceCycles(4);
    }

    switch (addr) {
        case 0x0000 ... 0x7FFF:
        {
            u8 mbc_type = cartridge.GetMBCType();
            if (!mbc_type) {
                break;
            }

            WriteMBC(mbc_type, addr, value);
            break;
        }

        case 0x8000 ... 0x9FFF:
            vram[addr - 0x8000] = value;
            ppu.UpdateTile(addr);
            break;

        case 0xA000 ... 0xBFFF:
            if (!mbc_ram_enabled) {
                // LWARN("bus: attempted to write to cartridge RAM while it is disabled (0x%02X to 0x%04X)", value, addr);
                break;
            }

            LDEBUG("bus: writing 0x%02X to 0x%04X (Cartridge RAM)", value, addr);
            cartridge_ram[addr - 0xA000] = value;
            break;

        case 0xC000 ... 0xDFFF:
            // LDEBUG("bus: writing 0x%02X to 0x%04X (WRAM)", value, addr);
            wram[addr - 0xC000] = value;
            break;

        case 0xE000 ... 0xFDFF:
            // LWARN("bus: writing to echo RAM (0x%02X to 0x%04X)", value, addr);
            wram[addr - 0xE000] = value;
            break;

        case 0xFE00 ... 0xFE9F:
            // LDEBUG("bus: writing 0x%02X to 0x%04X (OAM / Sprite Attribute Table)", value, addr);
            oam[addr - 0xFE00] = value;
            break;

        case 0xFEA0 ... 0xFEFF:
            // LWARN("bus: attempted to write to unusable memory (0x%02X to 0x%04X)", value, addr);
            break;

        case 0xFF00 ... 0xFF7F:
            WriteIO(addr & 0xFF, value);
            break;

        case 0xFF80 ... 0xFFFE:
            // LDEBUG("bus: writing 0x%02X to 0x%04X (Zero Page)", value, addr);
            hram[addr - 0xFF80] = value;
            break;

        case 0xFFFF:
            // Interrupt enable
            ie = value;
            break;

        default:
            UNREACHABLE();
    }
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
                    mbc_ram_enabled = ((value & 0xF) == 0xA);
                    LDEBUG("MBC1: RAM %s", mbc_ram_enabled ? "enabled" : "disabled");
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
                        LDEBUG("MBC1: bank2=%02X", mbc1_bank2);
                    } else if (mbc1_mode == 1) {
                        mbc1_ram_bank = value & 3;
                        LDEBUG("MBC1: ram bank=%02X", mbc1_ram_bank);
                    }

                    break;
                case 0x6000:
                case 0x7000:
                    // TODO: 00h = ROM Banking Mode (up to 8KByte RAM, 2MByte ROM) (default)
                    //       01h = RAM Banking Mode (up to 32KByte RAM, 512KByte ROM)
                    mbc1_mode = value;
                    LDEBUG("MBC1: set mode=%u", value);
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
                    mbc_ram_enabled = ((value & 0xF) == 0xA);
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
        case 0x02:
        {
            u8 value = io[0x02];

            // Bits 6-1 are unused
            value |= 0x7E;

            LDEBUG("bus: reading 0x%02X from Serial control (0xFF02)", value);
            return value;
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

            // Highest 5 bits are unused
            tac |= 0xF8;

            LDEBUG("bus: reading 0x%02X from TAC (0xFF07)", tac);
            return tac;
        }
        case 0x0F:
        {
            // Interrupt fetch
            u8 value = io[0x0F];

            // Highest 3 bits are unused
            value |= 0xE0;

            return value;
        }
        case 0x10:
        {
            // Audio channel 1 sweep
            u8 value = io[0x10];

            // Highest bit is unused
            value |= 0x80;

            LDEBUG("bus: reading 0x%02X to NR10 (0xFF10)", value);
            return value;
        }
        case 0x1A:
        {
            // Audio channel 3 on/off
            u8 value = io[0x1A];

            // Bits 6-0 are unused
            value |= 0x7F;

            LDEBUG("bus: reading 0x%02X from NR30 (0xFF1A)", value);
            return value;
        }
        case 0x1C:
        {
            // Audio channel 3 select output level
            u8 value = io[0x1C];

            // Bits 7 and 4-0 are unused
            value |= 0x9F;

            LDEBUG("bus: reading 0x%02X from NR32 (0xFF1C)", value);
            return value;
        }
        case 0x20:
        {
            // Audio channel 4 sound length
            u8 value = io[0x20];

            // Bits 7-6 are unused
            value |= 0xC0;

            LDEBUG("bus: reading 0x%02X from NR41 (0xFF20)", value);
            return value;
        }
        case 0x23:
        {
            // Audio channel 4 counter
            u8 value = io[0x23];

            // Bits 5-0 are unused
            value |= 0x3F;

            LDEBUG("bus: reading 0x%02X from NR44 (0xFF23)", value);
            return value;
        }
        case 0x26:
        {
            // Sound on/off
            u8 value = io[0x26];

            // TODO: lowest 4 bits are read-only
            // Bits 6-4 are unused
            value |= 0x70;

            LDEBUG("bus: reading 0x%02X from NR52 (0xFF26)", value);
            return value;
        }
        case 0x40:
        {
            u8 lcdc = ppu.GetLCDC();
            LDEBUG("bus: reading 0x%02X from LCDC (0xFF40)", lcdc);
            return lcdc;
        }
        case 0x41:
        {
            u8 status = ppu.GetSTAT();

            // Bit 7 is unused
            status |= 0x80;

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
        case 0x4A:
        {
            u8 wy = ppu.GetWY();
            // LDEBUG("bus: reading 0x%02X from Window Y (0xFF4A)", wy);
            return wy;
        }
        case 0x4B:
        {
            u8 wx = ppu.GetWX();
            // LDEBUG("bus: reading 0x%02X from Window X (0xFF4B)", wx);
            return wx;
        }

        // These are CGB registers.
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
            return 0xFF;

        // These are unused registers.
        case 0x03:
        case 0x08 ... 0x0E:
        case 0x15:
        case 0x1F:
        case 0x27 ... 0x29:
        case 0x4C:
        case 0x4E ... 0x4F:
        // 0xFF50 is write-only
        case 0x50:
        case 0x51 ... 0x55:
        case 0x57 ... 0x6B:
        case 0x6D ... 0x6F:
        case 0x71:
        case 0x78 ... 0x7F:
            return 0xFF;

        default:
            LDEBUG("bus: reading 0x%02X from 0xFF%02X (unknown IO)", io[addr], addr);
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
#ifdef HELIAGE_PRINT_SERIAL_BYTES
            printf("%02X\n", value);
            fflush(stdout);
#else
            // putchar(value);
#endif
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
            io[0x0F] = value;
            return;
        case 0x40:
            LDEBUG("bus: writing 0x%02X to LCDC (0xFF40)", value);
            ppu.SetLCDC(value);
            io[0x40] = value;
            return;
        case 0x41:
            LDEBUG("bus: writing 0x%02X to STAT (0xFF41)", value);
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
        case 0x4A:
            LDEBUG("bus: writing 0x%02X to Window Y (0xFF4A)", value);
            ppu.SetWY(value);
            io[0x4A] = value;
            return;
        case 0x4B:
            LDEBUG("bus: writing 0x%02X to Window X (0xFF4B)", value);
            ppu.SetWX(value);
            io[0x4B] = value;
            return;
        case 0x50:
            if (boot_rom_enabled && value & 0b1) {
                LINFO("bus: disabling bootrom");
                boot_rom_enabled = false;
            }

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
