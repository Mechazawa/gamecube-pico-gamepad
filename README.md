Work in progress gamecube controller USB converter.

The gamecube controller driver is a modified version of the work done by David Pagels on [Retro Pico Switch](https://github.com/DavidPagels/retro-pico-switch)

## USB

The device emulates Nintendo's **GameCube Controller Adapter for Wii U**
(WUP-028, USB `057E:0337`) on top of Adafruit TinyUSB. Dolphin (and a Switch)
auto-detect it by VID:PID over libusb with no configuration, including rumble.
The attached controller appears on port 1. On Linux, Dolphin needs access to the
device — use the same udev rule as a real adapter, or run with privileges.

The protocol (interface 0, two interrupt endpoints): host sends `0x13` to start
streaming and `{0x11,r0,r1,r2,r3}` to set per-port rumble; the device streams
37-byte input packets (`0x21` + four 9-byte ports). See `src/GcAdapter.cpp`.

## Build & flash

```sh
tools/reflash.sh            # build, drop to BOOTSEL, flash (no button combo)
tools/reflash.sh --no-build # flash the existing build
```

The release firmware is the bare GC adapter (no CDC serial), so `reflash.sh`
reboots it into BOOTSEL with a vendor "reboot magic" over libusb
(`tools/gcadapter.py reboot`, needs sudo / a udev rule) and then `picotool load`.

`tools/gcadapter.py test` exercises the protocol like Dolphin does (start
streaming, print port-1 input, pulse rumble).

### Debug build

`-DGCCPICO_DIAG=1` adds a USB-CDC serial console (115200) for logging and the
1200-baud touch reset, with serial commands `b`=BOOTSEL `r`/`1`/`0`=rumble
`s`=state `v`=version. It adds extra interfaces, so it is **not**
Dolphin-compatible — use it only for bring-up. `tools/pio.sh` wraps PlatformIO
(works around a venv built for an older Python).
