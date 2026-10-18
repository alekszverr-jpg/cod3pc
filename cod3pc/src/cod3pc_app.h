// cod3pc - ReXGlue Recompiled Project
//
// Customize your app by overriding virtual hooks from rex::ReXApp.

#pragma once

#include <rex/rex_app.h>
#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/system/kernel_state.h>
#include <rex/ui/ui_event.h>
#include <rex/ui/keybinds.h>
#include <rex/ui/imgui_drawer.h>
#include <rex/ui/immediate_drawer.h>
#include "runtime/fiber_task.h"

void InstallCod3CrashTrace();
void Cod3TraceLoaderState();
void PrepareCod3TitleHandoff();
void RestoreCod3TitleHandoff();
void RelaunchCod3TitleIfRequested();
void ReportCod3TitleThreads();
bool Cod3TitleAudioReady();
bool DrainCod3TitleGpu(rex::system::IGraphicsSystem* graphics);
REXCVAR_DECLARE(bool, cod3_title_gpu_drain);
REXCVAR_DECLARE(bool, cod3_title_dispatch_drain);
void DumpCod3TitleConsumers();
bool RequestCod3MovieSkip();
void ResetCod3MovieSkip();
std::size_t Cod3ScriptContextCount();

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
    ResetCod3MovieSkip();
    InstallCod3CrashTrace();
    RestoreCod3TitleHandoff();
    REXLOG_INFO("COD3_GPU_SETTINGS memexport={} memexport_fast={} resolve={} page_refresh={}",
        rex::cvar::GetFlagByName("readback_memexport"),
        rex::cvar::GetFlagByName("readback_memexport_fast"),
        rex::cvar::GetFlagByName("readback_resolve"),
        rex::cvar::GetFlagByName("clear_memory_page_state"));
  }

  void OnGuestThreadExit(rex::system::XThread*) override {
    ResetCod3MovieSkip();
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
        if (detached) {
          const bool gpu_ready = DrainCod3TitleGpu(runtime()->graphics_system());
          if (REXCVAR_GET(cod3_title_dispatch_drain)) {
            if (gpu_ready) {
              const bool complete = runtime()->kernel_state()->QuiesceDispatchForTitleShutdown(5000);
              REXLOG_INFO("COD3_TITLE_DISPATCH_DRAIN complete={}", complete);
              if (!complete) DumpCod3TitleConsumers();
              else {
                const auto reaped = runtime()->kernel_state()->ReapExitedThreadObjects();
                const auto pending = runtime()->kernel_state()->PendingExitedThreadObjects();
                REXLOG_INFO("COD3_TITLE_EXIT_REFS reaped={} pending={}", reaped, pending);
                if (pending) DumpCod3TitleConsumers();
                const auto contexts = Cod3ScriptContextCount();
                const auto fibers = Cod3FiberTask::LiveCount();
                REXLOG_INFO("COD3_TITLE_SCRIPT_DRAIN contexts={} fibers={} complete={}",
                    contexts, fibers, contexts == 0 && fibers == 0);
                if (contexts || fibers) DumpCod3TitleConsumers();
              }
            } else {
              REXLOG_ERROR("COD3_TITLE_DISPATCH_DRAIN_SKIPPED reason=gpu_not_drained");
            }
          }
        }
        else REXLOG_ERROR("COD3_TITLE_GPU_DRAIN_SKIPPED reason=ui_detach_failed");
      } else {
        REXLOG_ERROR("COD3_TITLE_GPU_DRAIN_SKIPPED reason=guest_audio_not_drained");
      }
    }
    RelaunchCod3TitleIfRequested();
  }

  void OnKeyDown(rex::ui::KeyEvent& event) override {
    const auto key = event.virtual_key();
    if (!event.prev_state() && !event.is_alt_pressed() &&
        !event.is_ctrl_pressed() && !event.is_super_pressed() &&
        (key == rex::ui::VirtualKey::kSpace || key == rex::ui::VirtualKey::kReturn) &&
        RequestCod3MovieSkip()) {
      event.set_handled(true);
      return;
    }
    rex::ui::ProcessKeyEvent(event);
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
