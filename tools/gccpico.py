"""Shared host-side helpers for locating the GCCPico's device nodes.

Imported by bootsel.py and rumble.py (running a script puts its own directory
on sys.path, so a plain `import gccpico` works).
"""
import glob
import os

# USB vendor ids we recognise: 2e8a = RP2040 (bootloader / GCCPico builds),
# 057e = the emulated Nintendo GameCube adapter.
VIDS = ("2e8a", "057e")
VID = "2e8a"  # kept for callers that compare a single RP2040 vid

_NAME_HINTS = ("gccpico", "gamecube", "adapter", "nintendo", "rp2", "pico")


def _is_gccpico(path):
    low = path.lower()
    return any(h in low for h in _NAME_HINTS) or any(v in low for v in VIDS)


def realpath_glob(pattern, require_match=True):
    """Resolve the first device matching `pattern`.

    With require_match (default) only a node that looks like the GCCPico is
    returned (else None); otherwise the first match is returned regardless.
    """
    paths = sorted(glob.glob(pattern))
    if require_match:
        for p in paths:
            if _is_gccpico(p):
                return os.path.realpath(p)
        return None
    return os.path.realpath(paths[0]) if paths else None


def sysfs_climb(start, filename, levels=6):
    """Walk up the sysfs tree from `start` looking for `filename`."""
    cur = os.path.realpath(start)
    for _ in range(levels):
        cand = os.path.join(cur, filename)
        if os.path.exists(cand):
            return cand
        parent = os.path.dirname(cur)
        if parent == cur:
            break
        cur = parent
    return None
