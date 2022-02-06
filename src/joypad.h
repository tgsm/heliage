#pragma once

#include "common/types.h"

class Bus;

class Joypad {
public:
    enum class Button {
        Up,
        Down,
        Left,
        Right,
        A,
        B,
        Select,
        Start,
    };

    Joypad() = default;

    u8 Read();
    void Write(u8 value);

    void PressButton(Button button);
    void ReleaseButton(Button button);
private:
    bool buttons_selected = false;
    bool directions_selected = false;

    bool up = false;
    bool down = false;
    bool left = false;
    bool right = false;
    bool a = false;
    bool b = false;
    bool select = false;
    bool start = false;

    u8 buttons_state = 0xFF;
};
