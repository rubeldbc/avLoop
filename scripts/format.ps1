# Formats the C++ sources with clang-format (.clang-format: LLVM, 120 columns). -Check changes nothing and fails on
# any difference (spec 14.1, the Form gate).
#   scripts\format.ps1 [-Check]
param([switch]$Check)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot

$tool = Get-Command clang-format.exe -ErrorAction SilentlyContinue
if ($tool) { $tool = $tool.Source } else {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    $vs = & $vswhere -latest -products * -property installationPath
    $tool = Join-Path $vs 'VC\Tools\Llvm\x64\bin\clang-format.exe'
    if (-not (Test-Path $tool)) { throw 'clang-format not found (Visual Studio: C++ Clang tools)' }
}

$files = Get-ChildItem -Path (Join-Path $root 'src'), (Join-Path $root 'tests'), (Join-Path $root 'tools') -Recurse -File -Include *.cpp, *.h |
    ForEach-Object { $_.FullName }
if (-not $files) { Write-Output 'No C++ files'; exit 0 }

if ($Check) {
    $bad = @()
    foreach ($f in $files) {
        & $tool --dry-run --Werror $f 2>$null | Out-Null
        if ($LASTEXITCODE) { $bad += $f.Substring($root.Length + 1) }
    }
    if ($bad.Count) {
        $bad | ForEach-Object { Write-Output "FAIL $_ is not formatted" }
        Write-Output "FAIL: $($bad.Count) of $($files.Count) files need clang-format"
        exit 1
    }
    Write-Output "PASS: $($files.Count) files formatted"
} else {
    & $tool -i $files
    Write-Output "Formatted $($files.Count) files"
}
