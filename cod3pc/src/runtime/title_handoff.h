#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace cod3 {
inline constexpr size_t kCampaignLaunchBytes = 664;
std::wstring QuoteWindowsArgument(const std::wstring& value);
std::vector<std::wstring> ProcessArguments();
bool IsCampaignRelaunch(const std::string& path, uint32_t flags,
                       bool present, size_t bytes);
// Both functions throw on failure. Only the pipe and old-process wait handle
// are inherited; the payload never touches disk. Receive waits before returning.
void SendTitleHandoff(const std::wstring& executable,
                      const std::vector<std::wstring>& arguments,
                      std::span<const uint8_t> data);
std::vector<uint8_t> ReceiveTitleHandoff(const std::vector<std::wstring>& arguments);
std::vector<std::wstring> RelaunchArguments(const std::vector<std::wstring>& arguments,
                                          const std::wstring& suffix);
}
