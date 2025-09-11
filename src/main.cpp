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

    // gamepad.init();
    gamepad.connect();
}

void loop()
{
    // controller->updateState();

    // gamepad.setLX(state->ax());
    // gamepad.setLY(state->ay());
    // gamepad.setCX(state->cx());
    // gamepad.setCY(state->cy());
    //
    // gamepad.setLT(state->al());
    // gamepad.setRT(state->ar());
    //
    // gamepad.setButton(GamecubeHID::START, state->start());
    // gamepad.setButton(GamecubeHID::A, state->a());
    // gamepad.setButton(GamecubeHID::B, state->b());
    // gamepad.setButton(GamecubeHID::X, state->x());
    // gamepad.setButton(GamecubeHID::Y, state->y());
    // gamepad.setButton(GamecubeHID::L, state->l());
    // gamepad.setButton(GamecubeHID::R, state->r());
    // gamepad.setButton(GamecubeHID::Z, state->z());
    //
    // // Note the DPad is not a hat and allows multiple directions to be pressed at once
    // gamepad.setDpad(
    //     state->dpadUp(),
    //     state->dpadRight(),
    //     state->dpadDown(),
    //     state->dpadLeft()
    // );

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

    // if (gamepad.pollRumble()) {
    //     // Rumble changed, update controller state
    //     controller->setRumble(gamepad.rumble());
    // }

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

    // gamepad.send();
    delay(10);
}
