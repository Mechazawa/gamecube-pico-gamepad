#include <Arduino.h>

#include <PicoGamepad.h>

#include "Controller.h"

PicoGamepad gamepad;

// 16 bit integer for holding input values
int val;

Controller *controller = nullptr;

void setup() {
    auto *initParams = new InitParams();
    initParams->pin = 10;

    Controller::initPio(initParams);

    controller = new Controller(initParams, 8);
    controller->init();

    // pinMode(LED_BUILTIN, OUTPUT);

    // X Potentiometer on pin 26
    // pinMode(26, INPUT);
    // // Y Potentiometer on pin 27
    // pinMode(27, INPUT);
    //
    // // Button on pin
    // pinMode(28, INPUT_PULLUP);
}

uint16_t scaleAnalog(const double value) {
    return static_cast<uint16_t>((value * 2 * 32767) - 32767);
}

void loop() {
    controller->updateState();

    // // Get input value from Pico analog pin
    // val = analogRead(26);
    //
    // // // Map analog 0-1023 value from pin to max HID range -32767 - 32767
    // val = map(val, 0, 1023, -32767, 32767);
    //
    // // Send value to HID object
    // gamepad.SetX(val);
    //
    // // Repeat with Y pin
    // val = analogRead(27);
    // val = map(val, 0, 1023, -32767, 32767);
    // gamepad.SetY(val);

    // Other Axis Options
    /* BY INDEX 0-15 */
    // gamepad.SetAxis(0, val);

    /* BY NAME */
    // gamepad.SetZ(val);
    // gamepad.SetRx(val);
    // gamepad.SetRy(val);
    // gamepad.SetRz(val);
    // gamepad.SetSlider(val);
    // gamepad.SetDial(val);
    // gamepad.SetWheel(val);
    // gamepad.SetVx(val);
    // gamepad.SetVy(val);
    // gamepad.SetVz(val);
    // gamepad.SetVbrx(val);
    // gamepad.SetVbry(val);
    // gamepad.SetVbrz(val);
    // gamepad.SetVno(val);
    // gamepad.SetUndefined(val);

    const uint8_t *controllerState = controller->getControllerState();

    // Buttons
    gamepad.SetButton(0, controllerState[0] & GC_MASK_START);
    gamepad.SetButton(1, controllerState[0] & GC_MASK_A);
    gamepad.SetButton(2, controllerState[0] & GC_MASK_B);
    gamepad.SetButton(3, controllerState[0] & GC_MASK_X);
    gamepad.SetButton(4, controllerState[0] & GC_MASK_Y);
    gamepad.SetButton(5, controllerState[1] & GC_MASK_L);
    gamepad.SetButton(6, controllerState[1] & GC_MASK_R);
    gamepad.SetButton(7, controllerState[1] & GC_MASK_Z);

    // Set hat direction, 4 hats available. direction is clockwise 0=N 1=NE 2=E 3=SE 4=S 5=SW 6=W 7=NW 8=CENTER
    switch (GC_MASK_DPAD & controllerState[1]) {
        case GC_MASK_DPAD_UP:
            gamepad.SetHat(0, HAT_DIR_N);
            break;
        case GC_MASK_DPAD_UPRIGHT:
            gamepad.SetHat(0, HAT_DIR_NE);
            break;
        case GC_MASK_DPAD_RIGHT:
            gamepad.SetHat(0, HAT_DIR_E);
            break;
        case GC_MASK_DPAD_DOWNRIGHT:
            gamepad.SetHat(0, HAT_DIR_SE);
            break;
        case GC_MASK_DPAD_DOWN:
            gamepad.SetHat(0, HAT_DIR_S);
            break;
        case GC_MASK_DPAD_DOWNLEFT:
            gamepad.SetHat(0, HAT_DIR_SW);
            break;
        case GC_MASK_DPAD_LEFT:
            gamepad.SetHat(0, HAT_DIR_W);
            break;
        case GC_MASK_DPAD_UPLEFT:
            gamepad.SetHat(0, HAT_DIR_NW);
            break;
        default:
            gamepad.SetHat(0, HAT_DIR_C);
            break;
    }

    gamepad.SetX(scaleAnalog(controller->getX()));
    gamepad.SetY(scaleAnalog(controller->getY()));
    gamepad.SetZ(scaleAnalog(controller->getL()));
    gamepad.SetRx(scaleAnalog(controller->getCX()));
    gamepad.SetRy(scaleAnalog(controller->getCY()));
    gamepad.SetRz(scaleAnalog(controller->getR()));

    // gamepad.SetButton(0, controller.)

    // Send all inputs via HID
    // Nothing is sent to your computer until this is called.
    gamepad.send_update();

    // Flash the LED just for fun
    // digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    delay(10);
}


