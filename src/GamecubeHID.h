#pragma once
#include <Arduino.h>
#include <PluggableUSBHID.h>

class GamecubeHID : public USBHID
{
public:
    GamecubeHID(uint16_t vid = 0xCafe, uint16_t pid = 0x4010)
        : USBHID(_gcDesc, sizeof(_gcDesc), vid, pid, 0x0100, false)
    {
        memset(&_in, 0, sizeof(_in));
        USBHID::connect();
    }

    void begin()
    {
        // Center sticks, zero triggers
        _in.report_id = 0x01;
        _in.buttons = 0;
        _in.lx = 0;
        _in.ly = 0;
        _in.cx = 0;
        _in.cy = 0;
        _in.lt = 0;
        _in.rt = 0;
        send();
    }

    // ---- Buttons ----
    enum Btn : uint8_t { A = 0, B = 1, Y = 2, X = 3, START = 4, DU = 5, DR = 6, DD = 7, DL = 8, Z = 9, L = 10, R = 11 };

    void press(uint8_t bit) { if (bit < 12) _in.buttons |= (1u << bit); }
    void release(uint8_t bit) { if (bit < 12) _in.buttons &= ~(1u << bit); }
    void setButton(uint8_t bit, bool on) { on ? press(bit) : release(bit); }

    // ---- Axes ----
    void setLX(int v) { _in.lx = clampS8(v); }
    void setLY(int v) { _in.ly = clampS8(v); }
    void setCX(int v) { _in.cx = clampS8(v); }
    void setCY(int v) { _in.cy = clampS8(v); }
    void setLT(int v) { _in.lt = clampU8(v); }
    void setRT(int v) { _in.rt = clampU8(v); }

    // D-pad helper
    void setDpad(bool up, bool right, bool down, bool left)
    {
        setButton(DU, up);
        setButton(DR, right);
        setButton(DD, down);
        setButton(DL, left);
    }

    // ---- Send current input report ----
    bool send()
    {
        USBHID::report_t r;
        r.data = (uint8_t*)&_in;
        r.length = sizeof(_in);
        return USBHID::send(r);
    }

    // ---- Poll OUT report (rumble on/off). Returns true if changed. ----
    bool pollRumble()
    {
        USBHID::report_t r;
        uint8_t buf[2];
        r.data = buf;
        r.length = sizeof(buf);
        if (!USBHID::read(&r)) return false;

        bool prev = _rumble;
        // Accept either [0x02, payload] or [payload] depending on host
        if (r.length >= 2 && buf[0] == 0x02)
        {
            _rumble = (buf[1] & 0x01);
        }
        else if (r.length >= 1)
        {
            _rumble = (buf[0] & 0x01);
        }
        return _rumble != prev;
    }

    bool rumble() const { return _rumble; }

protected:
    // Provide our HID report descriptor to the mbed core
    virtual const uint8_t* report_desc() { return _gcDesc; }

private:
    // Packed input report (Report ID 1)
    struct __attribute__((packed)) InReport
    {
        uint8_t report_id; // = 1
        uint16_t buttons; // bits 0..11 used, 12..15 padding
        int8_t lx; // -127..127
        int8_t ly; // -127..127
        int8_t cx; // -127..127
        int8_t cy; // -127..127
        uint8_t lt; // 0..255
        uint8_t rt; // 0..255
    } _in;

    bool _rumble = false;

    static int8_t clampS8(int v)
    {
        if (v < -127) return -127;
        if (v > 127) return 127;
        return (int8_t)v;
    }

    static uint8_t clampU8(int v)
    {
        if (v < 0) return 0;
        if (v > 255) return 255;
        return (uint8_t)v;
    }

    // HID Report Descriptor (Report ID 1 input, Report ID 2 output 1-bit)
    static constexpr uint8_t _gcDesc[] = {
        0x05, 0x01, // Usage Page (Generic Desktop)
        0x09, 0x05, // Usage (Game Pad)
        0xA1, 0x01, // Collection (Application)

        0x85, 0x01, //  Report ID (1) - INPUT

        // 12 buttons (A,B,Y,X,START,UP,RIGHT,DOWN,LEFT,Z,L,R)
        0x05, 0x09, //  Usage Page (Button)
        0x19, 0x01, //  Usage Min (1)
        0x29, 0x0C, //  Usage Max (12)
        0x15, 0x00, //  Logical Min (0)
        0x25, 0x01, //  Logical Max (1)
        0x95, 0x0C, //  Report Count (12)
        0x75, 0x01, //  Report Size (1)
        0x81, 0x02, //  Input (Data,Var,Abs)
        0x95, 0x04, 0x75, 0x01, 0x81, 0x03, //  padding

        // LX, LY (-127..127)
        0x05, 0x01, 0x09, 0x30, 0x09, 0x31,
        0x15, 0x81, 0x25, 0x7F, // -127..127
        0x75, 0x08, 0x95, 0x02, 0x81, 0x02,

        // CX, CY on Rx, Ry (-127..127)
        0x09, 0x33, 0x09, 0x34,
        0x15, 0x81, 0x25, 0x7F,
        0x75, 0x08, 0x95, 0x02, 0x81, 0x02,

        // LT, RT on Z, Rz (0..255)
        0x09, 0x32, 0x09, 0x35,
        0x15, 0x00, 0x26, 0xFF, 0x00, // 0..255
        0x75, 0x08, 0x95, 0x02, 0x81, 0x02,

        // Rumble OUT: Report ID 2, 1 bit on/off (+7 pad)
        0x85, 0x02, // Report ID (2)
        0x06, 0x00, 0xFF, 0x09, 0x01, // Vendor page 0xFF00, Usage 1
        0x15, 0x00, 0x25, 0x01, // 0..1
        0x75, 0x01, 0x95, 0x01, 0x91, 0x02, // Output 1 bit
        0x95, 0x07, 0x75, 0x01, 0x91, 0x03, // pad to 1 byte

        0xC0
    };
};
