// Copyright 2026 RingOut contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstdint>
#include <cstring>

// The six guest PCs the run loop hooks for the FMV HLE (the CRI Sofdec movie
// library), per disc. They used to be US literals compared on every disc, so on
// JP and PAL they could never fire -- the US addresses are not even entry points
// there. The module's game_id picks the row.
//
// An unset hook is kStaticRecompNoFmvHook, which is not 4-byte aligned and so
// can never equal a guest PC -- no argument about which addresses a module
// covers is needed. A disc with no row gets all unset, which is exactly how JP
// and PAL behaved before this table existed.
inline constexpr std::uint32_t kStaticRecompNoFmvHook = 0xFFFFFFFFu;

struct StaticRecompFmvHookPcs
{
  std::uint32_t start_afs = kStaticRecompNoFmvHook;       // mwPlyStartAfs(r3=handle,r4=patid,r5=fno)
  std::uint32_t cnv_frm = kStaticRecompNoFmvHook;         // mwPlyFxCnvFrmARGB(r3=hnd,r4=desc,r5=dst)
  std::uint32_t exec_svr = kStaticRecompNoFmvHook;        // mwPlyExecSvrHndl
  std::uint32_t next_frm_ready = kStaticRecompNoFmvHook;  // mwPlyIsNextFrmReady
  std::uint32_t rel_cur_frm = kStaticRecompNoFmvHook;     // mwPlyRelCurFrm
  std::uint32_t getfrm = kStaticRecompNoFmvHook;          // getfrm
};

struct StaticRecompFmvHookRow
{
  const char* game_id;
  StaticRecompFmvHookPcs pcs;
  // Whether the native movie player has been playtested on this disc. A row
  // that has not been is only used when the player opts in (see below): the
  // hooks write guest RAM, and nothing but a playtest shows the disc's movie
  // numbering, frame sizes and rate agree with what the player assumes.
  bool playtested;
};

// Mapped by .github/scripts/map-fmv-hooks.py. Keep in step with the
// --dispatch-pc lists in setup.sh / setup.ps1 (the fmv_hooks test checks both): a
// hooked PC must stay a dispatch point or --direct-calls jumps past the hook.
inline constexpr StaticRecompFmvHookRow kStaticRecompFmvHookTable[] = {
    // US.
    {"GRSEAF", {0x8020C1E8u, 0x80209138u, 0x8020D3B8u, 0x80207E90u, 0x80207EE8u, 0x80208244u},
     true},
    // Plus is a hack of the US text: its movie library is byte-identical at all
    // six addresses (delta 0, 23-24/24 votes), and it was playtested.
    {"GRSEPS", {0x8020C1E8u, 0x80209138u, 0x8020D3B8u, 0x80207E90u, 0x80207EE8u, 0x80208244u},
     true},
    // PAL: the whole library sits +0x7750 from the US one (8-20/24 votes, one
    // shared delta across all six). Playtested 2026-09-27 once the player took
    // the game's frame number: this 50 Hz disc displays 20 frames a second and
    // skips the rest to hold audio sync.
    {"GRSPAF", {0x80213938u, 0x80210888u, 0x80214B08u, 0x8020F5E0u, 0x8020F638u, 0x8020F994u},
     true},
    // JP (GRSJAF): not mapped. Its library is rearranged rather than relocated,
    // so it has no row and its movies stay with the game's own decoder.
};

// The row for game_id, or all unset. allow_unplaytested admits rows not yet
// playtested (STATICRECOMP_FMV_UNPLAYTESTED=1 in the runtime).
inline StaticRecompFmvHookPcs StaticRecompFindFmvHooks(const char* game_id,
                                                       bool allow_unplaytested)
{
#ifdef __SWITCH__
  // The native movie player decodes through an ffmpeg subprocess, and Horizon
  // cannot spawn one. With no hooks the game plays its movies with its own
  // Sofdec decoder, as on real hardware -- slower, but correct, instead of the
  // black picture a failed takeover leaves behind.
  (void)game_id;
  (void)allow_unplaytested;
  return {};
#endif
  if (game_id == nullptr)
    return {};
  for (const auto& row : kStaticRecompFmvHookTable)
  {
    if (std::strcmp(row.game_id, game_id) != 0)
      continue;
    if (!row.playtested && !allow_unplaytested)
      return {};
    return row.pcs;
  }
  return {};
}
