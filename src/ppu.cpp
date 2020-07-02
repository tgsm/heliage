#include <cmath>
#include "bus.h"
#include "logging.h"
#include "ppu.h"
#include "frontend/frontend.h"

PPU::PPU(Bus& bus)
    : bus(bus) {
    vcycles = 0;
    ly = 0x00;
    stat = 0x80;
    mode = Mode::AccessOAM;
}

void PPU::AdvanceCycles(u64 cycles) {
    vcycles += cycles;
}

void PPU::Tick() {
    switch (mode) {
        case Mode::AccessOAM:
            // TODO: block memory access to VRAM and OAM during this mode
            if (vcycles < 80) {
                return;
            }

            vcycles %= 80;
            stat |= 0x3;
            mode = Mode::AccessVRAM;
            break;
        case Mode::AccessVRAM: {
            // TODO: block memory access to VRAM during this mode
            if (vcycles < 172) {
                return;
            }

            if (stat & (1 << 3)) {
                // STAT interrupt
                bus.Write8(0xFF0F, bus.Read8(0xFF0F, false) | 0x2, false);
            }

            vcycles %= 172;

            stat &= ~0x7;

            bool lyc_interrupt = stat & (1 << 6);
            bool lyc_equals_ly = (lyc == ly);
            if (lyc_interrupt && lyc_equals_ly) {
                // LY conincidence interrupt
                bus.Write8(0xFF0F, bus.Read8(0xFF0F, false) | 0x2, false);
            }

            if (lyc_equals_ly) {
                stat |= 0x7;
            }

            mode = Mode::HBlank;
        }
            break;
        case Mode::HBlank:
            if (vcycles < 204) {
                return;
            }

            RenderScanline();

            ly++;
            vcycles %= 204;

            if (ly == 144) {
                mode = Mode::VBlank;
                stat &= ~0x3;
                stat |= 0x1;
                bus.Write8(0xFF0F, bus.Read8(0xFF0F, false) | 0x1, false);
            } else {
                stat &= ~0x3;
                stat |= 0x2;
                mode = Mode::AccessOAM;
            }

            break;
        case Mode::VBlank:
            if (vcycles < 456) {
                return;
            }

            ly++;
            vcycles %= 456;

            if (ly == 154) {
                // TODO: draw
                RenderSprites();
                DrawFramebuffer(framebuffer);
                HandleEvents(bus.GetJoypad());
                ly = 0;
                mode = Mode::AccessOAM;
                stat &= ~0x3;
                stat |= 0x2;
            }

            break;
        default:
            break;
    }
}

void PPU::UpdateTile(u16 addr) {
    if (addr >= 0x9800) {
        return;
    }

    u8 byte1 = 0;
    u8 byte2 = 0;
    if (addr % 2 == 0) {
        byte1 = bus.Read8(addr, false);
        byte2 = bus.Read8(addr + 1, false);
    } else {
        byte1 = bus.Read8(addr - 1, false);
        byte2 = bus.Read8(addr, false);
    }
    u16 index = addr - 0x8000;

    u16 tile_index = index / 16;
    u8 row = (index % 16) / 2; // 2 bytes per row

    for (u8 col = 0; col < 8; col++) {
        u8 tile = (((byte2 >> (7 - col)) & 0b1) << 1) | ((byte1 >> (7 - col)) & 0b1);
        tiles[tile_index][row][col] = static_cast<Color>(tile);
    }
}

void PPU::RenderScanline() {
    if (IsBGDisplayEnabled()) {
        RenderBackgroundScanline();
    }

    if (IsWindowDisplayEnabled()) {
        // TODO: render window
    }
}

