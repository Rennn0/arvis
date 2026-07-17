<#
.SYNOPSIS
    Run clang-format over the project's own C++ sources (include/ + src/).

.DESCRIPTION
    Formats every .hpp/.cpp under include/ and src/ in place using the repo's
    .clang-format. external/ and build/ are never touched (they aren't scanned,
    and external/.clang-format disables formatting there as a second guard).

    Editors that honour .clang-format (VS Code + ms-vscode.cpptools, Visual
    Studio 2022, clangd, CLion/Rider) format on save automatically; this script
    is for a full-tree sweep or a CI gate.

    clang-format is located on PATH first, otherwise from the Visual Studio 2022
    LLVM tools via vswhere.

.PARAMETER Check
    Don't rewrite files — fail (exit 1) if any file is not already formatted.
    Intended for CI. Prints the offending files.

.EXAMPLE
    scripts/format.ps1
    # format all project sources in place

.EXAMPLE
    scripts/format.ps1 -Check
    # CI gate: non-zero exit if anything is unformatted
#>
[CmdletBinding()]
param(
    [switch] $Check
)

$ErrorActionPreference = 'Stop'

# Repo root is the parent of this scripts/ folder.
$root = Split-Path -Parent $PSScriptRoot

function Find-ClangFormat {
    $onPath = Get-Command clang-format -ErrorAction SilentlyContinue
    if ($onPath) { return $onPath.Source }

    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (Test-Path $vswhere) {
        $vs = & $vswhere -latest -products * -property installationPath 2>$null
        foreach ($sub in 'VC\Tools\Llvm\x64\bin', 'VC\Tools\Llvm\bin') {
            $candidate = Join-Path $vs "$sub\clang-format.exe"
            if ($vs -and (Test-Path $candidate)) { return $candidate }
        }
    }
    throw "clang-format not found. Install LLVM, or the 'C++ Clang tools for Windows' VS component."
}

$clangFormat = Find-ClangFormat
Write-Host "using $clangFormat"

$files = Get-ChildItem -Path (Join-Path $root 'include'), (Join-Path $root 'src') `
    -Recurse -File -Include *.hpp, *.cpp, *.h, *.cc

if (-not $files) { Write-Host 'no sources found'; exit 0 }

if ($Check) {
    $bad = @()
    foreach ($f in $files) {
        # --dry-run --Werror: exits non-zero when the file would change.
        & $clangFormat --dry-run --Werror $f.FullName 2>$null
        if ($LASTEXITCODE -ne 0) { $bad += $f.FullName }
    }
    if ($bad) {
        Write-Host "not formatted:`n  $($bad -join "`n  ")"
        exit 1
    }
    Write-Host "$($files.Count) file(s) already formatted"
    exit 0
}

& $clangFormat -i $files.FullName
Write-Host "formatted $($files.Count) file(s)"
