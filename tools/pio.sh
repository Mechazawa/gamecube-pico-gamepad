#!/usr/bin/env bash
# PlatformIO wrapper.
#
# The user's ~/.platformio virtualenv was built for Python 3.13, but the system
# Python has since moved to 3.14, which breaks the `pio` launcher
# ("No module named 'platformio'"). PlatformIO Core is pure-Python enough to run
# fine under 3.14 when pointed at the existing install via PYTHONPATH, so we fall
# back to that when the normal launcher is broken.
set -euo pipefail

if command -v pio >/dev/null 2>&1 && pio --version >/dev/null 2>&1; then
    exec pio "$@"
fi

# Fallback: run the installed Core with the system python.
shopt -s nullglob
sp=("$HOME"/.platformio/penv/lib/python3.*/site-packages)
if [ ${#sp[@]} -gt 0 ]; then
    export PYTHONPATH="${sp[0]}${PYTHONPATH:+:$PYTHONPATH}"
fi
exec python3 -m platformio "$@"
