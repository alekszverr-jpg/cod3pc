param([ValidateSet('Debug','RelWithDebInfo','Release')][string]$Configuration = 'RelWithDebInfo')
. "$PSScriptRoot/Enter-Toolchain.ps1"
$buildDirectory = Join-Path $ProjectRoot ('cod3pc/out/build/win-amd64-' + $Configuration.ToLowerInvariant())
$dump = Join-Path $buildDirectory 'cod3pc-crash.dmp'
if (!(Test-Path -LiteralPath $dump)) { throw 'No local crash dump for this configuration.' }
# LLVM LLDB on Windows needs the Python runtime DLL even in batch mode.
$env:PATH = "$(Split-Path $Python);$env:PATH"
& 'C:/Program Files/LLVM/bin/lldb.exe' --batch --file "$buildDirectory/cod3pc.exe" --core $dump `
    -o 'thread backtrace -c 24' -o 'register read' 2>&1 | Tee-Object "$ProjectRoot/logs/crash-lldb.log"
if ($LASTEXITCODE -ne 0) { throw 'LLDB dump analysis failed' }
