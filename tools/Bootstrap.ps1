# Downloads public tools only. Never uploads game data or invokes rexauto pipeline.
. "$PSScriptRoot/Enter-Toolchain.ps1"
function Run([string]$Exe, [string[]]$Arguments) {
    & $Exe @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Exe failed: $LASTEXITCODE" }
}
Push-Location $ProjectRoot
try {
    New-Item -ItemType Directory -Force external | Out-Null
    foreach ($item in @(
        @('rexglue-sdk','https://github.com/rexglue/rexglue-sdk.git','c94f5ebdcb3c9d1a460ca48e04f9758448f8d518'),
        @('rexauto','https://github.com/Player124413/rexauto.git','dac49b331f2632f0b050f469e35747721300b8b2')
    )) {
        $dest = "external/$($item[0])"
        if (!(Test-Path -LiteralPath "$dest/.git")) {
            Run $Git @('clone',$item[1],$dest)
            Run $Git @('-C',$dest,'checkout','--detach',$item[2])
        }
        $revision = & $Git -C $dest rev-parse HEAD
        if ($revision -ne $item[2]) { throw "$dest has a different revision; preserve changes and review it." }
    }
    Run $Git @('-C','external/rexglue-sdk','submodule','update','--init','--recursive')
    Run $Python @('-m','pip','install','--target','external/python-tools','cmake==4.4.4','ninja==1.13.2')
    $zip = 'external/rexglue-sdk-0.10.0-win-amd64.zip'
    if (!(Test-Path -LiteralPath $zip)) {
        Invoke-WebRequest 'https://github.com/rexglue/rexglue-sdk/releases/download/v0.10.0/rexglue-sdk-0.10.0-win-amd64.zip' -OutFile $zip
    }
    if ((Get-FileHash -LiteralPath $zip -Algorithm SHA256).Hash -ne 'AD4485DA853FBE932E9F5DBB643F9B5FE85DC73775EBBAD4DF6879FE6DF0C11B') { throw 'SDK archive hash mismatch' }
    if (!(Test-Path -LiteralPath $Rexglue)) { Expand-Archive -LiteralPath $zip -DestinationPath external/sdk-v0.10.0 }
} finally { Pop-Location }
