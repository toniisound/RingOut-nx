// Copyright 2026 RingOut contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "moderngekko/module_abi.h"

namespace ringout_nx
{
// The module linked into this .nro, or nullptr for a build without one
// (interpreter only -- useful for bringing up the runtime, far too slow to play).
const ModernGekkoModuleDesc* GetLinkedModule();

// Answers Common::DynamicLibrary::GetSymbolAddress() for the module's exports.
void* ResolveModuleSymbol(const char* name);
}  // namespace ringout_nx
