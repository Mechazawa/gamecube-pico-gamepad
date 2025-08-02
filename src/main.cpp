#include <Arduino.h>

#include <PicoGamepad.h>

#include "Controller.h"
#include "pico/bootrom.h"

PicoGamepad gamepad;
Controller* controller = nullptr;

void setup()
{
    auto* initParams = new InitParams();
    initParams->pin = 10;

    Controller::initPio(initParams);

    controller = new Controller(initParams, 8);
    controller->init();
}

inline uint16_t scaleAnalog(const double value)
{
    return static_cast<uint16_t>((value * 2 * 32767) - 32767);
}

void loop()
{
    controller->updateState();

    const uint8_t* controllerState = controller->getControllerState();

    // Buttons
    gamepad.SetButton(0, controllerState[0] & GC_MASK_START);
    gamepad.SetButton(1, controllerState[0] & GC_MASK_A);
    gamepad.SetButton(2, controllerState[0] & GC_MASK_B);
    gamepad.SetButton(3, controllerState[0] & GC_MASK_X);
    gamepad.SetButton(4, controllerState[0] & GC_MASK_Y);
    gamepad.SetButton(5, controllerState[1] & GC_MASK_L);
    gamepad.SetButton(6, controllerState[1] & GC_MASK_R);
    gamepad.SetButton(7, controllerState[1] & GC_MASK_Z);

    // DPad
    gamepad.SetButton(8, GC_MASK_DPAD & controllerState[1] & GC_MASK_DPAD_UP);
    gamepad.SetButton(9, GC_MASK_DPAD & controllerState[1] & GC_MASK_DPAD_RIGHT);
    gamepad.SetButton(10, GC_MASK_DPAD & controllerState[1] & GC_MASK_DPAD_DOWN);
    gamepad.SetButton(11, GC_MASK_DPAD & controllerState[1] & GC_MASK_DPAD_LEFT);

    // Analog values
    gamepad.SetX(scaleAnalog(controller->getX()));
    gamepad.SetY(scaleAnalog(controller->getY()));
    gamepad.SetZ(scaleAnalog(controller->getL()));
    gamepad.SetRx(scaleAnalog(controller->getCX()));
    gamepad.SetRy(scaleAnalog(controller->getCY()));
    gamepad.SetRz(scaleAnalog(controller->getR()));

    if (
        (controllerState[1] & GC_MASK_L) &&
        (controllerState[1] & GC_MASK_R) &&
        (controllerState[0] & GC_MASK_START) &&
        (controllerState[0] & GC_MASK_A) &&
        (controllerState[0] & GC_MASK_B) &&
        (controllerState[0] & GC_MASK_X) &&
        (controllerState[0] & GC_MASK_Y)
    )
    {
        reset_usb_boot(0, 0);
    }

    gamepad.send_update();
    delay(10);
}
