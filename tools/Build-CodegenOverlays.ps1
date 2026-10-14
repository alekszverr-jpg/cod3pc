. "$PSScriptRoot/Enter-Toolchain.ps1"
$taskCodegenRoot = Join-Path $ProjectRoot 'external/rexglue-sdk'
if ((& $Git -C $taskCodegenRoot rev-parse HEAD) -ne 'c94f5ebdcb3c9d1a460ca48e04f9758448f8d518') {
    throw 'Overlay codegen requires the pinned ReXGlue 0.10.0 source.'
}
$taskOverlayPatch = Join-Path $PSScriptRoot 'patches/rexglue-v0100-dll-overlay-groups.patch'
& $Git -C $taskCodegenRoot apply --reverse --check --ignore-whitespace $taskOverlayPatch 2>$null
if ($LASTEXITCODE -ne 0) {
    & $Git -C $taskCodegenRoot apply --check --ignore-whitespace $taskOverlayPatch
    if ($LASTEXITCODE -ne 0) { throw 'Codegen source differs from the reviewed overlay patch.' }
    & $Git -C $taskCodegenRoot apply --ignore-whitespace $taskOverlayPatch
    if ($LASTEXITCODE -ne 0) { throw 'Applying overlay group patch failed.' }
}
if (!(Test-Path "$taskCodegenRoot/out/build/win-amd64/CMakeCache.txt")) {
    Push-Location $taskCodegenRoot
    try { & $CMake --preset win-amd64 } finally { Pop-Location }
    if ($LASTEXITCODE -ne 0) { throw 'Overlay codegen configuration failed.' }
}
& $CMake --build "$taskCodegenRoot/out/build/win-amd64" --config RelWithDebInfo --target rexglue --parallel 4 `
    2>&1 | Tee-Object "$ProjectRoot/logs/codegen-overlays-build.log"
if ($LASTEXITCODE -ne 0) { throw 'Overlay codegen build failed.' }
