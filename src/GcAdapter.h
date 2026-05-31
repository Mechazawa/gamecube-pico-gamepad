#pragma once

#include <stdint.h>

// Emulates the Nintendo "GameCube Controller Adapter for Wii U" (WUP-028,
// USB 057E:0337). Dolphin (and the Switch) auto-detect this by VID:PID, claim
// interface 0 over libusb, and talk a tiny vendor protocol on two interrupt
// endpoints:
//   - host -> device: 1 byte 0x13 starts input streaming; 5 bytes
//     {0x11, r0, r1, r2, r3} set per-port rumble.
//   - device -> host: 37-byte packets {0x21, then 4x 9-byte ports}.
//
// We expose a single wired controller on port 1. Implemented as a TinyUSB
// application class driver (the built-in vendor driver is disabled via
// -DCFG_TUD_VENDOR=0) plus an Adafruit_USBD_Interface for the descriptor, so it
// coexists with the CDC serial interface used for reflashing.

namespace GcAdapter {
    // Register the vendor interface. Call before Serial.begin() so this lands
    // on interface 0 (Dolphin claims interface 0).
    void begin();

    bool started();  // host sent the 0x13 start command
    bool rumble();   // host requested rumble on port 1

    // Queue a 37-byte input packet built from a 9-byte port-1 payload
    // (status, buttons1, buttons2, stickX, stickY, cX, cY, triggerL, triggerR).
    // No-op until started / while the IN endpoint is busy.
    void sendPort1(const uint8_t port[9]);
}
