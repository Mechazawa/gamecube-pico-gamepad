#pragma once

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


#define DEFAULT_ANALOG_CONTROLLER_SCALE_MIN (-0.01)
#define DEFAULT_ANALOG_CONTROLLER_SCALE_MAX 0.01

class AnalogScaler
{
private:
    bool _dirty = false;

public:
    double min;
    double max;

    explicit AnalogScaler(const double min = DEFAULT_ANALOG_CONTROLLER_SCALE_MIN, const double max = DEFAULT_ANALOG_CONTROLLER_SCALE_MAX)
        : min(min), max(max) {}

    double scale(const double value)
    {
        _dirty = _dirty || value > max || value < min;

        max = value > max ? value : max;
        min = value < min ? value : min;

        return value > 0 ? value / max : -value / min;
    }

    bool dirty() const
    {
        return _dirty;
    }
};

typedef struct {
    AnalogScaler ax;
    AnalogScaler ay;
    AnalogScaler cx;
    AnalogScaler cy;
    AnalogScaler al;
    AnalogScaler ar;
} ScalerBundle;

class ControllerState
{
public:
    unsigned char* state = {};
    ScalerBundle scalers;

    // explicit ControllerState(Controller* controller, ScalerBundle scalers = ScalerBundle())
    //     : state(controller->getRawControllerState()), scalers(scalers) {};

    explicit ControllerState(unsigned char state[8], ScalerBundle scalers = ScalerBundle())
        : state(state), scalers(scalers) {};

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

    double ax() {
        return scalers.ax.scale(state[2]);
    }

    double ay() {
        return scalers.ay.scale(state[3]);
    }

    double cx() {
        return scalers.cx.scale(state[4]);
    }

    double cy() {
        return scalers.cy.scale(state[5]);
    }

    double al() {
        return scalers.al.scale(state[6]);
    }

    double ar() {
        return scalers.ar.scale(state[7]);
    }
};
