// Copyright 2026 RingOut contributors
// SPDX-License-Identifier: GPL-3.0-or-later

// Ring Out for Nintendo Switch: libnx entry point.
//
// Layout on the SD card (everything except the .nro is created or supplied by
// the player, never shipped):
//
//   sdmc:/switch/RingOut/RingOut.nro
//   sdmc:/switch/RingOut/game/        extracted disc (sys/main.dol, files/...),
//                                     the same game/ folder setup.sh makes on PC
//   sdmc:/switch/RingOut/user/        Dolphin user directory: saves, config
//   sdmc:/switch/RingOut/nx.cfg       optional key=value overrides (see below)
//   sdmc:/switch/RingOut/ringout.log  the log (stderr) when nxlink is not running;
//                                     stdout goes to ringout-stdout.log beside it
//
// Dolphin's Sys/ folder is packed into the .nro's RomFS. A Sys/ folder next to
// the .nro wins over it, which is handy for editing GameSettings without a
// rebuild.

#include <cstdio>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <ostream>
#include <map>
#include <string>
#include <unistd.h>

#include <switch.h>

#include "Common/DynamicLibrary.h"
#include "Common/FileUtil.h"
#include "Common/Config/Config.h"
#include "Common/Logging/LogManager.h"
#include "Core/Config/GraphicsSettings.h"
#include "Core/Config/MainSettings.h"
#include "moderngekko/runtime.hpp"
#include "profiler.h"
#include "static_module.h"

