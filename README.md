Work in progress gamecube controller USB converter.

The gamecube controller driver is a modified version of the work done by David Pagels on [Retro Pico Switch](https://github.com/DavidPagels/retro-pico-switch)

The USB side uses Adafruit TinyUSB (`-DUSE_TINYUSB`) with a hand-built HID
descriptor: a gamepad input report plus a PID force-feedback collection so the
host can drive rumble (Linux `hid-pidff`, Windows DirectInput, SDL). The PID
descriptor is vendored from the MIT-licensed [ArduinoJoystickWithFFBLibrary](https://github.com/YukMingLaw/ArduinoJoystickWithFFBLibrary).

## Build & flash

```sh
tools/reflash.sh            # build, drop to BOOTSEL, flash (no button combo)
tools/reflash.sh --no-build # flash the existing build
```

Reflashing needs no `A+B+L+R+Start`: `tools/bootsel.py` does a 1200-baud touch
on the USB serial port to reboot into BOOTSEL, or send `b` over the serial
console. `tools/pio.sh` wraps PlatformIO (works around a venv built for an older
Python). The serial console (115200) also accepts `r`/`1`/`0` to drive rumble,
`s` for controller state, `v` for the version banner.

## Rumble

```sh
tools/rumble.py verify   # check the device advertises force feedback
tools/rumble.py play     # upload + play an effect (turns the motor on)
```

Diagnostics are logged over USB serial; build with `-DGCCPICO_DIAG=0` for a
quiet release.
