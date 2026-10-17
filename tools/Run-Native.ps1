param([int]$Seconds = 30, [switch]$Headless, [switch]$Interactive,
    [ValidateSet('Debug','RelWithDebInfo','Release')][string]$Configuration = 'RelWithDebInfo',
    [switch]$FirstMission, [switch]$SynchronousMemexport, [switch]$GpuCoherencyTest,
    [switch]$FullGpuReadback, [switch]$GrassShaderTest, [switch]$StockGpu,
    [switch]$ShaderDiagnostics, [switch]$CooperativeWorkerExit, [switch]$TransitionDump,
    [switch]$TitleThreadDrain,
    [ValidateRange(0,100)][int]$DofStrength = 100,
    [ValidatePattern('^[a-z][a-z0-9_]*$')][string]$DebugMission,
    [ValidatePattern('^[a-z][a-z0-9_]*$')][string]$DebugProfile,
    [switch]$ListLevels, [switch]$DryRun)
. "$PSScriptRoot/Enter-Toolchain.ps1"
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
if ($TransitionDump) { $arguments += '--cod3_transition_dump' }
if ($Interactive) { $arguments += @('--mnk_mode','true') }
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
if ($FullGpuReadback) {
    $arguments += @('--readback_resolve=full','--readback_memexport=true','--readback_memexport_fast=false')
}
if ($taskGpuPlugin -eq 'xenos-grass') {
    $arguments += @('--cod3_grass_scalar_pair_fix=true', "--cod3_dof_strength=$DofStrength")
}
$windowStyle = if ($Interactive) { 'Normal' } else { 'Hidden' }
if ($DryRun) {
    [pscustomobject]@{executable=$app;arguments=$arguments;debug_mission=$DebugMission;user_data_root=$taskUserRoot} |
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
New-Item -ItemType Directory -Force "$ProjectRoot/reports" | Out-Null
[pscustomobject]@{pid=$proc.Id;log=$logPath;started=[DateTime]::UtcNow.ToString('o');dof_strength=$DofStrength;debug_mission=$DebugMission;user_data_root=$taskUserRoot} |
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
