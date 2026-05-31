#pragma once

#include <stdint.h>

#define GC_JOYSTICK_MIN 0x00
#define GC_JOYSTICK_MID 0x80
#define GC_JOYSTICK_MAX 0xFF

// GC First Byte
#define GC_MASK_A (0x1)
#define GC_MASK_B (0x1 << 1)
#define GC_MASK_X (0x1 << 2)
#define GC_MASK_Y (0x1 << 3)
#define GC_MASK_START (0x1 << 4)

// GC Second Byte
#define GC_MASK_DPAD (0xF)
#define GC_MASK_Z (0x1 << 4)
#define GC_MASK_R (0x1 << 5)
#define GC_MASK_L (0x1 << 6)
#define GC_MASK_DPAD_UP 0x8
#define GC_MASK_DPAD_UPRIGHT 0xA
#define GC_MASK_DPAD_RIGHT 0x2
#define GC_MASK_DPAD_DOWNRIGHT 0x6
#define GC_MASK_DPAD_DOWN 0x4
#define GC_MASK_DPAD_DOWNLEFT 0x5
#define GC_MASK_DPAD_LEFT 0x1
#define GC_MASK_DPAD_UPLEFT 0x9

// HID hat-switch values. 0 = neutral (reported as the null state); 1..8 are the
// eight directions, matching the values the arduino-pico Joystick library used
// so the host sees an unchanged dpad.
enum Hat : uint8_t {
    HAT_IDLE = 0,
    HAT_UP = 1,
    HAT_UP_RIGHT = 2,
    HAT_RIGHT = 3,
    HAT_DOWN_RIGHT = 4,
    HAT_DOWN = 5,
    HAT_DOWN_LEFT = 6,
    HAT_LEFT = 7,
    HAT_UP_LEFT = 8,
};

class ControllerState
{
public:
    unsigned char* state = {};

    explicit ControllerState(unsigned char state[8])
        : state(state) {};

    bool start() const { return state[0] & GC_MASK_START; }
    bool a() const { return state[0] & GC_MASK_A; }
    bool b() const { return state[0] & GC_MASK_B; }
    bool x() const { return state[0] & GC_MASK_X; }
    bool y() const { return state[0] & GC_MASK_Y; }
    bool l() const { return state[1] & GC_MASK_L; }
    bool r() const { return state[1] & GC_MASK_R; }
    bool z() const { return state[1] & GC_MASK_Z; }

    bool dpadUp() const { return state[1] & GC_MASK_DPAD & GC_MASK_DPAD_UP; }
    bool dpadRight() const { return state[1] & GC_MASK_DPAD & GC_MASK_DPAD_RIGHT; }
    bool dpadDown() const { return state[1] & GC_MASK_DPAD & GC_MASK_DPAD_DOWN; }
    bool dpadLeft() const { return state[1] & GC_MASK_DPAD & GC_MASK_DPAD_LEFT; }

    Hat dpad() const
    {
        switch (state[1] & GC_MASK_DPAD) {
            case GC_MASK_DPAD_UP: return HAT_UP;
            case GC_MASK_DPAD_UPRIGHT: return HAT_UP_RIGHT;
            case GC_MASK_DPAD_RIGHT: return HAT_RIGHT;
            case GC_MASK_DPAD_DOWNRIGHT: return HAT_DOWN_RIGHT;
            case GC_MASK_DPAD_DOWN: return HAT_DOWN;
            case GC_MASK_DPAD_DOWNLEFT: return HAT_DOWN_LEFT;
            case GC_MASK_DPAD_LEFT: return HAT_LEFT;
            case GC_MASK_DPAD_UPLEFT: return HAT_UP_LEFT;
            default: return HAT_IDLE;
        }
    }

    unsigned char ax() const { return state[2]; }
    unsigned char ay() const { return state[3]; }
    unsigned char cx() const { return state[4]; }
    unsigned char cy() const { return state[5]; }
    unsigned char al() const { return state[6]; }
    unsigned char ar() const { return state[7]; }
};
