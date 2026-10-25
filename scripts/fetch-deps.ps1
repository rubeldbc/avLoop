# Fetches FFmpeg 7.1 (LGPL, shared, win64) into third_party/ffmpeg (spec 3.1, reading R-9).
# The pin: BtbN's dated build n7.1.5, checked by SHA-256. When BtbN has pruned it, the newest n7.1 LGPL shared
# asset of BtbN's releases is taken instead and recorded in third_party/ffmpeg/VERSION.txt.
#   scripts\fetch-deps.ps1 [-Force]
param([switch]$Force)
$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

$root = Split-Path -Parent $PSScriptRoot
$third = Join-Path $root 'third_party'
$dest = Join-Path $third 'ffmpeg'
$downloads = Join-Path $third '_downloads'
$pin = @{
    Name   = 'ffmpeg-n7.1.5-12-g1fdbca85aa-win64-lgpl-shared-7.1.zip'
    Url    = 'https://github.com/BtbN/FFmpeg-Builds/releases/download/autobuild-2026-07-31-14-10/ffmpeg-n7.1.5-12-g1fdbca85aa-win64-lgpl-shared-7.1.zip'
    Sha256 = '0f376f96fb38554ccefb1b2ae9c7c6a7b351f0e60a372b38262c320e8392c5d0'
}

if ((Test-Path (Join-Path $dest 'include\libavcodec\avcodec.h')) -and -not $Force) {
    Write-Output "FFmpeg present: $(Get-Content (Join-Path $dest 'VERSION.txt') -TotalCount 1)"
    exit 0
}
New-Item -ItemType Directory -Force $downloads | Out-Null

function Get-Newest71 {
    $headers = @{ 'User-Agent' = 'avloop-fetch-deps' }
    if ($env:GITHUB_TOKEN) { $headers['Authorization'] = "Bearer $env:GITHUB_TOKEN" }
    $releases = Invoke-RestMethod -Uri 'https://api.github.com/repos/BtbN/FFmpeg-Builds/releases?per_page=100' -Headers $headers
    foreach ($r in ($releases | Sort-Object published_at -Descending)) {
        $a = $r.assets | Where-Object { $_.name -match '^ffmpeg-n7\.1.*-win64-lgpl-shared-7\.1\.zip$' } | Select-Object -First 1
        if ($a) { return @{ Name = $a.name; Url = $a.browser_download_url; Sha256 = $null } }
    }
    throw 'No FFmpeg n7.1 win64 LGPL shared build found in BtbN releases'
}

$chosen = $pin
$zip = Join-Path $downloads $chosen.Name
if (-not (Test-Path $zip)) {
    try {
        Write-Output "Downloading $($chosen.Url)"
        Invoke-WebRequest -Uri $chosen.Url -OutFile $zip -UseBasicParsing
    } catch {
        Write-Output "The pin is gone ($($_.Exception.Message)); looking for the newest n7.1 LGPL shared build"
        $chosen = Get-Newest71
        $zip = Join-Path $downloads $chosen.Name
        Invoke-WebRequest -Uri $chosen.Url -OutFile $zip -UseBasicParsing
    }
}
$hash = (Get-FileHash -Algorithm SHA256 $zip).Hash.ToLowerInvariant()
if ($chosen.Sha256 -and $hash -ne $chosen.Sha256) {
    Remove-Item $zip -Force
    throw "SHA-256 mismatch for $($chosen.Name): $hash"
}

$staging = Join-Path $downloads 'extract'
if (Test-Path $staging) { Remove-Item -Recurse -Force $staging }
Expand-Archive -Path $zip -DestinationPath $staging
$inner = Get-ChildItem $staging -Directory | Select-Object -First 1
if (Test-Path $dest) { Remove-Item -Recurse -Force $dest }
Move-Item $inner.FullName $dest
Remove-Item -Recurse -Force $staging

$version = @(
    $chosen.Name,
    $chosen.Url,
    "sha256 $hash",
    "fetched $(Get-Date -Format 'yyyy-MM-dd')"
)
Set-Content -Path (Join-Path $dest 'VERSION.txt') -Value $version -Encoding ASCII
Write-Output "FFmpeg ready in third_party\ffmpeg: $($chosen.Name)"
