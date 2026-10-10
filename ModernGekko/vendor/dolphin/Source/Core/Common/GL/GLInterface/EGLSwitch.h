// Copyright 2026 RingOut contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Common/GL/GLInterface/EGL.h"

// EGL on the Nintendo Switch (switch-mesa, nouveau). The window is the
// application's default NWindow, which the Switch platform passes as
// WindowSystemInfo::render_surface.
class GLContextEGLSwitch final : public GLContextEGL
{
protected:
  EGLDisplay OpenEGLDisplay() override;
  EGLNativeWindowType GetEGLNativeWindow(EGLConfig config) override;
  bool GetFallbackSurfaceSize(int* width, int* height) override;
};
