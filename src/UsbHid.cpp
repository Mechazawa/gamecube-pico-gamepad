#include "UsbHid.h"

#include <Adafruit_TinyUSB.h>

#define REPORT_ID_GAMEPAD 1
#define REPORT_ID_RUMBLE  2

// HID report descriptor.
//
// Input (id 1) mirrors the core GAMEPAD16 layout so existing host-side
// mappings keep working. Output (id 2) is a single vendor-defined byte the
// host writes to drive the rumble motor.
static uint8_t const desc_hid_report[] = {
    HID_USAGE_PAGE(HID_USAGE_PAGE_DESKTOP),
    HID_USAGE(HID_USAGE_DESKTOP_GAMEPAD),
    HID_COLLECTION(HID_COLLECTION_APPLICATION),
        HID_REPORT_ID(REPORT_ID_GAMEPAD)
        // 6x 16-bit axes: X, Y, Z, Rz, Rx, Ry  (-32767 .. 32767)
        HID_USAGE_PAGE(HID_USAGE_PAGE_DESKTOP),
        HID_USAGE(HID_USAGE_DESKTOP_X),
        HID_USAGE(HID_USAGE_DESKTOP_Y),
        HID_USAGE(HID_USAGE_DESKTOP_Z),
        HID_USAGE(HID_USAGE_DESKTOP_RZ),
        HID_USAGE(HID_USAGE_DESKTOP_RX),
        HID_USAGE(HID_USAGE_DESKTOP_RY),
        HID_LOGICAL_MIN_N(-32767, 2),
        HID_LOGICAL_MAX_N(32767, 2),
        HID_REPORT_COUNT(6),
        HID_REPORT_SIZE(16),
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),
        // Hat / dpad
        HID_USAGE_PAGE(HID_USAGE_PAGE_DESKTOP),
        HID_USAGE(HID_USAGE_DESKTOP_HAT_SWITCH),
        HID_LOGICAL_MIN(1),
        HID_LOGICAL_MAX(8),
        HID_PHYSICAL_MIN(0),
        HID_PHYSICAL_MAX_N(315, 2),
        HID_REPORT_COUNT(1),
        HID_REPORT_SIZE(8),
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),
        // 32 buttons
        HID_USAGE_PAGE(HID_USAGE_PAGE_BUTTON),
        HID_USAGE_MIN(1),
        HID_USAGE_MAX(32),
        HID_LOGICAL_MIN(0),
        HID_LOGICAL_MAX(1),
        HID_REPORT_COUNT(32),
        HID_REPORT_SIZE(1),
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),
        // Rumble output: one vendor-defined magnitude byte (0 = off)
        HID_REPORT_ID(REPORT_ID_RUMBLE)
        HID_USAGE_PAGE_N(HID_USAGE_PAGE_VENDOR, 2),
        HID_USAGE(0x01),
        HID_LOGICAL_MIN(0),
        HID_LOGICAL_MAX_N(255, 2),
        HID_REPORT_COUNT(1),
        HID_REPORT_SIZE(8),
        HID_OUTPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),
    HID_COLLECTION_END
};

static Adafruit_USBD_HID usb_hid;

static volatile bool     s_rumble_on = false;
static volatile uint8_t  s_rumble_raw = 0;
static volatile uint32_t s_rx_count = 0;

// Host asked us for an input report over the control pipe; we just send the
// last gamepad state. Not commonly used (interrupt IN is the normal path).
static uint16_t get_report_cb(uint8_t report_id, hid_report_type_t report_type,
                              uint8_t *buffer, uint16_t reqlen) {
    (void) report_id;
    (void) report_type;
    (void) buffer;
    (void) reqlen;
    return 0;
}

// Host wrote an output report. Two delivery paths:
//   - SET_REPORT (control): report_id is the real id, buffer holds the data.
//   - OUT endpoint:         report_id == 0, buffer[0] is the id, data follows.
static void set_report_cb(uint8_t report_id, hid_report_type_t report_type,
                          uint8_t const *buffer, uint16_t bufsize) {
    (void) report_type;

    uint8_t id = report_id;
    uint8_t const *data = buffer;
    uint16_t len = bufsize;
    if (id == 0 && len >= 1) {
        id = buffer[0];
        data = buffer + 1;
        len -= 1;
    }

    if (id == REPORT_ID_RUMBLE && len >= 1) {
        s_rumble_raw = data[0];
        s_rumble_on = (data[0] != 0);
        s_rx_count++;
    }
}

namespace UsbHid {

void begin(uint16_t vid, uint16_t pid, const char *manufacturer, const char *product) {
    if (!TinyUSBDevice.isInitialized()) {
        TinyUSBDevice.begin(0);
    }

    TinyUSBDevice.setID(vid, pid);
    if (manufacturer) {
        TinyUSBDevice.setManufacturerDescriptor(manufacturer);
    }
    if (product) {
        TinyUSBDevice.setProductDescriptor(product);
    }

    usb_hid.setPollInterval(2);
    usb_hid.enableOutEndpoint(true);
    usb_hid.setReportDescriptor(desc_hid_report, sizeof(desc_hid_report));
    usb_hid.setReportCallback(get_report_cb, set_report_cb);
    usb_hid.begin();

    // If the core already enumerated us before begin() registered the HID
    // interface, force a re-enumeration so the host sees the gamepad.
    if (TinyUSBDevice.mounted()) {
        TinyUSBDevice.detach();
        delay(10);
        TinyUSBDevice.attach();
    }
}

bool mounted() {
    return TinyUSBDevice.mounted();
}

bool ready() {
    return usb_hid.ready();
}

bool sendGamepad(const GamepadReport &report) {
    return usb_hid.sendReport(REPORT_ID_GAMEPAD, &report, sizeof(report));
}

bool rumbleOn() {
    return s_rumble_on;
}

uint8_t rumbleRaw() {
    return s_rumble_raw;
}

uint32_t rxCount() {
    return s_rx_count;
}

} // namespace UsbHid
