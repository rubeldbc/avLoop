# Runs AvLoop's unit tests (spec 14.1): ctest with output on failure and the five-minute ceiling.
#   scripts\test.ps1 [-Filter <regex>] [-BuildDir <dir>]
param([string]$Filter = 'avloop', [string]$BuildDir)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
if (-not $BuildDir) { $BuildDir = Join-Path $root 'build' }
ctest --test-dir $BuildDir --output-on-failure --timeout 300 -R $Filter
exit $LASTEXITCODE
