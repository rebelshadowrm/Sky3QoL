param([string]$Zig = 'zig')
$ErrorActionPreference = 'Stop'
$testRoot = Join-Path $PSScriptRoot 'build\tests'
New-Item -ItemType Directory -Path $testRoot -Force | Out-Null
$source = [IO.File]::ReadAllText((Join-Path $PSScriptRoot 'src\Sky3Cursor.h'))
$map = @{
    0x620000=0x20000000; 0x660000=0x20100000; 0x990000=0x20200000
    0x9b0000=0x20300000; 0x9d0000=0x20400000; 0x8d0000=0x20500000
}
$fixture = [regex]::Replace($source,'0x[0-9a-fA-F]+',{
    param($match)
    $number = [Convert]::ToUInt32($match.Value.Substring(2),16)
    foreach ($address in $map.Keys) {
        if ($number -ge $address -and $number -lt ($address+0x10000)) {
            return ('0x{0:x}' -f ($number-$address+$map[$address]))
        }
    }
    return $match.Value
})
$fixture = $fixture.Replace('0x55fd98','0x2100fd98').Replace('0x55fd85','0x2100fd85')
[IO.File]::WriteAllText((Join-Path $testRoot 'cursor-test-header.h'),$fixture,[Text.Encoding]::ASCII)
$exe = Join-Path $testRoot 'cursor-regression.exe'
& $Zig cc -target x86-windows-gnu -O0 "-I$testRoot" (Join-Path $PSScriptRoot 'tests\cursor-regression.c') -o $exe -luser32
if ($LASTEXITCODE -ne 0) { throw 'Regression fixture compilation failed.' }
& $exe
if ($LASTEXITCODE -ne 0) { throw 'Cursor regression failed.' }
