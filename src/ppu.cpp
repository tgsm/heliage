#include <cmath>
#include "bus.h"
#include "logging.h"
#include "ppu.h"
#include "frontend/sdl.h"

PPU::PPU(Bus& bus)
    : bus(bus) {
    vcycles = 0;
    ly = 0x00;
    stat = 0x80;
    mode = Mode::AccessOAM;

    framebuffer = static_cast<PPU::Color*>(malloc(160 * 144 * sizeof(PPU::Color)));
}

void PPU::Tick(u8 cycles) {
    vcycles += cycles;

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
                bus.Write8(0xFF0F, bus.Read8(0xFF0F) | 0x2);
            }

            vcycles %= 172;

            stat &= ~0x7;

            bool lyc_interrupt = stat & (1 << 6);
            bool lyc_equals_ly = (lyc == ly);
            if (lyc_interrupt && lyc_equals_ly) {
                // LY conincidence interrupt
                bus.Write8(0xFF0F, bus.Read8(0xFF0F) | 0x2);
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
                bus.Write8(0xFF0F, bus.Read8(0xFF0F) | 0x1);
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
        byte1 = bus.Read8(addr);
        byte2 = bus.Read8(addr + 1);
    } else {
        byte1 = bus.Read8(addr - 1);
        byte2 = bus.Read8(addr);
    }
    u16 index = addr - 0x8000;

    u16 tile_index = index / 16;
    u8 row = (index % 16) / 2; // 2 bytes per row

    for (u8 col = 0; col < 8; col++) {
        u8 tile = (((byte2 >> (7 - col)) & 0b1) << 1) | ((byte1 >> (7 - col)) & 0b1);
        // TODO: palette switching
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

    if (IsSpriteDisplayEnabled()) {
        // TODO: render sprites
    }
}

void PPU::RenderBackgroundScanline() {
    u16 offset = GetBGTileMapDisplayOffset();
    u8 screen_y = ly;
    for (u8 screen_x = 0; screen_x < 160; screen_x++) {
        u16 tile_offset = offset + (screen_x / 8) + (screen_y / 8 * 32);
        // LFATAL("%04X", tile_offset);
        u8 tile_id = bus.Read8(tile_offset);

        u8 tile_y = screen_y % 8;
        u8 tile_x = screen_x % 8;
        framebuffer[160 * screen_y + screen_x] = tiles[tile_id][tile_y][tile_x];
    }
}

void PPU::RenderSprites() {
    // Sprites can be 8x16 rather than 8x8
    [[maybe_unused]] bool double_height = AreSpritesDoubleHeight();

    // There are 160 bytes of sprite memory available to us;
    // 4 bytes per sprite means we can have up to 40 sprites.
    // TODO: we are limited to 10 sprites per scanline.
    for (u8 sprite_index = 0; sprite_index < 160 / 4; sprite_index++) {
        u16 oam_address = 0xFE00 + (sprite_index * 4);
        u8 y = bus.Read8(oam_address);
        u8 x = bus.Read8(oam_address + 1);
        [[maybe_unused]] u8 tile_index = bus.Read8(oam_address + 2);
        u8 attributes = bus.Read8(oam_address + 3);

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

u8 PPU::GetLCDC() {
    return lcdc;
}

void PPU::SetLCDC(u8 value) {
    lcdc = value;
}

u8 PPU::GetSTAT() {
    return stat;
}

void PPU::SetSTAT(u8 value) {
    stat = value;
}

u8 PPU::GetLY() {
    return ly;
}

u8 PPU::GetLYC() {
    return lyc;
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
