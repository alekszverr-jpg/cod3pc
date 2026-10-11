. "$PSScriptRoot/Enter-Toolchain.ps1"
$entries = @()
foreach ($tool in @(@('git',$Git),@('python',$Python),@('cmake',$CMake),@('ninja',$Ninja),@('rexglue',$Rexglue))) {
    $path = $tool[1]
    $available = Test-Path -LiteralPath $path
    $result = if ($available) { (& $path --version 2>&1 | Out-String).Trim() } else { 'missing' }
    $entries += [pscustomobject]@{tool=$tool[0];path=$path;available=$available;version=$result}
}
foreach ($name in @('clang','clang++','clang-cl','cl','java')) {
    $cmd = Get-Command $name -ErrorAction SilentlyContinue
    $result = if ($cmd -and $name -notin @('cl','java')) { (& $cmd.Source --version 2>&1 | Out-String).Trim() } else { 'not probed' }
    $entries += [pscustomobject]@{tool=$name;path=$cmd.Source;available=[bool]$cmd;version=$result}
}
New-Item -ItemType Directory -Force "$ProjectRoot/reports" | Out-Null
[pscustomobject]@{tools=$entries;visualStudio=$VsRoot;windowsSdk=$env:WindowsSdkDir;include=$env:INCLUDE} |
    ConvertTo-Json -Depth 5 | Set-Content "$ProjectRoot/reports/environment.json" -Encoding utf8
$entries | Format-Table tool,available,path -AutoSize
if (!$VsRoot -or !(Get-Command clang++ -ErrorAction SilentlyContinue) -or !$env:WindowsSdkDir) {
    Write-Error 'C++ toolchain incomplete; see reports/environment.json and docs/BUILDING.md'
}
