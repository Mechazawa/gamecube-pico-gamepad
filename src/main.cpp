#include <Arduino.h>

#include "Controller.h"
#include "ControllerState.h"
#include <Joystick.h>

Controller *controller = nullptr;
ControllerState *state = nullptr;

void setup() {
    auto *initParams = new InitParams();
    initParams->pin = 10;

    Controller::initPio(initParams);

    controller = new Controller(initParams, 8);
    controller->init();

    state = new ControllerState(controller->getRawControllerState());

    Joystick.begin();
    Joystick.useManualSend(true);

    // Give USB time to enumerate
    delay(2000);

    controller->setRumble(true);
    controller->updateState();
    delay(200);
    controller->setRumble(false);
}

void loop() {
    controller->updateState();

    Joystick.setButton(0, state->a());
    Joystick.setButton(1, state->x());
    Joystick.setButton(2, state->start());
    Joystick.setButton(3, state->y());
    Joystick.setButton(4, state->b());
    Joystick.setButton(5, state->l());
    Joystick.setButton(6, state->r());
    Joystick.setButton(7, state->z());

    Joystick.hat(state->dpad());

    Joystick.X(state->ax() * 4);
    Joystick.Y(1023 - (state->ay() * 4));
    Joystick.Z(state->al() * 4);
    Joystick.Zrotate(state->ar() * 4);
    Joystick.sliderLeft(state->cx() * 4);
    Joystick.sliderRight(1023 - (state->cy() * 4));

    Joystick.send_now();

    if (
        state->a() &&
        state->b() &&
        state->l() &&
        state->r() &&
        state->start()
    ) {
        rp2040.rebootToBootloader();
    }

    delay(10);
}
