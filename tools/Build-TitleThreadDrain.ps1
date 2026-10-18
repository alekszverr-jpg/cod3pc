. "$PSScriptRoot/Enter-Toolchain.ps1"
$taskSdkRoot = Join-Path $ProjectRoot 'external/rexglue-sdk'
if ((& $Git -C $taskSdkRoot rev-parse HEAD) -ne 'c94f5ebdcb3c9d1a460ca48e04f9758448f8d518') {
    throw 'Title thread drain requires pinned ReXGlue 0.10.0.'
}
foreach ($taskPatchName in @('rexglue-v0100-title-thread-drain.patch', 'rexglue-v0100-title-audio-drain.patch', 'rexglue-v0100-title-dispatch-drain.patch', 'rexglue-v0100-thread-exit-ownership.patch', 'rexglue-v0100-start-routine-ownership.patch', 'rexglue-v0100-title-runtime-teardown.patch')) {
    $taskPatch = Join-Path $PSScriptRoot "patches/$taskPatchName"
    & $Git -C $taskSdkRoot apply --reverse --check --ignore-whitespace $taskPatch 2>$null
    if ($LASTEXITCODE -ne 0) {
        & $Git -C $taskSdkRoot apply --check --ignore-whitespace $taskPatch
        if ($LASTEXITCODE -ne 0) { throw "Lifecycle patch context differs: $taskPatchName" }
        & $Git -C $taskSdkRoot apply --ignore-whitespace $taskPatch
        if ($LASTEXITCODE -ne 0) { throw "Applying lifecycle patch failed: $taskPatchName" }
    }
}
& $CMake --build "$taskSdkRoot/out/build/win-amd64" --config RelWithDebInfo --target rexruntime --parallel 4 `
    2>&1 | Tee-Object "$ProjectRoot/logs/title-thread-drain-sdk-build.log"
if ($LASTEXITCODE -ne 0) { throw 'Title thread drain SDK build failed.' }
# Stage matched runtime, import library, symbols and headers as one version.
# The application's post-build dependency copy supplies its matching runtime.
$taskOutputs = @(
    @('out/win-amd64/RelWithDebInfo/rexruntimerd.dll', 'bin/rexruntimerd.dll'),
    @('out/win-amd64/RelWithDebInfo/rexruntimerd.lib', 'lib/rexruntimerd.lib'),
    @('out/win-amd64/RelWithDebInfo/rexruntimerd.pdb', 'bin/rexruntimerd.pdb'),
    @('include/rex/system/kernel_state.h', 'include/rex/system/kernel_state.h'),
    @('include/rex/system/title_thread_gate.h', 'include/rex/system/title_thread_gate.h'),
    @('include/rex/thread/fiber.h', 'include/rex/thread/fiber.h'),
    @('include/rex/rex_app.h', 'include/rex/rex_app.h'),
    @('src/ui/rex_app.cpp', 'share/rexglue/rex_app.cpp'),
    @('include/rex/audio/audio_system.h', 'include/rex/audio/audio_system.h'),
    @('include/rex/audio/xma/decoder.h', 'include/rex/audio/xma/decoder.h')
)
$taskBackup = Join-Path $ProjectRoot 'reports/title-thread-drain/previous-sdk'
foreach ($taskPair in $taskOutputs) {
    $taskSource = Join-Path $taskSdkRoot $taskPair[0]
    $taskDestination = Join-Path $SdkPrefix $taskPair[1]
    $taskPrevious = Join-Path $taskBackup $taskPair[1]
    if ((Test-Path -LiteralPath $taskDestination) -and
        (Get-FileHash -LiteralPath $taskSource).Hash -eq (Get-FileHash -LiteralPath $taskDestination).Hash) {
        continue
    }
    if ((Test-Path -LiteralPath $taskDestination) -and !(Test-Path -LiteralPath $taskPrevious)) {
        New-Item -ItemType Directory -Force (Split-Path $taskPrevious) | Out-Null
        Copy-Item -LiteralPath $taskDestination -Destination $taskPrevious -ErrorAction Stop
    }
    Copy-Item -LiteralPath $taskSource -Destination $taskDestination -Force -ErrorAction Stop
}
Get-FileHash -LiteralPath (Join-Path $SdkPrefix 'bin/rexruntimerd.dll') -Algorithm SHA256
