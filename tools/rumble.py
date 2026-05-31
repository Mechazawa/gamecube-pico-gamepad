#!/usr/bin/env python3
"""
Verify and exercise GCCPico rumble from the host.

  tools/rumble.py verify          # check the device advertises force feedback
  tools/rumble.py play            # upload + play a rumble effect, then stop
  tools/rumble.py play --type constant --ms 1500

"verify" checks two things, no special privileges needed beyond the udev ACL
that already grants the logged-in user access to the event node:
  1. the HID report descriptor declares the PID (force-feedback) usage page,
  2. the kernel created an EV_FF event device exposing FF_RUMBLE / FF_CONSTANT
     / FF_PERIODIC (i.e. hid-pidff bound).

"play" uploads an effect with EVIOCSFF and starts it; on the GCCPico this turns
the GameCube rumble motor on. Standard library only (no pyserial / evdev).
"""
import os
import sys
import time
import struct
import fcntl

from gccpico import realpath_glob, sysfs_climb

# ---- input subsystem constants ----
EV_FF = 0x15
FF_RUMBLE, FF_PERIODIC, FF_CONSTANT, FF_SPRING, FF_FRICTION = 0x50, 0x51, 0x52, 0x53, 0x54
FF_DAMPER, FF_INERTIA, FF_RAMP, FF_SQUARE, FF_TRIANGLE = 0x55, 0x56, 0x57, 0x58, 0x59
FF_SINE, FF_SAW_UP, FF_SAW_DOWN, FF_CUSTOM, FF_GAIN, FF_AUTOCENTER = 0x5a, 0x5b, 0x5c, 0x5d, 0x60, 0x61
FF_NAMES = {
    FF_RUMBLE: "RUMBLE", FF_PERIODIC: "PERIODIC", FF_CONSTANT: "CONSTANT",
    FF_SPRING: "SPRING", FF_FRICTION: "FRICTION", FF_DAMPER: "DAMPER",
    FF_INERTIA: "INERTIA", FF_RAMP: "RAMP", FF_SQUARE: "SQUARE",
    FF_TRIANGLE: "TRIANGLE", FF_SINE: "SINE", FF_SAW_UP: "SAW_UP",
    FF_SAW_DOWN: "SAW_DOWN", FF_CUSTOM: "CUSTOM", FF_GAIN: "GAIN",
    FF_AUTOCENTER: "AUTOCENTER",
}

FF_EFFECT_SIZE = 48  # sizeof(struct ff_effect) on LP64


def _ioc(d, t, nr, size):
    return (d << 30) | (size << 16) | (ord(t) << 8) | nr


def find_hidraw():
    return (realpath_glob("/dev/input/by-id/*-hidraw")
            or realpath_glob("/dev/input/by-id/*-hidraw", require_match=False))


def find_event():
    return realpath_glob("/dev/input/by-id/*event*")


def report_descriptor_for_event(evpath):
    # /sys/class/input/eventN/device/.../report_descriptor (the hid device)
    name = os.path.basename(evpath)
    rd = sysfs_climb(f"/sys/class/input/{name}/device", "report_descriptor")
    if rd:
        with open(rd, "rb") as f:
            return f.read()
    return None


def descriptor_has_pid(desc):
    # Scan for Usage Page (Physical Interface) = 0x05 0x0F, and an Output item.
    has_pid_page = False
    has_output = False
    i = 0
    while i + 1 < len(desc):
        if desc[i] == 0x05 and desc[i + 1] == 0x0F:
            has_pid_page = True
        if desc[i] == 0x91:  # Output item (short item, 1 data byte)
            has_output = True
        i += 1
    return has_pid_page, has_output


