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

    struct MonochromePalette {
        Color zero;
        Color one;
        Color two;
        Color three;
    };

    PPU(Bus& bus);

    void AdvanceCycles(u64 cycles);

    void Tick();
    void UpdateTile(u16 addr);
    void RenderScanline();
    void RenderBackgroundScanline();
    void RenderSprites();

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

    bool IsLCDEnabled();
    u16 GetWindowTileMapDisplayOffset();
    bool IsWindowDisplayEnabled();
    u16 GetBGWindowTileDataOffset();
    u16 GetBGTileMapDisplayOffset();
    bool AreSpritesDoubleHeight();
    bool IsSpriteDisplayEnabled();
    bool IsBGDisplayEnabled();
private:
    Bus& bus;
    u64 vcycles;
    u8 lcdc;
    u8 stat;
    u8 scx;
    u8 scy;
    u8 ly;
    u8 lyc;
    Mode mode;

    std::array<Color, 160 * 144> framebuffer;
    Color tiles[384][8][8];
};
