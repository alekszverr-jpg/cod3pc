#include "title_handoff.h"

#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/kernel/xam/module.h>
#include <cstdio>
#include <cstdlib>

REXCVAR_DEFINE_BOOL(cod3_relaunch, true, "Game", "Honor the game's native campaign title restart");
REXCVAR_DEFINE_STRING(cod3_relaunch_pipe, "", "Debug", "Internal inherited title handoff pipe").debug_only();
REXCVAR_DEFINE_STRING(cod3_relaunch_parent, "", "Debug", "Internal previous-title wait handle").debug_only();

namespace {
std::vector<uint8_t> pending_launch_data;
}
void ResetCod3WorkerExit();

bool CaptureCod3InProcessHandoff() {
  const auto xam = rex::system::kernel_state()->GetKernelModule<rex::kernel::xam::XamModule>("xam.xex");
  if (!xam) return false;
  const auto& data = xam->loader_data();
  if (!cod3::IsCampaignRelaunch(data.launch_path, data.launch_flags,
      data.launch_data_present, data.launch_data.size())) return false;
  pending_launch_data = data.launch_data;
  REXLOG_INFO("COD3_INPROCESS_HANDOFF_CAPTURED bytes={}", pending_launch_data.size());
  return true;
}

void PrepareCod3TitleHandoff() {
  try {
    // Called before constructing ReXApp: no game window/runtime exists while
    // the successor waits for the original process to finish shutting down.
    pending_launch_data = cod3::ReceiveTitleHandoff(cod3::ProcessArguments());
  } catch (const std::exception& error) {
    std::fprintf(stderr, "cod3pc title handoff startup failed: %s\n", error.what());
    std::exit(EXIT_FAILURE);
  }
}

void RestoreCod3TitleHandoff() {
  ResetCod3WorkerExit();
  if (pending_launch_data.empty()) return;
  auto xam = rex::system::kernel_state()->GetKernelModule<rex::kernel::xam::XamModule>("xam.xex");
  // Use the same host LoaderData storage as the pinned Set/GetLaunchData APIs.
  // Original startup still reads/interprets the bytes and chooses the next map.
  xam->loader_data().launch_data = std::move(pending_launch_data);
  xam->loader_data().launch_data_present = true;
  REXLOG_INFO("COD3_RELAUNCH_RESTORED bytes={}", xam->loader_data().launch_data.size());
}

void RelaunchCod3TitleIfRequested() {
  const auto xam = rex::system::kernel_state()->GetKernelModule<rex::kernel::xam::XamModule>("xam.xex");
  if (!xam) return;
  const auto& data = xam->loader_data();
  if (!REXCVAR_GET(cod3_relaunch) || !cod3::IsCampaignRelaunch(data.launch_path,
      data.launch_flags, data.launch_data_present, data.launch_data.size())) {
    if (!data.launch_path.empty()) REXLOG_WARN("COD3_RELAUNCH_SKIPPED path='{}' flags={} bytes={}",
        data.launch_path, data.launch_flags, data.launch_data.size());
    return;
  }
  try {
    auto args = cod3::ProcessArguments();
    auto child_args = cod3::RelaunchArguments(args, L"-relaunch");
    // Preserve the current DoF value as well as the original CLI/settings file.
    const auto dof = rex::cvar::GetFlagByName("cod3_dof_strength");
    if (!dof.empty()) child_args.push_back(L"--cod3_dof_strength=" +
        std::wstring(dof.begin(), dof.end()));
    cod3::SendTitleHandoff(args.at(0), child_args, data.launch_data);
    REXLOG_INFO("COD3_RELAUNCH_QUEUED bytes={} successor_waits_for_exit=true", data.launch_data.size());
  } catch (const std::exception& error) {
    REXLOG_ERROR("COD3_RELAUNCH_FAILED {}", error.what());
  }
}
