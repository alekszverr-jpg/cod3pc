param([int]$Seconds = 30, [switch]$Headless, [switch]$Interactive,
    [ValidateSet('Debug','RelWithDebInfo','Release')][string]$Configuration = 'RelWithDebInfo',
    [switch]$FirstMission, [switch]$SynchronousMemexport, [switch]$GpuCoherencyTest,
    [switch]$FullGpuReadback, [switch]$GrassShaderTest, [switch]$StockGpu,
    [ValidateSet('none','fast','some','full')][string]$ResolveReadback = 'fast',
    [switch]$ShaderDiagnostics, [switch]$OcclusionTrace, [switch]$OcclusionSnapshots,
    [switch]$ShadowDepthSource, [switch]$OriginalShadowDepth, [switch]$GpuQueryTrace,
    [switch]$CooperativeWorkerExit, [switch]$TransitionDump,
    [switch]$TitleThreadDrain, [switch]$TitleAudioDrain, [switch]$TitleGpuDrain,
    [switch]$TitleDispatchDrain, [switch]$TitleRuntimeTeardown, [switch]$TitleInProcessRestart,
    [switch]$ProcessRelaunch,
    [ValidateRange(0,100)][int]$DofStrength = 100,
    [ValidatePattern('^[a-z][a-z0-9_]*$')][string]$DebugMission,
    [ValidatePattern('^[a-z][a-z0-9_]*$')][string]$DebugProfile,
    [switch]$ListLevels, [switch]$DryRun)
