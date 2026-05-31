#include <Arduino.h>
#include <Adafruit_TinyUSB.h>

#include "Controller.h"
#include "GcAdapter.h"

#define FW_VERSION "0.4.0-gcadapter"

// Build profile:
//   GCCPICO_DIAG = 0 (default, release): the device is ONLY the Nintendo GC
//       adapter vendor interface (interface 0), exactly like the real WUP-028,
//       so Dolphin auto-detects it. Reflash via the vendor reboot magic
//       (tools/gcadapter.py reboot).
//   GCCPICO_DIAG = 1 (debug): also keeps the USB-CDC serial interface for
//       logging + the 1200-baud touch reset. NOT Dolphin-compatible (the extra
//       interface/endpoints break Dolphin's interface-0 / endpoint detection).
#ifndef GCCPICO_DIAG
#define GCCPICO_DIAG 0
#endif
#if GCCPICO_DIAG
#define DIAG(...) Serial.printf(__VA_ARGS__)
#define DIAGLN(s) Serial.println(s)
#else
#define DIAG(...) ((void) 0)
#define DIAGLN(s) ((void) 0)
#endif

Controller *controller = nullptr;

// Manual rumble override over the serial console (debug builds only).
static bool g_manual_sticky = false;
static unsigned long g_manual_until = 0;

static bool manualRumbleActive() {
    return g_manual_sticky || (long)(g_manual_until - millis()) > 0;
}

// Build the adapter's 9-byte port payload from the raw GameCube poll response.
//   st[0]: A=0x01 B=0x02 X=0x04 Y=0x08 Start=0x10
//   st[1]: Left=0x01 Right=0x02 Down=0x04 Up=0x08 Z=0x10 R=0x20 L=0x40
//   st[2..7]: stickX, stickY, cX, cY, triggerL, triggerR
static void buildPort(uint8_t port[9], const uint8_t *st) {
    port[0] = 0x10; // wired controller connected
    port[1] = (uint8_t)((st[0] & 0x0F) | ((st[1] & 0x0F) << 4)); // A,B,X,Y | Left,Right,Down,Up
    port[2] = (uint8_t)(((st[0] >> 4) & 0x01)          // Start
                        | (((st[1] >> 4) & 0x01) << 1)  // Z
                        | (((st[1] >> 5) & 0x01) << 2)  // R
                        | (((st[1] >> 6) & 0x01) << 3)); // L
    port[3] = st[2];
    port[4] = st[3];
    port[5] = st[4];
    port[6] = st[5];
    port[7] = st[6];
    port[8] = st[7];
}

#if GCCPICO_DIAG
static void printBanner() {
    DIAGLN("");
    DIAGLN("=== GameCube USB adapter (WUP-028 emulation, DEBUG build) ===");
    DIAG("firmware: %s\n", FW_VERSION);
    DIAGLN("note: debug build adds CDC serial and is NOT Dolphin-compatible");
    DIAGLN("serial commands: b=BOOTSEL  r=rumble pulse  1=rumble on  0=off  s=state  v=version");
}

static void printState() {
    uint8_t *st = controller->getRawControllerState();
    DIAG("[state] %02x %02x %02x %02x %02x %02x %02x %02x\n",
         st[0], st[1], st[2], st[3], st[4], st[5], st[6], st[7]);
}

static void handleSerial() {
    while (Serial.available()) {
        int c = Serial.read();
        switch (c) {
            case 'b':
            case 'B':
                DIAGLN("[cmd] rebooting to BOOTSEL...");
                Serial.flush();
                delay(50);
                rp2040.rebootToBootloader();
                break;
            case 'r':
            case 'R':
                g_manual_until = millis() + 400;
                DIAGLN("[cmd] rumble pulse 400ms");
                break;
            case '1':
                g_manual_sticky = true;
                DIAGLN("[cmd] rumble ON (manual sticky)");
                break;
            case '0':
                g_manual_sticky = false;
                DIAGLN("[cmd] rumble OFF (manual sticky cleared)");
                break;
            case 's':
            case 'S':
                printState();
                break;
            case 'v':
            case 'V':
                printBanner();
                break;
            default:
                break;
        }
    }
}

static bool g_last_rumble_logged = false;
static unsigned long g_last_hb = 0;

static void logRumble(bool rumble) {
    if (rumble != g_last_rumble_logged) {
        DIAG("[rumble] state=%s host=%d manual=%d\n",
             rumble ? "ON" : "OFF", (int) GcAdapter::rumble(), (int) manualRumbleActive());
        g_last_rumble_logged = rumble;
    }
}

static void heartbeat(bool rumble) {
    unsigned long now = millis();
    if (now - g_last_hb >= 2000) {
        g_last_hb = now;
        uint8_t *st = controller->getRawControllerState();
        DIAG("[hb] t=%lus mounted=%d started=%d rumble=%d state=%02x%02x%02x%02x%02x%02x%02x%02x\n",
             now / 1000, (int) TinyUSBDevice.mounted(), (int) GcAdapter::started(), (int) rumble,
             st[0], st[1], st[2], st[3], st[4], st[5], st[6], st[7]);
    }
}
#endif // GCCPICO_DIAG

void setup() {
    if (!TinyUSBDevice.isInitialized()) {
        TinyUSBDevice.begin(0);
    }

#if !GCCPICO_DIAG
    // Drop the CDC interface that Adafruit adds by default so the GC-adapter
    // vendor interface is interface 0 (what Dolphin claims).
    TinyUSBDevice.clearConfiguration();
#endif

    TinyUSBDevice.setID(0x057E, 0x0337);
    TinyUSBDevice.setManufacturerDescriptor("Nintendo");
    TinyUSBDevice.setProductDescriptor("Wii U GameCube Controller Adapter");

    GcAdapter::begin();
#if GCCPICO_DIAG
    Serial.begin(115200);
#endif

    if (TinyUSBDevice.mounted()) {
        TinyUSBDevice.detach();
        delay(10);
        TinyUSBDevice.attach();
    }

    auto *initParams = new InitParams();
    initParams->pin = 10;
    Controller::initPio(initParams);
    controller = new Controller(initParams, 8);
    controller->init();

#if GCCPICO_DIAG
    printBanner();
#endif

    // Brief boot rumble pulse: a sign of life and a quick GameCube-link check.
    controller->setRumble(true);
    controller->updateState();
    delay(150);
    controller->setRumble(false);
    controller->updateState();
}

void loop() {
#ifdef TINYUSB_NEED_POLLING_TASK
    TinyUSBDevice.task();
#endif

#if GCCPICO_DIAG
    handleSerial();
#endif

    if (!TinyUSBDevice.mounted()) {
        yield();
        return;
    }

    bool rumble = GcAdapter::rumble() || manualRumbleActive();
    controller->setRumble(rumble);
    controller->updateState();

    uint8_t port[9];
    buildPort(port, controller->getRawControllerState());
    GcAdapter::sendPort1(port);

#if GCCPICO_DIAG
    logRumble(rumble);
    heartbeat(rumble);
#endif

    yield();
}
