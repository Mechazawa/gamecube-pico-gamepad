#include <Arduino.h>

// #include <PicoGamepad.h>

#include "Controller.h"
#include "ControllerState.h"
#include "GamecubeHID.h"
#include "pico/bootrom.h"

GamecubeHID gamepad;
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
}

void loop()
{
    controller->updateState();

    gamepad.setLX(state->ax());
    gamepad.setLY(state->ay());
    gamepad.setCX(state->cx());
    gamepad.setCY(state->cy());

    gamepad.setLT(state->al());
    gamepad.setRT(state->ar());

    gamepad.setButton(GamecubeHID::START, state->start());
    gamepad.setButton(GamecubeHID::A, state->a());
    gamepad.setButton(GamecubeHID::B, state->b());
    gamepad.setButton(GamecubeHID::X, state->x());
    gamepad.setButton(GamecubeHID::Y, state->y());
    gamepad.setButton(GamecubeHID::L, state->l());
    gamepad.setButton(GamecubeHID::R, state->r());
    gamepad.setButton(GamecubeHID::Z, state->z());

    gamepad.setDpad(
        state->dpadUp(),
        state->dpadRight(),
        state->dpadDown(),
        state->dpadLeft()
    );

    if (gamepad.pollRumble()) {
        // Rumble changed, update controller state
        controller->setRumble(gamepad.rumble());
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

    gamepad.send();
    delay(10);
}
