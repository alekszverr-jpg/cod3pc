#pragma once

#include <chrono>
#include <cstdint>
#include <vector>

// An observation of actual Windows thread exit, independent of guest object
// signaling/running_ and of the SDK registry that TerminateTitle clears.
class Cod3ThreadExitProbe {
 public:
  struct Pending { uint32_t guest_id; uint32_t native_id; bool wait_failed; };
  ~Cod3ThreadExitProbe();
  Cod3ThreadExitProbe() = default;
  Cod3ThreadExitProbe(const Cod3ThreadExitProbe&) = delete;
  Cod3ThreadExitProbe& operator=(const Cod3ThreadExitProbe&) = delete;
  void Add(void* native_handle, uint32_t guest_id, uint32_t native_id);
  std::vector<Pending> Wait(std::chrono::milliseconds timeout) const;
  size_t size() const { return entries_.size(); }
 private:
  struct Entry { void* handle; uint32_t guest_id; uint32_t native_id; };
  std::vector<Entry> entries_;
};

void CaptureCod3TitleThreads();
void ReportCod3TitleThreads();
