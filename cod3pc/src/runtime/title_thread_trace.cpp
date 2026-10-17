#include "thread_exit_probe.h"
#include <rex/logging.h>
#include <rex/system/kernel_state.h>
#include <rex/system/xthread.h>
#include <rex/thread.h>
#include <rex/cvar.h>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <filesystem>
#include <memory>
#include <mutex>

REXCVAR_DECLARE(bool, cod3_title_thread_drain);

REXCVAR_DEFINE_BOOL(cod3_transition_dump, false, "Debug",
    "Capture pending old-title workers before process handoff").debug_only();

namespace {
std::mutex probe_mutex;
std::unique_ptr<Cod3ThreadExitProbe> probe;
bool incomplete = false;

void DumpPendingTitleThreads() {
  // DbgHelp must run outside the target: dumping oneself may deadlock on
  // loader/heap locks held by the very workers being investigated.
  wchar_t image[32768];
  const auto count = GetModuleFileNameW(nullptr, image, 32768);
  if (!count || count == 32768) {
    REXLOG_ERROR("COD3_TITLE_DUMP_FAILED stage=executable_path");
    return;
  }
  const auto helper = std::filesystem::path(image).parent_path() / L"cod3_dump_threads.exe";
  const auto directory = std::filesystem::current_path() / L"reports" / L"transition-threads";
  std::error_code error;
  std::filesystem::create_directories(directory, error);
  if (error) {
    REXLOG_ERROR("COD3_TITLE_DUMP_FAILED stage=directory error={}", error.message());
    return;
  }
  const auto output = directory / (L"old-title-" + std::to_wstring(GetCurrentProcessId()) +
      L"-" + std::to_wstring(GetTickCount64()) + L".dmp");
  std::wstring command = L"\"" + helper.wstring() + L"\" " +
      std::to_wstring(GetCurrentProcessId()) + L" \"" + output.wstring() + L"\"";
  STARTUPINFOW startup{};
  startup.cb = sizeof(startup);
  PROCESS_INFORMATION child{};
  if (!CreateProcessW(helper.c_str(), command.data(), nullptr, nullptr, FALSE,
      CREATE_NO_WINDOW, nullptr, nullptr, &startup, &child)) {
    REXLOG_ERROR("COD3_TITLE_DUMP_FAILED stage=create error={}", GetLastError());
    return;
  }
  CloseHandle(child.hThread);
  const auto waited = WaitForSingleObject(child.hProcess, 15000);
  DWORD result = STILL_ACTIVE;
  GetExitCodeProcess(child.hProcess, &result);
  CloseHandle(child.hProcess);
  REXLOG_INFO("COD3_TITLE_DUMP_RESULT pid={} wait={} result={} path='{}'",
      GetCurrentProcessId(), waited, result, output.string());
  // A failed/timed-out diagnostic never authorizes teardown or changes the
  // existing handoff path. The helper has no game/window/backend role.
}
}

void CaptureCod3TitleThreads() {
  // Called just before the original LaunchTitle import. No thread is suspended,
  // terminated or removed; duplicate handles remain valid after SDK cleanup.
  auto snapshot = std::make_unique<Cod3ThreadExitProbe>();
  bool failed = false;
  const auto objects = rex::system::kernel_state()->object_table()->GetAllObjects();
  for (const auto& object : objects) {
    if (object->type() != rex::system::XObject::Type::Thread) continue;
    auto* thread = static_cast<rex::system::XThread*>(object.get());
    if (!thread->is_guest_thread() || !thread->thread()) continue;
    REXLOG_INFO("COD3_TITLE_THREAD_CAPTURE guest={} native={} start={:08X}",
        thread->thread_id(), thread->thread()->system_id(), thread->creation_params()->start_address);
    try {
      snapshot->Add(thread->thread()->native_handle(), thread->thread_id(), thread->thread()->system_id());
    } catch (const std::exception& error) {
      failed = true;
      REXLOG_WARN("COD3_TITLE_THREADS_CAPTURE_FAILED guest={} {}", thread->thread_id(), error.what());
    }
  }
  REXLOG_INFO("COD3_TITLE_THREADS_CAPTURED count={} incomplete={}", snapshot->size(), failed);
  std::lock_guard lock(probe_mutex);
  probe = std::move(snapshot);
  incomplete = failed;
}

void ReportCod3TitleThreads() {
  std::unique_ptr<Cod3ThreadExitProbe> snapshot;
  bool failed = false;
  {
    std::lock_guard lock(probe_mutex);
    snapshot = std::move(probe);
    failed = incomplete;
  }
  if (!snapshot) return; // An ordinary quit may not request LaunchTitle.
  const auto start = std::chrono::steady_clock::now();
  const auto pending = snapshot->Wait(std::chrono::milliseconds(1000));
  const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now() - start).count();
  REXLOG_INFO("COD3_TITLE_THREADS_OBSERVED captured={} pending={} incomplete={} elapsed_ms={}",
      snapshot->size(), pending.size(), failed, elapsed);
  for (const auto& item : pending)
    REXLOG_WARN("COD3_TITLE_THREAD_PENDING guest={} native={} wait_failed={}",
        item.guest_id, item.native_id, item.wait_failed);
  if (!pending.empty() && REXCVAR_GET(cod3_transition_dump)) DumpPendingTitleThreads();
  if (REXCVAR_GET(cod3_title_thread_drain)) {
    const auto drain = rex::system::kernel_state()->WaitForTitleThreadExit(5000);
    REXLOG_INFO("COD3_TITLE_THREAD_DRAIN sealed={} creation_complete={} tracked={} pending={}",
        drain.sealed, drain.creation_complete, drain.tracked, drain.pending);
    // Still observational: audio/GPU callbacks and teardown are separate gates.
  }
  // This is evidence, not a permission to free memory: threads created after
  // the snapshot and GPU/audio host workers are outside this observation.
}
