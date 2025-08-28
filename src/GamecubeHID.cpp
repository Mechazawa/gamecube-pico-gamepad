#include "GamecubeHID.h"
#include <string.h>
#include <usb_phy_api.h>

// ---- HID Report Descriptor ----
// 12 buttons -> 12 bits (+4 padding)
// X,Y,Rx,Ry  -> 4 bytes (0..255)
// Z,Rz       -> 2 bytes (0..255)
// OUT 1 byte -> rumble (vendor page)
static constexpr uint8_t HID_DESC[] = {
    0x05, 0x01,       // Usage Page (Generic Desktop)
    0x09, 0x05,       // Usage (Game Pad)
    0xA1, 0x01,       // Collection (Application)

    // Buttons 1..12
    0x05, 0x09,       //   Usage Page (Button)
    0x19, 0x01,       //   Usage Minimum (1)
    0x29, 0x0C,       //   Usage Maximum (12)
    0x15, 0x00,       //   Logical Min (0)
    0x25, 0x01,       //   Logical Max (1)
    0x75, 0x01,       //   Report Size (1)
    0x95, 0x0C,       //   Report Count (12)
    0x81, 0x02,       //   Input (Data,Var,Abs)
    0x75, 0x01,       //   Report Size (1)
    0x95, 0x04,       //   Report Count (4)
    0x81, 0x03,       //   Input (Const,Var,Abs)

    // Axes: X,Y,Rx,Ry (0..255)
    0x05, 0x01,       //   Usage Page (Generic Desktop)
    0x15, 0x00,       //   Logical Min (0)
    0x26, 0xFF, 0x00, //   Logical Max (255)
    0x75, 0x08,       //   Report Size (8)
    0x95, 0x04,       //   Report Count (4)
    0x09, 0x30,       //   Usage (X)
    0x09, 0x31,       //   Usage (Y)
    0x09, 0x33,       //   Usage (Rx)
    0x09, 0x34,       //   Usage (Ry)
    0x81, 0x02,       //   Input (Data,Var,Abs)

    // Triggers: Z,Rz (0..255)
    0x95, 0x02,       //   Report Count (2)
    0x09, 0x32,       //   Usage (Z)
    0x09, 0x35,       //   Usage (Rz)
    0x81, 0x02,       //   Input (Data,Var,Abs)

    // Rumble OUT 1 byte (vendor page)
    0x06, 0x00, 0xFF, //   Usage Page (Vendor 0xFF00)
    0x09, 0x01,       //   Usage (1)
    0x15, 0x00,       //   Logical Min (0)
    0x26, 0xFF, 0x00, //   Logical Max (255)
    0x75, 0x08,       //   Report Size (8)
    0x95, 0x01,       //   Report Count (1)
    0x91, 0x02,       //   Output (Data,Var,Abs)

    0xC0              // End Collection
};

const uint8_t GamecubeHID::kReportDesc[] = { /* unused, keep symbol */ };

GamecubeHID::GamecubeHID()
    // mbed USBHID ctor: (output_len, input_len, vendor_id, product_id, product_release)
    : USBHID(get_usb_phy(), sizeof(Report), 0, 0xCafe, 0x4000, 0x0001)
{}

void GamecubeHID::setButton(Button b, bool pressed) {
    const uint16_t mask = uint16_t(1) << uint8_t(b);
    if (pressed) buttons_ |= mask;
    else         buttons_ &= ~mask;
}

void GamecubeHID::setDpad(bool up, bool right, bool down, bool left) {
    setButton(DPAD_UP,    up);
    setButton(DPAD_RIGHT, right);
    setButton(DPAD_DOWN,  down);
    setButton(DPAD_LEFT,  left);
}

bool GamecubeHID::pollRumble() {
    HID_REPORT r;
    if (read(&r) && r.length >= 1) {
        rumble_ = r.data[0];
        rumble_dirty_ = true;
        return true;
    }
    if (!rumble_dirty_) return false;
    rumble_dirty_ = false;
    return true;
}

void GamecubeHID::send() {
    HID_REPORT r;
    r.length = sizeof(Report);
    Report p { buttons_, lx_, ly_, rx_, ry_, lt_, rt_ };
    memcpy(r.data, &p, sizeof(p));
    USBHID::send(&r);
}

const uint8_t* GamecubeHID::report_desc() {
    return HID_DESC;
}

uint16_t GamecubeHID::report_desc_length() {
    return sizeof(HID_DESC);
}