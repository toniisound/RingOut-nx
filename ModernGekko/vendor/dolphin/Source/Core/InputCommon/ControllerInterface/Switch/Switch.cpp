// Copyright 2026 RingOut contributors
// SPDX-License-Identifier: GPL-2.0-or-later

// Nintendo Switch controllers (libnx HID) for ControllerInterface.
//
// One device per player slot, "Switch/0/Player 1" .. "Player 4". Player 1 also
// reads the console's attached Joy-Con (handheld mode), so a docked Pro
// Controller and the handheld both drive port 1 with no setup.
//
// INPUT NAMES ARE DOLPHIN'S SDL GAMEPAD NAMES, by position, not by label:
// "Button S" is the bottom face button (labelled B on a Switch), "Button E" the
// right one (A), and so on. RingOut's GameCube profile and its pause-menu pad
// navigation (RecompMenu.cpp) are both written against those names, so the
// same profile and the same menu controls work here unchanged -- and the
// GameCube's big A lands under the thumb where it sits on the real pad.
//
// Sticks follow the SDL backend's convention too: "Left Y+" is UP.

#include "InputCommon/ControllerInterface/Switch/Switch.h"

#include <algorithm>
#include <string>

#include <switch.h>

#include "Common/Logging/Log.h"
#include "InputCommon/ControllerInterface/ControllerInterface.h"
#include "InputCommon/ControllerInterface/CoreDevice.h"

namespace ciface::Switch
{
namespace
{
constexpr const char* kSource = "Switch";
constexpr int kPlayers = 4;

class Device final : public Core::Device
{
public:
  explicit Device(int player);

  std::string GetName() const override { return "Player " + std::to_string(m_player + 1); }
  std::string GetSource() const override { return kSource; }
  // Ahead of anything else a port might fall back to.
  int GetSortPriority() const override { return 10 - m_player; }
  Core::DeviceRemoval UpdateInput() override;

private:
  class Button final : public Input
  {
  public:
    Button(const char* name, const u64& buttons, u64 mask)
        : m_name(name), m_buttons(buttons), m_mask(mask)
    {
    }
    std::string GetName() const override { return m_name; }
    ControlState GetState() const override { return (m_buttons & m_mask) != 0 ? 1.0 : 0.0; }

  private:
    const char* const m_name;
    const u64& m_buttons;
    const u64 m_mask;
  };

  // One half of a stick axis: positive or negative direction, 0..1.
  class Axis final : public Input
  {
  public:
    Axis(const char* name, const s32& value, bool positive)
        : m_name(name), m_value(value), m_positive(positive)
    {
    }
    std::string GetName() const override { return m_name; }
    ControlState GetState() const override
    {
      const ControlState v = ControlState(m_value) / JOYSTICK_MAX;
      return m_positive ? std::max(v, 0.0) : std::max(-v, 0.0);
    }

  private:
    const char* const m_name;
    const s32& m_value;
    const bool m_positive;
  };

  int m_player;
  PadState m_pad{};
  u64 m_buttons = 0;
  s32 m_lx = 0, m_ly = 0, m_rx = 0, m_ry = 0;
};

Device::Device(int player) : m_player(player)
{
  const auto id = static_cast<HidNpadIdType>(HidNpadIdType_No1 + player);
  if (player == 0)
    padInitialize(&m_pad, id, HidNpadIdType_Handheld);
  else
    padInitialize(&m_pad, id);

  // clang-format off
  AddInput(new Button("Button S",   m_buttons, HidNpadButton_B));
  AddInput(new Button("Button E",   m_buttons, HidNpadButton_A));
  AddInput(new Button("Button W",   m_buttons, HidNpadButton_Y));
  AddInput(new Button("Button N",   m_buttons, HidNpadButton_X));
  AddInput(new Button("Shoulder L", m_buttons, HidNpadButton_L));
  AddInput(new Button("Shoulder R", m_buttons, HidNpadButton_R));
  // ZL/ZR are digital on every Switch controller: 0 or full press.
  AddInput(new Button("Trigger L",  m_buttons, HidNpadButton_ZL));
  AddInput(new Button("Trigger R",  m_buttons, HidNpadButton_ZR));
  AddInput(new Button("Start",      m_buttons, HidNpadButton_Plus));
  AddInput(new Button("Back",       m_buttons, HidNpadButton_Minus));
  AddInput(new Button("Thumb L",    m_buttons, HidNpadButton_StickL));
  AddInput(new Button("Thumb R",    m_buttons, HidNpadButton_StickR));
  AddInput(new Button("Pad N",      m_buttons, HidNpadButton_Up));
  AddInput(new Button("Pad S",      m_buttons, HidNpadButton_Down));
  AddInput(new Button("Pad W",      m_buttons, HidNpadButton_Left));
  AddInput(new Button("Pad E",      m_buttons, HidNpadButton_Right));

  AddInput(new Axis("Left X-",  m_lx, false));
  AddInput(new Axis("Left X+",  m_lx, true));
  AddInput(new Axis("Left Y-",  m_ly, false));
  AddInput(new Axis("Left Y+",  m_ly, true));
  AddInput(new Axis("Right X-", m_rx, false));
  AddInput(new Axis("Right X+", m_rx, true));
  AddInput(new Axis("Right Y-", m_ry, false));
  AddInput(new Axis("Right Y+", m_ry, true));
  // clang-format on
}

Core::DeviceRemoval Device::UpdateInput()
{
  padUpdate(&m_pad);
  m_buttons = padGetButtons(&m_pad);
  const HidAnalogStickState left = padGetStickPos(&m_pad, 0);
  const HidAnalogStickState right = padGetStickPos(&m_pad, 1);
  m_lx = left.x;
  m_ly = left.y;
  m_rx = right.x;
  m_ry = right.y;
  // Slots are fixed; a disconnected controller simply reads idle.
  return Core::DeviceRemoval::Keep;
}

class InputBackend final : public ciface::InputBackend
{
public:
  explicit InputBackend(ControllerInterface* controller_interface)
      : ciface::InputBackend(controller_interface)
  {
    // Up to four controllers in the standard styles (Pro Controller, Joy-Con
    // pair, handheld). Overrides the frontend's own single-player setup.
    padConfigureInput(kPlayers, HidNpadStyleSet_NpadStandard);
    hidSetNpadJoyHoldType(HidNpadJoyHoldType_Horizontal);
  }

  void PopulateDevices() override
  {
    GetControllerInterface().RemoveDevice(
        [](const auto* dev) { return dev->GetSource() == kSource; });
    for (int player = 0; player < kPlayers; ++player)
      GetControllerInterface().AddDevice(std::make_shared<Device>(player));
    INFO_LOG_FMT(CONTROLLERINTERFACE, "Switch: {} controller slots", kPlayers);
  }
};
}  // namespace

std::unique_ptr<ciface::InputBackend> CreateInputBackend(ControllerInterface* controller_interface)
{
  return std::make_unique<InputBackend>(controller_interface);
}
}  // namespace ciface::Switch
