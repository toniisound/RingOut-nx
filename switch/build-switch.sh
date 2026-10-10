#!/usr/bin/env bash
# Build Ring Out for Nintendo Switch (.nro), from Linux or WSL.
#
#   ./switch/build-switch.sh <linux-package-dir>          full build
#   ./switch/build-switch.sh --no-module                  runtime only (interpreter)
#
# <linux-package-dir> is an unzipped RingOut-<version>-linux-x86_64 release in
# which ./setup.sh has already been run on YOUR disc. That run is what extracts
# the disc into game/ and translates main.dol into C under work/out/generated/,
# with the right per-disc recompiler flags; this script only cross-compiles
# that C for the Switch and links it into the .nro. The release must be the
# same version as this tree (see VERSION), or the generated C and the runtime
# will not agree on the module ABI.
#
# Needs devkitPro with: sudo dkp-pacman -S switch-dev switch-cmake switch-pkg-config switch-mesa switch-libdrm_nouveau
# plus cmake, ninja and python3 on the host.
#
# Output: build-switch/sd/switch/RingOut/RingOut.nro. Copy that folder to the
# SD card, then copy the package's game/ folder next to the .nro. Neither the
# .nro (it contains your disc's code) nor game/ may be redistributed.

set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO="$(cd "$HERE/.." && pwd)"
BUILD="${BUILD_DIR:-$REPO/build-switch}"
DEVKITPRO="${DEVKITPRO:-/opt/devkitpro}"
TOOLCHAIN="$DEVKITPRO/cmake/Switch.cmake"
MODULE_SRC="$REPO/dist/RingOut-1.0-dist/module-src"
DEPS="$MODULE_SRC/deps"
JOBS="${JOBS:-$(nproc)}"

die() { echo "error: $*" >&2; exit 1; }

PKG=""
NO_MODULE=0
for arg in "$@"; do
    case "$arg" in
        --no-module) NO_MODULE=1 ;;
        -h|--help) sed -n 2,22p "$0"; exit 0 ;;
        *) PKG="$arg" ;;
    esac
done

[ -f "$TOOLCHAIN" ] || die "$TOOLCHAIN not found. Install devkitPro and run: sudo dkp-pacman -S switch-dev switch-cmake switch-pkg-config switch-mesa switch-libdrm_nouveau"
for tool in cmake ninja python3; do
    command -v "$tool" >/dev/null || die "$tool is not installed"
done
ELF2NRO="$DEVKITPRO/tools/bin/elf2nro"
NACPTOOL="$DEVKITPRO/tools/bin/nacptool"
[ -x "$ELF2NRO" ] && [ -x "$NACPTOOL" ] || die "elf2nro/nacptool missing (switch-tools package)"
export DEVKITPRO

VERSION="$(tr -d '[:space:]' < "$REPO/VERSION")"

# devkitPro's toolchain makes Release mean "-g -O2". RingOut measured its
# module 9.1% slower at -O2 than at -O3 (README, "What was measured and
# rejected"), so both the module and the runtime are built at -O3 here; -g only
# makes the ELF bigger. -ffp-contract=off is the module's own reproducibility
# flag and is added by its CMakeLists either way. OPT=-O2 restores the default.
OPT="${OPT:--O3}"
RELEASE_FLAGS=(
    "-DCMAKE_C_FLAGS_RELEASE=$OPT -DNDEBUG"
    "-DCMAKE_CXX_FLAGS_RELEASE=$OPT -DNDEBUG"
)
MODULE_ARGS=()

