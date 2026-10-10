// Copyright 2026 RingOut contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Common/GL/GLInterface/EGLSwitch.h"

#include <switch.h>

namespace
{
// The NWindow's buffers are the size the backbuffer will be. 720p is what the
// handheld screen shows natively; docked, the console scales it to the TV. The
// game itself renders at 640x480-ish internally, so going higher would only
// cost fill rate for a stretched picture.
constexpr u32 kSurfaceWidth = 1280;
constexpr u32 kSurfaceHeight = 720;
}  // namespace

EGLDisplay GLContextEGLSwitch::OpenEGLDisplay()
{
  return eglGetDisplay(EGL_DEFAULT_DISPLAY);
}

EGLNativeWindowType GLContextEGLSwitch::GetEGLNativeWindow(EGLConfig)
{
  NWindow* window = static_cast<NWindow*>(m_wsi.render_surface);
  // Fails harmlessly (AlreadyInitialized) when Dolphin's probe context already
  // configured the window with these same dimensions.
  nwindowSetDimensions(window, kSurfaceWidth, kSurfaceHeight);
  m_backbuffer_width = static_cast<int>(kSurfaceWidth);
  m_backbuffer_height = static_cast<int>(kSurfaceHeight);
  return reinterpret_cast<EGLNativeWindowType>(window);
}

// switch-mesa answers eglQuerySurface(EGL_WIDTH/HEIGHT) with 0 until the first
// buffer has been dequeued, and Dolphin sized its backbuffer -- viewport,
// presenter, on-screen UI -- from that answer: everything was drawn into a
// 1x1 rectangle and the screen stayed black. The NWindow knows its size.
bool GLContextEGLSwitch::GetFallbackSurfaceSize(int* width, int* height)
{
  NWindow* window = static_cast<NWindow*>(m_wsi.render_surface);
  u32 w = 0, h = 0;
  if (!window || R_FAILED(nwindowGetDimensions(window, &w, &h)) || w == 0 || h == 0)
  {
    w = kSurfaceWidth;
    h = kSurfaceHeight;
  }
  *width = static_cast<int>(w);
  *height = static_cast<int>(h);
  return true;
}
