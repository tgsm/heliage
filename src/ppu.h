#pragma once

#include <array>
#include "types.h"

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

    u8 GetLCDC() { return lcdc; }
    void SetLCDC(u8 value) { lcdc = value; }

    u8 GetSTAT() { return stat; }
    void SetSTAT(u8 value) { stat = value; }

    u8 GetSCY() { return scy; }
    void SetSCY(u8 value) { scy = value; }

    u8 GetSCX() { return scx; }
    void SetSCX(u8 value) { scx = value; }

    u8 GetLY() { return ly; }

    u8 GetLYC() { return lyc; }
    void SetLYC(u8 value) { lyc = value; }

    void SetBGWindowPalette(u8 value);

    u8 GetWY() { return wy; }
    void SetWY(u8 value) { wy = value; }

    u8 GetWX() { return wx; }
    void SetWX(u8 value) { wx = value; }

    bool IsLCDEnabled();
    u16 GetWindowTileMapDisplayOffset();
    bool IsWindowDisplayEnabled();
    u16 GetBGWindowTileDataOffset();
    u16 GetBGTileMapDisplayOffset();
    bool AreSpritesDoubleHeight();
    bool IsSpriteDisplayEnabled();
    bool IsBGDisplayEnabled();

    void SetBGDrawingEnabled(bool enabled) { background_drawing_enabled = enabled; }
    void SetWindowDrawingEnabled(bool enabled) { window_drawing_enabled = enabled; }
private:
    Bus& bus;
    u64 vcycles;
    u8 lcdc;
    u8 stat;
    u8 scx;
    u8 scy;
    u8 ly;
    u8 lyc;
    u8 wy;
    u8 wx;
    Mode mode;

    bool lyc_interrupt_fired = false;
    void CheckForLYCoincidence();

    struct {
        Color three;
        Color two;
        Color one;
        Color zero;
    } bg_window_palette;

    std::array<Color, 160 * 144> framebuffer;
    Color tiles[384][8][8];

    Color GetColorFromPalette(Color color);

    void RenderScanline();
    void RenderBackgroundScanline();
    void RenderWindowScanline();
    void RenderSprites();

    u8 window_line_counter = 0;

    bool window_drawing_enabled = true;
    bool background_drawing_enabled = true;
};
