# Ring Out NX — Soul Calibur II for Nintendo Switch

![Platform](https://img.shields.io/badge/platform-Nintendo_Switch-e60012?style=for-the-badge&logo=nintendoswitch&logoColor=white)
![Status](https://img.shields.io/badge/status-experimental-orange?style=for-the-badge)
![License](https://img.shields.io/badge/license-GPL-blue?style=for-the-badge)

An unofficial **Nintendo Switch homebrew port** (`.nro`) of
**[Ring Out](https://github.com/jackpoison-prog/RingOut)**, the static
recompilation of the GameCube version of *Soul Calibur II*.

The game is not emulated instruction by instruction: its PowerPC executable is
translated to C ahead of time and compiled as native ARM64 code, running inside
Ring Out's Dolphin-derived runtime, which provides the graphics, controllers and
the rest of the GameCube hardware.

> [!IMPORTANT]
> **Step 1 is on the original developer's GitHub.** You need the **Linux release
> of Ring Out 1.6.7** by Jack Poison to prepare the game from your own disc:
>
> [![Download Ring Out 1.6.7 (Linux) from the original GitHub](https://img.shields.io/badge/STEP_1-Download_Ring_Out_1.6.7_%28Linux%29-2ea44f?style=for-the-badge&logo=github)](https://github.com/jackpoison-prog/RingOut/releases/tag/v1.6.7)

## Credits first

**All credit for Ring Out goes to its developer, [Jack Poison](https://github.com/jackpoison-prog).**
The Soul Calibur II recompilation, the runtime, the per-disc profiles and
everything that makes the game run as native code are his work. This fork, by
**Toniisound**, only adapts it to the Nintendo Switch. If you enjoy it, please
support and star **[the original project](https://github.com/jackpoison-prog/RingOut)**.

Ring Out itself stands on [Dolphin](https://dolphin-emu.org),
[ModernGekko](https://github.com/ExpansionPak/ModernGekko) and
[DolRecomp](https://github.com/ExpansionPak/DolRecomp). Full credits:
**[`switch/CREDITS.md`](switch/CREDITS.md)**.

## No game included

> [!WARNING]
> This repository contains **no ROMs, ISOs, game data or game code**, and no
> compiled `.nro` is distributed. The `.nro` you build contains the game's code,
> translated from **your own disc**: keep it, and the `game/` folder, to yourself.

This is an unofficial fan project. It is not affiliated with, endorsed by or
connected to Jack Poison, Bandai Namco, Nintendo or the Dolphin project.
*Soul Calibur* and its logo belong to Bandai Namco Entertainment; *Nintendo
Switch* and *GameCube* are trademarks of Nintendo.

## Status

**Experimental.** It boots, renders, takes controller input and is stable, but
fights do not reach full speed yet.

| | Nintendo Switch, CPU at 1785 MHz |
| --- | --- |
| Menus | full speed, 50 FPS (PAL) |
| Fights | ~53% speed, ~26 FPS (slow motion) |
| Audio | not yet |
| Save data | untested |

Measured with Ring Out 1.6.6. In fights the CPU is the limit: about 80% of its
time is the game's own code. Without an overclock it is noticeably slower.

Tested with the PAL disc (`GRSPAF`). The other discs Ring Out supports
(`GRSEAF`, `GRSJAF`, `GRSEPS`) should work the same way, but are untested.

## Requirements

- A Nintendo Switch running custom firmware (Atmosphère) able to launch homebrew.
- Recommended: [sys-clk](https://github.com/retronx-team/sys-clk) with the CPU at 1785 MHz.
- A PC with Linux, or Windows with WSL (Ubuntu).
- Your own disc image of *Soul Calibur II* for GameCube.

## Building

Everything below runs on Linux or in WSL.

### 1. Install devkitPro

Follow the [devkitPro instructions](https://devkitpro.org/wiki/Getting_Started),
then install the Switch packages and the host tools:

```sh
sudo dkp-pacman -S switch-dev switch-cmake switch-pkg-config switch-mesa switch-libdrm_nouveau
sudo apt install cmake ninja-build python3 unzip git
```

### 2. Prepare the game with Ring Out (original GitHub)

Download the **Linux** zip of **Ring Out 1.6.7** from
**[Jack Poison's releases](https://github.com/jackpoison-prog/RingOut/releases/tag/v1.6.7)**.
It must be the same version as this fork (see [`VERSION`](VERSION)).
Unzip it and run its `setup.sh` on your disc image:

```sh
unzip RingOut-1.6.7-linux-x86_64.zip -d RingOut-1.6.7-linux
cd RingOut-1.6.7-linux
chmod +x setup.sh RingOut bin/* tools/*
./setup.sh /path/to/your/disc.iso
```

This extracts the disc into `game/` and translates the game's code to C in
`work/out/generated/`. It takes several minutes.

### 3. Get the Switch port

Either clone this repository:

```sh
git clone -b switch https://github.com/toniisound/RingOut-nx.git
cd RingOut-nx
```

or apply the single patch from
[this repository's releases](https://github.com/toniisound/RingOut-nx/releases)
on top of the original Ring Out:

```sh
git clone https://github.com/jackpoison-prog/RingOut.git
cd RingOut
git checkout v1.6.7
git am /path/to/ringout-switch-1.6.7.patch
```

### 4. Build the `.nro`

```sh
./switch/build-switch.sh /path/to/RingOut-1.6.7-linux
```

The first build takes a while, since it compiles Dolphin. The result is
`build-switch/sd/switch/RingOut/RingOut.nro`.

## Installing

1. Copy `build-switch/sd/switch/RingOut/` to the SD card as `sdmc:/switch/RingOut/`.
2. Copy the `game/` folder from the Ring Out Linux package to `sdmc:/switch/RingOut/game/`.
3. Open the Homebrew Menu **through a game** (hold R while launching any game),
   not from the Album: the Album gives homebrew too little memory.

## Controls

Pro Controller, a Joy-Con pair, or handheld mode. Player 1 is the first
controller (or handheld), player 2 the second. Buttons map by position, as on
the GameCube pad:

| GameCube | Switch |
| --- | --- |
| A / B / X / Y | B / A / Y / X (bottom / right / left / top) |
| Z | R |
| L / R | ZL / ZR |
| Start | + |
| Control stick / C-stick | left / right stick |
| D-pad | D-pad |

- Hold `-` for half a second to open Ring Out's pause menu (D-pad to move, B to
  select, A to go back, `-` to close). Remapping in its CONTROLS tab is kept.
- `+` and `-` together quit.

## Configuration

The log is written to `sdmc:/switch/RingOut/ringout.log`: check it first if
something goes wrong.

`sdmc:/switch/RingOut/nx.cfg` is optional, one `key=value` per line:

| Option | Effect |
| --- | --- |
| `show_fps=0` | hide the on-screen FPS counter |
| `perf_log=0` | stop the once-a-second `[perf]` lines (speed, FPS, per-thread load) |
| `resolution_scale=2` | internal resolution (default 1) |
| `sync_gpu=1` | keep the GPU thread in step with the CPU; slower, a fallback if `Unknown Opcode` errors appear |
| `pad_profile=reset` | rewrite the default controller mapping |
| `log=none` | turn Dolphin's log off (default `all`); `log_level=warning`, `error`, `info`, `debug` |
| `profile=1` | sampling profiler: writes where the CPU spends its time to `profile.txt`; L3+R3 clears the counts. Read it on the PC with `./switch/profile-report.py profile.txt` |
| `video=Null` | no graphics (debugging only) |
| `clear_test=1` | clear the screen to magenta instead of black (debugging only) |

## How it differs from the PC build

| PC | Switch | Why |
| --- | --- | --- |
| `g<ID>_recomp.so`, loaded with `dlopen` | `g<ID>_recomp.a`, linked into the `.nro` | Horizon has no dynamic loader |
| JIT fallback for non-recompiled code | interpreter only (`ENABLE_GENERIC`) | executable memory needs the JIT service |
| Vulkan | OpenGL through switch-mesa (EGL on the default window) | no Vulkan driver for homebrew |
| `mmap`-backed emulated memory | heap-backed (`MemArenaSwitch.cpp`) | no shared-memory views |
| threads float across cores | pinned: CPU thread on core 1, GPU thread on core 2 | Horizon never migrates threads |
| libcurl, libusb, hidapi | stubbed out | nothing the game needs |

Switch-specific code lives in `ModernGekko/platform/switch/`, `switch/`, and
`__SWITCH__` / `DOLPHIN_SWITCH` / `MODERNGEKKO_PLATFORM_SWITCH` blocks in the
Dolphin tree.

## Roadmap

- Audio.
- Fight speed: build the game module with clang and its PGO profile.
- Memory card saves.

## License

Like Ring Out, this is distributed under the GPL: the Dolphin-derived runtime is
GPL-2.0-or-later and DolRecomp is GPL-3.0-or-later. See [`LICENSE`](LICENSE).

The README of the original Ring Out project is kept in
[`docs/ringout-original-README.md`](docs/ringout-original-README.md).

## Donations

This emulator is completely free and open-source. Optional donations to support the development are always welcome:

[![Ko-fi](https://img.shields.io/badge/Ko--fi-Support-FF5E5B?style=for-the-badge&logo=ko-fi&logoColor=white)](https://ko-fi.com/toniisound)
[![PayPal](https://img.shields.io/badge/PayPal-Donate-00457C?style=for-the-badge&logo=paypal&logoColor=white)](https://paypal.me/toniisound)
