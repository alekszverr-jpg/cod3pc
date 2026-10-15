param(
    [Parameter(Mandatory=$true)][ValidateSet('Save','Restore')][string]$Action,
    [Parameter(Mandatory=$true)][ValidatePattern('^[a-z][a-z0-9_]*$')][string]$Name,
    [ValidatePattern('^[a-z][a-z0-9_]*$')][string]$DebugProfile
)
$ErrorActionPreference = 'Stop'
$taskRoot = Split-Path $PSScriptRoot -Parent
$taskSnapshots = Join-Path $taskRoot 'userdata/checkpoints'
$taskSnapshot = Join-Path $taskSnapshots $Name
$taskUser = Join-Path $taskRoot 'userdata'
if ($DebugProfile) { $taskUser = Join-Path $taskUser "debug-levels/$DebugProfile" }
$taskAccount = 'B13EBABEBABEBABE'
$taskRelativeSave = "$taskAccount/415607E1/00000001/COD3_SaveSPV1.0/COD3_SaveSPV1.0"
$taskRelativeHeader = "$taskAccount/415607E1/Headers/00000001/COD3_SaveSPV1.0.header"
# Never copy an autosave while the game may be updating it. No process is stopped.
if (@(Get-Process -Name cod3pc -ErrorAction SilentlyContinue).Count) {
    throw 'Close cod3pc normally before saving or restoring a checkpoint snapshot.'
}
if ($Action -eq 'Save') {
    if (Test-Path -LiteralPath $taskSnapshot) { throw "Snapshot '$Name' already exists; use a new name." }
    foreach ($taskRequired in @($taskRelativeSave,$taskRelativeHeader)) {
        if (!(Test-Path -LiteralPath (Join-Path $taskUser $taskRequired) -PathType Leaf)) {
            throw "No complete original checkpoint in $taskUser"
        }
    }
    $taskSource = Join-Path $taskUser "$taskAccount/415607E1"
    $taskFiles = @(Get-ChildItem -LiteralPath $taskSource -Recurse -File | ForEach-Object {
        [pscustomobject]@{
            path=[IO.Path]::GetRelativePath($taskUser,$_.FullName)
            bytes=$_.Length
            sha256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash
        }
    })
    New-Item -ItemType Directory -Path $taskSnapshot -Force | Out-Null
    foreach ($taskFile in $taskFiles) {
        $taskDestination = Join-Path $taskSnapshot $taskFile.path
        New-Item -ItemType Directory -Force (Split-Path $taskDestination -Parent) | Out-Null
        Copy-Item -LiteralPath (Join-Path $taskUser $taskFile.path) -Destination $taskDestination
        if ((Get-FileHash -LiteralPath $taskDestination).Hash -cne $taskFile.sha256) {
            throw "Snapshot verification failed: $($taskFile.path)"
        }
    }
    [pscustomobject]@{format=1;created=[DateTime]::UtcNow.ToString('o');source_profile=$DebugProfile;files=$taskFiles} |
        ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $taskSnapshot 'snapshot.json') -Encoding utf8
    Write-Output "Saved verified original checkpoint: $taskSnapshot"
} else {
    if (!$DebugProfile) { throw 'Restore requires DebugProfile; the main campaign is never overwritten.' }
    if (Test-Path -LiteralPath $taskUser) { throw "Debug profile already exists: $taskUser. Choose a fresh name." }
    $taskManifest = Get-Content -LiteralPath (Join-Path $taskSnapshot 'snapshot.json') -Raw | ConvertFrom-Json
    if ($taskManifest.format -ne 1 -or !$taskManifest.files.Count) { throw 'Invalid checkpoint snapshot manifest.' }
    # Verify every file and its containment before creating the destination.
    foreach ($taskFile in $taskManifest.files) {
        $taskSourceFile = [IO.Path]::GetFullPath((Join-Path $taskSnapshot $taskFile.path))
        $taskDestination = [IO.Path]::GetFullPath((Join-Path $taskUser $taskFile.path))
        if (!$taskSourceFile.StartsWith($taskSnapshot + [IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase) -or
            !$taskDestination.StartsWith($taskUser + [IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)) {
            throw 'Checkpoint manifest contains a path outside its profile.'
        }
        if ((Get-Item -LiteralPath $taskSourceFile).Length -ne $taskFile.bytes -or
            (Get-FileHash -LiteralPath $taskSourceFile).Hash -cne $taskFile.sha256) { throw 'Checkpoint hash mismatch.' }
    }
    foreach ($taskRequired in @($taskRelativeSave,$taskRelativeHeader)) {
        if ($taskRequired.Replace('/','\') -notin @($taskManifest.files.path | ForEach-Object { $_.Replace('/','\') })) {
            throw 'Snapshot does not contain the original save and header.'
        }
    }
    foreach ($taskFile in $taskManifest.files) {
        $taskDestination = Join-Path $taskUser $taskFile.path
        New-Item -ItemType Directory -Force (Split-Path $taskDestination -Parent) | Out-Null
        Copy-Item -LiteralPath (Join-Path $taskSnapshot $taskFile.path) -Destination $taskDestination
        if ((Get-FileHash -LiteralPath $taskDestination).Hash -cne $taskFile.sha256) { throw 'Restored checkpoint hash mismatch.' }
    }
    Write-Output "Restored isolated checkpoint profile: $taskUser"
    Write-Output "Run: ./tools/Run-Native.ps1 -Seconds 0 -Interactive -DebugProfile $DebugProfile -DofStrength 35"
}
