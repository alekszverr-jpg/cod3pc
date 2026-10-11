param([int]$Seconds = 30, [switch]$Headless)
. "$PSScriptRoot/Enter-Toolchain.ps1"
$app = Join-Path $ProjectRoot 'cod3pc/out/build/win-amd64-debug/cod3pc.exe'
if (!(Test-Path -LiteralPath $app)) { throw 'Build cod3pc first' }
$arguments = @('--gpu_plugin','xenos','--game_data_root',('"{0}"' -f "$ProjectRoot/assets/game"),
    '--user_data_root',('"{0}"' -f "$ProjectRoot/userdata"),
    '--log_file',('"{0}"' -f "$ProjectRoot/logs/native-run.log"),'--log_level','debug')
if ($Headless) { $arguments += '--headless' }
$proc = Start-Process -FilePath $app -ArgumentList $arguments -WorkingDirectory (Split-Path $app) -WindowStyle Hidden -PassThru
Write-Output "Started native cod3pc PID $($proc.Id)"
if ($Seconds -gt 0) {
    if (!$proc.WaitForExit($Seconds * 1000)) {
        # Stop only this process, launched above, after the bounded smoke test.
        Stop-Process -Id $proc.Id
        Write-Output 'Smoke test time limit reached; process stopped.'
    } else { Write-Output "Exit code: $($proc.ExitCode)" }
    Get-Content "$ProjectRoot/logs/native-run.log" -Tail 25 -ErrorAction SilentlyContinue
}
