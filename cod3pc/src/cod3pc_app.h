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
#include "runtime/movie_skip_overlay.h"
#include "runtime/pc_menu.h"
#include "runtime/pc_input.h"
#include "runtime/native_video_menu.h"
#include "runtime/menu_prompts.h"
#include "runtime/video_options.h"
#include <Windows.h>
#include <array>

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
REXCVAR_DECLARE(bool, cod3_title_runtime_teardown);
REXCVAR_DECLARE(bool, cod3_title_inprocess_restart);
bool CaptureCod3InProcessHandoff();
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
    title_consumers_ready_ = false;
    inprocess_pending_ = false;
    REXLOG_INFO("COD3_RUNTIME_GENERATION generation={} pid={} hwnd={}",
        ++runtime_generation_, GetCurrentProcessId(), window() ? window()->GetNativeWindowHandle() : nullptr);
    ResetCod3MovieSkip();
    cod3::pcmenu::Reset();
     cod3::input::Reset();
    cod3::video::Reset();
    cod3::menuprompts::Reset();
    REXLOG_INFO("COD3_VIDEO_SETTINGS dof={} aa={} af={} tearing={}",
        rex::cvar::GetFlagByName("cod3_dof_strength"), rex::cvar::GetFlagByName("swap_post_effect"),
        rex::cvar::GetFlagByName("anisotropic_override"),
        rex::cvar::GetFlagByName("d3d12_allow_variable_refresh_rate_and_tearing"));
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
          FlushVideoSettings();
          cod3::pcmenu::Detach(window());
          cod3::input::Detach(window());
          movie_skip_overlay_.reset();
          movie_skip_font_ = nullptr;
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
                title_consumers_ready_ = pending == 0 && contexts == 0 && fibers == 0;
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
    if (REXCVAR_GET(cod3_title_inprocess_restart)) {
      // Snapshot the original launch bytes while Xam and old memory are alive.
      // No child process is queued in this opt-in mode, including on failure.
      inprocess_pending_ = title_consumers_ready_ && CaptureCod3InProcessHandoff();
      if (!inprocess_pending_) REXLOG_ERROR("COD3_INPROCESS_RESTART_SKIPPED reason=not_ready_or_no_handoff");
    } else {
      RelaunchCod3TitleIfRequested();
    }
  }

  void OnGuestThreadReleased() override {
    if (!REXCVAR_GET(cod3_title_runtime_teardown)) return;
    if (!title_consumers_ready_) {
      REXLOG_ERROR("COD3_TITLE_RUNTIME_DRAIN complete=false reason=consumers_not_drained");
      return;
    }
    // The watcher has released its XThread reference. Only now may object-table
    // and guest memory teardown run. The current handoff still waits for this
    // process to exit; this diagnostic does not start a new Runtime in it.
    const std::array dlls{L"cod3pc_saint_lo.dll", L"cod3pc_island.dll", L"cod3pc_nightd.dll"};
    unsigned loaded_before = 0;
    for (const auto* name : dlls) loaded_before += GetModuleHandleW(name) != nullptr;
    const bool released = ShutdownTitleRuntime();
    unsigned loaded_after = 0;
    for (const auto* name : dlls) loaded_after += GetModuleHandleW(name) != nullptr;
    const bool complete = released && loaded_after == 0;
    REXLOG_INFO("COD3_TITLE_RUNTIME_DRAIN globals_cleared={} campaign_dlls_before={} campaign_dlls_after={} complete={}",
        released, loaded_before, loaded_after, complete);
    if (inprocess_pending_) {
      const bool requested = complete && RequestTitleRuntimeRestart();
      REXLOG_INFO("COD3_INPROCESS_RESTART_REQUESTED complete={}", requested);
    }
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

  void OnConfigureFonts(ImFontAtlas* atlas) override {
    movie_skip_font_ = Cod3MovieSkipOverlay::LoadFont(atlas);
  }

  void OnConfigurePaths(rex::PathConfig& paths) override {
    video_config_path_ = paths.config_path;
  }

  void FlushVideoSettings() {
    const bool video = cod3::video::TakeSaveRequest(), input = cod3::input::TakeSaveRequest();
    if (video || input) rex::cvar::SaveConfig(video_config_path_);
  }

  void OnCreateDialogs(rex::ui::ImGuiDrawer* drawer) override {
    movie_skip_overlay_ = std::make_unique<Cod3MovieSkipOverlay>(drawer, movie_skip_font_);
    cod3::input::Attach(window(), 64, [drawer]() {
      const auto& io = drawer->GetIO(); return io.WantCaptureMouse || io.WantCaptureKeyboard;
    });
    cod3::pcmenu::Attach(window(), 63, [drawer]() {
      const auto& io = drawer->GetIO();
      return io.WantCaptureMouse || io.WantCaptureKeyboard;
    });
    movie_skip_overlay_->on_frame = [this]() {
      FlushVideoSettings();
      cod3::pcmenu::UpdateCursor(window());
      cod3::input::OnUiFrame();
    };
  }

  void OnShutdown() override {
    FlushVideoSettings();
    cod3::pcmenu::Detach(window());
    cod3::input::Detach(window());
    ResetCod3MovieSkip();
    movie_skip_overlay_.reset();
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
 private:
  std::filesystem::path video_config_path_;
  ImFont* movie_skip_font_ = nullptr;
  std::unique_ptr<Cod3MovieSkipOverlay> movie_skip_overlay_;
  bool title_consumers_ready_ = false;
  bool inprocess_pending_ = false;
  unsigned runtime_generation_ = 0;
};
