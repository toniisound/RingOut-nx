// Copyright 2026 RingOut contributors
// SPDX-License-Identifier: GPL-2.0-or-later

// NoGUI platform for Nintendo Switch homebrew (libnx).
//
// There is no window system: the render target is the application's default
// NWindow, which GLContextSwitch turns into an EGL surface. The main loop's
// only jobs are servicing Dolphin's host job queue and honouring the applet
// lifecycle -- HOME-menu exit, or the + and - buttons held together, which is
// the conventional "quit" chord for homebrew that owns the whole pad.

#include <chrono>
#include <cstdio>
#include <thread>

#include <switch.h>

#include "Core/Core.h"
#include "Core/System.h"
#include "DolphinNoGUI/Platform.h"

namespace
{
class PlatformSwitch final : public Platform
{
public:
  bool Init() override;
  void SetTitle(const std::string& title) override;
  void MainLoop() override;

  WindowSystemInfo GetWindowSystemInfo() const override;

private:
  PadState m_pad{};
};

bool PlatformSwitch::Init()
{
  // Our own pad reader for the quit chord only; the emulated controllers read
  // HID through InputCommon's Switch backend.
  padConfigureInput(1, HidNpadStyleSet_NpadStandard);
  padInitializeDefault(&m_pad);
  return Platform::Init();
}

void PlatformSwitch::SetTitle(const std::string& title)
{
  // No title bar. The runtime calls this about once a second with the FPS in
  // it, which is worth keeping in the log while bringing the port up.
  std::fprintf(stderr, "[title] %s\n", title.c_str());
}

void PlatformSwitch::MainLoop()
{
  while (m_running.IsSet())
  {
    if (!appletMainLoop())
    {
      // HOME menu "close", or the system shutting down: stop right away, the
      // applet will not wait for a graceful one.
      Stop();
      break;
    }

    padUpdate(&m_pad);
    const u64 held = padGetButtons(&m_pad);
    if ((held & HidNpadButton_Plus) && (held & HidNpadButton_Minus))
      RequestShutdown();

    UpdateRunningFlag();
    Core::HostDispatchJobs(Core::System::GetInstance());
    std::this_thread::sleep_for(std::chrono::milliseconds(16));
  }
}

WindowSystemInfo PlatformSwitch::GetWindowSystemInfo() const
{
  WindowSystemInfo wsi;
  wsi.type = WindowSystemType::Switch;
  wsi.display_connection = nullptr;
  wsi.render_window = nwindowGetDefault();
  wsi.render_surface = wsi.render_window;
  return wsi;
}
}  // namespace

std::unique_ptr<Platform> Platform::CreateSwitchPlatform()
{
  return std::make_unique<PlatformSwitch>();
}
