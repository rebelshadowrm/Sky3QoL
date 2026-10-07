param([Parameter(Mandatory=$true)][string]$GameDirectory)
$ErrorActionPreference = 'Stop'
$target = [IO.Path]::GetFullPath($GameDirectory).TrimEnd('\')
$exe = Join-Path $target 'ed6_win3_DX9.exe'
if (-not (Test-Path -LiteralPath $exe -PathType Leaf)) { throw 'Select the game folder containing ed6_win3_DX9.exe.' }
$expected = '6939b938b71b280ad9115e30ace16f97923a5c232414ce71f75f7f326ab9c75a'
if ((Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash.ToLowerInvariant() -ne $expected) {
    throw 'This executable has not been tested. No files were installed; see docs/COMPATIBILITY.md.'
}
foreach ($game in @(Get-Process -Name ed6_win3,ed6_win3_DX9 -ErrorAction SilentlyContinue)) {
    if ($game.Path -and [IO.Path]::GetDirectoryName($game.Path).TrimEnd('\') -eq $target) {
        throw 'Save and close this game before installing.'
    }
}
$payload = Join-Path $PSScriptRoot 'data\plugins'
if (-not (Test-Path -LiteralPath (Join-Path $payload 'Sky3QoL.dll'))) {
    $payload = Join-Path $PSScriptRoot 'dist\Sky3QoL\data\plugins'
}
foreach ($name in @('Sky3QoL.dll','Sky3QoL.ini')) {
    if (-not (Test-Path -LiteralPath (Join-Path $payload $name) -PathType Leaf)) { throw 'Download a release ZIP or run build.ps1 first.' }
}
$pluginTarget = Join-Path $target 'data\plugins'
$backup = Join-Path $target ('Sky3QoL-Backups\'+(Get-Date -Format 'yyyyMMdd-HHmmss-ffff'))
foreach ($name in @('Sky3QoL.dll','Sky3QoL.ini')) {
    $existing = Join-Path $pluginTarget $name
    if (Test-Path -LiteralPath $existing -PathType Leaf) {
        New-Item -ItemType Directory -Path $backup -Force | Out-Null
        Copy-Item -LiteralPath $existing -Destination (Join-Path $backup $name)
    }
}
New-Item -ItemType Directory -Path $pluginTarget -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $payload 'Sky3QoL.dll') -Destination $pluginTarget -Force
$config = Join-Path $pluginTarget 'Sky3QoL.ini'
if (-not (Test-Path -LiteralPath $config)) { Copy-Item -LiteralPath (Join-Path $payload 'Sky3QoL.ini') -Destination $config }
Write-Output 'Sky3QoL installed. Existing plugin settings were preserved. Start ed6_win3_DX9.exe normally.'
if (-not (Test-Path -LiteralPath (Join-Path $target 'd3dxof.dll'))) { Write-Output 'Install LB-ARK from its own project before launching; this release does not bundle the loader.' }
