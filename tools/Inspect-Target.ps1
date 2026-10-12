param([Parameter(Mandatory=$true)][ValidatePattern('^0x[0-9A-Fa-f]{8}$')][string]$Address,
    [ValidateSet('default.xex','saint_lo.dll')][string]$Module = 'default.xex')
. "$PSScriptRoot/Enter-Toolchain.ps1"
$jdk = Get-ChildItem 'C:/Program Files/Eclipse Adoptium' -Directory | Where-Object Name -Like 'jdk-21*' | Select-Object -First 1
if (!$jdk) { throw 'Temurin JDK 21 missing' }
$env:JAVA_HOME = $jdk.FullName
$headless = Join-Path $ProjectRoot 'external/ghidra/ghidra_12.1.3_PUBLIC/support/analyzeHeadless.bat'
& $headless "$ProjectRoot/reports/ghidra" cod3 -process $Module -noanalysis `
    -scriptPath "$ProjectRoot/tools/ghidra" -postScript InspectCod3.java $Address `
    -log "$ProjectRoot/logs/ghidra.log" -scriptlog "$ProjectRoot/logs/ghidra-$Address.log" `
    2>&1 | Tee-Object "$ProjectRoot/logs/ghidra-$Address-console.log"
if ($LASTEXITCODE -ne 0) { throw 'Ghidra failed' }