namespace
{
namespace fs = std::filesystem;

constexpr const char* kBaseDir = "sdmc:/switch/RingOut";

int s_nxlink_fd = -1;

// Text console for errors that happen before the renderer exists. Blocks until
// the player presses +, then returns so main() can exit normally.
void ShowFatalScreen(const std::string& title, const std::string& detail)
{
  std::fprintf(stderr, "[nx] fatal: %s: %s\n", title.c_str(), detail.c_str());
  std::fflush(stderr);

  consoleInit(nullptr);
  PadState pad;
  padConfigureInput(1, HidNpadStyleSet_NpadStandard);
  padInitializeDefault(&pad);

  std::printf("\n  Ring Out (Switch)\n\n  %s\n\n", title.c_str());
  std::printf("  %s\n\n", detail.c_str());
  std::printf("  Press + to exit.\n");
  consoleUpdate(nullptr);

  while (appletMainLoop())
  {
    padUpdate(&pad);
    if (padGetButtonsDown(&pad) & HidNpadButton_Plus)
      break;
    consoleUpdate(nullptr);
  }
  consoleExit(nullptr);
}

void SetUpLogging()
{
  // nxlink (`nxlink -s RingOut.nro` on the PC) streams stdout/stderr live,
  // which is the useful way to watch a boot. Without it, log to the SD card.
  if (R_SUCCEEDED(socketInitializeDefault()))
    s_nxlink_fd = nxlinkStdio();
  if (s_nxlink_fd >= 0)
    return;

  // Two files, not one: two FILE streams on the same file each keep their own
  // write offset on Horizon, so whichever wrote last overwrote the other.
  // Nearly everything (Dolphin, the runtime, this frontend) logs to stderr.
  const std::string log_path = std::string(kBaseDir) + "/ringout.log";
  const std::string out_path = std::string(kBaseDir) + "/ringout-stdout.log";
  std::freopen(log_path.c_str(), "w", stderr);
  std::freopen(out_path.c_str(), "w", stdout);
  // Unbuffered stderr: if the process dies, the last lines must be on disk.
  std::setvbuf(stderr, nullptr, _IONBF, 0);
  std::setvbuf(stdout, nullptr, _IOLBF, 0);
}

// Tiny key=value reader for nx.cfg. Unknown keys are ignored; '#' starts a
// comment. Kept deliberately dumb: the in-game menu is where settings belong,
// this file only exists for bring-up switches.
std::map<std::string, std::string> ReadConfig(const fs::path& path)
{
  std::map<std::string, std::string> values;
  std::ifstream in(path);
  std::string line;
  while (std::getline(in, line))
  {
    if (const auto hash = line.find('#'); hash != std::string::npos)
      line.resize(hash);
    const auto eq = line.find('=');
    if (eq == std::string::npos)
      continue;
    auto trim = [](std::string s) {
      const auto first = s.find_first_not_of(" \t\r");
      const auto last = s.find_last_not_of(" \t\r");
      return first == std::string::npos ? std::string{} : s.substr(first, last - first + 1);
    };
    values[trim(line.substr(0, eq))] = trim(line.substr(eq + 1));
  }
  return values;
}

bool IsTrue(const std::map<std::string, std::string>& cfg, const char* key)
{
  const auto it = cfg.find(key);
  return it != cfg.end() && (it->second == "1" || it->second == "true" || it->second == "yes");
}

std::string GetOr(const std::map<std::string, std::string>& cfg, const char* key,
                  std::string fallback)
{
  const auto it = cfg.find(key);
  return it != cfg.end() && !it->second.empty() ? it->second : fallback;
}

std::string Upper(std::string s)
{
  for (char& c : s)
    c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
  return s;
}

// Dolphin's own log, to stderr (ringout.log). Must run after Runtime::Create,
// which is what initialises the log manager.
//
//   log=all | none | DVD,VI,BOOT,...   (Dolphin's short names; default all)
//   log_level=error|warning|info|notice (default warning; OSReport lines from
//                                        the game itself are notices and
//                                        always pass)
void ConfigureDolphinLog(const std::map<std::string, std::string>& cfg)
{
  using namespace Common::Log;
  LogManager* manager = LogManager::GetInstance();
  if (!manager)
    return;

  const std::string which = Upper(GetOr(cfg, "log", "all"));
  if (which == "NONE")
    return;

  const std::string level_name = Upper(GetOr(cfg, "log_level", "warning"));
  LogLevel level = LogLevel::LWARNING;
  if (level_name == "NOTICE")
    level = LogLevel::LNOTICE;
  else if (level_name == "ERROR")
    level = LogLevel::LERROR;
  else if (level_name == "INFO")
    level = LogLevel::LINFO;
  else if (level_name == "DEBUG")
    level = LogLevel::LDEBUG;
  manager->SetConfigLogLevel(level);

  const std::string wanted = "," + which + ",";
  for (int i = 0; i < static_cast<int>(LogType::NUMBER_OF_LOGS); ++i)
  {
    const auto type = static_cast<LogType>(i);
    const bool on =
        which == "ALL" || wanted.find("," + Upper(manager->GetShortName(type)) + ",") !=
                              std::string::npos;
    manager->SetEnable(type, on);
  }
  manager->EnableListener(LogListener::CONSOLE_LISTENER, true);
  std::fprintf(stderr, "[nx] Dolphin log: %s at %s\n", which.c_str(), level_name.c_str());
}
// The GameCube pad profile for ports 1 and 2, written before the runtime loads
// its config. Input names are the SDL-style names the Switch backend exposes
// (InputCommon/ControllerInterface/Switch), and the map is the desktop one
// (tools/frontend_config.cpp, WriteGCPadSection) so both behave the same:
//
//   GC A / B / X / Y   bottom / right / left / top face button  (Switch B A Y X)
//   GC Z               R
//   GC L / R           ZL / ZR (digital, so full press = analog full too)
//   GC Start           +
//   Main stick / C     left / right stick
//   D-pad              D-pad
//
// - held for half a second opens RingOut's pause menu; + and - together quit.
void WriteGCPadSection(std::ostream& out, int port)
{
  out << "[GCPad" << port << "]\n"
      << "Device = Switch/0/Player " << port << "\n"
      << "Buttons/A = `Button S`\n"
         "Buttons/B = `Button E`\n"
         "Buttons/X = `Button W`\n"
         "Buttons/Y = `Button N`\n"
         "Buttons/Z = `Shoulder R`\n"
         "Buttons/Start = `Start`\n"
         "Triggers/L = `Trigger L`\n"
         "Triggers/R = `Trigger R`\n"
         "Triggers/L-Analog = `Trigger L`\n"
         "Triggers/R-Analog = `Trigger R`\n"
         "D-Pad/Up = `Pad N`\n"
         "D-Pad/Down = `Pad S`\n"
         "D-Pad/Left = `Pad W`\n"
         "D-Pad/Right = `Pad E`\n"
         "Main Stick/Up = `Left Y+`\n"
         "Main Stick/Down = `Left Y-`\n"
         "Main Stick/Left = `Left X-`\n"
         "Main Stick/Right = `Left X+`\n"
         "Main Stick/Calibration = 100.00 141.42 100.00 141.42 100.00 141.42 100.00 141.42\n"
         "C-Stick/Up = `Right Y+`\n"
         "C-Stick/Down = `Right Y-`\n"
         "C-Stick/Left = `Right X-`\n"
         "C-Stick/Right = `Right X+`\n"
         "C-Stick/Calibration = 100.00 141.42 100.00 141.42 100.00 141.42 100.00 141.42\n";
}

// Writes the profile unless the existing one already binds a Switch
// controller -- so a player's own remapping (CONTROLS tab) is kept, but the
// device-less stub Dolphin saved on exit before this backend existed is not.
// pad_profile=reset in nx.cfg rewrites it regardless.
void EnsureGCPadProfile(const fs::path& user_dir, bool force)
{
  std::error_code ec;
  const fs::path config_dir = user_dir / "Config";
  const fs::path ini = config_dir / "GCPadNew.ini";
  fs::create_directories(config_dir, ec);

  if (!force && fs::is_regular_file(ini, ec))
  {
    std::ifstream in(ini);
    const std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (text.find("Switch/0/") != std::string::npos)
      return;
    fs::copy_file(ini, config_dir / "GCPadNew.ini.bak", fs::copy_options::overwrite_existing, ec);
  }

  std::ofstream out(ini, std::ios::trunc);
  WriteGCPadSection(out, 1);
  out << "\n";
  WriteGCPadSection(out, 2);
  std::fprintf(stderr, "[nx] wrote the GameCube pad profile for ports 1 and 2\n");
}
}  // namespace

