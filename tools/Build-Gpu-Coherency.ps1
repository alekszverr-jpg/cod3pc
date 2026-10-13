param([ValidateSet('Build')][string]$Stage = 'Build')
. "$PSScriptRoot/Enter-Toolchain.ps1"
$taskSdkRoot = Join-Path $ProjectRoot 'external/rexglue-sdk'
$taskPinnedSdk = 'c94f5ebdcb3c9d1a460ca48e04f9758448f8d518'
$taskSdkHead = & $Git -C $taskSdkRoot rev-parse HEAD
if ($LASTEXITCODE -ne 0 -or $taskSdkHead -ne $taskPinnedSdk) {
    throw 'GPU patch requires the reviewed ReXGlue 0.10.0 source commit.'
}
$taskPatch = Join-Path $PSScriptRoot 'patches/rexglue-v0100-vertex-residency.patch'
foreach ($taskGrassPatchName in @('rexglue-v0100-cod3-grass-alu.patch','rexglue-v0100-cod3-grass-position.patch')) {
    $taskGrassPatch = Join-Path $PSScriptRoot "patches/$taskGrassPatchName"
    & $Git -C $taskSdkRoot apply --reverse --check --ignore-whitespace $taskGrassPatch 2>$null
    if ($LASTEXITCODE -eq 0) {
        & $Git -C $taskSdkRoot apply --reverse --ignore-whitespace $taskGrassPatch
        if ($LASTEXITCODE -ne 0) { throw 'Reverting the separate grass compatibility patch failed.' }
    }
}
& $Git -C $taskSdkRoot apply --reverse --check --ignore-whitespace $taskPatch 2>$null
if ($LASTEXITCODE -ne 0) {
    & $Git -C $taskSdkRoot apply --check --ignore-whitespace $taskPatch
    if ($LASTEXITCODE -ne 0) { throw 'GPU source differs from the reviewed patch context.' }
    & $Git -C $taskSdkRoot apply --ignore-whitespace $taskPatch
    if ($LASTEXITCODE -ne 0) { throw 'Applying GPU patch failed.' }
}
& $CMake --build "$taskSdkRoot/out/build/win-amd64" --config RelWithDebInfo --target rexgpu-xenos --parallel 4 `
    2>&1 | Tee-Object "$ProjectRoot/logs/gpu-coherency-build.log"
if ($LASTEXITCODE -ne 0) { throw 'GPU build failed.' }
$taskGpuSource = Join-Path $taskSdkRoot 'out/win-amd64/RelWithDebInfo/rexgpu-xenosrd.dll'
$taskGpuTarget = Join-Path $ProjectRoot 'cod3pc/out/build/win-amd64-relwithdebinfo/rexgpu-xenos-coherencyrd.dll'
if (!(Test-Path -LiteralPath $taskGpuSource)) { throw 'Build the RelWithDebInfo GPU plugin first.' }
Copy-Item -LiteralPath $taskGpuSource -Destination $taskGpuTarget -Force
Get-FileHash -LiteralPath $taskGpuTarget -Algorithm SHA256
Write-Output 'Test with Run-Native.ps1 -GpuCoherencyTest. The stock xenos plugin remains available.'
