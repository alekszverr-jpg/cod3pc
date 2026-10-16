param([ValidateSet('SDK','Codegen','Diagnostic','Build','Analyze','Tests')][string]$Stage = 'Build',
    [ValidateSet('Debug','RelWithDebInfo','Release')][string]$Configuration = 'RelWithDebInfo')
. "$PSScriptRoot/Enter-Toolchain.ps1"
New-Item -ItemType Directory -Force "$ProjectRoot/logs" | Out-Null
function Invoke-Logged([string]$Exe, [string[]]$Arguments, [string]$Log) {
    & $Exe @Arguments 2>&1 | Tee-Object -FilePath "$ProjectRoot/logs/$Log"
    if ($LASTEXITCODE -ne 0) { throw "$Exe failed ($LASTEXITCODE); see logs/$Log" }
}
Push-Location $ProjectRoot
try {
    if ($Stage -in @('Codegen','Diagnostic','Build')) {
        & "$PSScriptRoot/Build-CodegenOverlays.ps1"
        $script:Rexglue = Join-Path $ProjectRoot 'external/rexglue-sdk/out/win-amd64/RelWithDebInfo/rexgluerd.exe'
        $xex = Join-Path $ProjectRoot 'assets/game/default.xex'
        # Original and reviewed Russian 098 variants. Ghidra comparison:
        # objective branch at 824B5EAC and optional loading text at 82068D98.
        # Section layout and function boundaries match; see docs/LOCALIZATION_098.md.
        $expected = @(
            '2944EEC7D1231AD6798B5F9F8ADF8855F5E489296B22EAB45B27A577CEE23692',
            'B176596E2F206E6FDF0D3CDAE10C44957C3DA559C2B838F3DFD1D8F302EDD23B',
            'E057FFCD129AF913B9C7334C847F749115FA6A4E3C0A9EDACDD2DB728F003682'
        )
        if (!(Test-Path -LiteralPath $xex) -or (Get-FileHash -LiteralPath $xex -Algorithm SHA256).Hash -notin $expected) {
            throw 'default.xex differs from the reviewed original/Russian-098 SHA-256 values; reanalyze before using function overrides.'
        }
        $levelXex = Join-Path $ProjectRoot 'assets/game/sp/saint_lo/saint_lo.dll'
        $levelHash = '49E38A116B8F3C5E81C5206FD461496A6DA518CF9427843B38F2CAD33910EF2B'
        if (!(Test-Path -LiteralPath $levelXex) -or (Get-FileHash -LiteralPath $levelXex -Algorithm SHA256).Hash -ne $levelHash) {
            throw 'saint_lo.dll differs from reviewed SHA-256; reanalyze the level.'
        }
        $islandXex = Join-Path $ProjectRoot 'assets/game/sp/island/island.dll'
        $islandHash = '679A5B58552ABCE1F8EDD80499A56F1CA4480E285734DC2719528400D8CD4A35'
        if (!(Test-Path -LiteralPath $islandXex) -or (Get-FileHash -LiteralPath $islandXex -Algorithm SHA256).Hash -ne $islandHash) {
            throw 'island.dll differs from reviewed SHA-256; reanalyze the level.'
        }
    }
    switch ($Stage) {
        'Tests' {
            Invoke-Logged $Python @('-m','unittest','discover','-s','tests','-v') 'tests.log'
            $nativeBuild = 'cod3pc/out/build/win-amd64-' + $Configuration.ToLowerInvariant()
            if (Test-Path "$nativeBuild/CMakeCache.txt") {
                Invoke-Logged $CMake @('--build',$nativeBuild,'--target','cod3_jump_test','cod3_fiber_test','cod3_handoff_test','cod3_thread_exit_test','--parallel','4') 'jump-test-build.log'
                Invoke-Logged (Join-Path (Split-Path $CMake) 'ctest.exe') @('--test-dir',$nativeBuild,'--output-on-failure','-R','guest_nonlocal_jump|script_fiber_continuations|native_title_handoff|native_thread_exit_probe') 'jump-tests.log'
            }
        }
        'Analyze' { Invoke-Logged $Python @('tools/inspect_game.py','assets/game') 'analysis.log' }
        'SDK' {
            Invoke-Logged $Python @('tools/materialize_mspack.py') 'sdk-symlinks.log'
            Push-Location external/rexglue-sdk
            try {
                Invoke-Logged $CMake @('--preset','win-amd64') 'sdk-configure.log'
                Invoke-Logged $CMake @('--build','out/build/win-amd64','--config','Release','--target','install','--parallel','4') 'sdk-build.log'
            } finally { Pop-Location }
        }
        'Codegen' {
            Push-Location cod3pc
            try { Invoke-Logged $Rexglue @('--log-file','../logs/codegen.log','codegen','cod3pc_manifest.toml','--ignore-stamp') 'codegen-console.log' }
            finally { Pop-Location }
        }
        'Diagnostic' {
            Push-Location cod3pc
            try { Invoke-Logged $Rexglue @('--force','--log-file','../logs/codegen-diagnostic.log','codegen','cod3pc_manifest.toml') 'codegen-diagnostic-console.log' }
            finally { Pop-Location }
        }
        'Build' {
            # Never hide validation errors by forcing codegen during a build.
            Push-Location cod3pc
            try {
                Invoke-Logged $Rexglue @('codegen','cod3pc_manifest.toml','--ignore-stamp') 'build-validation.log'
                $preset = 'win-amd64-' + $Configuration.ToLowerInvariant()
                Invoke-Logged $CMake @('--preset',$preset,"-DCMAKE_PREFIX_PATH=$SdkPrefix", "-DCOD3_CODEGEN_EXECUTABLE=$Rexglue") 'app-configure.log'
                Invoke-Logged $CMake @('--build','--preset',$preset,'--parallel','4') 'app-build.log'
                if ($Configuration -eq 'RelWithDebInfo') {
                    & "$PSScriptRoot/Build-GpuGrass.ps1"
                }
            } finally { Pop-Location }
        }
    }
} finally { Pop-Location }
