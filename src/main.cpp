#include <Arduino.h>

// #include <PicoGamepad.h>

#include "Controller.h"
#include "ControllerState.h"
#include "GamepadHID.h"
#include "pico/bootrom.h"

GamepadHID gamepad;
Controller* controller = nullptr;
ControllerState* state = nullptr;

void setup()
{
    auto* initParams = new InitParams();
    initParams->pin = 10;

    Controller::initPio(initParams);

    controller = new Controller(initParams, 8);
    controller->init();

    state = new ControllerState(controller->getRawControllerState());

    gamepad.connect();
}

void loop()
{
    uint8_t lx = state->ax();
    uint8_t ly = state->ay();
    uint8_t rx = state->cx();
    uint8_t ry = state->cy();

    // Triggers (0..255)
    uint8_t lt = state->al();
    uint8_t rt = state->ar();

    uint16_t buttons =
        (state->start() << 0) |
        (state->a() << 1) |
        (state->b() << 2) |
        (state->x() << 3) |
        (state->y() << 4) |
        (state->l() << 5) |
        (state->r() << 6) |
        (state->z() << 7);

    gamepad.sendState(
        buttons,
        lx,
        ly,
        rx /*Z*/,
        lt /*Rx*/,
        rt /*Ry*/,
        ry /*Rz (Right Y)*/
    );

    // Handle rumble OUT report
    bool rumbleOn;
    if (gamepad.pollRumble(rumbleOn))
    {
        controller->setRumble(rumbleOn);
    }

    if (
        state->a() &&
        state->b() &&
        state->l() &&
        state->r() &&
        state->start()
    )
    {
        reset_usb_boot(0, 0);
    }

    delay(10);
}
