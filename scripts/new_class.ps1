<#
.SYNOPSIS
    Scaffold a new C++ class: header + source + CMake registration.

.DESCRIPTION
    Creates include/<Module>/<file>.hpp and src/<Module>/<file>.cpp pre-filled
    with #pragma once, the right namespace, a class skeleton, and the matching
    #include, then registers both in CMakeLists.txt: the .cpp goes into the
    add_executable(arvis ...) list (so it compiles) and the .hpp into the
    target_sources(arvis PRIVATE ...) list (so it shows up in the generated
    Visual Studio / IDE project) — so you never forget to register either.

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

# --- register in CMakeLists.txt ----------------------------------------------
# The .cpp goes into add_executable(<Target> ...) so it gets compiled; the .hpp
# goes into target_sources(<Target> PRIVATE ...) so it shows up (and is
# editable) in the generated Visual Studio / IDE project. Both lists are edited
# the same way: find the opening "<block>(<Target>" line, then insert just
# before the line that closes it (a lone ')').

# Insert "$Entry" into the "<BlockName>(<Target> ...)" list. Idempotent; warns
# and returns the input array unchanged if the block can't be located.
function Add-ToCMakeBlock {
    param(
        [string[]] $Lines,
        [string]   $BlockName,
        [string]   $Target,
        [string]   $Entry,
        [string]   $Rel
    )

    if ($Lines -contains $Entry) {
        Write-Host "CMake    already lists $Rel (skipped)"
        return $Lines
    }

    $startIdx = -1
    for ($i = 0; $i -lt $Lines.Count; $i++) {
        if ($Lines[$i] -match "$([regex]::Escape($BlockName))\(\s*$([regex]::Escape($Target))\b") {
            $startIdx = $i
            break
        }
    }
    if ($startIdx -lt 0) {
        Write-Warning "Could not find $BlockName($Target ...) in CMakeLists.txt; add '$Rel' manually."
        return $Lines
    }

    $closeIdx = -1
    for ($i = $startIdx; $i -lt $Lines.Count; $i++) {
        if ($Lines[$i].Trim() -eq ')') { $closeIdx = $i; break }
    }
    if ($closeIdx -lt 0) {
        Write-Warning "Could not find end of $BlockName($Target ...); add '$Rel' manually."
        return $Lines
    }

    $updated = @()
    $updated += $Lines[0..($closeIdx - 1)]
    $updated += $Entry
    $updated += $Lines[$closeIdx..($Lines.Count - 1)]
    Write-Host "CMake    registered $Rel in $BlockName($Target ...)"
    return $updated
}

$lines = Get-Content $cmakeAbs
$lines = Add-ToCMakeBlock -Lines $lines -BlockName 'add_executable' -Target $Target -Entry "    $sourceRel" -Rel $sourceRel
$lines = Add-ToCMakeBlock -Lines $lines -BlockName 'target_sources'  -Target $Target -Entry "    $headerRel" -Rel $headerRel
Set-Content -Path $cmakeAbs -Value $lines -Encoding utf8

Write-Host ""
Write-Host "Done. Reconfigure to pick it up:  cmake --preset windows"