int main(int, char**)
{
  std::error_code ec;
  fs::create_directories(kBaseDir, ec);
  SetUpLogging();
  std::fprintf(stderr, "[nx] Ring Out for Switch starting\n");

  // Applet mode (launched from the Album) gets a few hundred MB of heap;
  // Dolphin alone needs more than that before the game has loaded anything.
  const AppletType applet_type = appletGetAppletType();
  if (applet_type != AppletType_Application && applet_type != AppletType_SystemApplication)
  {
    ShowFatalScreen("Not enough memory in applet mode.",
                    "Launch Ring Out through a game's title override\n"
                    "  (hold R while starting a game), not from the Album.");
    return 1;
  }

  const fs::path base = kBaseDir;
  const auto cfg = ReadConfig(base / "nx.cfg");

  // Once a second: emulation speed, FPS and VI rate ("[perf] ..." lines). The
  // only way to tell "running" from "stuck" when video=Null draws nothing, and
  // cheap enough to leave on while the port is being brought up.
  // perf_log=0 in nx.cfg turns it off.
  if (GetOr(cfg, "perf_log", "1") != "0")
    setenv("STATICRECOMP_SPEED", "1", 1);
  // clear_test=1: clear the screen to magenta instead of black every frame
  // (VideoCommon/Present.cpp). Tells "the window works, the game's draws do
  // not" apart from "nothing reaches the screen".
  if (IsTrue(cfg, "clear_test"))
    setenv("RINGOUT_NX_CLEAR_TEST", "1", 1);

  // Sys/: next to the .nro if present, else the copy packed in RomFS.
  std::string sys_dir;
  if (fs::is_directory(base / "Sys", ec))
    sys_dir = (base / "Sys").string();
  else if (R_SUCCEEDED(romfsInit()))
    sys_dir = "romfs:/Sys";
  if (sys_dir.empty())
  {
    ShowFatalScreen("Dolphin's Sys folder is missing.",
                    "The .nro has no RomFS and sdmc:/switch/RingOut/Sys does not exist.");
    return 1;
  }
  File::SetSysDirectory(sys_dir);
  std::fprintf(stderr, "[nx] Sys directory: %s\n", sys_dir.c_str());

  Common::DynamicLibrary::SetStaticSymbolResolver(&ringout_nx::ResolveModuleSymbol);

  moderngekko::RuntimeConfig config;
  config.game_root = base / "game";
  config.user_directory = base / "user";
  fs::create_directories(config.user_directory, ec);
  EnsureGCPadProfile(config.user_directory, GetOr(cfg, "pad_profile", "") == "reset");

  if (const ModernGekkoModuleDesc* module = ringout_nx::GetLinkedModule())
  {
    config.module = moderngekko::ModuleSource::AttachedDescriptor(module);
    std::fprintf(stderr, "[nx] recompiled module linked in\n");
  }
  else
  {
    std::fprintf(stderr, "[nx] no module linked in: interpreter only\n");
  }
  // Without a module there is nothing but the interpreter; with one, nx.cfg
  // can still allow falling back to it if the module is rejected.
  config.allow_interpreter = !config.module.descriptor || IsTrue(cfg, "allow_interpreter");

  // video=Null boots the whole core with no GPU work at all -- the first thing
  // to get working, because it separates CPU-side problems from GL ones.
  config.graphics.backend = GetOr(cfg, "video", "OGL");
  if (const auto it = cfg.find("resolution_scale"); it != cfg.end())
    config.graphics.internal_resolution_scale = std::atoi(it->second.c_str());
  config.audio.backend = GetOr(cfg, "audio", "");
  config.show_fps_in_title = true;

  int exit_code = 0;
  {
    moderngekko::RuntimeCreateResult created = moderngekko::Runtime::Create(std::move(config));
    if (!created)
    {
      ShowFatalScreen("Could not start the runtime.",
                      created.error ? created.error->message : std::string("unknown error"));
      exit_code = 1;
    }
    else
    {
      ConfigureDolphinLog(cfg);
      // Dolphin's own FPS/speed overlay, drawn by the renderer on top of the
      // game. While bringing up the picture it splits "nothing reaches the
      // screen" (no overlay either) from "the game's image is black" (overlay
      // visible). show_fps=0 in nx.cfg hides it.
      if (GetOr(cfg, "show_fps", "1") != "0")
      {
        Config::SetBase(Config::GFX_SHOW_FPS, true);
        Config::SetBase(Config::GFX_SHOW_SPEED, true);
      }
      // sync_gpu=1: Dolphin's "Synchronize GPU thread". Keeps the GPU thread
      // within a few thousand cycles of the CPU thread -- the fallback if the
      // dual-core FIFO still desyncs. Costs speed.
      if (IsTrue(cfg, "sync_gpu"))
      {
        Config::SetBase(Config::MAIN_SYNC_GPU, true);
        std::fprintf(stderr, "[nx] sync_gpu on\n");
      }
      std::fprintf(stderr, "[nx] booting %s (%s)\n",
                   created.runtime->GetGameMetadata().game_name.c_str(),
                   created.runtime->GetGameMetadata().disc_id.c_str());
      // Idle-skip: on desktop, worth about half the frame rate. If this says
      // "none", the OS idle loop is being executed for real.
      if (const auto& idle = created.runtime->GetGameMetadata().idle_pc)
        std::fprintf(stderr, "[nx] idle loop at 0x%08X (idle-skip on)\n", *idle);
      else
        std::fprintf(stderr, "[nx] idle loop: none found (idle-skip OFF)\n");
      // profile=1: sample the CPU thread (see profiler.h). profile_hz sets the
      // rate; the default costs a few percent of the CPU thread.
      const bool profile = IsTrue(cfg, "profile");
      if (profile)
      {
        ringout_nx::StartProfiler(std::string(kBaseDir) + "/profile.txt",
                                  static_cast<unsigned>(std::atoi(GetOr(cfg, "profile_hz", "500").c_str())));
      }
      const moderngekko::RuntimeRunResult result = created.runtime->Run();
      if (profile)
        ringout_nx::StopProfiler();
      if (result.error)
      {
        ShowFatalScreen("The game stopped with an error.", result.error->message);
        exit_code = 1;
      }
    }
  }

  std::fprintf(stderr, "[nx] exiting (%d)\n", exit_code);
  std::fflush(stderr);
  romfsExit();
  if (s_nxlink_fd >= 0)
    close(s_nxlink_fd);
  socketExit();
  return exit_code;
}
