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
    echo ">> dropping board into BOOTSEL (1200-baud touch)"
    python3 tools/bootsel.py || true
    echo -n ">> waiting for BOOTSEL "
    for _ in $(seq 1 40); do
        if lsusb | grep -q '2e8a:0003'; then echo "ok"; break; fi
        echo -n "."; sleep 0.25
    done
    lsusb | grep -q '2e8a:0003' || { echo; echo "!! BOOTSEL did not appear"; exit 1; }
fi

echo ">> flashing $UF2"
picotool load -x "$UF2"

echo -n ">> waiting for app serial port "
for _ in $(seq 1 40); do
    if compgen -G "/dev/ttyACM*" >/dev/null; then echo "ok"; break; fi
    echo -n "."; sleep 0.25
done
ls -l /dev/ttyACM* 2>/dev/null || true
echo ">> done"
