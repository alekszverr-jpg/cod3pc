$ErrorActionPreference = 'Stop'
$script:ProjectRoot = Split-Path $PSScriptRoot -Parent
$script:SdkPrefix = Join-Path $ProjectRoot 'external/sdk-v0.10.0/win-amd64'
$script:Rexglue = Join-Path $SdkPrefix 'bin/rexglue.exe'
$taskOverlayCodegen = Join-Path $ProjectRoot 'external/rexglue-sdk/out/win-amd64/RelWithDebInfo/rexgluerd.exe'
if (Test-Path -LiteralPath $taskOverlayCodegen) { $script:Rexglue = $taskOverlayCodegen }
$script:CMake = Join-Path $ProjectRoot 'external/python-tools/cmake/data/bin/cmake.exe'
$script:Ninja = Join-Path $ProjectRoot 'external/python-tools/bin/ninja.exe'
$script:Git = 'C:/Program Files/Git/cmd/git.exe'
$script:Python = Join-Path $env:USERPROFILE '.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
if (!(Test-Path -LiteralPath $Python)) { $script:Python = (Get-Command python -ErrorAction Stop).Source }
function Set-Cod3ToolPath([string[]]$Prepend = @()) {
    $taskSeenPaths = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    $env:PATH = (@($Prepend) + @($env:PATH -split ';') | Where-Object {
        $_ -and $taskSeenPaths.Add($_.Replace('/','\').TrimEnd('\'))
    }) -join ';'
}
Set-Cod3ToolPath @((Split-Path $CMake),(Split-Path $Ninja),(Split-Path $Git),"$SdkPrefix/bin")
$script:VsRoot = $null
$vswhere = 'C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe'
if (Test-Path -LiteralPath $vswhere) {
    $script:VsRoot = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
}
if ($VsRoot) {
    $vcvars = Join-Path $VsRoot 'VC/Auxiliary/Build/vcvars64.bat'
    # This command only imports environment variables; paths come from vswhere.
    # Build helpers source this file again in the same PowerShell process.
    # Re-running vcvars grows PATH/INCLUDE until cmd.exe rejects its own line.
    $taskVsReady = $env:VSCMD_ARG_TGT_ARCH -eq 'x64' -and $env:INCLUDE -and
        $env:VSINSTALLDIR -and $env:VSINSTALLDIR.TrimEnd('\') -ieq $VsRoot.TrimEnd('\')
    if (!$taskVsReady) {
        $vcEnvironment = & $env:ComSpec /d /c "call `"$vcvars`" >nul && set"
        if ($LASTEXITCODE -ne 0) { throw 'vcvars64 failed' }
        foreach ($line in $vcEnvironment) {
            if ($line -match '^([^=]+)=(.*)$') {
                [Environment]::SetEnvironmentVariable($matches[1], $matches[2], 'Process')
            }
        }
    }
    $llvm = Join-Path $VsRoot 'VC/Tools/Llvm/x64/bin'
    if (!(Test-Path -LiteralPath "$llvm/clang++.exe")) { $llvm = Join-Path $VsRoot 'VC/Tools/Llvm/bin' }
    if (Test-Path -LiteralPath "$llvm/clang++.exe") { Set-Cod3ToolPath @($llvm) }
}
if (Test-Path -LiteralPath 'C:/Program Files/LLVM/bin/clang++.exe') {
    Set-Cod3ToolPath @('C:/Program Files/LLVM/bin')
}
$jdkRoot = 'C:/Program Files/Eclipse Adoptium'
if (Test-Path -LiteralPath $jdkRoot) {
    $jdk = Get-ChildItem -LiteralPath $jdkRoot -Directory | Where-Object Name -Like 'jdk-21*' | Select-Object -First 1
    if ($jdk) { $env:JAVA_HOME = $jdk.FullName; Set-Cod3ToolPath @("$($jdk.FullName)/bin") }
}
Set-Cod3ToolPath