def read_ff_bits(evpath):
    fd = os.open(evpath, os.O_RDONLY | os.O_NONBLOCK)
    try:
        nbytes = (FF_AUTOCENTER // 8) + 1 + 8
        buf = bytearray(nbytes)
        EVIOCGBIT_FF = _ioc(2, 'E', 0x20 + EV_FF, nbytes)
        fcntl.ioctl(fd, EVIOCGBIT_FF, buf, True)
        bits = set()
        for byte_i, b in enumerate(buf):
            for bit in range(8):
                if b & (1 << bit):
                    bits.add(byte_i * 8 + bit)
        return bits
    finally:
        os.close(fd)


def cmd_verify():
    ev = find_event()
    hr = find_hidraw()
    print(f"event node : {ev}")
    print(f"hidraw node: {hr}")
    ok = True

    desc = report_descriptor_for_event(ev) if ev else None
    if desc:
        pid, out = descriptor_has_pid(desc)
        print(f"HID descriptor: {len(desc)} bytes, PID usage page={'yes' if pid else 'NO'}, "
              f"Output item={'yes' if out else 'NO'}")
        ok = ok and pid and out
    else:
        print("HID descriptor: not found")
        ok = False

    if ev:
        bits = read_ff_bits(ev)
        names = sorted(FF_NAMES.get(b, hex(b)) for b in bits)
        print(f"EV_FF effects: {', '.join(names) if names else '(none)'}")
        usable = bits & {FF_RUMBLE, FF_CONSTANT, FF_PERIODIC}
        ok = ok and bool(usable)
    else:
        print("no EV_FF event device -> hid-pidff did not bind")
        ok = False

    print()
    print("RESULT:", "PASS - rumble is advertised and OS-triggerable" if ok else "FAIL")
    return 0 if ok else 1


def make_effect(kind, ms):
    eid = -1
    direction = 0x4000
    replay = struct.pack('<HH', ms, 0)  # length, delay
    trigger = struct.pack('<HH', 0, 0)
    if kind == "rumble":
        head = struct.pack('<Hh H', FF_RUMBLE, eid, 0) + trigger + replay + b'\x00\x00'
        body = struct.pack('<HH', 0xFFFF, 0xFFFF) + b'\x00' * 28
    elif kind == "constant":
        head = struct.pack('<Hh H', FF_CONSTANT, eid, direction) + trigger + replay + b'\x00\x00'
        # ff_constant_effect: __s16 level; ff_envelope(8)
        body = struct.pack('<h', 0x7FFF) + b'\x00' * 30
    elif kind == "periodic":
        head = struct.pack('<Hh H', FF_PERIODIC, eid, direction) + trigger + replay + b'\x00\x00'
        # waveform, period, magnitude, offset, phase, envelope(8), custom_len, *custom
        body = struct.pack('<HHhHH', FF_SINE, 100, 0x7FFF, 0, 0) + b'\x00' * 22
    else:
        raise SystemExit(f"unknown effect type {kind}")
    b = bytearray(head + body)
    assert len(b) == FF_EFFECT_SIZE, len(b)
    return b


def cmd_play(kind, ms):
    ev = find_event()
    if not ev:
        print("no event device", file=sys.stderr)
        return 1
    EVIOCSFF = _ioc(1, 'E', 0x80, FF_EFFECT_SIZE)
    EVIOCRMFF = _ioc(1, 'E', 0x81, 4)
    fd = os.open(ev, os.O_RDWR)
    try:
        buf = make_effect(kind, ms)
        fcntl.ioctl(fd, EVIOCSFF, buf, True)
        eid = struct.unpack('<h', bytes(buf[2:4]))[0]
        print(f"uploaded {kind} effect id={eid}, playing for {ms} ms")
        os.write(fd, struct.pack('<qqHHi', 0, 0, EV_FF, eid, 1))
        time.sleep(ms / 1000.0)
        os.write(fd, struct.pack('<qqHHi', 0, 0, EV_FF, eid, 0))
        try:
            fcntl.ioctl(fd, EVIOCRMFF, eid)
        except OSError:
            pass
        print("stopped")
        return 0
    finally:
        os.close(fd)


def main(argv):
    cmd = argv[0] if argv else "verify"
    if cmd == "verify":
        return cmd_verify()
    if cmd == "play":
        kind = "rumble"
        ms = 1500
        a = argv[1:]
        i = 0
        while i < len(a):
            if a[i] == "--type":
                kind = a[i + 1]; i += 2
            elif a[i] == "--ms":
                ms = int(a[i + 1]); i += 2
            else:
                i += 1
        return cmd_play(kind, ms)
    print(__doc__)
    return 2


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
