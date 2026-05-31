#include "GcAdapter.h"

#include <Arduino.h> // rp2040.rebootToBootloader()
#include <Adafruit_TinyUSB.h>
#include <device/usbd_pvt.h>
#include <string.h>

#define GC_EP_SIZE   64
#define GC_IN_SIZE   37   // device -> host input packet
#define GC_RX_SIZE   64   // host -> device buffer (init / rumble)

#define GC_CMD_INIT   0x13
#define GC_CMD_RUMBLE 0x11
#define GC_INPUT_ID   0x21

// Host-triggered reboot to BOOTSEL: a 4-byte magic on the OUT endpoint that the
// real adapter protocol never uses, so tools/gcadapter.py can reflash without a
// button combo (the device has no CDC serial in the release build).
static const uint8_t GC_REBOOT_MAGIC[4] = {0x52, 0x42, 0x54, 0x21}; // "RBT!"

namespace {

uint8_t s_rhport = 0;
uint8_t s_ep_in = 0;
uint8_t s_ep_out = 0;
volatile bool s_started = false;
volatile bool s_rumble = false;

uint8_t s_rx[GC_RX_SIZE];
uint8_t s_tx[GC_IN_SIZE];

// Interface descriptor: vendor-specific interface 0 with two interrupt
// endpoints. Adafruit calls getInterfaceDescriptor once with a real buffer to
// build the cached config descriptor, and with buf==NULL only to size it.
class GcInterface : public Adafruit_USBD_Interface {
public:
    bool begin() {
        _strid = TinyUSBDevice.addStringDescriptor("GameCube Adapter");
        return TinyUSBDevice.addInterface(*this);
    }

    uint16_t getInterfaceDescriptor(uint8_t itfnum_deprecated, uint8_t *buf,
                                    uint16_t bufsize) override {
        (void) itfnum_deprecated;
        const uint16_t len = 9 + 2 * 7;
        if (!buf) {
            return len;
        }
        if (bufsize < len) {
            return 0;
        }
        const uint8_t itfnum = TinyUSBDevice.allocInterface(1);
        const uint8_t ep_in = TinyUSBDevice.allocEndpoint(TUSB_DIR_IN);
        const uint8_t ep_out = TinyUSBDevice.allocEndpoint(TUSB_DIR_OUT);
        const uint8_t desc[] = {
            // Interface: vendor-specific, 2 endpoints
            9, TUSB_DESC_INTERFACE, itfnum, 0x00, 0x02, TUSB_CLASS_VENDOR_SPECIFIC, 0x00, 0x00, _strid,
            // EP OUT, interrupt, 1 ms
            7, TUSB_DESC_ENDPOINT, ep_out, TUSB_XFER_INTERRUPT, (uint8_t)(GC_EP_SIZE & 0xFF), (uint8_t)(GC_EP_SIZE >> 8), 1,
            // EP IN, interrupt, 1 ms
            7, TUSB_DESC_ENDPOINT, ep_in, TUSB_XFER_INTERRUPT, (uint8_t)(GC_EP_SIZE & 0xFF), (uint8_t)(GC_EP_SIZE >> 8), 1,
        };
        memcpy(buf, desc, len);
        return len;
    }
};

GcInterface s_itf;

// ---- TinyUSB application class driver for the two interrupt endpoints ----

void drv_init(void) {}

bool drv_deinit(void) {
    return true;
}

void drv_reset(uint8_t rhport) {
    (void) rhport;
    s_started = false;
    s_rumble = false;
    s_ep_in = 0;
    s_ep_out = 0;
}

uint16_t drv_open(uint8_t rhport, tusb_desc_interface_t const *itf, uint16_t max_len) {
    if (itf->bInterfaceClass != TUSB_CLASS_VENDOR_SPECIFIC) {
        return 0;
    }
    const uint16_t drv_len =
        (uint16_t)(sizeof(tusb_desc_interface_t) + itf->bNumEndpoints * sizeof(tusb_desc_endpoint_t));
    TU_VERIFY(max_len >= drv_len, 0);

    s_rhport = rhport;
    uint8_t const *p = tu_desc_next(itf);
    for (uint8_t i = 0; i < itf->bNumEndpoints; i++) {
        tusb_desc_endpoint_t const *ep = (tusb_desc_endpoint_t const *) p;
        usbd_edpt_open(rhport, ep);
        if (tu_edpt_dir(ep->bEndpointAddress) == TUSB_DIR_IN) {
            s_ep_in = ep->bEndpointAddress;
        } else {
            s_ep_out = ep->bEndpointAddress;
        }
        p = tu_desc_next(p);
    }

    // Arm the OUT endpoint to receive init / rumble commands.
    if (s_ep_out) {
        usbd_edpt_xfer(rhport, s_ep_out, s_rx, sizeof(s_rx));
    }
    return drv_len;
}

bool drv_control_xfer(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request) {
    (void) rhport;
    (void) stage;
    (void) request;
    return false; // no class-specific control requests
}

bool drv_xfer(uint8_t rhport, uint8_t ep_addr, xfer_result_t result, uint32_t xferred) {
    if (ep_addr == s_ep_out) {
        if (result == XFER_RESULT_SUCCESS && xferred >= 1) {
            if (xferred >= 4 && memcmp(s_rx, GC_REBOOT_MAGIC, 4) == 0) {
                rp2040.rebootToBootloader(); // does not return
            } else if (s_rx[0] == GC_CMD_INIT) {
                s_started = true;
            } else if (s_rx[0] == GC_CMD_RUMBLE && xferred >= 2) {
                s_rumble = (s_rx[1] != 0); // port 1
            }
        }
        usbd_edpt_xfer(rhport, s_ep_out, s_rx, sizeof(s_rx)); // re-arm
    }
    return true;
}

usbd_class_driver_t const s_driver = {
    .name = "GCAD",
    .init = drv_init,
    .deinit = drv_deinit,
    .reset = drv_reset,
    .open = drv_open,
    .control_xfer_cb = drv_control_xfer,
    .xfer_cb = drv_xfer,
    .sof = nullptr,
};

} // namespace

// Hook for TinyUSB to pick up our class driver (weak in the stack).
extern "C" usbd_class_driver_t const *usbd_app_driver_get_cb(uint8_t *driver_count) {
    *driver_count = 1;
    return &s_driver;
}

namespace GcAdapter {

void begin() {
    s_itf.begin();
}

bool started() {
    return s_started;
}

bool rumble() {
    return s_rumble;
}

void sendPort1(const uint8_t port[9]) {
    if (!s_started || !s_ep_in) {
        return;
    }
    if (usbd_edpt_busy(s_rhport, s_ep_in)) {
        return;
    }
    s_tx[0] = GC_INPUT_ID;
    memset(s_tx + 1, 0, GC_IN_SIZE - 1); // ports 2-4 absent
    memcpy(s_tx + 1, port, 9);           // port 1
    usbd_edpt_xfer(s_rhport, s_ep_in, s_tx, GC_IN_SIZE);
}

} // namespace GcAdapter
