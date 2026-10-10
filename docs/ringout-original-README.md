> This is the README of the original **[Ring Out](https://github.com/jackpoison-prog/RingOut)** project by Jack Poison, kept here for reference. For the Nintendo Switch port, see the [main README](../README.md).

# Ring Out

A native PC port of a GameCube fighting game (disc ID `GRSEAF`), produced by **static
recompilation** rather than emulation: the disc's PowerPC executable is translated
ahead of time into C, compiled for x86-64, and run as native code inside a
Dolphin-derived runtime that provides the graphics, audio, input and hardware
emulation around it.

**No game data and no game code are distributed here.** You supply a disc image you
already own; setup extracts it and recompiles it on your machine.

> **This is still early.** Expect rough edges: the game is playable start to
> finish, but features arrive before their polish does, and something that worked
> last release can break in the next one. Polish is planned throughout rather
> than saved up for a 1.0 — and bug reports genuinely help, because most of what
> gets fixed here was found by someone hitting it.

---

## Status

Fully playable — boots to menu and through gameplay. **Every block of the game's
own executable runs as native code**; the recompiler translates 535,368
instructions with 0 unknown opcodes and no interpreter fallback inside them.

What the interpreter does still cover is the PowerPC exception vectors —
`0x500` (external interrupt), `0xC00` (system call), `0x800` (FP unavailable).
Those handlers are written into low RAM by the OS at boot rather than living in
the disc's executable, so there is no recompiled code to dispatch to and the
interpreter runs them until control returns. They account for about **0.7% of
executed steps**, steadily, because interrupts fire for the life of the session
— but only **0.2% of cycles**, so there is nothing to win by recompiling them.
An earlier version of this page claimed "zero interpreter fallback", which the
runtime's own shutdown line has always contradicted.

Rendering is Vulkan on the GPU, with the CPU emulation and the runtime on
separate cores.

**Steam Deck**: supported, with its own package, and it runs in both Desktop and
Game Mode at 45–49 fps in a match. Two ways to get a module onto it, so it works
whether or not the Deck is the only machine you have:

- **Build it on the Deck.** Install a toolchain once and SteamOS compiles its own
  module, needing no bundled libraries at all — no second computer anywhere in
  the process. Tested end to end on SteamOS 3.8.25. This is also the way to get
  the full PGO win there, since clang 20 cannot read the shipped profiles.
- **Build it on a desktop and copy it over.** No toolchain on the Deck at all, if
  you have another Linux machine to hand.

[`dist/RingOut-1.0-deck/BUILD-ON-THE-DECK.md`](../dist/RingOut-1.0-deck/BUILD-ON-THE-DECK.md)
walks through the first route, and ships inside the Deck package too.

**Netplay**: working. Delay-based (not rollback) over a deterministic dual-core
setup, with a lobby showing live ping and per-player game status; two peers
stayed byte-identical over 6,470 frames.

---

## Getting it

From the [Releases](../../releases) page. Each is named for the release you
download, so `<version>` below is that release's number:

| | for | needs a toolchain? |
| --- | --- | --- |
| `RingOut-<version>-linux-x86_64.zip` | desktop Linux | yes — compiles on your machine |
| `RingOut-<version>-steamdeck-x86_64.zip` | Steam Deck / SteamOS | only to build a module |
| `RingOut-<version>-windows-x64-setup.exe` / `-windows-x64.zip` | Windows (test build) | no — bundled |

The Deck package ships no module, so you build one and copy `game/` and
`bin/gGRSEAF_recomp.so` into it. You can do that **on the Deck itself** — download
both zips, since the Linux one is what carries `module-src/` and does the
building — or on a desktop if you have one. `setup.sh --deck` installs the result
into the Deck package for you when it can see it. Add `RingOut` to Steam as a
non-Steam game to launch it from Game Mode.

For desktop, unzip and run:

```sh
./RingOut
```

On first run it asks for your disc image (`.iso` / `.gcm` / `.nkit.iso` / `.rvz`),
then extracts and recompiles it — several minutes, once, with each stage
numbered as it starts (`==> 1/3  Extracting disc`, `2/3` recompiling, `3/3`
building the module; `--pgo` adds a fourth) and a closing line saying whether a
PGO profile was used. Every run after that starts straight away. You can also
pass the image directly:

```sh
./RingOut /path/to/disc.iso     # or: ./setup.sh /path/to/disc.iso
```

### Requirements

- A GameCube disc image you already own
- `cmake`, `ninja`, `python3`, and `clang` (or `gcc`)
  - Arch: `sudo pacman -S cmake ninja clang python`
  - Debian/Ubuntu: `sudo apt install cmake ninja-build clang python3`
  - Fedora: `sudo dnf install cmake ninja-build clang python3`
  - Bazzite, Silverblue and other image-based Fedora systems:
    `sudo rpm-ostree install cmake ninja-build clang python3`, then
    `sudo systemctl reboot` — layered packages only exist after a reboot

  `setup.sh` names the right command for whichever of these you are on, so if
  something is missing it tells you what to run rather than leaving you to work
  out which example applies.
- A working Vulkan driver
- ~1.5 GB free for the extracted disc and build output

The release binary is built in a Debian 12 container and needs **glibc 2.36**,
which every current distro clears — so the bundled glibc in `libc-fallback/` is
a fallback almost nobody takes. (This page previously said 2.44, which was the
glibc of the machine the runtime used to be built on. The launcher believed the
same number and sent every host between 2.36 and 2.44 down the bundled path for
no reason, where the Vulkan loader then finds no driver. Both are fixed, and
packaging now fails if the two disagree.)

---

## Features

- **Static recompilation** — native x86-64 execution; every block of the game's own
  executable is recompiled, with the interpreter left only the OS exception
  vectors (0.2% of cycles — see [Status](#status))
- **Vulkan renderer** with internal-resolution scaling, AA, anisotropic filtering
- **Widescreen (16:9)** — `Alt+W`
- **Any region** — US, Japanese and PAL discs all recompile. The OS idle loop is
  found in *your* disc rather than assumed, so a new region needs no new constant,
  and each ships its own cheat list and PGO profile
- **Community "Plus" disc mods run too** — a modded disc (ID `GRSEPS`) recompiles
  and plays with no special handling and no extra flags: its idle loop is detected
  like any other disc, and it is given the US PGO profile because it hooks the US
  executable in place rather than replacing it. Three of its code blocks rewrite
  themselves at run time, fail the chunk hash and run interpreted, which is the
  whole of its measured cost: 5-7% more CPU than the stock disc over the same
  arcade route — but only **1.7 points of on-screen speed on a Steam Deck**,
  because the display is not what the extra work competes with. It keeps a
  **separate save file** from the stock disc, so neither can see the other's
  saved data
- **Reads the disc formats the file picker offers** — `.iso`, `.gcm`, `.wbfs`,
  `.rvz`, `.gcz`, `.wia` and NKit variants
- **HD texture pack support** — drop a pack in `userdata/Load/Textures/GRS/`
  (`GRS` matches any region; a folder named for your exact disc replaces it
  rather than adding to it)
- **Mods** — drop the `.rar`/`.zip` you downloaded into `userdata/Load/Mods/` and
  it is unpacked and listed in the MODS tab under the archive's name. Texture
  mods toggle on and off and outrank the HD pack; character skins are written
  into the game itself, and are installed from the same tab when their data
  matches the slot it replaces
- **Knows when the game data has been modified** — a skin patches the disc's own
  archive, which no emulator can see, so the game hashes it against what came off
  your disc. Netplay is refused while it differs, with the reason on screen, and
  the original is restored from your own disc image on request
- **In-game overlay**: pause menu with staged settings and Reset Game; Video, Audio,
  System, Controls, Cheats and Mods tabs
- **Full controller remapping**
- **Match replays** — a **REPLAYS** tab in the pause menu: Start Recording, or
  pick a saved replay to play it back, with **Enter**. A blinking `REC` sits in
  the corner while a recording is running, and Stop Recording writes the file
  without interrupting the session. Recordings land in
  `userdata/Replays/`, named for when they were taken. `--record <file.dtm>` and
  `--replay <file.dtm>` do the same from the command line. It is a recording of the inputs, not a video: a
  few tens of kilobytes for a whole match, and it re-runs on the real emulator.
  This works here because the core is deterministic — a recorded session and its
  replay were compared frame by frame over 7,000 frames of a real fight and the
  guest's memory was identical on every one. Replays are portable: one recorded
  on a desktop plays back identically on a Steam Deck, with a differently built
  module. They do depend on **your save data**, though — the game's own saved
  settings are part of what a replay re-runs, so a replay from someone whose
  save differs may not reproduce their match
- **Save states** — `Shift+F1`–`F8` to save, `F1`–`F8` to load
- **Free camera** — fly the camera anywhere in a match
- **FMV playback** via FFmpeg, replacing the software Sofdec decoder
- **Cheat codes for all three regions** in `GameSettings/`, none enabled until you
  say so. The US and Japanese lists were checked code by code for this project;
  the PAL one is the list Dolphin already shipped and nobody could see
- **Netplay** — a direct connection (LAN, VPN or a forwarded port) with a lobby,
  live ping and per-player game status. It is **delay-based, not rollback**: both
  peers run the same inputs on the same frame, which is the model the determinism
  work validated. The host sets the input buffer — 5 frames by default, about
  83 ms at 60 fps, adjustable from 1 to 20 in the lobby. A peer holding a
  different disc, a differently built module or modified game data is refused at
  connect and told which, rather than timing out with no explanation
- **Its own icon** — the disc banner and the memory-card icon are extracted from
  your disc and saves on your machine, and a desktop entry is written for you.
  None of that artwork ships; it is the publisher's.

### Controls

| Key | Action |
| --- | --- |
| `Escape` | settings menu |
| Arrow keys | navigate; Left/Right change a value or switch tab |
| `Space` | confirm / activate |
| `Alt+W` | toggle widescreen |
| `Alt+Enter` | fullscreen |
| `F1`–`F8` / `Shift+F1`–`F8` | load / save state |
| `Shift+Escape` | quit |

Free camera (enable in the Video tab): `Shift+WASD` move, `Shift+Q/E` down/up,
`Shift+arrows` look, `Shift+Z/C` roll, `Shift+1/2` speed, `Shift+R` reset.

---

## Performance work

Everything below is measured on a **fixed-work gameplay benchmark** — the same
emulated frames, driven by a frame-keyed input script, counted in retired CPU
cycles rather than wall time. That harness exists because the earlier one did
not measure the right thing: it ran boot and an idle menu, which carry ~0.1% of
a real session's paired-single traffic, so a change could look free there and
cost 3% in a match. **Earlier figures in this README were taken that way and have
been removed rather than restated.** How the harness works, and the checks that
stop a run from measuring the wrong thing, are in
[docs/measuring.md](measuring.md).

### What shipped

Up to 1.6.1:

| Change | Effect |
| --- | --- |
| Inline the paired-single helper chain | psq cost per unit of work −43% |
| Compile loop back-edges as native `goto` | dispatches −66%, CPU −11.5% |
| Defer FPRF classification until FPSCR is observable | −2.14% cycles (Deck) |
| `-flto=auto` | module relink 22 min → 6 |
| Profile-guided optimisation | **−10.3% desktop, −14.1% Steam Deck** |
| Per-region profiles | −12.33% for a JP module vs the US profile |
| On-device profile training (`setup.sh --pgo`) | recovers the full win on any clang |

Since 1.6.1 (September). Every row is on all four discs (US, JP, PAL and the
Plus mod) unless it says otherwise, and every figure is CPU cycles over 16,000
frames of a real arcade match with **both arms' PGO profiles retrained** —
several of these looked dead or harmful until that was done, because a stale
profile is silently discarded:

| Change | Effect |
| --- | --- |
| Reduced entry switch (`--leader-cases`): a switch case only where control can arrive | −8.96% (US) |
| Inline the common paired-single load/store path (`MODULE_PSQ_FAST`) | −16.51% for the full US configuration vs 1.6.1 |
| Leaner inlined RAM access (`MODULE_MEM_FAST`): no journal, reservation or NULL tests | −6.32% on top (US), −9.97% instructions |
| The above together, per disc, against 1.6.1 | −20.0% JP, −20.7% Plus, −21.3% PAL |
| Direct calls (`--direct-calls`): a call into another chunk skips the dispatcher | −22.64% (US, vs 1.6.1); dispatches halved |
| Self calls (`--self-calls`): calls within a chunk likewise | −5.79% on top (US) |
| Direct + self calls on the other discs | −16.3% to −17.0% on top; **−34.0% to −34.8% per disc vs 1.6.1** |
| Check a psq pair's 8-byte span once | −0.66% to −1.05% |
| Burn the 24 MB RAM bound into the fast paths as a constant | −0.79% to −2.11% |
| Unpack the condition register, one byte per field | −2.57% to −3.20% |
| Cache `ctx->ram` in a chunk-entry local | −1.42% to −2.01% |
| `PSQ_SIMD`, `FMA_LAZY`, `DC_LOCAL` (both psq lanes in one SSE register; lazy FPRF in fma; downcount in a local) | −1.42% to −2.32% together |
| Twin chunks: a hot copy entered only where training saw it, with read-mostly registers in locals, and the ordinary chunk as a cold fallback | **−5.65% to −6.67% (Deck)** |
| Chunk overhang (`--chunk-overhang 512`): a chunk runs past its window to close a straddled hot loop — US, Plus, PAL | −1.84% (US, Deck); 206–270 M fewer dispatches per run |
| RAM-only store bases (`--ram-bases 1,2,13`): stores off r1/r2/r13 skip the RAM/MMIO test | −0.39% to −1.07% (Deck) |
| Twin hot lists re-recorded after the overhang changed which entries are hot — US, Plus, PAL | −1.50% to −2.47% (Deck) |

Direct and self calls change guest timing slightly, so frame hashes differ from
a build without them. They were gated instead on determinism over two routes
(arcade and a VS fight, each run twice), on a static proof that no PC the run
loop hooks is bypassed, and on a hands-on playtest of each disc. Replays now
carry a timing marker, so one recorded on a build with different timing warns
when played back instead of silently desyncing. The European disc passes its
own six movie-player hook addresses as `--dispatch-pc`; the Japanese disc
passes none, because the US hook addresses are not entry points in its
executable at all. The chunk overhang is left off the Japanese disc for the
same timing reason: its layout does not straddle the loop, and the shifted
event timing changed its path for no gain.

The FPRF one is the shape most of these take: the FP condition register is
written 3.07 billion times per run and read 15,885 times — mandatory by
architecture, dead in practice — so it is now computed only where something can
observe it, with frame hashes proving guest state is unchanged.

### What was measured and rejected

Kept here because the negative results cost as much to establish as the wins,
and each one looks plausible enough to be retried:

| Idea | Result |
| --- | --- |
| BOLT post-link layout | **8.1% slower** |
| `-O2` instead of `-O3` | **9.1% slower**, despite 12.5% less code |
| LLVM object backend | **4% slower**, even removing 43% of dispatches |
| Leaders-only entry labels | 81% of execution fell out to the interpreter |
| Block-local register allocation | tied; no cross-iteration residency to exploit |
| CR / XER[CA] elision | ceiling ~0.05% and ~0.2% respectively |
| Optical-flow frame generation | needs 120Hz+; halves speed with V-Sync on, +8.3ms latency |

PGO was itself in the rejected table for a long time — "does not build, SIGBUS
before the first frame". That was true when it was written and is not any more.
It is now the largest single win here, and the details, including the several
ways it silently does nothing, are in
[docs/profile-guided-optimisation.md](profile-guided-optimisation.md).

Frame generation is the odd one out: it works, and the shader does what it
says. It is rejected on physics rather than on codegen. Presenting a synthesised
midpoint costs a second present, which on a 60Hz panel means a second vblank and
therefore emulation at 30 FPS; turning V-Sync off restores 60 but tears. It also
adds ~8.3ms of input latency, because a midpoint cannot exist until both of its
endpoints do — a poor trade in a fighting game. It would need a 120Hz+ display
to have a spare refresh to live in, and every panel this project runs on is
60Hz. Kept on the `framegen` branch rather than deleted.

The pattern among the rest: BOLT, `-O2` and the LLVM backend all bought
instruction locality or fewer dispatches, and all three lost by retiring more
instructions at lower IPC.
PMU attribution explains why — front-end starvation is 2.4% of cycles and the
back end is saturated at IPC 1.92. The workload is not inefficient, just large:
86.4 million host instructions per emulated frame.


---

## What's next

Ordered by how much they would help, not by effort. Nothing here is promised.

**1. Make it easier to get running.** This is the biggest thing between the
project and anyone using it. Today setup wants `cmake`, `ninja`, `clang` and
`python3` on your machine and takes several minutes before you see the game.

The compile itself cannot go away: the module is your disc's own executable
translated to native code, so it is game data and cannot be distributed — that
constraint is the whole design, not an oversight. What *can* go is everything
around it:

- **Bundle the toolchain in the package**, so "install these four things first"
  stops being step one. This is probably the single biggest reduction in people
  who never get it running.
- **A Deck path that does not need a desktop.** Getting a module there still
  means owning another machine or installing a toolchain on the handheld.

**2. Mods, further than they go now.** Texture packs and texture mods work, and
character skins install from the menu when their data is exactly the size of the
entry they replace. Two gaps are known and neither is hidden by the UI:

- A skin whose data is a *different* size needs the game's archive rebuilt —
  every later entry shifts and the parent index has to be rewritten. Those are
  still a job for [olkviewer](https://gamebanana.com/tools/21443).
- Removing one skin means restoring all the game data, because the original
  bytes are not kept anywhere. Per-mod uninstall would need them saved first.

Beyond closing those:

- **A mod browser in the menu** — browse and install without leaving the game.
  The download half is already proven; it is how the mods used to test this
  feature were fetched in the first place.
- **A texture-dump helper in the Video tab.** The runtime can already dump the
  game's textures; exposing it would let people *make* packs rather than only
  install them, which is what a mod scene actually needs.

**3. Whether any performance is left.** September's work took more than a
third off CPU cycles on every disc, so the answer so far has been yes. The
stage-by-stage figures predate all of it, though; if the slow stages now hold
full speed, the remaining ideas matter less — which is worth knowing either
way.

**4. Windows as a first-class target.** Every release since 1.6 ships a Windows
installer and zip, built in CI, with the toolchain bundled; `setup.ps1` carries
the same per-disc flags as `setup.sh` with a gating check over its shipped
lines, and the recompiler's Windows binary matches the Linux one's output on the
US disc. What keeps it a test build is coverage: it has been played on one
low-end laptop, against far more hands-on time on Linux and the Deck.

**5. Staying current with upstream.** The recompiler and the Dolphin-derived
runtime both have upstreams that keep moving. Most of the delta is work this
fork has measured and rejected, but not all of it.

---

## Known issues

- **The Deck package is SteamOS-only.** Its runtime is built against glibc 2.36 in
  a Debian 12 container, which clears SteamOS and essentially every current
  distro. A build made natively on SteamOS instead has a 2.38 floor and will not
  run on older SteamOS releases — so the container build stays the shipped one.
- **Windows is a test build.** Linux and the Steam Deck are the primary
  targets. The Windows installer and zip ship with every release since 1.6, but
  have been tested on one low-end laptop only. On it, the
  mid-September module (direct and self calls, `MEM_FAST`) was **31.57% faster
  in wall time** there than the one before that work, over a fixed 6,000-frame
  route, with a profile trained under one clang accepted by another — see
  [docs/measuring.md](docs/measuring.md#wall-time-on-a-machine-that-cannot-hit-the-frame-cap).
- The `-march=native` build is machine-specific by design; setup compiles on your
  own machine. It matters in exactly one place: the Deck package ships no module
  and sends you here to build one, and a module built on a Zen 4/5 or recent
  Intel desktop carries instructions the Deck's Zen 2 cannot execute — an instant
  crash, not a load error. **Use `./setup.sh --deck <disc>` for a module you
  intend to copy to a Deck**; it builds `-march=x86-64-v3` and checks the glibc
  floor, the other way a copied module fails.

### Fixed since the last revision of this page

- **Netplay was described here as stubbed out.** It is not: there is a lobby with
  live ping and per-player game status, automatic pad assignment, and a
  host-controlled input buffer.
- **The shutdown abort** (`terminate called without an active exception`) was a
  real bug rather than a cosmetic one, and is fixed.
- **The bundled-glibc fallback on SteamOS** is no longer the plan. The predicted
  NSS failure is moot: the Deck package is built in a container against an old
  glibc instead, and is verified on hardware in both Desktop and Game Mode.

---

## Building from source

```sh
cmake -S ModernGekko -B ModernGekko/build -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja -C ModernGekko/build
```

Then build the recompiled module for your disc with `dist/RingOut-1.0-dist/setup.sh`,
which drives `dolrecomp` and compiles the generated C.

Releases are assembled by script, never by zipping a working directory — those
hold the extracted disc, saves and build output. Each stage is built from an
allowlist and then checked: no disc-derived files, no personal data, and for the
desktop package, that the prebuilt runtime can actually load a module built from
the sources shipped beside it.

```sh
.github/scripts/package-deck.sh     # Steam Deck zip
.github/scripts/package-dist.sh     # desktop Linux zip
.github/scripts/regen-source.sh     # refresh the GPL source shipment
```

### Repository layout

| Path | What it is |
| --- | --- |
| `ModernGekko/` | the runtime fork — chassis, settings overlay, StaticRecomp core |
| `ModernGekko/vendor/dolphin/` | the vendored Dolphin tree the runtime is built from |
| `DolRecomp/` | the static recompiler fork (PowerPC → C) |
| `dist/RingOut-1.0-dist/` | the desktop redistributable: launcher, `setup.sh`, module build recipe |
| `dist/RingOut-1.0-deck/` | the Steam Deck package scaffolding |
| `dist/shared/gc-art.py` | extracts the game's banner and icon on the player's machine |
| `.github/scripts/` | packaging, the privacy scan, the GPL source shipment, benchmarks |
| `.github/deck-notes/` | the toolchain notes kept on the test Deck's desktop |
| `docs/measuring.md` | how performance is measured, and the checks that keep it honest |
| `docs/profile-guided-optimisation.md` | PGO: what it is worth, and how it silently does nothing |
| `work/mg_userdir/GameSettings/` | the cheat lists this project authored (US, JP) |

Everything is vendored as plain files — clone and build, no submodule init needed.

---

## Credits

Almost none of the heavy lifting here is original. This is a thin layer on other
people's work:

- **[Dolphin Emulator Project](https://dolphin-emu.org)** — the runtime *is* Dolphin.
  Graphics, audio, input, save states, GameCube hardware emulation and the Vulkan
  backend are all theirs. GPL-2.0-or-later.
- **[ExpansionPak — ModernGekko](https://github.com/ExpansionPak/ModernGekko)** — the
  chassis that hosts a statically recompiled game inside Dolphin, and the
  StaticRecomp core that hands execution between recompiled code and the emulator.
- **[ExpansionPak — DolRecomp](https://github.com/ExpansionPak/DolRecomp)** — the
  static recompiler. GPL-3.0-or-later.
- **Dear ImGui** (Omar Cornut and contributors) — the settings overlay UI. MIT.
- **FFmpeg** — used as a separate program to decode the game's Sofdec video.
- **Bandai Namco Entertainment** — the original game, its code and all its assets.
  Not included, not redistributed, and not ours. All rights remain theirs.
- Action Replay cheat codes are published community data (Codejunkies, via Almar's
  Guides). None were written here.
- The r/decomps post that described the recompilation workflow this project followed.

Assembled by **Jack Poison**, working with AI assistance (Anthropic's Claude). The AI
wrote and debugged a large share of the code — the settings overlay, controller
remapping and cheat pages, the free-camera wiring, the FMV replacement path, several
recompiler codegen fixes and the packaging — under direction, with testing and
decisions made by a human.

---

## Licence

**GPL-2.0-or-later.** See [`LICENSE`](../LICENSE).

The runtime derives from Dolphin (GPL-2.0-or-later) and ModernGekko, whose sources
are tagged `GPL-2.0-or-later`; DolRecomp is GPL-3.0-or-later and is used as a
separate build-time tool. Because the GPL obliges anyone receiving a binary to be
able to get the matching source, the full Dolphin and recompiler trees are vendored
in this repository rather than referenced.

`ModernGekko/LICENSE` and `ModernGekko/vendor/dolphin/COPYING` are upstream's own
terms and are left untouched.

---

## Disclaimer

An unofficial fan project. Not affiliated with, endorsed by, or connected to any of
the projects or companies named above. No game data or game code is included, and
none will be provided — do not ask. Anything setup builds from your disc is derived
from the game itself; keep it to yourself.
