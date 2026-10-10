// Copyright 2026 RingOut contributors
// SPDX-License-Identifier: GPL-2.0-or-later

// MemoryUtil for Horizon (Nintendo Switch homebrew, libnx).
//
// No mmap/mprotect. Page allocations come from the newlib heap, page-aligned
// and zeroed like anonymous mappings are. Protection changes are accepted and
// ignored: their only users are the JITs (guard pages, W^X toggling), none of
// which exist in this ENABLE_GENERIC build. Executable memory would need the
// jit service, so asking for it is reported as the failure it is.

#include "Common/MemoryUtil.h"

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <malloc.h>

#include <switch.h>

#include "Common/Logging/Log.h"
#include "Common/MsgHandler.h"

namespace Common
{
namespace
{
constexpr size_t kPageSize = 0x1000;

int& JITPageWriteNestCounter()
{
  static thread_local int nest_counter = 0;
  return nest_counter;
}
}  // namespace

void* AllocateExecutableMemory(size_t)
{
  PanicAlertFmt("Executable memory is not available in the Switch build (no JIT)");
  return nullptr;
}

void JITPageWriteEnableExecuteDisable()
{
  JITPageWriteNestCounter()++;
}

void JITPageWriteDisableExecuteEnable()
{
  if (--JITPageWriteNestCounter() < 0)
    PanicAlertFmt("JITPageWriteNestCounter() underflowed");
}

void* AllocateMemoryPages(size_t size)
{
  const size_t rounded = (size + kPageSize - 1) & ~(kPageSize - 1);
  void* ptr = memalign(kPageSize, rounded);
  if (ptr == nullptr)
  {
    PanicAlertFmt("Failed to allocate raw memory");
    return nullptr;
  }
  // Anonymous mappings start zeroed, and callers rely on it.
  std::memset(ptr, 0, rounded);
  return ptr;
}

void* AllocateAlignedMemory(size_t size, size_t alignment)
{
  void* ptr = memalign(alignment < sizeof(void*) ? sizeof(void*) : alignment, size);
  if (ptr == nullptr)
    PanicAlertFmt("Failed to allocate aligned memory");
  return ptr;
}

bool FreeMemoryPages(void* ptr, size_t)
{
  std::free(ptr);
  return true;
}

void FreeAlignedMemory(void* ptr)
{
  std::free(ptr);
}

bool ReadProtectMemory(void*, size_t)
{
  return true;
}

bool WriteProtectMemory(void*, size_t, bool)
{
  return true;
}

bool UnWriteProtectMemory(void*, size_t, bool)
{
  return true;
}

size_t MemPhysical()
{
  u64 total = 0;
  if (R_FAILED(svcGetInfo(&total, InfoType_TotalMemorySize, CUR_PROCESS_HANDLE, 0)))
    return 0;
  return static_cast<size_t>(total);
}
}  // namespace Common
