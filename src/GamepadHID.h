#pragma once

#include <Arduino.h>
#include <Joystick.h>

class GamepadHID {
private:
    bool _initialized = false;
    uint32_t _lastSendTime = 0;
    static const uint32_t MIN_SEND_INTERVAL = 8; // 8ms = ~125Hz max

public:
    GamepadHID() = default;
    
    void init() {
        if (!_initialized) {
            Joystick.begin();
            // Use manual send to control timing
            Joystick.useManualSend(true);
            _initialized = true;
        }
    }
    
    void connect() {
        // Auto-connects with Arduino-Pico
    }
    
    void wait_ready() {
        // Not needed with Arduino-Pico
    }
    
    // Send current state (buttons + axes) with rate limiting
    bool sendState(const uint16_t buttons,
                   const uint8_t x, const uint8_t y,
                   const uint8_t z, const uint8_t rx,
                   const uint8_t ry, const uint8_t rz) {
        
        if (!_initialized) {
            return false;
        }

        // Rate limiting to prevent overwhelming USB stack
        uint32_t now = millis();
        if (now - _lastSendTime < MIN_SEND_INTERVAL) {
            return true; // Skip but return success
        }
        _lastSendTime = now;

        // Map GameCube buttons to joystick buttons
        // GameCube: Start, A, B, X, Y, L, R, Z (8 buttons)
        for (int i = 0; i < 8; i++) {
            Joystick.setButton(i, (buttons & (1 << i)) != 0);
        }

        // Map axes (convert from 0-255 to joystick range)
        // Arduino-Pico Joystick expects 0-1023 by default
        Joystick.X(x * 4);                // Left stick X (0-1023)  
        Joystick.Y(y * 4);                // Left stick Y (0-1023)
        Joystick.Z(z * 4);                // Right stick X (0-1023)
        Joystick.Zrotate(rz * 4);         // Right stick Y (0-1023) 
        Joystick.sliderLeft(rx * 4);      // Left trigger (0-1023)
        Joystick.sliderRight(ry * 4);     // Right trigger (0-1023)

        // Send the report
        Joystick.send_now();
        return true;
    }
    
    // Poll OUT reports (rumble) - not implemented yet
    bool pollRumble(bool &on) {
        return false; // Use fallback rumble for now
    }
};