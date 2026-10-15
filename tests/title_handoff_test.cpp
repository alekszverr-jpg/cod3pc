#include "../cod3pc/src/runtime/title_handoff.h"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shellapi.h>
#include <cstdio>
#include <stdexcept>

void Require(bool value) { if (!value) throw std::runtime_error("handoff test failed"); }
std::vector<uint8_t> Payload() {
  std::vector<uint8_t> bytes(cod3::kCampaignLaunchBytes);
  for (size_t i = 0; i < bytes.size(); ++i) bytes[i] = static_cast<uint8_t>(i * 37);
  return bytes;
}
int wmain() {
  try {
    const auto args = cod3::ProcessArguments();
    if (args.size() > 2 && args[1] == L"--receiver") {
      Require(cod3::ReceiveTitleHandoff(args) == Payload());
      const auto event = OpenEventW(EVENT_MODIFY_STATE, FALSE, args[2].c_str());
      Require(event != nullptr);
      Require(SetEvent(event));
      CloseHandle(event);
      return 0;
    }
    if (args.size() > 2 && args[1] == L"--worker") {
      cod3::SendTitleHandoff(args[0], {L"--receiver", args[2]}, Payload());
      // Receiver must not initialize until this process exits.
      const auto event = OpenEventW(SYNCHRONIZE, FALSE, args[2].c_str());
      Require(event != nullptr);
      Require(WaitForSingleObject(event, 150) == WAIT_TIMEOUT);
      CloseHandle(event);
      return 0;
    }
    Require(cod3::IsCampaignRelaunch("GAME:/DEFAULT.XEX", 0, true, 664));
    Require(cod3::IsCampaignRelaunch("\\Device\\Harddisk0\\Partition1\\default.xex", 0, true, 664));
    Require(!cod3::IsCampaignRelaunch("\\Device\\Harddisk0\\Partition2\\default.xex", 0, true, 664));
    Require(!cod3::IsCampaignRelaunch("\\Device\\Harddisk0\\Partition1\\codmp_xenonf.xex", 0, true, 664));
    for (const auto path : {"", "game:\\codmp_xenonf.xex", "default.xex", "game:\\sp\\island\\island.dll"})
      Require(!cod3::IsCampaignRelaunch(path, 0, true, 664));
    Require(!cod3::IsCampaignRelaunch("game:\\default.xex", 1, true, 664));
    Require(!cod3::IsCampaignRelaunch("game:\\default.xex", 0, false, 664));
    Require(!cod3::IsCampaignRelaunch("game:\\default.xex", 0, true, 663));
    Require(cod3::ReceiveTitleHandoff(args).empty());
    for (const auto malformed : {std::vector<std::wstring>{L"--cod3_relaunch_pipe=1"},
                                std::vector<std::wstring>{L"--cod3_relaunch_pipe=", L"--cod3_relaunch_parent=1"}}) {
      bool rejected = false;
      try { cod3::ReceiveTitleHandoff(malformed); } catch (...) { rejected = true; }
      Require(rejected);
    }
    const std::vector<std::wstring> values{L"", L"plain", L"C:\\path with spaces\\", L"text\"quoted", L"\\\\\"", L"Русский путь"};
    std::wstring command = L"test.exe";
    for (const auto& arg : values) command += L" " + cod3::QuoteWindowsArgument(arg);
    int count = 0;
    const auto decoded = CommandLineToArgvW(command.c_str(), &count);
    Require(decoded && count == values.size() + 1);
    for (size_t i = 0; i < values.size(); ++i) Require(values[i] == decoded[i + 1]);
    LocalFree(decoded);
    const auto rebuilt = cod3::RelaunchArguments({L"test.exe", L"--cl", L"+devmap saint_lo",
        L"--user_data_root", L"D:\\profile with spaces", L"--cod3_dof_strength=35",
        L"--log_file=D:\\logs\\native.log", L"--cod3_relaunch_pipe=4",
        L"--cod3_relaunch_parent=8"}, L"-next");
    Require(rebuilt == std::vector<std::wstring>{L"--user_data_root", L"D:\\profile with spaces",
        L"--cod3_dof_strength=35", L"--log_file=D:\\logs\\native-next.log", L"--cl="});

    // Actual Windows process/pipe round trip: worker exits, successor restores
    // all synthetic bytes. A named event only reports success to the test root.
    const auto name = L"Local\\cod3-handoff-test-" + std::to_wstring(GetCurrentProcessId());
    const auto event = CreateEventW(nullptr, TRUE, FALSE, name.c_str());
    Require(event != nullptr);
    auto launch = cod3::QuoteWindowsArgument(args[0]) + L" --worker " + cod3::QuoteWindowsArgument(name);
    STARTUPINFOW startup{sizeof(startup)};
    PROCESS_INFORMATION worker{};
    Require(CreateProcessW(args[0].c_str(), launch.data(), nullptr, nullptr, FALSE,
        CREATE_NO_WINDOW, nullptr, nullptr, &startup, &worker));
    Require(WaitForSingleObject(worker.hProcess, 5000) == WAIT_OBJECT_0);
    DWORD exit_code = 1;
    Require(GetExitCodeProcess(worker.hProcess, &exit_code) && exit_code == 0);
    Require(WaitForSingleObject(event, 5000) == WAIT_OBJECT_0);
    CloseHandle(worker.hProcess);
    CloseHandle(worker.hThread);
    CloseHandle(event);
    std::puts("Native handoff policy, arguments, process wait and opaque payload passed");
    return 0;
  } catch (const std::exception& error) {
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
  }
}
