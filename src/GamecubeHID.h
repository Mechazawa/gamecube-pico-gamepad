#pragma once
#include <stdint.h>
#include <usb/USBHID.h>  // mbed USB stack

class GamecubeHID : public USBHID {
public:
    enum Button : uint8_t {
        A = 0, B, Y, X,
        START,
        DPAD_UP, DPAD_RIGHT, DPAD_DOWN, DPAD_LEFT,
        Z, L, R
    };

    GamecubeHID();

    // Axes (0..255)
    inline void setLX(uint8_t v) { lx_ = v; }
    inline void setLY(uint8_t v) { ly_ = v; }
    inline void setCX(uint8_t v) { rx_ = v; } // C-stick → Rx
    inline void setCY(uint8_t v) { ry_ = v; } // C-stick → Ry
    inline void setLT(uint8_t v) { lt_ = v; } // LT → Z
    inline void setRT(uint8_t v) { rt_ = v; } // RT → Rz

    void setButton(Button b, bool pressed);
    void setDpad(bool up, bool right, bool down, bool left);

    // Rumble OUT (0..255)
    inline uint8_t rumble() const { return rumble_; }
    bool pollRumble();   // reads OUT report; returns true when changed

    void send();

protected:
    // mbed USBHID hooks
    virtual const uint8_t* report_desc() override;
    virtual uint16_t report_desc_length() override;

private:
    struct __attribute__((packed)) Report {
        uint16_t buttons; // bits 0..11 used
        uint8_t  lx, ly;  // X, Y
        uint8_t  rx, ry;  // Rx, Ry (C-stick)
        uint8_t  lt, rt;  // Z, Rz (analog triggers)
    };

    volatile uint16_t buttons_ = 0;
    uint8_t lx_ = 128, ly_ = 128, rx_ = 128, ry_ = 128;
    uint8_t lt_ = 0,   rt_ = 0;

    volatile uint8_t rumble_ = 0;
    volatile bool rumble_dirty_ = false;

    static const uint8_t kReportDesc[];
};