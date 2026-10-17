// cod3pc - ReXGlue Recompiled Project
//
// Customize your app by overriding virtual hooks from rex::ReXApp.

#pragma once

#include <rex/rex_app.h>
#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/ui/imgui_drawer.h>
#include <rex/ui/immediate_drawer.h>

void InstallCod3CrashTrace();
void Cod3TraceLoaderState();
void PrepareCod3TitleHandoff();
void RestoreCod3TitleHandoff();
void RelaunchCod3TitleIfRequested();
void ReportCod3TitleThreads();
bool Cod3TitleAudioReady();
bool DrainCod3TitleGpu(rex::system::IGraphicsSystem* graphics);
REXCVAR_DECLARE(bool, cod3_title_gpu_drain);

class Cod3pcApp : public rex::ReXApp {
 public:
  using rex::ReXApp::ReXApp;

  static std::unique_ptr<rex::ui::WindowedApp> Create(
      rex::ui::WindowedAppContext& ctx) {
    PrepareCod3TitleHandoff();
    return std::unique_ptr<Cod3pcApp>(new Cod3pcApp(ctx, "cod3pc",
        PPCImageConfig));
  }

  void OnPreSetup(rex::RuntimeConfig& config) override {
    // Use the SDK's Xenos backend for the original game's GPU commands.
    if (config.gpu_plugin.empty()) {
#if defined(COD3_GRASS_GPU_AVAILABLE)
      config.gpu_plugin = "xenos-grass";
#else
      config.gpu_plugin = "xenos";
#endif
    }
  }

  void OnPostSetup() override {
    InstallCod3CrashTrace();
    RestoreCod3TitleHandoff();
    REXLOG_INFO("COD3_GPU_SETTINGS memexport={} memexport_fast={} resolve={} page_refresh={}",
        rex::cvar::GetFlagByName("readback_memexport"),
        rex::cvar::GetFlagByName("readback_memexport_fast"),
        rex::cvar::GetFlagByName("readback_resolve"),
        rex::cvar::GetFlagByName("clear_memory_page_state"));
  }

  void OnGuestThreadExit(rex::system::XThread*) override {
    Cod3TraceLoaderState();
    ReportCod3TitleThreads();
    if (REXCVAR_GET(cod3_title_gpu_drain)) {
      if (Cod3TitleAudioReady()) {
        const bool detached = app_context().CallInUIThreadSynchronous([this]() {
          if (imgui_drawer()) imgui_drawer()->SetPresenterAndImmediateDrawer(nullptr, nullptr);
          if (immediate_drawer()) immediate_drawer()->SetPresenter(nullptr);
          if (window()) {
            window()->SetPresenter(nullptr);
            REXLOG_INFO("COD3_TITLE_PRESENTATION_DETACHED hwnd={}", window()->GetNativeWindowHandle());
          }
        });
        if (detached) DrainCod3TitleGpu(runtime()->graphics_system());
        else REXLOG_ERROR("COD3_TITLE_GPU_DRAIN_SKIPPED reason=ui_detach_failed");
      } else {
        REXLOG_ERROR("COD3_TITLE_GPU_DRAIN_SKIPPED reason=guest_audio_not_drained");
      }
    }
    RelaunchCod3TitleIfRequested();
  }

  // Override virtual hooks for customization:
  // void OnPostInitLogging() override {}
  // void OnPreSetup(rex::RuntimeConfig& config) override {}
  // void OnLoadXexImage(std::string& xex_image) override {}
  // void OnPostLoadXexImage() override {}
  // void OnPostSetup() override {}
  // void OnCreateDialogs(rex::ui::ImGuiDrawer* drawer) override {}
  // std::unique_ptr<rex::ui::ImGuiDialog> CreateAchievementsOverlay() override;
  // std::unique_ptr<rex::ui::AchievementNotificationDialog>
  // CreateAchievementNotificationDialog() override;
  // void OnShutdown() override {}
  // void OnConfigurePaths(rex::PathConfig& paths) override {}
};
