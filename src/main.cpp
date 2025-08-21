#include <Arduino.h>

#include <PicoGamepad.h>

#include "Controller.h"
#include "ControllerState.h"
// #include "GamecubeHID.h"
#include "pico/bootrom.h"

PicoGamepad gamepad(true, 0xCafe, 0x4000, 0x0001);
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

void loop()
{
    controller->updateState();
    
    // Convert GameCube analog values (0-255) to PicoGamepad format (-32767 to 32767)
    // GameCube center is 128, so we convert: (value - 128) * 256
    gamepad.SetX((state->ax() - 128) * 256);
    gamepad.SetY((state->ay() - 128) * 256);
    gamepad.SetRx((state->cx() - 128) * 256);
    gamepad.SetRy((state->cy() - 128) * 256);

    // Triggers are 0-255, convert to 0-32767 for PicoGamepad
    gamepad.SetZ(state->al() * 128);
    gamepad.SetRz(state->ar() * 128);

    // Map GameCube buttons to PicoGamepad buttons
    gamepad.SetButton(0, state->a());        // A button
    gamepad.SetButton(1, state->b());        // B button  
    gamepad.SetButton(2, state->x());        // X button
    gamepad.SetButton(3, state->y());        // Y button
    gamepad.SetButton(4, state->start());    // Start button
    gamepad.SetButton(5, state->l());        // L button
    gamepad.SetButton(6, state->r());        // R button
    gamepad.SetButton(7, state->z());        // Z button

    // Map D-pad to hat switch (0=N, 2=E, 4=S, 6=W, 8=Center)
    uint8_t hatDir = HAT_DIR_C; // Default to center
    if (state->dpadUp() && !state->dpadRight() && !state->dpadDown() && !state->dpadLeft()) hatDir = HAT_DIR_N;
    else if (state->dpadUp() && state->dpadRight() && !state->dpadDown() && !state->dpadLeft()) hatDir = HAT_DIR_NE;
    else if (!state->dpadUp() && state->dpadRight() && !state->dpadDown() && !state->dpadLeft()) hatDir = HAT_DIR_E;
    else if (!state->dpadUp() && state->dpadRight() && state->dpadDown() && !state->dpadLeft()) hatDir = HAT_DIR_SE;
    else if (!state->dpadUp() && !state->dpadRight() && state->dpadDown() && !state->dpadLeft()) hatDir = HAT_DIR_S;
    else if (!state->dpadUp() && !state->dpadRight() && state->dpadDown() && state->dpadLeft()) hatDir = HAT_DIR_SW;
    else if (!state->dpadUp() && !state->dpadRight() && !state->dpadDown() && state->dpadLeft()) hatDir = HAT_DIR_W;
    else if (state->dpadUp() && !state->dpadRight() && !state->dpadDown() && state->dpadLeft()) hatDir = HAT_DIR_NW;
    
    gamepad.SetHat(0, hatDir);

    // Check for bootloader reset combo
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
