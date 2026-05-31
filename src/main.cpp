#include <Arduino.h>

#include "Controller.h"
#include "ControllerState.h"
#include "UsbHid.h"

#define FW_VERSION "0.3.0-pidff"

Controller *controller = nullptr;
ControllerState *state = nullptr;

// Manual rumble override driven over the serial console, independent of the
// host HID output report. Lets us exercise the GameCube rumble path on its own.
static bool g_manual_sticky = false;
static unsigned long g_manual_until = 0;

// Diagnostics bookkeeping.
static bool g_last_rumble_logged = false;
static uint32_t g_last_rx = 0;
static unsigned long g_last_hb = 0;

static bool manualRumbleActive() {
    return g_manual_sticky || (long)(g_manual_until - millis()) > 0;
}

// Map the project's historical 0..1023 axis convention to the 16-bit signed HID
// range, identical to the arduino-pico Joystick library's default (10-bit) mode
// so the host sees the same axis behaviour it always did.
static int16_t mapBits10(int v) {
    if (v < 0) {
        return 0;
    }
    if (v > 1023) {
        return 32767;
    }
    return (int16_t) map(v, 0, 1023, -32767, 32767);
}

static GamepadReport buildReport(ControllerState *s) {
    GamepadReport r = {};
    r.x = mapBits10(s->ax() * 4);          // main stick X
    r.y = mapBits10(1023 - s->ay() * 4);   // main stick Y (inverted)
    r.z = mapBits10(s->al() * 4);          // left analog trigger
    r.rz = mapBits10(s->ar() * 4);         // right analog trigger
    r.rx = mapBits10(s->cx() * 4);         // C-stick X
    r.ry = mapBits10(1023 - s->cy() * 4);  // C-stick Y (inverted)
    r.hat = (uint8_t) s->dpad();

    uint32_t b = 0;
    b |= (uint32_t) s->a() << 0;
    b |= (uint32_t) s->x() << 1;
    b |= (uint32_t) s->start() << 2;
    b |= (uint32_t) s->y() << 3;
    b |= (uint32_t) s->b() << 4;
    b |= (uint32_t) s->l() << 5;
    b |= (uint32_t) s->r() << 6;
    b |= (uint32_t) s->z() << 7;
    r.buttons = b;
    return r;
}

static void printBanner() {
    Serial.println();
    Serial.println("=== GameCube USB gamepad ===");
    Serial.print("firmware: ");
    Serial.println(FW_VERSION);
    Serial.println("usb stack: Adafruit TinyUSB (CDC + HID gamepad + rumble out)");
    Serial.println("serial commands: b=BOOTSEL  r=rumble pulse  1=rumble on  0=off  s=state  v=version");
}

static void printState() {
    uint8_t *st = controller->getRawControllerState();
    Serial.printf("[state] %02x %02x %02x %02x %02x %02x %02x %02x\n",
                  st[0], st[1], st[2], st[3], st[4], st[5], st[6], st[7]);
}

static void handleSerial() {
    while (Serial.available()) {
        int c = Serial.read();
        switch (c) {
            case 'b':
            case 'B':
                Serial.println("[cmd] rebooting to BOOTSEL...");
                Serial.flush();
                delay(50);
                rp2040.rebootToBootloader();
                break;
            case 'r':
            case 'R':
                g_manual_until = millis() + 400;
                Serial.println("[cmd] rumble pulse 400ms");
                break;
            case '1':
                g_manual_sticky = true;
                Serial.println("[cmd] rumble ON (manual sticky)");
                break;
            case '0':
                g_manual_sticky = false;
                Serial.println("[cmd] rumble OFF (manual sticky cleared)");
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

static void logRumble(bool rumble) {
    uint32_t rx = UsbHid::rxCount();
    if (rumble != g_last_rumble_logged || rx != g_last_rx) {
        Serial.printf("[rumble] state=%s raw=%u hostRx=%lu manual=%d\n",
                      rumble ? "ON" : "OFF", UsbHid::rumbleRaw(),
                      (unsigned long) rx, (int) manualRumbleActive());
        g_last_rumble_logged = rumble;
        g_last_rx = rx;
    }
}

static void heartbeat(bool rumble) {
    unsigned long now = millis();
    if (now - g_last_hb >= 2000) {
        g_last_hb = now;
        uint8_t *st = controller->getRawControllerState();
        Serial.printf("[hb] t=%lus mounted=%d rumble=%d hostRx=%lu state=%02x%02x%02x%02x%02x%02x%02x%02x\n",
                      now / 1000, (int) UsbHid::mounted(), (int) rumble,
                      (unsigned long) UsbHid::rxCount(),
                      st[0], st[1], st[2], st[3], st[4], st[5], st[6], st[7]);
    }
}

void setup() {
    Serial.begin(115200);

    auto *initParams = new InitParams();
    initParams->pin = 10;
    Controller::initPio(initParams);

    controller = new Controller(initParams, 8);
    controller->init();

    state = new ControllerState(controller->getRawControllerState());

    UsbHid::begin(0x2E8A, 0x0010, "GCCPico", "GCCPico Gamepad");

    printBanner();

    // Brief boot rumble pulse: a sign of life and a quick exercise of the
    // GameCube rumble path on startup.
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

    handleSerial();

    if (!UsbHid::mounted()) {
        yield();
        return;
    }

    bool rumble = UsbHid::rumbleOn() || manualRumbleActive();
    controller->setRumble(rumble);
    controller->updateState();

    if (UsbHid::ready()) {
        GamepadReport r = buildReport(state);
        UsbHid::sendGamepad(r);
    }

    logRumble(rumble);
    heartbeat(rumble);

    yield();
}
