#pragma once

#include <cstdio>
#include <Arduino.h>
#include "hardware/pio.h"

typedef enum { N64 = 0x05, Gamecube = 0x09 } ControllerType;

typedef struct {
    uint8_t pin;
    ControllerType controllerType;
    PIO pio;
    uint sm;
    pio_sm_config *c;
    uint offset;
} InitParams;

class Controller {
public:
    explicit Controller(InitParams *initParams, uint8_t sizeofControllerState = 8);

    void init();

    void setRumble(bool rumble) { _rumble = rumble; };

    static void initPio(InitParams *initParams);

    void transfer(uint8_t *request, uint8_t requestLength, uint8_t *response,
                  uint8_t responseLength);

    double getScaledAnalogAxis(double axisPos, double *minAxis, double *maxAxis);
    void updateState();

    uint8_t* getControllerState() const {
        return _controllerState;
    }

    double getX() {
        return getScaledAnalogAxis(_controllerState[2], &_minAnalogX, &_maxAnalogX);
    }

    double getY() {
        return getScaledAnalogAxis(_controllerState[3], &_minAnalogY, &_maxAnalogY);
    }

    double getCX() {
        return getScaledAnalogAxis(_controllerState[4], &_minCX, &_maxCX);
    }

    double getCY() {
        return getScaledAnalogAxis(_controllerState[5], &_minCY, &_maxCY);
    }

    double getL() {
        return getScaledAnalogAxis(_controllerState[6], &_minL, &_maxL);
    }

    double getR() {
        return getScaledAnalogAxis(_controllerState[7], &_minL, &_maxL);
    }

private:
    static void transfer(PIO pio, uint sm, uint8_t *request,
                         uint8_t requestLength, uint8_t *response,
                         uint8_t responseLength);

    static void sendRequest(PIO pio, uint sm, uint8_t *request,
                     uint8_t requestLength);

    static void getResponse(PIO pio, uint sm, uint8_t *response,
                     uint8_t responseLength);

protected:

    uint8_t _pin;
    PIO _pio;
    uint _sm;
    pio_sm_config *_c;
    uint _offset;
    uint8_t *_controllerState;
    uint8_t _sizeofControllerState;
    bool _rumble = false;
    double _maxAnalogX = 0.5;
    double _minAnalogX = -0.5;
    double _maxAnalogY = 0.5;
    double _minAnalogY = -0.5;
    double _maxCX = 0.5;
    double _minCX = -0.5;
    double _maxCY = 0.5;
    double _minCY = -0.5;
    double _maxL = 0.5;
    double _minL = -0.5;
    double _maxR = 0.5;
    double _minR = -0.5;
};


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
