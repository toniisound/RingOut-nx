# Credits

## Ring Out, by Jack Poison

This port is built entirely on **[Ring Out](https://github.com/jackpoison-prog/RingOut)**
by **[Jack Poison](https://github.com/jackpoison-prog)**: the static
recompilation of *Soul Calibur II*, the runtime, the per-disc profiles and all
the work that makes the game run as native code. The Switch port only adapts
that work to another platform. All credit for the original project is his.

Get Ring Out, and the Linux release this port needs, from his GitHub:
**https://github.com/jackpoison-prog/RingOut/releases**

## Nintendo Switch port

**Toniisound**: Nintendo Switch (libnx) port, hardware testing and project
direction. Made with AI assistance (Claude, by Anthropic), which wrote much of
the port's code under his direction.

## Projects it relies on

- **[Dolphin Emulator Project](https://dolphin-emu.org)**: the runtime is
  Dolphin (graphics, audio, input and GameCube hardware emulation).
  GPL-2.0-or-later.
- **[ExpansionPak · ModernGekko](https://github.com/ExpansionPak/ModernGekko)**:
  the chassis that hosts a recompiled game inside Dolphin, and the StaticRecomp
  CPU core.
- **[ExpansionPak · DolRecomp](https://github.com/ExpansionPak/DolRecomp)**:
  the static recompiler that translates GameCube PowerPC code into C.
  GPL-3.0-or-later.
- **[devkitPro](https://devkitpro.org)**: devkitA64, libnx and the Switch
  homebrew tools.
- **[Mesa](https://www.mesa3d.org)** (switch-mesa, nouveau): OpenGL on the Switch.
- **[Dear ImGui](https://github.com/ocornut/imgui)** by Omar Cornut and
  contributors: the pause menu UI. MIT.
- **[Atmosphère](https://github.com/Atmosphere-NX/Atmosphere)**, for making
  Switch homebrew possible.

## The game

*Soul Calibur II*, its code, graphics, music and logo belong to **Bandai Namco
Entertainment**. None of it is included or redistributed here, and all rights
remain theirs. The app icon uses the game's logo only to identify it.

*Nintendo Switch* and *GameCube* are trademarks of **Nintendo**.

## Notice

Unofficial, non-commercial fan project. Not affiliated with, endorsed by or
connected to Bandai Namco, Nintendo, the Dolphin project, ExpansionPak or Jack
Poison. Please report bugs in the Switch port here, not to the original project.