// void GamecubeController::getXboxReport(XboxReport *xboxReport) {
//   updateState();
//
//   // Map GameCube buttons to Xbox buttons
//   xboxReport->buttons = 0;
//   xboxReport->buttons |= (GC_MASK_A & _controllerState[0] ? XBOX_MASK_A : 0);
//   xboxReport->buttons |= (GC_MASK_B & _controllerState[0] ? XBOX_MASK_B : 0);
//   xboxReport->buttons |= (GC_MASK_X & _controllerState[0] ? XBOX_MASK_X : 0);
//   xboxReport->buttons |= (GC_MASK_Y & _controllerState[0] ? XBOX_MASK_Y : 0);
//   xboxReport->buttons |= (GC_MASK_START & _controllerState[0] ? XBOX_MASK_MENU : 0);
//   xboxReport->buttons |= (GC_MASK_L & _controllerState[1] ? XBOX_MASK_LB : 0);
//   xboxReport->buttons |= (GC_MASK_R & _controllerState[1] ? XBOX_MASK_RB : 0);
//   xboxReport->buttons |= (GC_MASK_Z & _controllerState[1] ? XBOX_MASK_VIEW : 0);
//
//   // Map D-pad
//   switch (GC_MASK_DPAD & _controllerState[1]) {
//     case GC_MASK_DPAD_UP:
//       xboxReport->buttons |= XBOX_MASK_DPAD_UP;
//       break;
//     case GC_MASK_DPAD_UPRIGHT:
//       xboxReport->buttons |= XBOX_MASK_DPAD_UP | XBOX_MASK_DPAD_RIGHT;
//       break;
//     case GC_MASK_DPAD_RIGHT:
//       xboxReport->buttons |= XBOX_MASK_DPAD_RIGHT;
//       break;
//     case GC_MASK_DPAD_DOWNRIGHT:
//       xboxReport->buttons |= XBOX_MASK_DPAD_DOWN | XBOX_MASK_DPAD_RIGHT;
//       break;
//     case GC_MASK_DPAD_DOWN:
//       xboxReport->buttons |= XBOX_MASK_DPAD_DOWN;
//       break;
//     case GC_MASK_DPAD_DOWNLEFT:
//       xboxReport->buttons |= XBOX_MASK_DPAD_DOWN | XBOX_MASK_DPAD_LEFT;
//       break;
//     case GC_MASK_DPAD_LEFT:
//       xboxReport->buttons |= XBOX_MASK_DPAD_LEFT;
//       break;
//     case GC_MASK_DPAD_UPLEFT:
//       xboxReport->buttons |= XBOX_MASK_DPAD_UP | XBOX_MASK_DPAD_LEFT;
//       break;
//   }
//
//   // Special combination: L + R + Start = Xbox Home button
//   if ((GC_MASK_L & _controllerState[1]) &&
//       (GC_MASK_R & _controllerState[1]) &&
//       (GC_MASK_START & _controllerState[0])) {
//     xboxReport->buttons |= XBOX_MASK_XBOX;
//   }
//
//   // Convert analog triggers (GameCube triggers are analog)
//   xboxReport->leftTrigger = convertToXboxTrigger(_controllerState[6]);
//   xboxReport->rightTrigger = convertToXboxTrigger(_controllerState[7]);
//
//   // Convert analog sticks
//   xboxReport->leftStickX = convertToXboxJoystick(_controllerState[2], &_minAnalogX, &_maxAnalogX);
//   xboxReport->leftStickY = -convertToXboxJoystick(_controllerState[3], &_minAnalogY, &_maxAnalogY); // Invert Y
//   xboxReport->rightStickX = convertToXboxJoystick(_controllerState[4], &_minCX, &_maxCX);
//   xboxReport->rightStickY = -convertToXboxJoystick(_controllerState[5], &_minCY, &_maxCY); // Invert Y
//
//   return;
// }
