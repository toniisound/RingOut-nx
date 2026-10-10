// Copyright 2026 RingOut contributors
// SPDX-License-Identifier: GPL-3.0-or-later

// The game module, linked statically into the .nro.
//
// On desktop the runtime dlopen()s g<ID>_recomp.so and looks up a few symbols
// by name. Horizon has no dynamic loader, so the module archive is linked in
// whole (see CMakeLists.txt) and this file answers those lookups instead:
// Common::DynamicLibrary::GetSymbolAddress() calls ResolveModuleSymbol().
//
// The references are strong, not weak, on purpose. A weak undefined reference
// does not make the linker pull a member out of an archive, so with weak
// declarations the whole module would silently drop out of the link and the
// game would run on the interpreter alone.

#include "static_module.h"

#include <cstring>

#ifdef RINGOUT_NX_HAVE_MODULE
extern "C" {
// module-template/module_export.c
const void* staticrecomp_get_module(void);
extern const unsigned int staticrecomp_timing_id;
// DolRecomp cpu/cpu.c. Declared without their parameters: only the address is
// taken here, and the runtime casts it back to the real signature.
void ppc_set_gather_pipe(void);
void ppc_set_mem_write_journal(void);
}
#endif

namespace ringout_nx
{
const ModernGekkoModuleDesc* GetLinkedModule()
{
#ifdef RINGOUT_NX_HAVE_MODULE
  // Same struct layout as StaticRecompModuleDesc; dolphin_runtime.cpp
  // static_asserts that.
  return static_cast<const ModernGekkoModuleDesc*>(staticrecomp_get_module());
#else
  return nullptr;
#endif
}

void* ResolveModuleSymbol(const char* name)
{
#ifdef RINGOUT_NX_HAVE_MODULE
  struct Entry
  {
    const char* name;
    void* address;
  };
  static const Entry kSymbols[] = {
      {"staticrecomp_get_module", reinterpret_cast<void*>(&staticrecomp_get_module)},
      {"staticrecomp_timing_id",
       const_cast<void*>(static_cast<const void*>(&staticrecomp_timing_id))},
      {"ppc_set_gather_pipe", reinterpret_cast<void*>(&ppc_set_gather_pipe)},
      {"ppc_set_mem_write_journal", reinterpret_cast<void*>(&ppc_set_mem_write_journal)},
  };
  for (const Entry& entry : kSymbols)
  {
    if (std::strcmp(entry.name, name) == 0)
      return entry.address;
  }
#else
  (void)name;
#endif
  return nullptr;
}
}  // namespace ringout_nx
