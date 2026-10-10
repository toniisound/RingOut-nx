// Copyright 2026 RingOut contributors
// SPDX-License-Identifier: GPL-2.0-or-later

// GCAdapter for targets with no USB host access (the Nintendo Switch port).
// The official GameCube adapter is never detected; every query answers the
// way the libusb implementation does with no adapter plugged in.

#include "InputCommon/GCAdapter.h"

#include "InputCommon/GCPadStatus.h"

namespace GCAdapter
{
void Init()
{
}

void ResetRumble()
{
}

void Shutdown()
{
}

void SetAdapterCallback(std::function<void(void)>)
{
}

GCPadStatus Input(int)
{
  return {};
}

void Output(int, u8)
{
}

bool IsDetected(const char** error_message)
{
  if (error_message)
    *error_message = "The GameCube adapter is not supported on this platform.";
  return false;
}

bool DeviceConnected(int)
{
  return false;
}

void ResetDeviceType(int)
{
}

double GetCurrentPollRate()
{
  return 0.0;
}
}  // namespace GCAdapter
