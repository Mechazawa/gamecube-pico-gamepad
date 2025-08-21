#include <Arduino.h>

#include <PicoGamepad.h>

#include "Controller.h"
#include "ControllerState.h"
#include "pico/bootrom.h"

PicoGamepad gamepad;
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
}

inline uint16_t scaleAnalog(const double value)
{
    return static_cast<uint16_t>((value * 2 * 32767) - 32767);
}

void loop()
{
    controller->updateState();

    // Buttons
    gamepad.SetButton(0, state->start());
    gamepad.SetButton(1, state->a());
    gamepad.SetButton(2, state->b());
    gamepad.SetButton(3, state->x());
    gamepad.SetButton(4, state->y());
    gamepad.SetButton(5, state->l());
    gamepad.SetButton(6, state->r());
    gamepad.SetButton(7, state->z());

    // DPad
    gamepad.SetButton(8, state->dpadUp());
    gamepad.SetButton(9, state->dpadRight());
    gamepad.SetButton(10, state->dpadDown());
    gamepad.SetButton(11, state->dpadLeft());

    // Analog values
    gamepad.SetX(scaleAnalog(state->ax()));
    gamepad.SetY(scaleAnalog(state->ay()));
    gamepad.SetZ(scaleAnalog(state->al()));
    gamepad.SetRx(scaleAnalog(state->cx()));
    gamepad.SetRy(scaleAnalog(state->cy()));
    gamepad.SetRz(scaleAnalog(state->ar()));

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

    gamepad.send_update();
    delay(10);
}
