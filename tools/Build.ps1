param([ValidateSet('SDK','Codegen','Diagnostic','Build','Analyze','Tests')][string]$Stage = 'Build')
. "$PSScriptRoot/Enter-Toolchain.ps1"
New-Item -ItemType Directory -Force "$ProjectRoot/logs" | Out-Null
function Invoke-Logged([string]$Exe, [string[]]$Arguments, [string]$Log) {
    & $Exe @Arguments 2>&1 | Tee-Object -FilePath "$ProjectRoot/logs/$Log"
    if ($LASTEXITCODE -ne 0) { throw "$Exe failed ($LASTEXITCODE); see logs/$Log" }
}
Push-Location $ProjectRoot
try {
    if ($Stage -in @('Codegen','Diagnostic','Build')) {
        $xex = Join-Path $ProjectRoot 'assets/game/default.xex'
        $expected = '2944EEC7D1231AD6798B5F9F8ADF8855F5E489296B22EAB45B27A577CEE23692'
        if (!(Test-Path -LiteralPath $xex) -or (Get-FileHash -LiteralPath $xex -Algorithm SHA256).Hash -ne $expected) {
            throw 'default.xex differs from reviewed SHA-256; reanalyze before using function overrides.'
        }
    }
    switch ($Stage) {
        'Tests' { Invoke-Logged $Python @('-m','unittest','discover','-s','tests','-v') 'tests.log' }
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
                Invoke-Logged $CMake @('--preset','win-amd64-debug',"-DCMAKE_PREFIX_PATH=$SdkPrefix") 'app-configure.log'
                Invoke-Logged $CMake @('--build','--preset','win-amd64-debug','--parallel','4') 'app-build.log'
            } finally { Pop-Location }
        }
    }
} finally { Pop-Location }
