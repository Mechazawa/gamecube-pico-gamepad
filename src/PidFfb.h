#pragma once

#include <stdint.h>

// Minimal USB PID (Physical Interface Device) force-feedback handler.
//
// Enough of the PID protocol is implemented for the Linux hid-pidff driver
// (and Windows DirectInput / SDL) to bind and create effects: the device
// answers the Create-New-Effect / Block-Load / Pool handshake and tracks
// effect start/stop. The GameCube motor is binary, so we don't synthesise
// forces; rumble is simply "is any effect currently playing with a non-zero
// magnitude". Ported from the MIT-licensed ArduinoJoystickWithFFBLibrary
// (YukMingLaw) PIDReportHandler, trimmed to what a binary rumble needs.

namespace PidFfb {
    void begin();

    // Host wrote an OUTPUT report (report[0] is the report id).
    void handleOutput(const uint8_t *report, uint16_t len);

    // Host wrote the Create-New-Effect FEATURE report (allocates an effect).
    void createNewEffect();

    // Fill GET FEATURE / INPUT responses (payload only, no report id).
    // Return the number of bytes written.
    uint16_t getBlockLoad(uint8_t *out); // feature report id 6
    uint16_t getPool(uint8_t *out);      // feature report id 7
    uint16_t getState(uint8_t *out);     // input report id 2

    // Rumble decision + diagnostics.
    bool rumbleRequested();
    int16_t lastMagnitude();
    uint32_t opCount();
}
