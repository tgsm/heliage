#include <cmath>
#include "bus.h"
#include "logging.h"
#include "ppu.h"
#include "frontend/frontend.h"

PPU::PPU(Bus& bus)
    : bus(bus) {
}

void PPU::AdvanceCycles(u64 cycles) {
    vcycles += cycles;
}

void PPU::CheckForLYCoincidence() {
    stat &= ~0x4;

    if (ly != lyc) {
        lyc_interrupt_fired = false;
        return;
    }

    stat |= 0x4;
    if (stat & (1 << 6) && !lyc_interrupt_fired) {
        lyc_interrupt_fired = true;
        bus.Write8(0xFF0F, bus.Read8(0xFF0F, false) | 0x2, false);
    }
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

            CheckForLYCoincidence();
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
            stat &= ~0x3;
            mode = Mode::HBlank;
            CheckForLYCoincidence();
        }
            break;
        case Mode::HBlank:
            if (vcycles < 204) {
                return;
            }

            RenderScanline();

            ly++;
            vcycles %= 204;

            CheckForLYCoincidence();

            if (ly == 144) {
                mode = Mode::VBlank;
                stat &= ~0x3;
                stat |= 0x1;
                bus.Write8(0xFF0F, bus.Read8(0xFF0F, false) | 0x1, false);

                if (stat & (1 << 4)) {
                    bus.Write8(0xFF0F, bus.Read8(0xFF0F, false) | 0x2, false);
                }
            } else {
                stat &= ~0x3;
                stat |= 0x2;
                mode = Mode::AccessOAM;

                if (stat & (1 << 5)) {
                    bus.Write8(0xFF0F, bus.Read8(0xFF0F, false) | 0x2, false);
                }
            }

            break;
        case Mode::VBlank:
            if (vcycles < 456) {
                return;
            }

            ly++;
            vcycles %= 456;

            if (ly == 154) {
                DrawFramebuffer(framebuffer);

                // Clear the framebuffer
                std::fill(framebuffer.begin(), framebuffer.end(), Color::White);

                HandleEvents(bus.GetJoypad());
                ly = 0;
                window_line_counter = 0;
                mode = Mode::AccessOAM;
                stat &= ~0x3;
                stat |= 0x2;

                if (stat & (1 << 5)) {
                    bus.Write8(0xFF0F, bus.Read8(0xFF0F, false) | 0x2, false);
                }
            }

            CheckForLYCoincidence();
            break;
        default:
            UNREACHABLE_MSG("invalid PPU mode {}", static_cast<u32>(mode));
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

void PPU::UpdateSprite(u16 addr) {
    u8 sprite_index = (addr - 0xFE00) / 4;
    u16 sprite_base = 0xFE00 + (4 * sprite_index);
    Sprite* sprite = &sprites[sprite_index];
    u8 attributes = bus.Read8(sprite_base + 3, false);

    sprite->y = bus.Read8(sprite_base, false);
    sprite->x = bus.Read8(sprite_base + 1, false);
    sprite->tile_index = bus.Read8(sprite_base + 2, false);
    sprite->priority = attributes & 0x80;
    sprite->flip_y = attributes & 0x40;
    sprite->flip_x = attributes & 0x20;
    sprite->use_obp1 = attributes & 0x10;
}

void PPU::RenderScanline() {
    if (IsBGDisplayEnabled() && background_drawing_enabled) {
        RenderBackgroundScanline();
    }

    if (IsWindowDisplayEnabled() && window_drawing_enabled) {
        RenderWindowScanline();
    }

    if (IsSpriteDisplayEnabled() && sprite_drawing_enabled) {
        RenderSpriteScanline();
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

        Color color = GetColorFromBGWindowPalette(tiles[tile_id][tile_y][tile_x]);
        framebuffer[160 * screen_y + screen_x] = color;
    }
}

void PPU::RenderWindowScanline() {
    if (ly < wy) {
        return;
    }

    if (wx < 7) {
        // FIXME: Hack. Fixes Link's Awakening's HUD.
        wx = 7;
    }

    if (wx >= 167) {
        return;
    }

    if (wy >= 144) {
        return;
    }

    u16 offset = GetWindowTileMapDisplayOffset();
    bool is_signed = (GetBGWindowTileDataOffset() == 0x8800);

    for (u8 screen_x = wx - 7; screen_x < 160; screen_x++) {
        u8 scroll_x = screen_x - (wx - 7);
        u8 scroll_y = window_line_counter;
        u16 tile_offset = offset + (scroll_x / 8) + (scroll_y / 8 * 32);
        u16 tile_id = bus.Read8(tile_offset, false);
        if (is_signed && tile_id < 0x80) {
            tile_id += 0x100;
        }

        u8 tile_y = scroll_y % 8;
        u8 tile_x = scroll_x % 8;

        Color color = GetColorFromBGWindowPalette(tiles[tile_id][tile_y][tile_x]);
        framebuffer[160 * ly + screen_x] = color;
    }

    window_line_counter++;
}

