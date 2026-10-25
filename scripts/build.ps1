# Builds AvLoop (spec 3.1): the MSVC x64 environment, Ninja, Qt 6.8, FFmpeg 7.1; outputs in build\bin with Qt's
# DLLs deployed by windeployqt beside them.
#   scripts\build.ps1 [-Fresh] [-Config Release|Debug|RelWithDebInfo] [-Sanitize] [-BuildDir <dir>]
param(
    [switch]$Fresh,
    [ValidateSet('Release', 'Debug', 'RelWithDebInfo')][string]$Config = 'Release',
    [switch]$Sanitize,
    [string]$BuildDir
)
$ErrorActionPreference = 'Stop'
# Native tools write notes to stderr; PowerShell 5.1 must not take those for errors. Exit codes decide.
function Invoke-Native([string]$exe, [string[]]$arguments) {
    $saved = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    & $exe @arguments 2>&1 | ForEach-Object { "$_" }
    $code = $LASTEXITCODE
    $ErrorActionPreference = $saved
    if ($code) { Write-Output "FAILED ($code): $exe"; exit $code }
}
$root = Split-Path -Parent $PSScriptRoot
if (-not $BuildDir) { $BuildDir = Join-Path $root $(if ($Sanitize) { 'build-asan' } elseif ($Config -eq 'Debug') { 'build-debug' } else { 'build' }) }

function Import-MsvcEnvironment {
    if (Get-Command cl.exe -ErrorAction SilentlyContinue) { return }
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    $vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (-not $vs) { throw 'Visual Studio 2022 with the C++ x64 tools was not found' }
    $vcvars = Join-Path $vs 'VC\Auxiliary\Build\vcvars64.bat'
    $lines = cmd /c "`"$vcvars`" >nul 2>&1 && set"
    foreach ($line in $lines) { if ($line -match '^([^=]+)=(.*)$') { Set-Item -Path "env:$($matches[1])" -Value $matches[2] } }
    if (-not (Get-Command ninja.exe -ErrorAction SilentlyContinue)) {
        $ninja = Join-Path $vs 'Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja'
        if (Test-Path $ninja) { $env:PATH = "$ninja;$env:PATH" }
    }
}

function Find-Qt {
    foreach ($candidate in @($env:QTDIR, $env:QT_ROOT_DIR, $env:Qt6_DIR)) {
        if (-not $candidate) { continue }
        foreach ($prefix in @($candidate, (Join-Path $candidate '..\..\..'))) {
            if (Test-Path (Join-Path $prefix 'bin\windeployqt.exe')) { return (Resolve-Path $prefix).Path }
        }
    }
    foreach ($drive in (Get-PSDrive -PSProvider FileSystem).Root) {
        $found = Get-ChildItem -Path (Join-Path $drive 'Qt') -Directory -Filter '6.8.*' -ErrorAction SilentlyContinue |
            Sort-Object Name -Descending | ForEach-Object { Join-Path $_.FullName 'msvc2022_64' } |
            Where-Object { Test-Path (Join-Path $_ 'bin\windeployqt.exe') } | Select-Object -First 1
        if ($found) { return $found }
    }
    throw 'Qt 6.8 (msvc2022_64) not found: set QTDIR (see CONTRIBUTING.md)'
}

Import-MsvcEnvironment
$qt = Find-Qt
if (-not (Test-Path (Join-Path $root 'third_party\ffmpeg\include'))) { & (Join-Path $PSScriptRoot 'fetch-deps.ps1') }

if ($Fresh -and (Test-Path $BuildDir)) { Remove-Item -Recurse -Force $BuildDir }
$asan = if ($Sanitize) { 'ON' } else { 'OFF' }
Invoke-Native 'cmake' @('-S', $root, '-B', $BuildDir, '-G', 'Ninja', "-DCMAKE_BUILD_TYPE=$Config", "-DCMAKE_PREFIX_PATH=$qt", "-DAVLOOP_ASAN=$asan")
Invoke-Native 'cmake' @('--build', $BuildDir)

$bin = Join-Path $BuildDir 'bin'
$exe = Join-Path $bin 'avloop.exe'
# Every build: a new Qt module (OpenGL at AV-003) needs its DLL beside the exe; windeployqt copies only what changed.
if (Test-Path $exe) {
    Invoke-Native (Join-Path $qt 'bin\windeployqt.exe') @('--no-translations', '--no-system-d3d-compiler', '--no-opengl-sw', '--no-compiler-runtime', $exe) | Out-Null
}
if ($Sanitize) {
    $asanDll = Get-Command clang_rt.asan_dynamic-x86_64.dll -ErrorAction SilentlyContinue
    if ($asanDll) { Copy-Item $asanDll.Source $bin -Force }
}
Write-Output "Built $Config in $BuildDir"
