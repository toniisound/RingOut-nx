/* Copyright 2026 RingOut contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * POSIX functions newlib declares but devkitA64/libnx does not implement.
 *
 * Every definition is weak: if a newer libnx or newlib ships the real one, the
 * real one wins and nothing here needs removing.
 *
 * - pread/pwrite: positioned I/O, emulated with lseek around read/write. Not
 *   atomic with respect to other users of the same descriptor, so one mutex
 *   serialises all of them; Dolphin only uses them for disc and file reads.
 * - dirname: plain POSIX semantics.
 * - sysconf: the few names Dolphin and the runtime ask for (page size, core
 *   count, clock ticks); anything else is EINVAL.
 * - process control (fork, exec, wait, kill, pipe): Horizon has no processes
 *   to spawn. They fail with ENOSYS, which every caller already handles as
 *   "the external tool could not be run" (mod extraction, "open in shell").
 */

#include <errno.h>
#include <libgen.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define WEAK __attribute__((weak))

static pthread_mutex_t s_positioned_io_lock = PTHREAD_MUTEX_INITIALIZER;

WEAK ssize_t pread(int fd, void* buf, size_t count, off_t offset)
{
    pthread_mutex_lock(&s_positioned_io_lock);
    const off_t saved = lseek(fd, 0, SEEK_CUR);
    ssize_t result = -1;
    if (saved != (off_t)-1 && lseek(fd, offset, SEEK_SET) != (off_t)-1)
    {
        result = read(fd, buf, count);
        const int saved_errno = errno;
        lseek(fd, saved, SEEK_SET);
        errno = saved_errno;
    }
    pthread_mutex_unlock(&s_positioned_io_lock);
    return result;
}

WEAK ssize_t pwrite(int fd, const void* buf, size_t count, off_t offset)
{
    pthread_mutex_lock(&s_positioned_io_lock);
    const off_t saved = lseek(fd, 0, SEEK_CUR);
    ssize_t result = -1;
    if (saved != (off_t)-1 && lseek(fd, offset, SEEK_SET) != (off_t)-1)
    {
        result = write(fd, buf, count);
        const int saved_errno = errno;
        lseek(fd, saved, SEEK_SET);
        errno = saved_errno;
    }
    pthread_mutex_unlock(&s_positioned_io_lock);
    return result;
}

WEAK char* dirname(char* path)
{
    static char dot[] = ".";
    static char slash[] = "/";
    if (path == NULL || *path == '\0')
        return dot;

    char* end = path + strlen(path) - 1;
    while (end > path && *end == '/')  /* trailing slashes */
        --end;
    while (end >= path && *end != '/')  /* last component */
        --end;
    if (end < path)
        return dot;
    while (end > path && *end == '/')  /* slashes before it */
        --end;
    if (end == path && *end == '/')
        return slash;
    /* "sdmc:/file" -> "sdmc:/": keep the device root's slash. */
    if (*end == ':')
        ++end;
    end[1] = '\0';
    return path;
}

WEAK long sysconf(int name)
{
    switch (name)
    {
    case _SC_PAGESIZE:
        return 0x1000;
    case _SC_NPROCESSORS_CONF:
    case _SC_NPROCESSORS_ONLN:
        return 3; /* cores 0-2 belong to applications */
    case _SC_CLK_TCK:
        return 100;
    default:
        errno = EINVAL;
        return -1;
    }
}

WEAK pid_t fork(void)
{
    errno = ENOSYS;
    return -1;
}

WEAK int execvp(const char* file, char* const argv[])
{
    (void)file;
    (void)argv;
    errno = ENOSYS;
    return -1;
}

WEAK pid_t waitpid(pid_t pid, int* status, int options)
{
    (void)pid;
    (void)status;
    (void)options;
    errno = ECHILD;
    return -1;
}

WEAK int kill(pid_t pid, int sig)
{
    (void)pid;
    (void)sig;
    errno = ENOSYS;
    return -1;
}

WEAK int pipe(int fildes[2])
{
    (void)fildes;
    errno = ENOSYS;
    return -1;
}

/* popen/pclose: same story. Only the native movie player calls them, and the
 * Switch build turns its hooks off (StaticRecompFmvHooks.h); these exist so
 * the image links, and say "could not run" if anything ever does call them. */
#include <stdio.h>

WEAK FILE* popen(const char* command, const char* mode)
{
    (void)command;
    (void)mode;
    errno = ENOSYS;
    return NULL;
}

WEAK int pclose(FILE* stream)
{
    (void)stream;
    errno = ECHILD;
    return -1;
}
