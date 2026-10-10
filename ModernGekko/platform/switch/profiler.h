// Copyright 2026 RingOut contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <string>

namespace ringout_nx
{
// Sampling profiler for the CPU thread (nx.cfg profile=1). A thread on core 0
// pauses the CPU thread `hz` times a second, reads its PC and LR, and lets it
// go again. Counts, as offsets into the executable, go to `out_path` every few
// seconds; switch/profile-report.py turns them into function names.
// Clicking both sticks (L3+R3) clears the counts, to profile one scene only.
void StartProfiler(const std::string& out_path, unsigned hz);
void StopProfiler();
}  // namespace ringout_nx
