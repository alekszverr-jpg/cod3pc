param([int]$Seconds = 30, [switch]$Headless, [switch]$Interactive,
    [ValidateSet('Debug','RelWithDebInfo','Release')][string]$Configuration = 'RelWithDebInfo',
    [switch]$FirstMission, [switch]$SynchronousMemexport, [switch]$GpuCoherencyTest,
    [switch]$FullGpuReadback, [switch]$GrassShaderTest)
. "$PSScriptRoot/Enter-Toolchain.ps1"
$app = Join-Path $ProjectRoot ('cod3pc/out/build/win-amd64-' + $Configuration.ToLowerInvariant() + '/cod3pc.exe')
if (!(Test-Path -LiteralPath $app)) { throw 'Build cod3pc first' }
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss-fff'
$logPath = Join-Path $ProjectRoot "logs/native-$stamp.log"
$taskGpuPlugin = if ($GrassShaderTest) { 'xenos-grass' } elseif ($GpuCoherencyTest) { 'xenos-coherency' } else { 'xenos' }
if ($GrassShaderTest -and $GpuCoherencyTest) { throw 'Select one GPU test plugin.' }
if ($GrassShaderTest -and $Configuration -ne 'RelWithDebInfo') {
    throw 'The grass shader test plugin is staged only for RelWithDebInfo.'
}
if ($GpuCoherencyTest -and $Configuration -ne 'RelWithDebInfo') {
    throw 'The GPU coherency test plugin is currently staged only for RelWithDebInfo.'
}
$arguments = @('--gpu_plugin',$taskGpuPlugin,'--game_data_root',('"{0}"' -f "$ProjectRoot/assets/game"),
    '--user_data_root',('"{0}"' -f "$ProjectRoot/userdata"),
    '--log_file',('"{0}"' -f $logPath),'--log_level','info')
if ($Headless) { $arguments += '--headless' }
if ($Interactive) { $arguments += @('--mnk_mode','true') }
if ($FirstMission) { $arguments += @('--cl','"+spmap saint_lo"') }
# Diagnostic comparison: retain vegetation and synchronously copy GPU-exported
# data back to guest memory, instead of the SDK's delayed double-buffered path.
if ($SynchronousMemexport) {
    # Plugin CVars register after CLI parsing: values must use --name=value.
    $arguments += @('--readback_memexport=true','--readback_memexport_fast=false')
}
if ($FullGpuReadback) {
    $arguments += @('--readback_resolve=full','--readback_memexport=true','--readback_memexport_fast=false')
}
if ($GrassShaderTest) { $arguments += '--cod3_grass_position_fix=true' }
$windowStyle = if ($Interactive) { 'Normal' } else { 'Hidden' }
$proc = Start-Process -FilePath $app -ArgumentList $arguments -WorkingDirectory (Split-Path $app) -WindowStyle $windowStyle -PassThru
Write-Output "Started native cod3pc PID $($proc.Id)"
New-Item -ItemType Directory -Force "$ProjectRoot/reports" | Out-Null
[pscustomobject]@{pid=$proc.Id;log=$logPath;started=[DateTime]::UtcNow.ToString('o')} |
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
