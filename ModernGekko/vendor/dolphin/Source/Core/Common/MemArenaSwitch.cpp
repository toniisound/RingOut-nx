// Copyright 2026 RingOut contributors
// SPDX-License-Identifier: GPL-2.0-or-later

// MemArena for Horizon (Nintendo Switch homebrew, libnx).
//
// Horizon has no mmap of shared memory, so a segment cannot be viewed at two
// addresses at once. That only matters for fastmem, which needs the JIT and is
// never built for this target (ENABLE_GENERIC). Without fastmem, Memmap asks
// for exactly one view per physical region -- RAM, L1 cache, fake VMEM -- and
// never aliases them, so a view can simply be a pointer into one ordinary heap
// block. The fastmem entry points report failure, which Memmap already treats
// as "fastmem unavailable".

#include "Common/MemArena.h"

#include <cstdlib>
#include <cstring>
#include <malloc.h>

#include "Common/Assert.h"
#include "Common/CommonTypes.h"
#include "Common/Logging/Log.h"

namespace Common
{
namespace
{
constexpr size_t kPageSize = 0x1000;

// A lazily committed region is a promise the OS keeps on desktop (MAP_NORESERVE)
// and cannot keep here: Horizon commits everything it hands out. Requests above
// this size are refused so callers take their non-lazy fallback instead of
// trying to allocate, say, the JIT's 64 GiB entry-point map for real.
constexpr size_t kMaxLazyRegion = 512u * 1024u * 1024u;
}  // namespace

MemArena::MemArena() = default;
MemArena::~MemArena()
{
  ReleaseSHMSegment();
}

void MemArena::GrabSHMSegment(size_t size, std::string_view)
{
  ASSERT(m_reserved_region == nullptr);
  const size_t rounded = (size + kPageSize - 1) & ~(kPageSize - 1);
  m_reserved_region = memalign(kPageSize, rounded);
  if (!m_reserved_region)
  {
    ERROR_LOG_FMT(MEMMAP, "MemArena: failed to allocate {} bytes of emulated memory", rounded);
    m_reserved_region_size = 0;
    return;
  }
  std::memset(m_reserved_region, 0, rounded);
  m_reserved_region_size = rounded;
}

void MemArena::ReleaseSHMSegment()
{
  if (m_reserved_region)
  {
    std::free(m_reserved_region);
    m_reserved_region = nullptr;
    m_reserved_region_size = 0;
  }
}

void* MemArena::CreateView(s64 offset, size_t size)
{
  if (!m_reserved_region || offset < 0 ||
      static_cast<size_t>(offset) + size > m_reserved_region_size)
  {
    ERROR_LOG_FMT(MEMMAP, "MemArena: view {:#x}+{:#x} is outside the segment", offset, size);
    return nullptr;
  }
  return static_cast<u8*>(m_reserved_region) + offset;
}

void MemArena::ReleaseView(void*, size_t)
{
  // Views are pointers into the segment; nothing to unmap.
}

u8* MemArena::ReserveMemoryRegion(size_t)
{
  // No fastmem on this target.
  return nullptr;
}

void MemArena::ReleaseMemoryRegion()
{
}

void* MemArena::MapInMemoryRegion(s64, size_t, void*, bool)
{
  return nullptr;
}

bool MemArena::ChangeMappingProtection(void*, size_t, bool)
{
  return false;
}

void MemArena::UnmapFromMemoryRegion(void*, size_t)
{
}

size_t MemArena::GetPageSize() const
{
  return kPageSize;
}

LazyMemoryRegion::LazyMemoryRegion() = default;

LazyMemoryRegion::~LazyMemoryRegion()
{
  Release();
}

void* LazyMemoryRegion::Create(size_t size)
{
  ASSERT(!m_memory);
  if (size == 0 || size > kMaxLazyRegion)
    return nullptr;

  void* memory = std::calloc(1, size);
  if (!memory)
  {
    NOTICE_LOG_FMT(MEMMAP, "Memory allocation of {} bytes failed.", size);
    return nullptr;
  }
  m_memory = memory;
  m_size = size;
  return memory;
}

void LazyMemoryRegion::Clear()
{
  ASSERT(m_memory);
  std::memset(m_memory, 0, m_size);
}

void LazyMemoryRegion::Release()
{
  if (m_memory)
  {
    std::free(m_memory);
    m_memory = nullptr;
    m_size = 0;
  }
}
}  // namespace Common
