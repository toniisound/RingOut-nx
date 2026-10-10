// Copyright 2019 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Common/DynamicLibrary.h"

#include <cstring>

#include <fmt/format.h>

#if defined(_WIN32)
#include <Windows.h>
#elif !defined(__SWITCH__)
#include <dlfcn.h>
#endif

namespace Common
{
DynamicLibrary::DynamicLibrary() = default;

DynamicLibrary::DynamicLibrary(const char* filename)
{
  Open(filename);
}

DynamicLibrary::DynamicLibrary(void* handle)
{
  m_handle = handle;
}

DynamicLibrary::~DynamicLibrary()
{
  Close();
}

DynamicLibrary& DynamicLibrary::operator=(void* handle)
{
  m_handle = handle;
  return *this;
}

std::string DynamicLibrary::GetUnprefixedFilename(const char* filename)
{
#if defined(_WIN32)
  return std::string(filename) + ".dll";
#elif defined(__APPLE__)
  return std::string(filename) + ".dylib";
#else
  return std::string(filename) + ".so";
#endif
}

std::string DynamicLibrary::GetVersionedFilename(const char* libname, int major, int minor)
{
#if defined(_WIN32)
  if (major >= 0 && minor >= 0)
    return fmt::format("{}-{}-{}.dll", libname, major, minor);
  else if (major >= 0)
    return fmt::format("{}-{}.dll", libname, major);
  else
    return fmt::format("{}.dll", libname);
#elif defined(__APPLE__)
  const char* prefix = std::strncmp(libname, "lib", 3) ? "lib" : "";
  if (major >= 0 && minor >= 0)
    return fmt::format("{}{}.{}.{}.dylib", prefix, libname, major, minor);
  else if (major >= 0)
    return fmt::format("{}{}.{}.dylib", prefix, libname, major);
  else
    return fmt::format("{}{}.dylib", prefix, libname);
#else
  const char* prefix = std::strncmp(libname, "lib", 3) ? "lib" : "";
  if (major >= 0 && minor >= 0)
    return fmt::format("{}{}.so.{}.{}", prefix, libname, major, minor);
  else if (major >= 0)
    return fmt::format("{}{}.so.{}", prefix, libname, major);
  else
    return fmt::format("{}{}.so", prefix, libname);
#endif
}

#ifdef __SWITCH__
namespace
{
DynamicLibrary::StaticSymbolResolver s_static_resolver = nullptr;
}

void DynamicLibrary::SetStaticSymbolResolver(StaticSymbolResolver resolver)
{
  s_static_resolver = resolver;
}

bool DynamicLibrary::Open(const char*)
{
  m_handle = nullptr;
  return false;
}

void DynamicLibrary::Close()
{
  m_handle = nullptr;
}

void* DynamicLibrary::GetSymbolAddress(const char* name) const
{
  return s_static_resolver ? s_static_resolver(name) : nullptr;
}
#else
bool DynamicLibrary::Open(const char* filename)
{
#ifdef _WIN32
  m_handle = reinterpret_cast<void*>(LoadLibraryA(filename));
#else
  m_handle = dlopen(filename, RTLD_NOW);
#endif
  return m_handle != nullptr;
}

void DynamicLibrary::Close()
{
  if (!IsOpen())
    return;

#ifdef _WIN32
  FreeLibrary(static_cast<HMODULE>(m_handle));
#else
  dlclose(m_handle);
#endif
  m_handle = nullptr;
}

void* DynamicLibrary::GetSymbolAddress(const char* name) const
{
#ifdef _WIN32
  return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(m_handle), name));
#else
  return reinterpret_cast<void*>(dlsym(m_handle, name));
#endif
}
#endif  // __SWITCH__
}  // namespace Common
