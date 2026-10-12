// Preserve evidence for an unhandled fault without recovering or changing game state.
#ifdef _WIN32
#include <Windows.h>
#include <DbgHelp.h>
#include <cstdio>
#include <cstring>

namespace {
LONG WINAPI TraceUnhandledFault(EXCEPTION_POINTERS* fault) {
  FILE* report = nullptr;
  fopen_s(&report, "cod3pc-crash.txt", "w");
  if (report) {
    std::fprintf(report, "exception=%08lX host_pc=%p thread=%lu\n",
                 fault->ExceptionRecord->ExceptionCode,
                 fault->ExceptionRecord->ExceptionAddress, GetCurrentThreadId());
    if (fault->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
      std::fprintf(report, "operation=%llu address=%016llX\n",
                   static_cast<unsigned long long>(fault->ExceptionRecord->ExceptionInformation[0]),
                   static_cast<unsigned long long>(fault->ExceptionRecord->ExceptionInformation[1]));
    const HANDLE process = GetCurrentProcess();
    SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
    // Local symbols only; no symbol-server downloads or transmission.
    if (SymInitialize(process, ".", TRUE)) {
      CONTEXT context = *fault->ContextRecord;
      STACKFRAME64 frame{};
      frame.AddrPC = {context.Rip, AddrModeFlat};
      frame.AddrFrame = {context.Rbp, AddrModeFlat};
      frame.AddrStack = {context.Rsp, AddrModeFlat};
      for (unsigned i = 0; i < 64 && frame.AddrPC.Offset; ++i) {
        alignas(SYMBOL_INFO) char storage[sizeof(SYMBOL_INFO) + MAX_SYM_NAME]{};
        auto* symbol = reinterpret_cast<SYMBOL_INFO*>(storage);
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = MAX_SYM_NAME;
        DWORD64 displacement = 0;
        const bool named = SymFromAddr(process, frame.AddrPC.Offset, &displacement, symbol);
        IMAGEHLP_LINE64 line{};
        line.SizeOfStruct = sizeof(line);
        DWORD line_offset = 0;
        const bool located = SymGetLineFromAddr64(process, frame.AddrPC.Offset, &line_offset, &line);
        std::fprintf(report, "%02u %016llX %s+%llX %s:%lu\n", i,
                     static_cast<unsigned long long>(frame.AddrPC.Offset), named ? symbol->Name : "?",
                     static_cast<unsigned long long>(displacement), located ? line.FileName : "?",
                     located ? line.LineNumber : 0);
        if (!StackWalk64(IMAGE_FILE_MACHINE_AMD64, process, GetCurrentThread(), &frame,
                         &context, nullptr, SymFunctionTableAccess64, SymGetModuleBase64, nullptr)) break;
      }
      SymCleanup(process);
    }
    std::fclose(report);
  }
  const HANDLE dump = CreateFileW(L"cod3pc-crash.dmp", GENERIC_WRITE, 0, nullptr,
                                  CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (dump != INVALID_HANDLE_VALUE) {
    MINIDUMP_EXCEPTION_INFORMATION info{GetCurrentThreadId(), fault, FALSE};
    MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), dump,
                     static_cast<MINIDUMP_TYPE>(MiniDumpNormal | MiniDumpWithThreadInfo),
                     &info, nullptr, nullptr);
    CloseHandle(dump);
  }
  return EXCEPTION_CONTINUE_SEARCH;
}
}
void InstallCod3CrashTrace() { SetUnhandledExceptionFilter(TraceUnhandledFault); }
#else
void InstallCod3CrashTrace() {}
#endif
