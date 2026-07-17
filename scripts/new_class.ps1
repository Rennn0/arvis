<#
.SYNOPSIS
    Scaffold a new C++ class: header + source + CMake registration.

.DESCRIPTION
    Creates include/<Module>/<file>.hpp and src/<Module>/<file>.cpp pre-filled
    with #pragma once, the right namespace, a class skeleton, and the matching
    #include, then inserts "src/<Module>/<file>.cpp" into the add_executable(arvis
    ...) list in CMakeLists.txt so you never forget to register it.

    File base name is derived from the class name (PascalCase -> snake_case);
    the namespace is derived from the module (av_net -> avNet). Override either
    with -FileName / -Namespace when the defaults don't fit (e.g. -Namespace avR).

.EXAMPLE
    scripts/new_class.ps1 av_net Downloader
    # -> include/av_net/downloader.hpp, src/av_net/downloader.cpp, namespace avNet

.EXAMPLE
    scripts/new_class.ps1 av_root Root -Namespace avR
    # -> include/av_root/root.hpp, src/av_root/root.cpp, namespace avR
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory, Position = 0)]
    [string] $Module,                       # include subfolder, e.g. av_net

    [Parameter(Mandatory, Position = 1)]
    [string] $ClassName,                    # C++ class name, e.g. Downloader

    [string] $Namespace,                    # override derived namespace
    [string] $FileName,                     # override derived file base name
    [string] $Target = 'arvis',             # add_executable target to register into
    [switch] $Force                         # overwrite existing files
)

$ErrorActionPreference = 'Stop'

# Repo root = parent of the scripts/ directory this file lives in.
$RepoRoot = Split-Path -Parent $PSScriptRoot

function ConvertTo-Snake([string] $s) {
    # Insert '_' at lower/digit -> Upper boundaries, then lowercase.
    ([regex]::Replace($s, '(?<=[a-z0-9])(?=[A-Z])', '_')).ToLower()
}

function ConvertTo-Namespace([string] $m) {
    $parts = $m -split '_'
    $ns = $parts[0].ToLower()
    for ($i = 1; $i -lt $parts.Count; $i++) {
        $p = $parts[$i]
        if ($p.Length -gt 0) {
            $ns += $p.Substring(0, 1).ToUpper() + $p.Substring(1).ToLower()
        }
    }
    $ns
}

if (-not $FileName)  { $FileName  = ConvertTo-Snake $ClassName }
if (-not $Namespace) { $Namespace = ConvertTo-Namespace $Module }

$headerRel = "include/$Module/$FileName.hpp"
$sourceRel = "src/$Module/$FileName.cpp"
$headerAbs = Join-Path $RepoRoot $headerRel
$sourceAbs = Join-Path $RepoRoot $sourceRel
$cmakeAbs  = Join-Path $RepoRoot 'CMakeLists.txt'

# --- guard against clobbering ------------------------------------------------
foreach ($f in @($headerAbs, $sourceAbs)) {
    if ((Test-Path $f) -and -not $Force) {
        throw "Refusing to overwrite existing file: $f (use -Force to override)"
    }
}

# --- header ------------------------------------------------------------------
$header = @"
#pragma once

namespace $Namespace
{
    class $ClassName
    {
    public:
        $ClassName();
        ~$ClassName();

    private:
    };
} // namespace $Namespace
"@

# --- source ------------------------------------------------------------------
$source = @"
#include <$Module/$FileName.hpp>

namespace $Namespace
{
    $ClassName::$ClassName()
    {
    }

    $ClassName::~$ClassName()
    {
    }
} // namespace $Namespace
"@

New-Item -ItemType Directory -Force -Path (Split-Path $headerAbs) | Out-Null
New-Item -ItemType Directory -Force -Path (Split-Path $sourceAbs) | Out-Null
Set-Content -Path $headerAbs -Value $header -Encoding utf8
Set-Content -Path $sourceAbs -Value $source -Encoding utf8
Write-Host "created  $headerRel"
Write-Host "created  $sourceRel"

# --- register the .cpp in add_executable(<Target> ...) -----------------------
$lines = Get-Content $cmakeAbs
$entry = "    $sourceRel"

if ($lines -contains $entry) {
    Write-Host "CMake    already lists $sourceRel (skipped)"
    return
}

# Find the add_executable(<Target> line, then the next line that is just ')'.
$startIdx = -1
for ($i = 0; $i -lt $lines.Count; $i++) {
    if ($lines[$i] -match "add_executable\(\s*$([regex]::Escape($Target))\b") {
        $startIdx = $i
        break
    }
}
if ($startIdx -lt 0) {
    Write-Warning "Could not find add_executable($Target ...) in CMakeLists.txt; add '$sourceRel' manually."
    return
}

$closeIdx = -1
for ($i = $startIdx; $i -lt $lines.Count; $i++) {
    if ($lines[$i].Trim() -eq ')') { $closeIdx = $i; break }
}
if ($closeIdx -lt 0) {
    Write-Warning "Could not find end of add_executable($Target ...); add '$sourceRel' manually."
    return
}

$updated = @()
$updated += $lines[0..($closeIdx - 1)]
$updated += $entry
$updated += $lines[$closeIdx..($lines.Count - 1)]
Set-Content -Path $cmakeAbs -Value $updated -Encoding utf8
Write-Host "CMake    registered $sourceRel in add_executable($Target ...)"
Write-Host ""
Write-Host "Done. Reconfigure to pick it up:  cmake --preset windows"
