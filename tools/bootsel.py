#!/usr/bin/env python3
"""
Reboot the RP2040 gamepad into BOOTSEL (UF2 bootloader) from the host, with no
physical button combo.

Mechanism: the arduino-pico / Adafruit-TinyUSB CDC stack reboots into the UF2
bootloader when the USB CDC port is opened at 1200 baud and DTR is de-asserted
("1200-baud touch"). This is the same trick the Arduino IDE / PlatformIO use for
auto-reset. Implemented with the Python standard library only (termios + ioctl),
so there is no pyserial dependency.

Usage:
    python3 tools/bootsel.py            # auto-detect the Pico CDC port and touch it
    python3 tools/bootsel.py /dev/ttyACM0
    python3 tools/bootsel.py --detect   # just print the detected port, don't touch
"""
import os
import sys
import glob
import time
import struct
import termios
import fcntl

from gccpico import VIDS, realpath_glob, sysfs_climb

TIOCMBIS = 0x5416  # set modem bits
TIOCMBIC = 0x5417  # clear modem bits
TIOCM_DTR = 0x002


def detect_port() -> "str | None":
    # Prefer the stable by-id symlink if present.
    port = realpath_glob("/dev/serial/by-id/*")
    if port:
        return port
    # Fall back to scanning ttyACM* for the Raspberry Pi vendor id.
    for tty in sorted(glob.glob("/dev/ttyACM*")):
        vid = sysfs_climb(f"/sys/class/tty/{os.path.basename(tty)}/device", "idVendor")
        if vid:
            try:
                if open(vid).read().strip().lower() in VIDS:
                    return tty
            except OSError:
                pass
    # Last resort: first ttyACM.
    accs = sorted(glob.glob("/dev/ttyACM*"))
    return accs[0] if accs else None


def touch1200(port: str) -> None:
    fd = os.open(port, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
    try:
        attrs = termios.tcgetattr(fd)
        attrs[4] = termios.B1200  # ispeed
        attrs[5] = termios.B1200  # ospeed
        termios.tcsetattr(fd, termios.TCSANOW, attrs)
        # Assert then de-assert DTR while at 1200 baud -> triggers UF2 reboot.
        fcntl.ioctl(fd, TIOCMBIS, struct.pack("I", TIOCM_DTR))
        time.sleep(0.05)
        fcntl.ioctl(fd, TIOCMBIC, struct.pack("I", TIOCM_DTR))
        time.sleep(0.05)
    finally:
        try:
            os.close(fd)
        except OSError:
            pass


def main(argv: list[str]) -> int:
    detect_only = "--detect" in argv
    args = [a for a in argv if not a.startswith("--")]
    port = args[0] if args else detect_port()
    if not port:
        print("bootsel: no Pico CDC serial port found", file=sys.stderr)
        return 1
    if detect_only:
        print(port)
        return 0
    print(f"bootsel: 1200-baud touch on {port}")
    try:
        touch1200(port)
    except OSError as e:
        print(f"bootsel: touch failed on {port}: {e}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
