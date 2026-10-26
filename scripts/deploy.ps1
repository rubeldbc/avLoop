# Makes deploy\ — what the installer installs (spec 3.1, 3.8): avloop.exe and avloop-cli.exe from a Release build,
# Qt's DLLs and plugins by windeployqt, the FFmpeg DLLs they link, the VC++ runtime beside them (app-local), and the
# licences in deploy\licenses. Then: ISCC installer\AvLoop.iss.
#   scripts\deploy.ps1 [-NoBuild] [-BuildDir <dir>]
param(
    [switch]$NoBuild,
    [string]$BuildDir
)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
if (-not $BuildDir) { $BuildDir = Join-Path $root 'build' }
# Native tools write notes to stderr; PowerShell 5.1 must not take those for errors. Exit codes decide.
function Invoke-Native([string]$exe, [string[]]$arguments) {
    $saved = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    & $exe @arguments 2>&1 | ForEach-Object { "$_" }
    $code = $LASTEXITCODE
    $ErrorActionPreference = $saved
    if ($code) { Write-Output "FAILED ($code): $exe"; exit $code }
}

if (-not $NoBuild) {
    & (Join-Path $PSScriptRoot 'build.ps1') -Config Release -BuildDir $BuildDir
    if ($LASTEXITCODE) { exit $LASTEXITCODE }
}
$bin = Join-Path $BuildDir 'bin'
foreach ($name in @('avloop.exe', 'avloop-cli.exe')) {
    if (-not (Test-Path (Join-Path $bin $name))) { throw "$name is not built in ${bin}: run scripts\build.ps1" }
}

# Qt: the windeployqt beside the Qt the build used (CMakeCache's Qt6_DIR).
$cache = Get-Content (Join-Path $BuildDir 'CMakeCache.txt')
$qtDir = ($cache | Where-Object { $_ -match '^Qt6_DIR:PATH=(.*)$' } | ForEach-Object { $matches[1] }) | Select-Object -First 1
$windeployqt = Join-Path (Resolve-Path (Join-Path $qtDir '..\..\..')).Path 'bin\windeployqt.exe'
if (-not (Test-Path $windeployqt)) { throw "windeployqt not found beside $qtDir" }

# The VC++ runtime: Visual Studio's redistributable folder (Microsoft.VC143.CRT), app-local.
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$crt = Get-ChildItem -Path (Join-Path $vs 'VC\Redist\MSVC') -Directory -ErrorAction SilentlyContinue |
    Sort-Object Name -Descending | ForEach-Object { Join-Path $_.FullName 'x64\Microsoft.VC143.CRT' } |
    Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $crt) { throw 'The VC++ runtime (VC\Redist\MSVC\*\x64\Microsoft.VC143.CRT) was not found' }

$deploy = Join-Path $root 'deploy'
if (Test-Path $deploy) { Remove-Item -Recurse -Force $deploy }
New-Item -ItemType Directory -Force (Join-Path $deploy 'licenses') | Out-Null
foreach ($name in @('avloop.exe', 'avloop-cli.exe')) { Copy-Item (Join-Path $bin $name) $deploy }
# No network: TUIO touch (generic) would bring Qt6Network and its TLS and network-information plugins.
Invoke-Native $windeployqt @('--release', '--no-translations', '--no-system-d3d-compiler', '--no-system-dxc-compiler',
    '--no-opengl-sw', '--no-compiler-runtime', '--no-network', '--skip-plugin-types', 'generic,tls,networkinformation',
    '--dir', $deploy, (Join-Path $deploy 'avloop.exe'), (Join-Path $deploy 'avloop-cli.exe')) |
    Out-Null
# FFmpeg: the DLLs the build copied beside its programs (the libraries they link and what those load).
Get-ChildItem -Path $bin -Filter '*.dll' | Where-Object { $_.Name -match '^(avcodec|avformat|avutil|avfilter|swscale|swresample)-\d+\.dll$' } |
    ForEach-Object { Copy-Item $_.FullName $deploy }
Get-ChildItem -Path $crt -Filter '*.dll' | ForEach-Object { Copy-Item $_.FullName $deploy }

$licenses = Join-Path $deploy 'licenses'
Copy-Item (Join-Path $root 'LICENSE') (Join-Path $licenses 'LICENSE.txt')
Copy-Item (Join-Path $root 'LICENSE-THIRD-PARTY.md') (Join-Path $licenses 'LICENSE-THIRD-PARTY.md')
Copy-Item (Join-Path $root 'licenses\LGPL-3.0.txt') $licenses
Copy-Item (Join-Path $root 'licenses\GPL-3.0.txt') $licenses

$files = Get-ChildItem -Path $deploy -Recurse -File
$mb = [math]::Round(($files | Measure-Object -Property Length -Sum).Sum / 1MB, 1)
$version = (Get-Item (Join-Path $deploy 'avloop.exe')).VersionInfo.ProductVersion
Write-Output "Deployed AvLoop $version in $deploy`: $($files.Count) files, $mb MB"
