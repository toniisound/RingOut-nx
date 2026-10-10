// Copyright 2026 RingOut contributors
// SPDX-License-Identifier: GPL-3.0-or-later

// Give every thread a desktop-sized stack.
//
// libnx creates a pthread with a 128 KiB stack unless told otherwise, and
// std::thread never says otherwise. Linux gives 8 MiB. The difference matters
// here more than in most ports: with --direct-calls and --self-calls the
// recompiled game calls guest functions as native C calls, so the CPU thread's
// stack depth follows the game's own call depth, frame by (large) chunk frame.
// 128 KiB would turn the first deep call chain into a silent crash.
//
// Linked with -Wl,--wrap=pthread_create (see CMakeLists.txt), which reroutes
// every pthread_create in the image -- libstdc++'s std::thread included --
// through here. A caller that asked for a stack size of its own keeps it.

#include <cstddef>
#include <pthread.h>

namespace
{
// Linux's default. Horizon commits it up front, so ~15 threads cost ~120 MiB
// of the ~3 GiB a title-override launch gets.
constexpr size_t kDefaultStackSize = 8u * 1024u * 1024u;
}  // namespace

extern "C" {
int __real_pthread_create(pthread_t* thread, const pthread_attr_t* attr,
                          void* (*start_routine)(void*), void* arg);

int __wrap_pthread_create(pthread_t* thread, const pthread_attr_t* attr,
                          void* (*start_routine)(void*), void* arg)
{
  pthread_attr_t local;
  if (attr)
  {
    size_t requested = 0;
    if (pthread_attr_getstacksize(attr, &requested) == 0 && requested != 0)
      return __real_pthread_create(thread, attr, start_routine, arg);
    local = *attr;
  }
  else
  {
    pthread_attr_init(&local);
  }
  pthread_attr_setstacksize(&local, kDefaultStackSize);
  const int result = __real_pthread_create(thread, &local, start_routine, arg);
  if (!attr)
    pthread_attr_destroy(&local);
  return result;
}
}
