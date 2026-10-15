#include "thread_exit_probe.h"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <algorithm>
#include <stdexcept>
#include <thread>

Cod3ThreadExitProbe::~Cod3ThreadExitProbe() {
  for (const auto& entry : entries_) CloseHandle(entry.handle);
}

void Cod3ThreadExitProbe::Add(void* native_handle, uint32_t guest_id, uint32_t native_id) {
  HANDLE copy = nullptr;
  if (!DuplicateHandle(GetCurrentProcess(), native_handle, GetCurrentProcess(),
      &copy, SYNCHRONIZE, FALSE, 0)) {
    throw std::runtime_error("Thread wait-handle duplication failed: " + std::to_string(GetLastError()));
  }
  try {
    entries_.push_back({copy, guest_id, native_id});
  } catch (...) {
    CloseHandle(copy);
    throw;
  }
}

std::vector<Cod3ThreadExitProbe::Pending> Cod3ThreadExitProbe::Wait(
    std::chrono::milliseconds timeout) const {
  const auto deadline = std::chrono::steady_clock::now() + timeout;
  for (;;) {
    std::vector<Pending> pending;
    for (const auto& entry : entries_) {
      const auto status = WaitForSingleObject(entry.handle, 0);
      if (status != WAIT_OBJECT_0)
        pending.push_back({entry.guest_id, entry.native_id, status != WAIT_TIMEOUT});
    }
    if (pending.empty() || std::chrono::steady_clock::now() >= deadline) return pending;
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
  }
}