. "$PSScriptRoot/Enter-Toolchain.ps1"
if ($FullGpuReadback) {
    if ($PSBoundParameters.ContainsKey('ResolveReadback') -and $ResolveReadback -ne 'full') {
        throw 'FullGpuReadback requires ResolveReadback full; omit one of the conflicting options.'
    }
    $ResolveReadback = 'full'
}
$app = Join-Path $ProjectRoot ('cod3pc/out/build/win-amd64-' + $Configuration.ToLowerInvariant() + '/cod3pc.exe')
if ($DebugMission -and $FirstMission) { throw 'Use DebugMission or FirstMission, not both.' }
if ($DebugMission -and $DebugProfile -and $DebugMission -cne $DebugProfile) {
    throw 'DebugMission already selects its own profile; use DebugProfile alone to resume a checkpoint.'
}
if ($DebugMission -or $ListLevels) {
    $taskLevelsJson = & $Python "$PSScriptRoot/list_debug_levels.py" --game-root "$ProjectRoot/assets/game" `
        --manifest "$ProjectRoot/cod3pc/cod3pc_manifest.toml" --build-root (Split-Path $app)
    if ($LASTEXITCODE -ne 0) { throw 'Debug level inventory failed.' }
    $taskLevels = @($taskLevelsJson | ConvertFrom-Json)
    if ($ListLevels) {
        $taskLevels | Select-Object level,map,kind,declared,native,ready | Format-Table -AutoSize
        return
    }
    $taskSelectedLevel = @($taskLevels | Where-Object { $_.level -ceq $DebugMission })
    if ($taskSelectedLevel.Count -ne 1) { throw "Unknown debug mission '$DebugMission'; use -ListLevels." }
    if (!$taskSelectedLevel[0].ready) { throw "Mission '$DebugMission' has no correctly mapped native DLL in this build." }
}
if (!(Test-Path -LiteralPath $app)) { throw 'Build cod3pc first' }
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss-fff'
$logPath = Join-Path $ProjectRoot "logs/native-$stamp.log"
$taskGpuPlugin = if ($StockGpu) { 'xenos' } elseif ($GpuCoherencyTest) { 'xenos-coherency' } else { 'xenos-grass' }
if ($StockGpu -and ($GrassShaderTest -or $GpuCoherencyTest)) { throw 'Select one GPU backend.' }
if ($GrassShaderTest -and $GpuCoherencyTest) { throw 'Select one GPU test plugin.' }
if ($DofStrength -ne 100 -and $taskGpuPlugin -ne 'xenos-grass') {
    throw 'Custom DoF strength requires the corrected xenos-grass plugin.'
}
if ($taskGpuPlugin -eq 'xenos-grass' -and $Configuration -ne 'RelWithDebInfo') {
    throw 'The corrected grass GPU plugin is currently staged only for RelWithDebInfo; use -StockGpu for another configuration.'
}
if ($GpuCoherencyTest -and $Configuration -ne 'RelWithDebInfo') {
    throw 'The GPU coherency test plugin is currently staged only for RelWithDebInfo.'
}
# The accepted normal path retains the process/window and requires every drain.
# Keep individual barrier switches for staged diagnostics with ProcessRelaunch.
if ($ProcessRelaunch -and $TitleInProcessRestart) {
    throw 'ProcessRelaunch and TitleInProcessRestart select different transition modes.'
}
if (!$ProcessRelaunch) {
    if ($taskGpuPlugin -ne 'xenos-grass' -or $Configuration -ne 'RelWithDebInfo') {
        throw 'Same-process transitions require the corrected RelWithDebInfo GPU plugin. Use -ProcessRelaunch for stock/diagnostic backends.'
    }
    $CooperativeWorkerExit = $true
    $TitleThreadDrain = $true
    $TitleAudioDrain = $true
    $TitleGpuDrain = $true
    $TitleDispatchDrain = $true
    $TitleRuntimeTeardown = $true
    $TitleInProcessRestart = $true
}
$taskUserRoot = Join-Path $ProjectRoot 'userdata'
$taskProfile = if ($DebugMission) { $DebugMission } else { $DebugProfile }
if ($taskProfile) { $taskUserRoot = Join-Path $taskUserRoot "debug-levels/$taskProfile" }
$arguments = @('--gpu_plugin',$taskGpuPlugin,'--game_data_root',('"{0}"' -f "$ProjectRoot/assets/game"),
    '--user_data_root',('"{0}"' -f $taskUserRoot),
    '--log_file',('"{0}"' -f $logPath),'--log_level','info')
if ($Headless) { $arguments += '--headless' }
if ($CooperativeWorkerExit) { $arguments += '--cod3_worker_cooperative_exit' }
if ($TitleThreadDrain) {
    if (!$CooperativeWorkerExit) { throw 'TitleThreadDrain requires CooperativeWorkerExit.' }
    $arguments += '--cod3_title_thread_drain'
}
if ($TitleAudioDrain) {
    if (!$TitleThreadDrain) { throw 'TitleAudioDrain requires TitleThreadDrain.' }
    $arguments += '--cod3_title_audio_drain'
}
if ($TitleGpuDrain) {
    if (!$TitleAudioDrain -or $taskGpuPlugin -ne 'xenos-grass' -or $Configuration -ne 'RelWithDebInfo') {
        throw 'TitleGpuDrain requires TitleAudioDrain and the corrected RelWithDebInfo GPU plugin.'
    }
    $arguments += '--cod3_title_gpu_drain'
}
if ($TitleDispatchDrain) {
    if (!$TitleGpuDrain) { throw 'TitleDispatchDrain requires TitleGpuDrain.' }
    $arguments += '--cod3_title_dispatch_drain'
}
if ($TransitionDump) { $arguments += '--cod3_transition_dump' }
if ($TitleRuntimeTeardown) {
    if (!$TitleDispatchDrain) { throw 'TitleRuntimeTeardown requires TitleDispatchDrain.' }
    $arguments += '--cod3_title_runtime_teardown'
}
if ($TitleInProcessRestart) {
    if (!$TitleRuntimeTeardown) { throw 'TitleInProcessRestart requires TitleRuntimeTeardown.' }
    $arguments += '--cod3_title_inprocess_restart'
}
# Explicit false values override saved SDK settings during diagnostic launches.
foreach ($taskTransitionFlag in @(
    @('cod3_worker_cooperative_exit', $CooperativeWorkerExit),
    @('cod3_title_thread_drain', $TitleThreadDrain),
    @('cod3_title_audio_drain', $TitleAudioDrain),
    @('cod3_title_gpu_drain', $TitleGpuDrain),
    @('cod3_title_dispatch_drain', $TitleDispatchDrain),
    @('cod3_title_runtime_teardown', $TitleRuntimeTeardown),
    @('cod3_title_inprocess_restart', $TitleInProcessRestart)
)) {
    if (!$taskTransitionFlag[1]) { $arguments += "--$($taskTransitionFlag[0])=false" }
}
if ($Interactive) { $arguments += @('--mnk_mode','true') }
$arguments += "--cod3_gpu_query_trace=$($GpuQueryTrace.ToString().ToLowerInvariant())"
if ($FirstMission) { $arguments += @('--cl','"+spmap saint_lo"') }
if ($DebugMission) { $arguments += @('--cl', ('"+devmap {0}"' -f $taskSelectedLevel[0].map)) }
if ($ShaderDiagnostics) {
    $taskShaderDump = Join-Path $ProjectRoot 'reports/shader-diagnostics'
    New-Item -ItemType Directory -Force $taskShaderDump | Out-Null
    $arguments += ('--dump_shaders="{0}"' -f $taskShaderDump)
}
# Diagnostic comparison: retain vegetation and synchronously copy GPU-exported
# data back to guest memory, instead of the SDK's delayed double-buffered path.
if ($SynchronousMemexport) {
    # Plugin CVars register after CLI parsing: values must use --name=value.
    $arguments += @('--readback_memexport=true','--readback_memexport_fast=false')
}
# Original flare visibility reads resolved depth in guest memory. With no
# readback, glows remain visible through walls. Fast retains the SDK's delayed
# path; full is a diagnostic comparison, not a forced synchronous default.
$arguments += "--readback_resolve=$ResolveReadback"
if ($FullGpuReadback) {
    $arguments += @('--readback_memexport=true','--readback_memexport_fast=false')
}
if ($taskGpuPlugin -eq 'xenos-grass') {
    $arguments += @('--cod3_grass_scalar_pair_fix=true', "--cod3_dof_strength=$DofStrength")
}
if ($OcclusionTrace) {
    if ($taskGpuPlugin -ne 'xenos-grass') { throw 'OcclusionTrace requires the corrected GPU plugin.' }
    $arguments += '--cod3_occlusion_trace=true'
} elseif ($taskGpuPlugin -eq 'xenos-grass') {
    $arguments += '--cod3_occlusion_trace=false'
}
if ($ShadowDepthSource -and $OriginalShadowDepth) {
    throw 'ShadowDepthSource and OriginalShadowDepth select different comparison modes.'
}
if ($ShadowDepthSource -and $taskGpuPlugin -ne 'xenos-grass') {
    throw 'ShadowDepthSource requires the corrected GPU plugin.'
}
if ($taskGpuPlugin -eq 'xenos-grass') {
    # Accepted in Night Drop: wall leakage gone, ordinary character shadows retained.
    $arguments += "--cod3_shadow_depth_source=$((!$OriginalShadowDepth).ToString().ToLowerInvariant())"
}
if ($OcclusionSnapshots) {
    if ($taskGpuPlugin -ne 'xenos-grass') { throw 'OcclusionSnapshots requires the corrected GPU plugin.' }
    $arguments += '--cod3_occlusion_snapshots=true'
} elseif ($taskGpuPlugin -eq 'xenos-grass') {
    $arguments += '--cod3_occlusion_snapshots=false'
}
$windowStyle = if ($Interactive) { 'Normal' } else { 'Hidden' }
if ($DryRun) {
    [pscustomobject]@{executable=$app;arguments=$arguments;debug_mission=$DebugMission;user_data_root=$taskUserRoot;resolve_readback=$ResolveReadback} |
        ConvertTo-Json -Depth 3
    return
}
if (@(Get-Process -Name cod3pc -ErrorAction SilentlyContinue).Count -gt 0) {
    throw 'cod3pc is already running. Close the existing game before launching another instance.'
}
if ($taskProfile) {
    # Explicit debug launches bypass progression with the game's own command.
    # Isolate all autosaves, profiles and caches from the normal campaign.
    New-Item -ItemType Directory -Force $taskUserRoot | Out-Null
    Write-Output "Isolated profile: $taskUserRoot; direct map: $DebugMission"
}
$proc = Start-Process -FilePath $app -ArgumentList $arguments -WorkingDirectory (Split-Path $app) -WindowStyle $windowStyle -PassThru
Write-Output "Started native cod3pc PID $($proc.Id)"
Write-Output "DoF strength: $DofStrength% (100 = original)"
Write-Output "Resolve readback: $ResolveReadback"
Write-Output "Title transitions: $(if ($TitleInProcessRestart) { 'same process/window' } else { 'diagnostic process relaunch' })"
New-Item -ItemType Directory -Force "$ProjectRoot/reports" | Out-Null
[pscustomobject]@{pid=$proc.Id;log=$logPath;started=[DateTime]::UtcNow.ToString('o');dof_strength=$DofStrength;debug_mission=$DebugMission;user_data_root=$taskUserRoot;resolve_readback=$ResolveReadback} |
    ConvertTo-Json | Set-Content "$ProjectRoot/reports/native-last-run.json" -Encoding utf8
if ($Seconds -gt 0) {
    if (!$proc.WaitForExit($Seconds * 1000)) {
        # Stop only this process, launched above, after the bounded smoke test.
        Stop-Process -Id $proc.Id
        Write-Output 'Smoke test time limit reached; process stopped.'
    } else {
        Write-Output "Exit code: $($proc.ExitCode)"
        if ($proc.ExitCode -ne 0) { throw "Native process exited with code $($proc.ExitCode); see $logPath" }
    }
    Get-Content -LiteralPath $logPath -Tail 25 -ErrorAction SilentlyContinue
    if (Select-String -LiteralPath $logPath -Pattern '\[FATAL\]' -Quiet) { throw "Native runtime failed; see $logPath" }
    if (Select-String -LiteralPath $logPath -Pattern 'Unhandled guest access violation' -Quiet) { throw "Guest access violation; see $logPath and cod3pc-crash.txt" }
    if (!(Select-String -LiteralPath $logPath -Pattern 'COD3_ORIGINAL_ENTRY_ENTER' -Quiet)) { throw 'Original entry was not confirmed' }
}
