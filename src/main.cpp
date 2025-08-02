#include <Arduino.h>

#include <PicoGamepad.h>

#include "Controller.h"

PicoGamepad gamepad;

// 16 bit integer for holding input values
int val;

Controller *controller = nullptr;

void setup() {
    Serial.begin(115200);

    controller = new Controller(new InitParams{.pin = 10});
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

void loop() {

    // Get input value from Pico analog pin
    // val = analogRead(26);
    //
    // // Map analog 0-1023 value from pin to max HID range -32767 - 32767
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

    // Set button 0 of 128 by reading button on digital pin 28
    // gamepad.SetButton(0, !digitalRead(28));

    // Set hat direction, 4 hats available. direction is clockwise 0=N 1=NE 2=E 3=SE 4=S 5=SW 6=W 7=NW 8=CENTER
    // gamepad.SetHat(0, 8);

    // gamepad.SetButton(0, controller.)

    // Send all inputs via HID
    // Nothing is send to your computer until this is called.
    gamepad.send_update();

    // Flash the LED just for fun
    // digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    delay(100);
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

