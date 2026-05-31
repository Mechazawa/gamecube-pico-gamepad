#!/usr/bin/env bash
# Build and flash the firmware with no physical button combo.
#
# Flow:
#   1. build (unless --no-build)
#   2. if the board is running the app, drop it into BOOTSEL via 1200-baud touch
#   3. flash the fresh UF2 with picotool and start it
#
# This relies on the firmware exposing a USB CDC serial port (it does: the
# TinyUSB stack provides Serial). The 1200-baud touch is done by tools/bootsel.py.
set -euo pipefail
cd "$(dirname "$0")/.."

PIO="${PIO:-tools/pio.sh}"
UF2=".pio/build/pico/firmware.uf2"

do_build=1
for a in "$@"; do
    [ "$a" = "--no-build" ] && do_build=0
done

if [ "$do_build" = 1 ]; then
    echo ">> building"
    "$PIO" run -e pico
fi

[ -f "$UF2" ] || { echo "!! $UF2 not found"; exit 1; }

if lsusb | grep -q '2e8a:0003'; then
    echo ">> board already in BOOTSEL"
else
    if python3 tools/bootsel.py --detect >/dev/null 2>&1; then
        # Debug build: a USB-CDC serial port is present -> 1200-baud touch.
        echo ">> dropping board into BOOTSEL (1200-baud touch)"
        python3 tools/bootsel.py || true
    elif lsusb | grep -q '057e:0337'; then
        # Release build: GC adapter, no CDC -> reboot magic over libusb.
        echo ">> dropping board into BOOTSEL (GC adapter reboot magic)"
        sudo python3 tools/gcadapter.py reboot || true
    else
        echo "!! no GCCPico / GC-adapter device found to reboot"; exit 1
    fi
    echo -n ">> waiting for BOOTSEL "
    for _ in $(seq 1 40); do
        if lsusb | grep -q '2e8a:0003'; then echo "ok"; break; fi
        echo -n "."; sleep 0.25
    done
    lsusb | grep -q '2e8a:0003' || { echo; echo "!! BOOTSEL did not appear"; exit 1; }
fi

echo ">> flashing $UF2"
picotool load -x "$UF2"

echo -n ">> waiting for app to re-enumerate "
for _ in $(seq 1 40); do
    if lsusb | grep -q '057e:0337' || compgen -G "/dev/ttyACM*" >/dev/null; then echo "ok"; break; fi
    echo -n "."; sleep 0.25
done
echo ">> done"
