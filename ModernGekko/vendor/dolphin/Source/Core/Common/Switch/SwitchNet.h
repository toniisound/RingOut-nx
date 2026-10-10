// Copyright 2026 RingOut contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Common/CommonTypes.h"

// Network configuration on Horizon, which has neither getifaddrs() nor
// res_init(). Kept out of line so callers do not include <switch.h>, whose
// global names (Result, Event, Mutex...) collide with Dolphin's.
namespace Common::SwitchNet
{
struct IpConfig
{
  // All in network byte order, as in in_addr::s_addr.
  u32 address = 0;
  u32 netmask = 0;
  u32 gateway = 0;
  u32 primary_dns = 0;
  u32 secondary_dns = 0;
};

// The active connection's configuration, or false when not connected.
bool GetCurrentIpConfig(IpConfig* config);
}  // namespace Common::SwitchNet
