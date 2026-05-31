#include "UsbHid.h"
#include "PidFfb.h"

#include <Adafruit_TinyUSB.h>
#include <string.h>

#define REPORT_ID_GAMEPAD 1

// HID report descriptor: gamepad input (report id 1, the unchanged GAMEPAD16
// layout) followed by the full PID force-feedback block (see UsbHidFfb.inc).
// The force-feedback section's trailing 0xC0 closes this Application
// collection, so we do NOT emit our own HID_COLLECTION_END here.
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
        // ===== PID force feedback (verbatim, closes the collection) =====
        #include "UsbHidFfb.inc"
};

static Adafruit_USBD_HID usb_hid;

// Invoked on GET_REPORT. For feature reports the PID driver reads Block Load /
// Pool here; for input report 2 it may read PID state.
static uint16_t get_report_cb(uint8_t report_id, hid_report_type_t report_type,
                              uint8_t *buffer, uint16_t reqlen) {
    if (report_type == HID_REPORT_TYPE_FEATURE) {
        if (report_id == 6) {
            return PidFfb::getBlockLoad(buffer);
        }
        if (report_id == 7) {
            return PidFfb::getPool(buffer);
        }
    } else if (report_type == HID_REPORT_TYPE_INPUT) {
        if (report_id == 2) {
            return PidFfb::getState(buffer);
        }
    }
    (void) reqlen;
    return 0;
}

// Invoked on SET_REPORT (control) or OUT-endpoint data. Feature report 5 is
// Create-New-Effect; everything else is a PID output report.
static void set_report_cb(uint8_t report_id, hid_report_type_t report_type,
                          uint8_t const *buffer, uint16_t bufsize) {
    if (report_type == HID_REPORT_TYPE_FEATURE) {
        if (report_id == 5) {
            PidFfb::createNewEffect();
        }
        return;
    }

    // OUTPUT report, delivered either via control SET_REPORT (report_id set,
    // buffer is payload only) or via the OUT endpoint (report_id 0, buffer
    // already starts with the report id). Normalise to [id, payload...].
    uint8_t buf[CFG_TUD_HID_EP_BUFSIZE];
    uint16_t len;
    if (report_id == 0) {
        len = bufsize > sizeof(buf) ? sizeof(buf) : bufsize;
        memcpy(buf, buffer, len);
    } else {
        uint16_t n = bufsize > sizeof(buf) - 1 ? sizeof(buf) - 1 : bufsize;
        buf[0] = report_id;
        memcpy(buf + 1, buffer, n);
        len = n + 1;
    }
    PidFfb::handleOutput(buf, len);
}

namespace UsbHid {

void begin(uint16_t vid, uint16_t pid, const char *manufacturer, const char *product) {
    PidFfb::begin();

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
    return PidFfb::rumbleRequested();
}

uint8_t rumbleRaw() {
    int16_t m = PidFfb::lastMagnitude();
    if (m < 0) {
        m = -m;
    }
    return m > 255 ? 255 : (uint8_t) m;
}

uint32_t rxCount() {
    return PidFfb::opCount();
}

} // namespace UsbHid
