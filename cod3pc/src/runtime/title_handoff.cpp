#include "title_handoff.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shellapi.h>

#include <algorithm>
#include <array>
#include <filesystem>
#include <stdexcept>

namespace cod3 {
namespace {
constexpr auto kPipe = L"--cod3_relaunch_pipe=";
constexpr auto kParent = L"--cod3_relaunch_parent=";
struct Handle {
  HANDLE value = nullptr;
  ~Handle() { if (value && value != INVALID_HANDLE_VALUE) CloseHandle(value); }
};
[[noreturn]] void WinError(const char* operation) {
  throw std::runtime_error(std::string(operation) + " failed, Windows error " +
                           std::to_string(GetLastError()));
}
HANDLE ParseHandle(const std::wstring& value) {
  if (value.empty() || value.find_first_not_of(L"0123456789") != std::wstring::npos)
    throw std::runtime_error("Invalid title handoff handle");
  size_t end = 0;
  const auto number = std::stoull(value, &end);
  if (end != value.size() || !number || number > UINTPTR_MAX)
    throw std::runtime_error("Invalid title handoff handle");
  return reinterpret_cast<HANDLE>(static_cast<uintptr_t>(number));
}
}

std::wstring QuoteWindowsArgument(const std::wstring& value) {
  std::wstring result = L"\"";
  size_t slashes = 0;
  for (const auto ch : value) {
    if (ch == L'\\') { ++slashes; continue; }
    result.append(slashes * (ch == L'"' ? 2 : 1), L'\\');
    slashes = 0;
    if (ch == L'"') result += L'\\';
    result += ch;
  }
  result.append(slashes * 2, L'\\');
  return result + L'"';
}

std::vector<std::wstring> ProcessArguments() {
  int count = 0;
  auto argv = CommandLineToArgvW(GetCommandLineW(), &count);
  if (!argv) WinError("CommandLineToArgvW");
  std::vector<std::wstring> result(argv, argv + count);
  LocalFree(argv);
  // argv[0] can be relative or a caller-supplied label. Relaunch the actual
  // current executable, independent of the caller's spelling/current directory.
  std::vector<wchar_t> module_path(32768);
  const auto length = GetModuleFileNameW(nullptr, module_path.data(), static_cast<DWORD>(module_path.size()));
  if (!length || length >= module_path.size()) WinError("GetModuleFileNameW");
  if (result.empty()) throw std::runtime_error("Missing executable argument");
  result[0].assign(module_path.data(), length);
  return result;
}

bool IsCampaignRelaunch(const std::string& path, uint32_t flags,
                       bool present, size_t bytes) {
  std::string normalized = path;
  std::transform(normalized.begin(), normalized.end(), normalized.begin(),
      [](unsigned char ch) { return ch >= 'A' && ch <= 'Z' ? ch + ('a' - 'A') : ch; });
  std::replace(normalized.begin(), normalized.end(), '/', '\\');
  return normalized == "game:\\default.xex" && flags == 0 && present &&
         bytes == kCampaignLaunchBytes;
}

std::vector<std::wstring> RelaunchArguments(const std::vector<std::wstring>& args,
                                          const std::wstring& suffix) {
  std::vector<std::wstring> result;
  for (size_t i = 1; i < args.size(); ++i) {
    const auto& arg = args[i];
    if (arg.starts_with(kPipe) || arg.starts_with(kParent)) continue;
    // Let original startup consume the campaign launch data instead of replaying
    // an initial +devmap/+spmap command. Keep profile and rendering CLI options.
    if (arg == L"--cl" || arg == L"--log_file") {
      if (++i == args.size()) throw std::runtime_error("Missing launch option value");
      if (arg == L"--log_file") {
        const std::filesystem::path log(args[i]);
        result.push_back(L"--log_file=" + (log.parent_path() /
            (log.stem().wstring() + suffix + log.extension().wstring())).wstring());
      }
      continue;
    }
    if (arg.starts_with(L"--cl=")) continue;
    if (arg.starts_with(L"--log_file=")) {
      const std::filesystem::path log(arg.substr(11));
      result.push_back(L"--log_file=" + (log.parent_path() /
          (log.stem().wstring() + suffix + log.extension().wstring())).wstring());
      continue;
    }
    result.push_back(arg);
  }
  result.push_back(L"--cl=");
  return result;
}

void SendTitleHandoff(const std::wstring& executable,
                      const std::vector<std::wstring>& arguments,
                      std::span<const uint8_t> data) {
  if (data.size() != kCampaignLaunchBytes)
    throw std::runtime_error("Unexpected campaign launch data size");
  SECURITY_ATTRIBUTES security{sizeof(security), nullptr, TRUE};
  Handle reader, writer, parent;
  if (!CreatePipe(&reader.value, &writer.value, &security, 4096)) WinError("CreatePipe");
  if (!SetHandleInformation(writer.value, HANDLE_FLAG_INHERIT, 0)) WinError("SetHandleInformation");
  if (!DuplicateHandle(GetCurrentProcess(), GetCurrentProcess(), GetCurrentProcess(),
      &parent.value, SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION, TRUE, 0))
    WinError("DuplicateHandle");
  DWORD written = 0;
  if (!WriteFile(writer.value, data.data(), static_cast<DWORD>(data.size()), &written, nullptr) ||
      written != data.size()) WinError("WriteFile");
  CloseHandle(writer.value);
  writer.value = nullptr;

  SIZE_T attribute_size = 0;
  InitializeProcThreadAttributeList(nullptr, 1, 0, &attribute_size);
  std::vector<uint8_t> storage(attribute_size);
  auto attributes = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(storage.data());
  if (!InitializeProcThreadAttributeList(attributes, 1, 0, &attribute_size))
    WinError("InitializeProcThreadAttributeList");
  struct Attributes { LPPROC_THREAD_ATTRIBUTE_LIST value; ~Attributes() {
    DeleteProcThreadAttributeList(value); } } cleanup{attributes};
  std::array<HANDLE, 2> inherited{reader.value, parent.value};
  if (!UpdateProcThreadAttribute(attributes, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
      inherited.data(), sizeof(inherited), nullptr, nullptr)) WinError("UpdateProcThreadAttribute");

  std::wstring command = QuoteWindowsArgument(executable);
  for (const auto& arg : arguments) command += L" " + QuoteWindowsArgument(arg);
  command += L" " + std::wstring(kPipe) + std::to_wstring(reinterpret_cast<uintptr_t>(reader.value));
  command += L" " + std::wstring(kParent) + std::to_wstring(reinterpret_cast<uintptr_t>(parent.value));
  if (command.size() >= 32767) throw std::runtime_error("Title relaunch command too long");
  STARTUPINFOEXW startup{};
  startup.StartupInfo.cb = sizeof(startup);
  startup.lpAttributeList = attributes;
  PROCESS_INFORMATION child{};
  if (!CreateProcessW(executable.c_str(), command.data(), nullptr, nullptr, TRUE,
      EXTENDED_STARTUPINFO_PRESENT | CREATE_NO_WINDOW, nullptr, nullptr,
      &startup.StartupInfo, &child)) WinError("CreateProcessW");
  CloseHandle(child.hThread);
  CloseHandle(child.hProcess);
}

std::vector<uint8_t> ReceiveTitleHandoff(const std::vector<std::wstring>& args) {
  std::wstring pipe_text, parent_text;
  for (const auto& arg : args) {
    if (arg.starts_with(kPipe)) {
      if (!pipe_text.empty()) throw std::runtime_error("Duplicate title pipe option");
      pipe_text = arg.substr(std::wstring(kPipe).size());
      if (pipe_text.empty()) throw std::runtime_error("Empty title pipe option");
    }
    if (arg.starts_with(kParent)) {
      if (!parent_text.empty()) throw std::runtime_error("Duplicate title parent option");
      parent_text = arg.substr(std::wstring(kParent).size());
      if (parent_text.empty()) throw std::runtime_error("Empty title parent option");
    }
  }
  if (pipe_text.empty() && parent_text.empty()) return {};
  if (pipe_text.empty() || parent_text.empty()) throw std::runtime_error("Incomplete title handoff");
  Handle reader{ParseHandle(pipe_text)}, parent{ParseHandle(parent_text)};
  if (GetFileType(reader.value) != FILE_TYPE_PIPE || !GetProcessId(parent.value) ||
      GetProcessId(parent.value) == GetCurrentProcessId())
    throw std::runtime_error("Invalid inherited title handoff handles");
  if (WaitForSingleObject(parent.value, 60000) != WAIT_OBJECT_0)
    throw std::runtime_error("Previous title did not exit within 60 seconds");
  std::vector<uint8_t> result(kCampaignLaunchBytes);
  DWORD count = 0;
  if (!ReadFile(reader.value, result.data(), static_cast<DWORD>(result.size()), &count, nullptr) ||
      count != result.size()) throw std::runtime_error("Incomplete campaign launch data");
  return result;
}
}
