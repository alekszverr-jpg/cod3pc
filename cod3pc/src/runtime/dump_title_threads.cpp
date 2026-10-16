// Out-of-process diagnostic only: never executes game code or changes its state.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <dbghelp.h>
#include <cerrno>
#include <cstdio>
#include <cwchar>
#include <limits>

int wmain(int argc, wchar_t** argv) {
  if (argc != 3) return 2;
  wchar_t* end = nullptr;
  errno = 0;
  const auto value = std::wcstoull(argv[1], &end, 10);
  if (errno || end == argv[1] || *end || !value ||
      value > std::numeric_limits<DWORD>::max()) return 2;
  const auto pid = static_cast<DWORD>(value);
  const HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
  if (!process) return 3;
  // Never overwrite evidence from another transition.
  const HANDLE file = CreateFileW(argv[2], GENERIC_WRITE, 0, nullptr,
      CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (file == INVALID_HANDLE_VALUE) { CloseHandle(process); return 4; }
  // Only thread contexts/stacks and module metadata; no full guest-memory dump.
  const bool success = MiniDumpWriteDump(process, pid, file,
      static_cast<MINIDUMP_TYPE>(MiniDumpNormal | MiniDumpWithThreadInfo),
      nullptr, nullptr, nullptr);
  const auto error = success ? ERROR_SUCCESS : GetLastError();
  CloseHandle(file);
  CloseHandle(process);
  std::fprintf(stderr, "Title thread dump: pid=%lu error=%lu\n", pid, error);
  return success ? 0 : 5;
}
