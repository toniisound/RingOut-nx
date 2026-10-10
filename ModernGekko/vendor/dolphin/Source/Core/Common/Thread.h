// Copyright 2008 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>
#include <thread>

#ifndef _WIN32
#include <tuple>
#endif

// Don't include Common.h here as it will break LogManager
#include "Common/CommonTypes.h"

// This may not be defined outside _WIN32
#ifndef _WIN32
#ifndef INFINITE
#define INFINITE 0xffffffff
#endif
#endif

namespace Common
{
int CurrentThreadId();

void SetThreadAffinity(std::thread::native_handle_type thread, u32 mask);
void SetCurrentThreadAffinity(u32 mask);

void SleepCurrentThread(int ms);
void SwitchCurrentThread();  // On Linux, this is equal to sleep 1ms

// Use this function during a spin-wait to make the current thread
// relax while another thread is working. This may be more efficient
// than using events because event functions use kernel calls.
inline void YieldCPU()
{
  std::this_thread::yield();
}

void SetCurrentThreadName(const char* name);

#ifndef _WIN32
// Returns the lowest address of the stack and the size of the stack
std::tuple<void*, size_t> GetCurrentThreadStack();

#ifdef __SWITCH__
// CPU time used by each named thread since the previous call, as
// "CPU thread 98% Video thread 41% ...". Threads register by naming themselves
// (SetCurrentThreadName). Bring-up profiling for the Switch port.
std::string SwitchThreadLoadReport();
// Kernel handle of the named thread (as passed to SetCurrentThreadName), or 0
// if no thread of that name has started yet. For the sampling profiler.
u32 SwitchFindThreadHandle(const char* name);
#endif
#endif

}  // namespace Common
