. "$PSScriptRoot/Enter-Toolchain.ps1"
$taskSdkRoot = Join-Path $ProjectRoot 'external/rexglue-sdk'
$taskPinnedSdk = 'c94f5ebdcb3c9d1a460ca48e04f9758448f8d518'
$taskSdkHead = & $Git -C $taskSdkRoot rev-parse HEAD
if ($LASTEXITCODE -ne 0 -or $taskSdkHead -ne $taskPinnedSdk) {
    throw 'Grass shader experiment requires the reviewed ReXGlue source commit.'
}
# The optional diagnostics extend the same lifecycle methods as title drain.
# Temporarily reverse this exact reviewed layer before validating its base,
# then reapply below. Never bypass the base patch verification.
$taskOcclusionTracePatch = Join-Path $PSScriptRoot 'patches/rexglue-v0100-cod3-occlusion-trace.patch'
& $Git -C $taskSdkRoot apply --reverse --check --ignore-whitespace $taskOcclusionTracePatch 2>$null
if ($LASTEXITCODE -eq 0) {
    & $Git -C $taskSdkRoot apply --reverse --ignore-whitespace $taskOcclusionTracePatch
    if ($LASTEXITCODE -ne 0) { throw 'Reversing the exact optional GPU diagnostic layer failed.' }
}
$taskPatch = Join-Path $PSScriptRoot 'patches/rexglue-v0100-cod3-grass-alu.patch'
$taskOldPositionPatch = Join-Path $PSScriptRoot 'patches/rexglue-v0100-cod3-grass-position.patch'
& $Git -C $taskSdkRoot apply --reverse --check --ignore-whitespace $taskOldPositionPatch 2>$null
if ($LASTEXITCODE -eq 0) {
    & $Git -C $taskSdkRoot apply --reverse --ignore-whitespace $taskOldPositionPatch
    if ($LASTEXITCODE -ne 0) { throw 'Reverting the earlier homogeneous-W experiment failed.' }
}
# Do not combine this experiment with the failed vertex-residency experiment.
$taskResidencyPatch = Join-Path $PSScriptRoot 'patches/rexglue-v0100-vertex-residency.patch'
& $Git -C $taskSdkRoot apply --reverse --check --ignore-whitespace $taskResidencyPatch 2>$null
if ($LASTEXITCODE -eq 0) {
    & $Git -C $taskSdkRoot apply --reverse --ignore-whitespace $taskResidencyPatch
    if ($LASTEXITCODE -ne 0) { throw 'Reverting the separate GPU coherency experiment failed.' }
}
& $Git -C $taskSdkRoot apply --reverse --check --ignore-whitespace $taskPatch 2>$null
if ($LASTEXITCODE -ne 0) {
    & $Git -C $taskSdkRoot apply --check --ignore-whitespace $taskPatch
    if ($LASTEXITCODE -ne 0) { throw 'GPU source differs from the reviewed patch context.' }
    & $Git -C $taskSdkRoot apply --ignore-whitespace $taskPatch
    if ($LASTEXITCODE -ne 0) { throw 'Applying grass shader patch failed.' }
}
$taskDofPatch = Join-Path $PSScriptRoot 'patches/rexglue-v0100-cod3-dof-strength.patch'
& $Git -C $taskSdkRoot apply --reverse --check --ignore-whitespace $taskDofPatch 2>$null
if ($LASTEXITCODE -ne 0) {
    & $Git -C $taskSdkRoot apply --check --ignore-whitespace $taskDofPatch
    if ($LASTEXITCODE -ne 0) { throw 'GPU source differs from the reviewed DoF setting patch.' }
    & $Git -C $taskSdkRoot apply --ignore-whitespace $taskDofPatch
    if ($LASTEXITCODE -ne 0) { throw 'Applying DoF setting patch failed.' }
}
$taskDrainPatch = Join-Path $PSScriptRoot 'patches/rexglue-v0100-title-gpu-drain.patch'
& $Git -C $taskSdkRoot apply --reverse --check --ignore-whitespace $taskDrainPatch 2>$null
if ($LASTEXITCODE -ne 0) {
    & $Git -C $taskSdkRoot apply --check --ignore-whitespace $taskDrainPatch
    if ($LASTEXITCODE -ne 0) { throw 'GPU lifecycle patch context differs.' }
    & $Git -C $taskSdkRoot apply --ignore-whitespace $taskDrainPatch
    if ($LASTEXITCODE -ne 0) { throw 'Applying GPU lifecycle patch failed.' }
}
$taskOcclusionTracePatch = Join-Path $PSScriptRoot 'patches/rexglue-v0100-cod3-occlusion-trace.patch'
& $Git -C $taskSdkRoot apply --reverse --check --ignore-whitespace $taskOcclusionTracePatch 2>$null
if ($LASTEXITCODE -ne 0) {
    & $Git -C $taskSdkRoot apply --check --ignore-whitespace $taskOcclusionTracePatch
    if ($LASTEXITCODE -ne 0) { throw 'GPU occlusion trace patch context differs.' }
    & $Git -C $taskSdkRoot apply --ignore-whitespace $taskOcclusionTracePatch
    if ($LASTEXITCODE -ne 0) { throw 'Applying GPU occlusion trace patch failed.' }
}
& $CMake --build "$taskSdkRoot/out/build/win-amd64" --config RelWithDebInfo --target rexgpu-xenos --parallel 4 `
    2>&1 | Tee-Object "$ProjectRoot/logs/gpu-grass-build.log"
if ($LASTEXITCODE -ne 0) { throw 'Grass test GPU build failed.' }
$taskGpuSource = Join-Path $taskSdkRoot 'out/win-amd64/RelWithDebInfo/rexgpu-xenosrd.dll'
$taskGpuTarget = Join-Path $ProjectRoot 'cod3pc/out/build/win-amd64-relwithdebinfo/rexgpu-xenos-grassrd.dll'
Copy-Item -LiteralPath $taskGpuSource -Destination $taskGpuTarget -Force -ErrorAction Stop
if ((Get-FileHash -LiteralPath $taskGpuSource).Hash -ne (Get-FileHash -LiteralPath $taskGpuTarget).Hash) {
    throw 'Staged GPU plugin hash differs from the built plugin.'
}
Get-FileHash -LiteralPath $taskGpuTarget -Algorithm SHA256
Write-Output 'Corrected grass plugin staged. Stock comparison: Run-Native.ps1 -StockGpu.'
