#pragma once

#include "USBHID.h"
// #include "mbed.h"

// using namespace mbed;

class GamepadHID : public USBHID {
public:
  // GamepadHID() : USBHID(get_usb_phy()) {
  //   connect();
  // }

  // Report descriptor:
  virtual const uint8_t *report_desc() {
    static const uint8_t desc[] = {
      0x05, 0x01,                    // Usage Page (Generic Desktop)
      0x09, 0x05,                    // Usage (Game Pad)
      0xA1, 0x01,                    // Collection (Application)

      // -------- Input Report (ID 1) --------
      0x85, 0x01,                    //   Report ID (1)

      // 12 Buttons (+4 bits padding)
      0x05, 0x09,                    //   Usage Page (Button)
      0x19, 0x01,                    //   Usage Min (1)
      0x29, 0x0C,                    //   Usage Max (12)
      0x15, 0x00,                    //   Logical Min (0)
      0x25, 0x01,                    //   Logical Max (1)
      0x95, 0x0C,                    //   Report Count (12)
      0x75, 0x01,                    //   Report Size (1)
      0x81, 0x02,                    //   Input (Data,Var,Abs)
      0x95, 0x04,                    //   Report Count (4)
      0x75, 0x01,                    //   Report Size (1)
      0x81, 0x03,                    //   Input (Const,Var,Abs) // padding

      // 6 Axes: X,Y,Z,Rx,Ry,Rz — all 0..255
      0x05, 0x01,                    //   Usage Page (Generic Desktop)
      0x09, 0x30,                    //   Usage (X)
      0x09, 0x31,                    //   Usage (Y)
      0x09, 0x32,                    //   Usage (Z)   -> Right X
      0x09, 0x33,                    //   Usage (Rx)  -> LT
      0x09, 0x34,                    //   Usage (Ry)  -> RT
      0x09, 0x35,                    //   Usage (Rz)  -> Right Y
      0x15, 0x00,                    //   Logical Min (0)
      0x26, 0xFF, 0x00,              //   Logical Max (255)
      0x75, 0x08,                    //   Report Size (8)
      0x95, 0x06,                    //   Report Count (6)
      0x81, 0x02,                    //   Input (Data,Var,Abs)

      // -------- Output Report (ID 2) for Rumble On/Off --------
      0x85, 0x02,                    //   Report ID (2)
      0x06, 0x00, 0xFF,              //   Usage Page (Vendor-defined 0xFF00)
      0x09, 0x01,                    //   Usage (0x01)
      0x15, 0x00,                    //   Logical Min (0)
      0x25, 0x01,                    //   Logical Max (1)
      0x75, 0x08,                    //   Report Size (8)
      0x95, 0x01,                    //   Report Count (1)
      0x91, 0x02,                    //   Output (Data,Var,Abs)

      0xC0                           // End Collection
    };
    return desc;
  }

  // Send current state (buttons + axes)
  void sendState(uint16_t buttons,
                 uint8_t x, uint8_t y,
                 uint8_t z, uint8_t rx,
                 uint8_t ry, uint8_t rz) {
    HID_REPORT rep{};
    rep.length = 1 + 2 + 6; // ID + buttons(2) + axes(6) = 9
    rep.data[0] = 0x01;     // Report ID 1
    rep.data[1] = buttons & 0xFF;
    rep.data[2] = (buttons >> 8) & 0xFF;
    rep.data[3] = x;
    rep.data[4] = y;
    rep.data[5] = z;
    rep.data[6] = rx;
    rep.data[7] = ry;
    rep.data[8] = rz;
    send(&rep);
  }

  // Poll OUT reports (rumble)
  bool pollRumble(bool &on) {
    HID_REPORT out{};
    if (read_nb(&out)) {
      if (out.length >= 2 && out.data[0] == 0x02) { // Report ID 2
        on = (out.data[1] != 0);
        return true;
      }
    }
    return false;
  }
};