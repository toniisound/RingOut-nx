// Copyright 2026 RingOut contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Common/Switch/SwitchNet.h"

#include <switch.h>

namespace Common::SwitchNet
{
bool GetCurrentIpConfig(IpConfig* config)
{
  if (R_FAILED(nifmInitialize(NifmServiceType_User)))
    return false;
  IpConfig result;
  const Result rc =
      nifmGetCurrentIpConfigInfo(&result.address, &result.netmask, &result.gateway,
                                 &result.primary_dns, &result.secondary_dns);
  nifmExit();
  if (R_FAILED(rc) || result.address == 0)
    return false;
  *config = result;
  return true;
}
}  // namespace Common::SwitchNet
