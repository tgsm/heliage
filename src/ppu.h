#pragma once

#include <array>
#include "common/bits.h"
#include "common/types.h"

class Bus;

class PPU {
public:
    enum class Mode {
        HBlank,
        VBlank,
        AccessOAM,
        AccessVRAM,
    };

    enum class Color : u8 {
        White = 0b00,
        LightGray = 0b01,
        DarkGray = 0b10,
        Black = 0b11,
    };

    PPU(Bus& bus);

    void AdvanceCycles(u64 cycles);

    void Tick();
    void UpdateTile(u16 addr);

    void UpdateSprite(u16 addr);

    u8 GetLCDC() const { return lcdc; }
    void SetLCDC(u8 value) { lcdc = value; }

    u8 GetSTAT() const { return stat; }
    void SetSTAT(u8 value) { stat = value; }

    u8 GetSCY() const { return scy; }
    void SetSCY(u8 value) { scy = value; }

    u8 GetSCX() const { return scx; }
    void SetSCX(u8 value) { scx = value; }

    u8 GetLY() const { return ly; }

    u8 GetLYC() const { return lyc; }
    void SetLYC(u8 value) { lyc = value; }

    void SetBGWindowPalette(u8 value);
    void SetOBP0(u8 value);
    void SetOBP1(u8 value);

    u8 GetWY() const { return wy; }
    void SetWY(u8 value) { wy = value; }

    u8 GetWX() const { return wx; }
    void SetWX(u8 value) { wx = value; }

    bool IsLCDEnabled() const { return Common::IsBitSet<7>(lcdc); }
    u16 GetWindowTileMapDisplayOffset() const { return (Common::IsBitSet<6>(lcdc)) ? 0x9C00 : 0x9800; }
    bool IsWindowDisplayEnabled() const { return Common::IsBitSet<5>(lcdc); }
    u16 GetBGWindowTileDataOffset() const { return Common::IsBitSet<4>(lcdc) ? 0x8000 : 0x8800; }
    u16 GetBGTileMapDisplayOffset() const { return Common::IsBitSet<3>(lcdc) ? 0x9C00 : 0x9800; }
    bool AreSpritesDoubleHeight() const { return Common::IsBitSet<2>(lcdc); }
    bool IsSpriteDisplayEnabled() const { return Common::IsBitSet<1>(lcdc); }
    bool IsBGDisplayEnabled() const { return Common::IsBitSet<0>(lcdc); }

    void SetBGDrawingEnabled(bool enabled) { background_drawing_enabled = enabled; }
    void SetWindowDrawingEnabled(bool enabled) { window_drawing_enabled = enabled; }
    void SetSpriteDrawingEnabled(bool enabled) { sprite_drawing_enabled = enabled; }
private:
    Bus& bus;
    u64 vcycles = 0;
    u8 lcdc = 0x00;
    u8 stat = 0x80;
    u8 scx = 0x00;
    u8 scy = 0x00;
    u8 ly = 0x00;
    u8 lyc = 0x00;
    u8 wy = 0x00;
    u8 wx = 0x00;
    Mode mode = Mode::AccessOAM;

    bool lyc_interrupt_fired = false;
    void CheckForLYCoincidence();

    struct {
        Color three;
        Color two;
        Color one;
        Color zero;
    } bg_window_palette;

    // Sprite palettes have 3 colors instead of 4. The lost color is used for transparency.
    struct SpritePalette {
        Color three;
        Color two;
        Color one;
    };

    SpritePalette obp0, obp1;

    struct Sprite {
        u8 y = 0;
        u8 x = 0;
        u8 tile_index = 0;
        bool use_obp1 = false;
        bool flip_x = false;
        bool flip_y = false;
        bool priority = false;
    };

    std::array<Sprite, 40> sprites = {};

    std::array<Color, 160 * 144> framebuffer;
    Color tiles[384][8][8];

    Color GetColorFromBGWindowPalette(Color color);
    Color GetColorFromSpritePalette(Color color, bool use_obp1);

    void RenderScanline();
    void RenderBackgroundScanline();
    void RenderWindowScanline();
    void RenderSpriteScanline();

    u8 window_line_counter = 0;

    bool background_drawing_enabled = true;
    bool window_drawing_enabled = true;
    bool sprite_drawing_enabled = true;
};