void PPU::RenderSpriteScanline() {
    // One of the Gameboy's limitations is that it can only display
    // 10 sprites per scanline.
    u8 sprites_this_scanline = 0;

    // Sprites can be 8x16 rather than 8x8
    bool double_height = AreSpritesDoubleHeight();

    // There are 160 bytes of sprite memory available to us;
    // 4 bytes per sprite means we can have up to 40 sprites.
    for (u8 sprite_index = 0; sprite_index < 160 / 4; sprite_index++) {
        Sprite* sprite = &sprites[sprite_index];

        if (ly < sprite->y - 16 || ly > sprite->y) {
            continue;
        }

        // Don't draw any more sprites on this scanline if we have at least 10 sprites on it.
        if (sprites_this_scanline >= 10) {
            return;
        }

        if (double_height) {
            sprite->tile_index &= ~0x1;
        }

        for (u8 row = 0; row < 8; row++) {
            if (sprite->y + row - 16 >= 144 || sprite->y + row - 16 < 0) {
                continue;
            }

            for (u8 col = 0; col < 8; col++) {
                if (sprite->x + col - 8 >= 160 || sprite->x + col - 8 < 0) {
                    continue;
                }

                u8 x = (sprite->flip_x) ? 7 - col : col;
                u8 y = (sprite->flip_y) ? 7 - row : row;

                // Color 0 is used for transparency.
                if (tiles[sprite->tile_index][y][x] == static_cast<Color>(0b00)) {
                    continue;
                }

                Color color = GetColorFromSpritePalette(tiles[sprite->tile_index][y][x], sprite->use_obp1);
                framebuffer[160 * (sprite->y + row - 16) + (sprite->x + col - 8)] = color;
            }
        }

        sprites_this_scanline++;
    }
}

void PPU::SetBGWindowPalette(u8 value) {
    bg_window_palette.three = static_cast<Color>((value >> 6) & 0b11);
    bg_window_palette.two = static_cast<Color>((value >> 4) & 0b11);
    bg_window_palette.one = static_cast<Color>((value >> 2) & 0b11);
    bg_window_palette.zero = static_cast<Color>(value & 0b11);

    LDEBUG("PPU: new background palette: {} {} {} {}", static_cast<u8>(bg_window_palette.three),
                                                       static_cast<u8>(bg_window_palette.two),
                                                       static_cast<u8>(bg_window_palette.one),
                                                       static_cast<u8>(bg_window_palette.zero));
}

void PPU::SetOBP0(u8 value) {
    obp0.three = static_cast<Color>((value >> 6) & 0b11);
    obp0.two = static_cast<Color>((value >> 4) & 0b11);
    obp0.one = static_cast<Color>((value >> 2) & 0b11);

    LDEBUG("PPU: new OBP0 palette: {} {} {}", static_cast<u8>(obp0.three),
                                              static_cast<u8>(obp0.two),
                                              static_cast<u8>(obp0.one));
}

void PPU::SetOBP1(u8 value) {
    obp1.three = static_cast<Color>((value >> 6) & 0b11);
    obp1.two = static_cast<Color>((value >> 4) & 0b11);
    obp1.one = static_cast<Color>((value >> 2) & 0b11);

    LDEBUG("PPU: new OBP1 palette: {} {} {}", static_cast<u8>(obp1.three),
                                              static_cast<u8>(obp1.two),
                                              static_cast<u8>(obp1.one));
}

PPU::Color PPU::GetColorFromBGWindowPalette(Color color) {
    switch (static_cast<u8>(color)) {
        case 0b00:
            return bg_window_palette.zero;
        case 0b01:
            return bg_window_palette.one;
        case 0b10:
            return bg_window_palette.two;
        case 0b11:
            return bg_window_palette.three;
        default:
            // UNREACHABLE_MSG("invalid PPU color {}", static_cast<u8>(color));
            return bg_window_palette.zero;
    }
}

PPU::Color PPU::GetColorFromSpritePalette(Color color, bool use_obp1) {
    SpritePalette palette = (use_obp1) ? obp1 : obp0;

    switch (static_cast<u8>(color)) {
        case 0b01:
            return palette.one;
        case 0b10:
            return palette.two;
        case 0b11:
            return palette.three;
        default:
            UNREACHABLE_MSG("invalid PPU color {}", static_cast<u8>(color));
    }
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
