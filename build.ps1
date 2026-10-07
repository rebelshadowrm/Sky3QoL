param([string]$Zig = 'zig')
$ErrorActionPreference = 'Stop'
$repoRoot = $PSScriptRoot
$outRoot = Join-Path $repoRoot 'dist\Sky3QoL'
$pluginRoot = Join-Path $outRoot 'data\plugins'
New-Item -ItemType Directory -Path $pluginRoot -Force | Out-Null
& $Zig cc -target x86-windows-gnu -shared -O2 (Join-Path $repoRoot 'src\Sky3QoL.c') -o (Join-Path $pluginRoot 'Sky3QoL.dll') -luser32
if ($LASTEXITCODE -ne 0) { throw 'Plugin compilation failed.' }
Copy-Item -LiteralPath (Join-Path $repoRoot 'config\Sky3QoL.ini') -Destination $pluginRoot
foreach ($name in @('README.md','LICENSE','CHANGELOG.md','Install.ps1')) {
    Copy-Item -LiteralPath (Join-Path $repoRoot $name) -Destination $outRoot
}
Copy-Item -LiteralPath (Join-Path $repoRoot 'docs') -Destination $outRoot -Recurse -Force
$releaseVersion = '0.1.0'
$zip = Join-Path $repoRoot "dist\Sky3QoL-$releaseVersion.zip"
Compress-Archive -Path $outRoot -DestinationPath $zip -Force
$hash = (Get-FileHash -LiteralPath $zip -Algorithm SHA256).Hash.ToLowerInvariant()
[IO.File]::WriteAllText((Join-Path $repoRoot 'dist\SHA256SUMS.txt'),"$hash  Sky3QoL-$releaseVersion.zip`n",[Text.Encoding]::ASCII)
Write-Output "Built $zip"
