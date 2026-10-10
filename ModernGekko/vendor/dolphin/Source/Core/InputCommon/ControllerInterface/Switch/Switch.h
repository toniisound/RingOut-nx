// Copyright 2026 RingOut contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>

#include "InputCommon/ControllerInterface/InputBackend.h"

namespace ciface::Switch
{
// "Switch" is the device source: devices are Switch/0/Player 1 .. Player 4.
std::unique_ptr<ciface::InputBackend> CreateInputBackend(ControllerInterface* controller_interface);
}  // namespace ciface::Switch