if [ "$NO_MODULE" = 0 ]; then
    [ -n "$PKG" ] || die "pass the Linux package directory where setup.sh was run (or --no-module)"
    PKG="$(cd "$PKG" && pwd)"
    GENERATED="$PKG/work/out/generated"
    [ -f "$PKG/game/sys/boot.bin" ] || die "$PKG/game is missing -- run ./setup.sh <disc> there first"
    [ -f "$GENERATED/generated.h" ] || die "$GENERATED is missing -- setup.sh did not finish its recompile stage"
    [ -f "$GENERATED/main.dol" ] || cp "$PKG/game/sys/main.dol" "$GENERATED/main.dol"
    DISC_ID="$(head -c 6 "$PKG/game/sys/boot.bin")"
    echo "==> disc $DISC_ID"

    # Same gate as setup.sh: these change chunk code shape, and the recompiler
    # flags setup.sh used for this disc assume them.
    MODULE_FLAGS=()
    case "$DISC_ID" in
        GRSEAF|GRSJAF|GRSPAF|GRSEPS) MODULE_FLAGS=(-DMODULE_PSQ_FAST=ON -DMODULE_MEM_FAST=ON) ;;
    esac

    echo "==> 1/3  Cross-compiling the game module (several minutes)"
    cmake -S "$MODULE_SRC" -B "$BUILD/module" -G Ninja \
        -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN" \
        -DCMAKE_BUILD_TYPE=Release \
        "${RELEASE_FLAGS[@]}" \
        -DMODULE_STATIC=ON \
        -DMODULE_LTO=OFF \
        "${MODULE_FLAGS[@]}" \
        -DGAME_ID="$DISC_ID" \
        -DGENERATED_DIR="$GENERATED" \
        -DDOLRECOMP_SRC="$DEPS/dolrecomp-src" \
        -DGXRUNTIME_INC="$DEPS/gxruntime-include" \
        -DCHASSIS_ABI_DIR="$DEPS/chassis-abi" \
        -DMODULE_TEMPLATE="$DEPS/module-template"
    cmake --build "$BUILD/module" -j "$JOBS"
    MODULE_LIB=""
    for candidate in "$BUILD/module/g${DISC_ID}_recomp.a" "$BUILD/module/libg${DISC_ID}_recomp.a"; do
        [ -f "$candidate" ] && MODULE_LIB="$candidate" && break
    done
    [ -n "$MODULE_LIB" ] || die "the module archive was not produced"
    MODULE_ARGS=(-DRINGOUT_NX_MODULE="$MODULE_LIB")
else
    echo "==> runtime only: no game module, the interpreter will run everything (very slow)"
    MODULE_ARGS=(-DRINGOUT_NX_MODULE=)
fi

echo "==> 2/3  Building the runtime (Dolphin core; long the first time)"
cmake -S "$REPO/ModernGekko" -B "$BUILD/runtime" -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN" \
    -DCMAKE_BUILD_TYPE=Release \
    "${RELEASE_FLAGS[@]}" \
    "${MODULE_ARGS[@]}"
cmake --build "$BUILD/runtime" --target ringout_nx -j "$JOBS"
ELF="$(find "$BUILD/runtime" -name 'RingOut.elf' | head -1)"
[ -n "$ELF" ] || die "RingOut.elf was not produced"

echo "==> 3/3  Packing the .nro"
OUT="$BUILD/sd/switch/RingOut"
ROMFS="$BUILD/romfs"
rm -rf "$ROMFS" && mkdir -p "$ROMFS" "$OUT"
cp -r "$REPO/ModernGekko/vendor/dolphin/Data/Sys" "$ROMFS/Sys"
"$NACPTOOL" --create "Ring Out" "Toniisound" "$VERSION" "$BUILD/RingOut.nacp"
ICON_ARGS=()
[ -f "$HERE/icon.jpg" ] && ICON_ARGS=(--icon="$HERE/icon.jpg")
"$ELF2NRO" "$ELF" "$OUT/RingOut.nro" --nacp="$BUILD/RingOut.nacp" --romfsdir="$ROMFS" "${ICON_ARGS[@]}"

echo
echo "Done: $OUT/RingOut.nro"
if [ "$NO_MODULE" = 0 ]; then
    echo "Copy $OUT to sdmc:/switch/RingOut and $PKG/game to sdmc:/switch/RingOut/game."
fi
echo "Launch it through a game's title override (hold R), not from the Album."
