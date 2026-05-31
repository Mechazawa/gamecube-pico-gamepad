#!/usr/bin/env python3
"""
Talk to the emulated Nintendo GameCube adapter (USB 057E:0337) over libusb,
the same way Dolphin does. Standard library only (ctypes + libusb-1.0).

  tools/gcadapter.py test     # start streaming, print port 1 input, pulse rumble
  tools/gcadapter.py reboot   # reboot the device into BOOTSEL (for reflashing)

Needs access to the USB device: run with sudo, or install a udev rule granting
your user access to 057e:0337 (the same rule Dolphin uses for a real adapter).

The release firmware exposes the vendor interface as interface 0 with endpoints
0x01 (OUT) / 0x81 (IN); override with --interface/--ep-out/--ep-in for the debug
build (interface 2, 0x02 / 0x83).
"""
import ctypes
import sys
import time

VID, PID = 0x057E, 0x0337
REBOOT_MAGIC = bytes([0x52, 0x42, 0x54, 0x21])  # "RBT!"

lib = ctypes.CDLL("libusb-1.0.so.0")
lib.libusb_open_device_with_vid_pid.restype = ctypes.c_void_p
lib.libusb_open_device_with_vid_pid.argtypes = [ctypes.c_void_p, ctypes.c_uint16, ctypes.c_uint16]
lib.libusb_claim_interface.argtypes = [ctypes.c_void_p, ctypes.c_int]
lib.libusb_release_interface.argtypes = [ctypes.c_void_p, ctypes.c_int]
lib.libusb_set_auto_detach_kernel_driver.argtypes = [ctypes.c_void_p, ctypes.c_int]
lib.libusb_close.argtypes = [ctypes.c_void_p]
lib.libusb_interrupt_transfer.argtypes = [
    ctypes.c_void_p, ctypes.c_ubyte, ctypes.c_void_p, ctypes.c_int,
    ctypes.POINTER(ctypes.c_int), ctypes.c_uint,
]


class Dev:
    def __init__(self, interface):
        self.interface = interface
        if lib.libusb_init(None) != 0:
            raise SystemExit("libusb_init failed")
        self.h = lib.libusb_open_device_with_vid_pid(None, VID, PID)
        if not self.h:
            raise SystemExit(f"device {VID:04x}:{PID:04x} not found (run with sudo?)")
        lib.libusb_set_auto_detach_kernel_driver(self.h, 1)
        if lib.libusb_claim_interface(self.h, interface) != 0:
            raise SystemExit(f"claim interface {interface} failed (run with sudo?)")

    def write(self, ep_out, data):
        buf = (ctypes.c_ubyte * len(data))(*data)
        n = ctypes.c_int(0)
        return lib.libusb_interrupt_transfer(self.h, ep_out, buf, len(data), ctypes.byref(n), 200)

    def read(self, ep_in, length=37, timeout=200):
        buf = (ctypes.c_ubyte * length)()
        n = ctypes.c_int(0)
        r = lib.libusb_interrupt_transfer(self.h, ep_in, buf, length, ctypes.byref(n), timeout)
        return r, bytes(buf[:n.value])

    def close(self):
        lib.libusb_release_interface(self.h, self.interface)
        lib.libusb_close(self.h)
        lib.libusb_exit(None)


def decode_port(p):
    if len(p) < 9:
        return "no data"
    b1, b2 = p[1], p[2]
    btns = []
    for bit, name in enumerate("A B X Y Left Right Down Up".split()):
        if b1 & (1 << bit):
            btns.append(name)
    for bit, name in enumerate("Start Z R L".split()):
        if b2 & (1 << bit):
            btns.append(name)
    return (f"type=0x{p[0]:02x} stick=({p[3]},{p[4]}) c=({p[5]},{p[6]}) "
            f"LR=({p[7]},{p[8]}) buttons=[{' '.join(btns)}]")


def cmd_test(args):
    d = Dev(args.interface)
    try:
        print(f"-> init (0x13) on ep 0x{args.ep_out:02x}")
        d.write(args.ep_out, [0x13])
        time.sleep(0.05)
        print("<- input packets:")
        for _ in range(5):
            r, data = d.read(args.ep_in)
            if r == 0 and len(data) == 37 and data[0] == 0x21:
                print("   port1:", decode_port(data[1:10]))
            else:
                print(f"   (rc={r} len={len(data)})")
            time.sleep(0.05)
        print("-> rumble ON (0x11,1)")
        d.write(args.ep_out, [0x11, 1, 0, 0, 0])
        time.sleep(1.0)
        print("-> rumble OFF (0x11,0)")
        d.write(args.ep_out, [0x11, 0, 0, 0, 0])
    finally:
        d.close()


def cmd_reboot(args):
    d = Dev(args.interface)
    try:
        print("-> reboot magic; device should drop into BOOTSEL")
        d.write(args.ep_out, list(REBOOT_MAGIC))
    finally:
        try:
            d.close()
        except Exception:
            pass


def main(argv):
    import argparse
    ap = argparse.ArgumentParser()
    ap.add_argument("cmd", choices=["test", "reboot"])
    ap.add_argument("--interface", type=int, default=0)
    ap.add_argument("--ep-out", type=lambda x: int(x, 0), default=0x01)
    ap.add_argument("--ep-in", type=lambda x: int(x, 0), default=0x81)
    args = ap.parse_args(argv)
    (cmd_test if args.cmd == "test" else cmd_reboot)(args)
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
