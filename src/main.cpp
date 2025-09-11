#include <Arduino.h>

#include "Controller.h"
#include "ControllerState.h"
#include "GamepadHID.h"

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

    gamepad.init();
    gamepad.connect();
    
    // Give USB time to enumerate
    delay(2000);
    
    gamepad.wait_ready();

    controller->setRumble(true);
    controller->updateState();
    delay(200);
    controller->setRumble(false);
}

void loop()
{
    controller->updateState();

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
        // (state->dpadUp() << (8 + 4)) |
        // (state->dpadRight() << (9 + 4)) |
        // (state->dpadDown() << (10 + 4)) |
        // (state->dpadLeft() << (11 + 4));

    // Send HID report every loop iteration
    gamepad.sendState(
        buttons,
        lx,
        ly,
        rx /*Z*/,
        lt /*Rx*/,
        rt /*Ry*/,
        ry /*Rz (Right Y)*/
    );

    // Handle rumble from HID OUT reports (host-controlled)
    // bool rumbleOn;
    // if (gamepad.pollRumble(rumbleOn)) {
    //     controller->setRumble(rumbleOn);
    // } else {
    //     // Fallback: manual rumble control (Start+A)
    //     // controller->setRumble(state->start() && state->a());
    // }

    if (
        state->a() &&
        state->b() &&
        state->l() &&
        state->r() &&
        state->start()
    )
    {
        rp2040.reboot();
    }

    delay(10);
}