void PPU::RenderBackgroundScanline() {
    u16 offset = GetBGTileMapDisplayOffset();
    bool is_signed = (GetBGWindowTileDataOffset() == 0x8800);
    u8 screen_y = ly;
    for (u8 screen_x = 0; screen_x < 160; screen_x++) {
        u16 scroll_x = screen_x + scx;
        u16 scroll_y = screen_y + scy;
        u16 bg_x = scroll_x % 256;
        u16 bg_y = scroll_y % 256;
        u16 tile_offset = offset + (bg_x / 8) + (bg_y / 8 * 32);
        u16 tile_id = bus.Read8(tile_offset, false);
        if (is_signed && tile_id < 0x80) {
            tile_id += 0x100;
        }

        u8 tile_y = bg_y % 8;
        u8 tile_x = bg_x % 8;

        Color color = tiles[tile_id][tile_y][tile_x];
        switch (static_cast<u8>(color)) {
            case 0b00:
                color = bg_window_palette.zero;
                break;
            case 0b01:
                color = bg_window_palette.one;
                break;
            case 0b10:
                color = bg_window_palette.two;
                break;
            case 0b11:
                color = bg_window_palette.three;
                break;
            default:
                break;
        }

        framebuffer[160 * screen_y + screen_x] = color;
    }
}

void PPU::RenderSprites() {
    if (!IsSpriteDisplayEnabled()) {
        return;
    }

    // Sprites can be 8x16 rather than 8x8
    [[maybe_unused]] bool double_height = AreSpritesDoubleHeight();

    // There are 160 bytes of sprite memory available to us;
    // 4 bytes per sprite means we can have up to 40 sprites.
    // TODO: we are limited to 10 sprites per scanline.
    for (u8 sprite_index = 0; sprite_index < 160 / 4; sprite_index++) {
        u16 oam_address = 0xFE00 + (sprite_index * 4);
        u8 y = bus.Read8(oam_address, false);
        u8 x = bus.Read8(oam_address + 1, false);
        [[maybe_unused]] u8 tile_index = bus.Read8(oam_address + 2, false);
        u8 attributes = bus.Read8(oam_address + 3, false);

        if (y == 0 || y >= 144 + 16) {
            continue;
        }

        if (x == 0 || x >= 160 + 8) {
            continue;
        }

        [[maybe_unused]] bool use_obp1 = attributes & (1 << 4);
        [[maybe_unused]] bool flip_x = attributes & (1 << 5);
        [[maybe_unused]] bool flip_y = attributes & (1 << 6);
        [[maybe_unused]] bool priority = attributes & (1 << 7);

        // printf("sprite %02u bytes: %02X %02X %02X %02X\n", sprite_index, y, x, tile_index, attributes);

        // TODO: actually draw sprites
    }
}

void PPU::SetBGWindowPalette(u8 value) {
    bg_window_palette.three = static_cast<Color>((value >> 6) & 0b11);
    bg_window_palette.two = static_cast<Color>((value >> 4) & 0b11);
    bg_window_palette.one = static_cast<Color>((value >> 2) & 0b11);
    bg_window_palette.zero = static_cast<Color>(value & 0b11);

    LDEBUG("PPU: new background palette: %u %u %u %u", static_cast<u8>(bg_window_palette.three),
                                                       static_cast<u8>(bg_window_palette.two),
                                                       static_cast<u8>(bg_window_palette.one),
                                                       static_cast<u8>(bg_window_palette.zero));
}

bool PPU::IsLCDEnabled() {
    return lcdc & (1 << 7);
}

u16 PPU::GetWindowTileMapDisplayOffset() {
    return (lcdc & (1 << 6)) ? 0x9C00 : 0x9800;
}

bool PPU::IsWindowDisplayEnabled() {
    return lcdc & (1 << 5);
}

u16 PPU::GetBGWindowTileDataOffset() {
    return (lcdc & (1 << 4)) ? 0x8000 : 0x8800;
}

u16 PPU::GetBGTileMapDisplayOffset() {
    return (lcdc & (1 << 3)) ? 0x9C00 : 0x9800;
}

bool PPU::AreSpritesDoubleHeight() {
    return lcdc & (1 << 2);
}

bool PPU::IsSpriteDisplayEnabled() {
    return lcdc & (1 << 1);
}

bool PPU::IsBGDisplayEnabled() {
    return lcdc & (1 << 0);
}
