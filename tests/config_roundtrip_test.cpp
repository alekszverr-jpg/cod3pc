#include <rex/cvar.h>
#include <Windows.h>
#include <filesystem>
#include <fstream>
#include <cstdio>
#include <stdexcept>

REXCVAR_DEFINE_STRING(cod3_test_path, "", "Tests/ConfigRoundtrip", "Config string probe");
REXCVAR_DEFINE_INT32(cod3_test_dof, 100, "Tests/ConfigRoundtrip", "Config number probe");

void Require(bool ok) { if (!ok) throw std::runtime_error("config round trip failed"); }
int main() {
  const auto path = std::filesystem::temp_directory_path() /
      ("cod3-config-test-" + std::to_string(GetCurrentProcessId()) + ".toml");
  try {
    char name[] = "config-test"; char* args[] = {name};
    rex::cvar::Init(1, args);
    const std::string value = "D:\\Porting\\Русский\\\"profile\"\nline\tend\\\x01";
    Require(rex::cvar::SetFlagByName("cod3_test_path", value));
    Require(rex::cvar::SetFlagByName("cod3_test_dof", "35"));
    rex::cvar::SaveConfig(path);
    for (int pass = 0; pass < 2; ++pass) {
      rex::cvar::ResetToDefault("cod3_test_path");
      rex::cvar::ResetToDefault("cod3_test_dof");
      rex::cvar::LoadConfig(path);
      Require(rex::cvar::GetFlagByName("cod3_test_path") == value);
      Require(rex::cvar::GetFlagByName("cod3_test_dof") == "35");
      if (!pass) {
        std::ofstream out(path);
        out << rex::cvar::SerializeToTOML("Tests/ConfigRoundtrip");
      }
    }
    std::filesystem::remove(path);
    std::puts("config path and number round trip passed"); return 0;
  } catch (const std::exception& e) {
    std::error_code ignored; std::filesystem::remove(path, ignored);
    std::puts(e.what()); return 1;
  }
}
