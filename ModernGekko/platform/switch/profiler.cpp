// Copyright 2026 RingOut contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "profiler.h"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <atomic>
#include <cstdio>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include <switch.h>

#include "Common/Thread.h"

// Reference point for addresses. switch.ld's __start__ is an ABSOLUTE symbol
// (0), so taking its address in a PIE gives 0, not the load address -- the
// first version of this file did that and every offset came out wrong.
// Instead the file records samples relative to this function, and the report
// adds its address as nm prints it.
extern "C" __attribute__((noinline, used)) void ringout_nx_profile_anchor()
{
  __asm__ volatile("");
}

namespace ringout_nx
{
namespace
{
std::atomic<bool> s_running{false};
std::thread s_thread;

struct Profile
{
  // (pc - anchor, lr - anchor), as a pair so the report can say who called a
  // hot leaf function.
  struct PairHash
  {
    size_t operator()(const std::pair<s64, s64>& k) const
    {
      return std::hash<s64>()(k.first) * 31u ^ std::hash<s64>()(k.second);
    }
  };
  std::unordered_map<std::pair<s64, s64>, u32, PairHash> hits;
  u64 samples = 0;
  u64 failures = 0;
  u64 resets = 0;
};

template <typename Map>
void WriteHits(std::FILE* f, const Map& counts)
{
  std::vector<std::pair<typename Map::key_type, u32>> rows(counts.begin(), counts.end());
  std::sort(rows.begin(), rows.end(),
            [](const auto& a, const auto& b) { return a.second > b.second; });
  if (rows.size() > 50000)
    rows.resize(50000);
  for (const auto& [key, count] : rows)
  {
    std::fprintf(f, "s %lld %lld %u\n", static_cast<long long>(key.first),
                 static_cast<long long>(key.second), count);
  }
}

void Write(const std::string& path, const Profile& p, unsigned hz)
{
  const std::string tmp = path + ".tmp";
  std::FILE* f = std::fopen(tmp.c_str(), "w");
  if (!f)
    return;
  std::fprintf(f, "# ringout-nx profile v2 (offsets from ringout_nx_profile_anchor)\n");
  std::fprintf(f, "# hz %u samples %llu failures %llu resets %llu\n", hz,
               static_cast<unsigned long long>(p.samples),
               static_cast<unsigned long long>(p.failures),
               static_cast<unsigned long long>(p.resets));
  WriteHits(f, p.hits);
  std::fclose(f);
  // Horizon's rename does not replace an existing file.
  std::remove(path.c_str());
  std::rename(tmp.c_str(), path.c_str());
}

void Run(std::string path, unsigned hz)
{
  // Above the other core-0 threads (default 0x2C), so samples stay evenly
  // spaced while the host loop and shader workers are busy.
  svcSetThreadPriority(CUR_THREAD_HANDLE, 0x2B);

  PadState pad;
  padInitialize(&pad, HidNpadIdType_No1, HidNpadIdType_Handheld);

  const s64 anchor = static_cast<s64>(reinterpret_cast<uintptr_t>(&ringout_nx_profile_anchor));
  const s64 period_ns = 1000000000LL / hz;
  const u64 write_every = static_cast<u64>(hz) * 5;
  Profile p;
  u32 handle = 0;
  u64 ticks = 0;
  bool reported_handle = false;

  while (s_running.load(std::memory_order_relaxed))
  {
    svcSleepThread(period_ns);
    ++ticks;

    if (!handle)
    {
      handle = Common::SwitchFindThreadHandle("CPU thread");
      if (!handle)
        continue;
      if (!reported_handle)
      {
        std::fprintf(stderr, "[prof] sampling the CPU thread at %u Hz -> %s\n", hz, path.c_str());
        reported_handle = true;
      }
    }

    padUpdate(&pad);
    const u64 held = padGetButtons(&pad);
    const u64 down = padGetButtonsDown(&pad);
    constexpr u64 kChord = HidNpadButton_StickL | HidNpadButton_StickR;
    if ((held & kChord) == kChord && (down & kChord) != 0)
    {
      p.hits.clear();
      p.samples = 0;
      p.failures = 0;
      ++p.resets;
      std::fprintf(stderr, "[prof] counts cleared (L3+R3)\n");
    }

    if (R_FAILED(svcSetThreadActivity(handle, ThreadActivity_Paused)))
    {
      ++p.failures;
      continue;
    }
    ThreadContext ctx{};
    const Result rc = svcGetThreadContext3(&ctx, handle);
    svcSetThreadActivity(handle, ThreadActivity_Runnable);
    if (R_FAILED(rc))
    {
      ++p.failures;
      continue;
    }
    ++p.samples;
    ++p.hits[{static_cast<s64>(ctx.pc.x) - anchor, static_cast<s64>(ctx.lr) - anchor}];

    if (ticks % write_every == 0)
      Write(path, p, hz);
  }
  Write(path, p, hz);
  std::fprintf(stderr, "[prof] %llu samples, %llu failed, written to %s\n",
               static_cast<unsigned long long>(p.samples),
               static_cast<unsigned long long>(p.failures), path.c_str());
}
}  // namespace

void StartProfiler(const std::string& out_path, unsigned hz)
{
  if (s_running.exchange(true))
    return;
  hz = std::clamp(hz, 50u, 4000u);
  s_thread = std::thread(Run, out_path, hz);
}

void StopProfiler()
{
  if (!s_running.exchange(false))
    return;
  if (s_thread.joinable())
    s_thread.join();
}
}  // namespace ringout_nx
