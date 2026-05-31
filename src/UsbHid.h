#pragma once

#include <Arduino.h>
#include <stdint.h>

// USB HID gamepad with a host-controllable rumble output.
//
// The input report (report id 1) is byte-for-byte compatible with the
// arduino-pico core "GAMEPAD16" report that this project used before the
// TinyUSB switch: 6x int16 axes, 1 hat, 32 buttons. The host therefore sees
// the same controller it always did.
//
// Rumble is exposed as an OUTPUT the host can write (report id 2). Iteration 1
// uses a simple vendor output report so the rumble motor can be driven over
// hidraw; a PID force-feedback collection (so the OS/games rumble it natively)
// is layered on top in a later iteration.

struct __attribute__((packed)) GamepadReport {
    int16_t x;        // main stick X
    int16_t y;        // main stick Y
    int16_t z;        // left analog trigger
    int16_t rz;       // right analog trigger
    int16_t rx;       // C-stick X
    int16_t ry;       // C-stick Y
    uint8_t hat;      // 0 = neutral, 1..8 = dpad direction
    uint32_t buttons; // bit0..bit31
};

namespace UsbHid {
    // Configure descriptor + callbacks and start the device. Safe to call once
    // from setup(); handles the rp2040 detach/attach re-enumeration dance.
    void begin(uint16_t vid, uint16_t pid, const char *manufacturer, const char *product);

    bool mounted();         // USB enumerated by a host
    bool ready();           // IN endpoint free for a new input report
    bool sendGamepad(const GamepadReport &report);

    // Rumble commanded by the host (latest output report). rumbleOn() is the
    // boolean the firmware feeds to the GameCube poll; rumbleRaw() is the last
    // magnitude byte; rxCount() counts output reports received (for diagnostics).
    bool rumbleOn();
    uint8_t rumbleRaw();
    uint32_t rxCount();
}
