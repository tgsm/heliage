#pragma once

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

    void Tick(u8 cycles);
    void UpdateTile(u16 addr);
    void RenderScanline();
    void RenderBackgroundScanline();
    void RenderSprites();

    u8 GetLCDC();
    void SetLCDC(u8 value);
    u8 GetSTAT();
    void SetSTAT(u8 value);
    u8 GetLY();
    u8 GetLYC();

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
    u8 ly;
    u8 lyc;
    Mode mode;

    Color* framebuffer = {};
    Color tiles[384][8][8];
};