#!/usr/bin/env python3
"""Turn sdmc:/switch/RingOut/profile.txt into function names.

    ./switch/profile-report.py /mnt/c/.../profile.txt [RingOut.elf]

profile.txt comes from nx.cfg profile=1 (ModernGekko/platform/switch/
profiler.cpp): samples of the CPU thread's PC and LR, as offsets into the
executable. They are looked up in RingOut.elf -- the one from the SAME build
as the .nro that recorded them, which build-switch.sh leaves under
build-switch/runtime/ -- with devkitA64's nm.
"""

import bisect
import collections
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)


def find_elf():
    root = os.path.join(os.environ.get("BUILD_DIR", os.path.join(REPO, "build-switch")), "runtime")
    for dirpath, _, files in os.walk(root):
        if "RingOut.elf" in files:
            return os.path.join(dirpath, "RingOut.elf")
    return None


def load_symbols(elf):
    devkitpro = os.environ.get("DEVKITPRO", "/opt/devkitpro")
    nm = os.path.join(devkitpro, "devkitA64", "bin", "aarch64-none-elf-nm")
    if not os.path.exists(nm):
        nm = "nm"
    out = subprocess.run([nm, "-n", "-C", "--defined-only", elf],
                         check=True, capture_output=True, text=True).stdout
    addrs, names = [], []
    for line in out.splitlines():
        parts = line.split(" ", 2)
        if len(parts) != 3 or parts[1] not in "tTwW":
            continue
        addrs.append(int(parts[0], 16))
        names.append(parts[2])
    return addrs, names


def lookup(addrs, names, offset):
    i = bisect.bisect_right(addrs, offset) - 1
    return names[i] if i >= 0 else "?"


CPU_C = re.compile(r"^(ppc_|mem_(read|write)\d+_slow|psq_|resolve_addr|gather_pipe_store|"
                   r"classify_f|f(32|64)_|exception_|dolrecomp_|clear_matching_reservation)")


def category(name):
    if re.match(r"^func_[0-9A-F]{8}_cold\b", name):
        return "game code, cold copies (func_*_cold)"
    if re.match(r"^func_[0-9A-F]{8}\b", name):
        return "game code (recompiled func_*)"
    if CPU_C.match(name):
        return "recompiler helpers (cpu.c)"
    if name.startswith("svc") or name.startswith("__nx_") or "Wait" in name and "svc" in name:
        return "kernel calls / waiting"
    if name in ("memcpy", "memset", "memmove", "memcmp", "strlen") or name.startswith("_malloc") \
            or name.startswith("_free") or name in ("malloc", "free"):
        return "libc (memcpy, malloc...)"
    m = re.match(r"^(?:[A-Za-z_]\w*::)*?((?:DSP|CoreTiming|Interpreter|PowerPC|StaticRecomp\w*|"
                 r"MMIO|SerialInterface|VideoInterface|ExpansionInterface|DVD\w*|GPFifo|"
                 r"CommandProcessor|AudioCommon|HW|Memory|Core|Common|OpcodeDecoder|"
                 r"VertexLoader\w*|IOS)\b)", name)
    if m:
        return "Dolphin: " + m.group(1)
    if "::" in name:
        return "Dolphin/other C++: " + name.split("::")[0].split("<")[0].split(" ")[-1]
    return "other: " + name.split("(")[0][:40]


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    elf = sys.argv[2] if len(sys.argv) > 2 else find_elf()
    if not elf or not os.path.exists(elf):
        sys.exit("RingOut.elf not found; pass it as the second argument")
    addrs, names = load_symbols(elf)

    anchor = addrs[names.index("ringout_nx_profile_anchor")] \
        if "ringout_nx_profile_anchor" in names else None
    header = ""
    by_func = collections.Counter()
    by_cat = collections.Counter()
    callers = collections.defaultdict(collections.Counter)
    total = 0
    with open(sys.argv[1]) as f:
        for line in f:
            if line.startswith("#"):
                if "profile v1" in line:
                    sys.exit("profile.txt is from the first profiler build, whose addresses "
                             "were wrong; record it again with the current .nro")
                header += line
                continue
            parts = line.split()
            if len(parts) != 4 or parts[0] != "s":
                continue
            if anchor is None:
                sys.exit("ringout_nx_profile_anchor is not in this RingOut.elf -- "
                         "is it the build that recorded the profile?")
            pc, lr, n = anchor + int(parts[1]), anchor + int(parts[2]), int(parts[3])
            fn = lookup(addrs, names, pc)
            by_func[fn] += n
            by_cat[category(fn)] += n
            callers[fn][lookup(addrs, names, lr)] += n
            total += n

    print(header.rstrip())
    print(f"# elf {elf}")
    print(f"# code span {(addrs[-1] - addrs[0]) / 1048576:.1f} MB")
    if not total:
        sys.exit("no samples")
    print(f"\n{total} samples\n\n== by category")
    for cat, n in by_cat.most_common():
        print(f"{100.0 * n / total:6.2f}%  {cat}")

    print("\n== top 50 functions")
    for fn, n in by_func.most_common(50):
        print(f"{100.0 * n / total:6.2f}%  {fn[:110]}")

    game = sum(n for fn, n in by_func.items() if fn.startswith("func_"))
    print("\n== who calls the hottest non-game functions (by LR)")
    shown = 0
    for fn, n in by_func.most_common():
        if fn.startswith("func_"):
            continue
        print(f"\n{100.0 * n / total:6.2f}%  {fn[:110]}")
        for caller, m in callers[fn].most_common(6):
            print(f"        {100.0 * m / n:5.1f}%  from {caller[:100]}")
        shown += 1
        if shown == 12:
            break
    print(f"\ngame code total: {100.0 * game / total:.2f}%")


if __name__ == "__main__":
    main()
