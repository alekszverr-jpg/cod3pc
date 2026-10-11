$ErrorActionPreference = 'Stop'
$script:ProjectRoot = Split-Path $PSScriptRoot -Parent
$script:SdkPrefix = Join-Path $ProjectRoot 'external/sdk-v0.10.0/win-amd64'
$script:Rexglue = Join-Path $SdkPrefix 'bin/rexglue.exe'
$script:CMake = Join-Path $ProjectRoot 'external/python-tools/cmake/data/bin/cmake.exe'
$script:Ninja = Join-Path $ProjectRoot 'external/python-tools/bin/ninja.exe'
$script:Git = 'C:/Program Files/Git/cmd/git.exe'
$script:Python = Join-Path $env:USERPROFILE '.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
if (!(Test-Path -LiteralPath $Python)) { $script:Python = (Get-Command python -ErrorAction Stop).Source }
$env:PATH = "$(Split-Path $CMake);$(Split-Path $Ninja);$(Split-Path $Git);$SdkPrefix/bin;$env:PATH"
$script:VsRoot = $null
$vswhere = 'C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe'
if (Test-Path -LiteralPath $vswhere) {
    $script:VsRoot = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
}
if ($VsRoot) {
    $vcvars = Join-Path $VsRoot 'VC/Auxiliary/Build/vcvars64.bat'
    # This command only imports environment variables; paths come from vswhere.
    $vcEnvironment = & $env:ComSpec /d /c "call `"$vcvars`" >nul && set"
    if ($LASTEXITCODE -ne 0) { throw 'vcvars64 failed' }
    foreach ($line in $vcEnvironment) {
        if ($line -match '^([^=]+)=(.*)$') {
            [Environment]::SetEnvironmentVariable($matches[1], $matches[2], 'Process')
        }
    }
    $llvm = Join-Path $VsRoot 'VC/Tools/Llvm/x64/bin'
    if (!(Test-Path -LiteralPath "$llvm/clang++.exe")) { $llvm = Join-Path $VsRoot 'VC/Tools/Llvm/bin' }
    if (Test-Path -LiteralPath "$llvm/clang++.exe") { $env:PATH = "$llvm;$env:PATH" }
}
if (Test-Path -LiteralPath 'C:/Program Files/LLVM/bin/clang++.exe') {
    $env:PATH = "C:/Program Files/LLVM/bin;$env:PATH"
}
