<#
.SYNOPSIS
    arvis installer for Windows — build from source and install (or run) the app.

.DESCRIPTION
    Clones the repo (shallow, with the vcpkg submodule), bootstraps vcpkg, and
    compiles a Release binary. libcurl comes from vcpkg; GLFW + Dear ImGui are
    fetched by CMake at configure time.

    Requires: Git, CMake >= 3.24, and Visual Studio 2022 with the
    "Desktop development with C++" workload.

.EXAMPLE
    Remote one-liner (defaults: install to $HOME\.local\bin):
        irm https://raw.githubusercontent.com/Rennn0/arvis/main/scripts/install.ps1 | iex

.EXAMPLE
    With options — download, then run:
        $s = irm https://raw.githubusercontent.com/Rennn0/arvis/main/scripts/install.ps1
        & ([scriptblock]::Create($s)) -Run

.EXAMPLE
    Local checkout:
        scripts\install.ps1 -Prefix C:\tools\bin
#>
[CmdletBinding()]
param(
    # Build and run without installing.
    [switch] $Run,
    # Install the binary to this directory (default: $HOME\.local\bin).
    [string] $Prefix = (Join-Path $HOME '.local\bin'),
    # Git branch or tag to build.
    [string] $Ref = 'main',
    # Git URL to clone.
    [string] $Repo = 'https://github.com/Rennn0/arvis.git',
    # Do not delete the temporary build directory on exit.
    [switch] $KeepBuild
)

$ErrorActionPreference = 'Stop'
$App = 'arvis'
$Src = $null

function Info($m) { Write-Host "==> $m" -ForegroundColor Cyan }
function Ok($m)   { Write-Host "==> $m" -ForegroundColor Green }
function Warn($m) { Write-Host "warning: $m" -ForegroundColor Yellow }
function Die($m)  { Write-Host "error: $m" -ForegroundColor Red; exit 1 }
function Have($cmd) { [bool](Get-Command $cmd -ErrorAction SilentlyContinue) }

# --- required tools ---------------------------------------------------------
function Check-Tools {
    $missing = @()
    foreach ($t in 'git', 'cmake') { if (-not (Have $t)) { $missing += $t } }
    if ($missing.Count) {
        Warn "missing required tools: $($missing -join ', ')"
        Write-Host "  Install Git and CMake >= 3.24, plus Visual Studio 2022"
        Write-Host "  with the 'Desktop development with C++' workload."
        Die 'prerequisites not satisfied.'
    }
}

# --- build steps ------------------------------------------------------------
function Fetch-Sources {
    $script:Src = Join-Path ([System.IO.Path]::GetTempPath()) "arvis-install-$([System.IO.Path]::GetRandomFileName())"
    Info "cloning $Repo@$Ref into $Src"
    git clone --depth 1 --branch $Ref --recurse-submodules --shallow-submodules $Repo $Src
    if ($LASTEXITCODE -ne 0) { Die "git clone failed (is '$Ref' a valid branch/tag on $Repo?)" }
}

function Bootstrap-Vcpkg {
    $vcpkgExe = Join-Path $Src 'external\vcpkg\vcpkg.exe'
    if (-not (Test-Path $vcpkgExe)) {
        Info 'bootstrapping vcpkg ...'
        & (Join-Path $Src 'external\vcpkg\bootstrap-vcpkg.bat') -disableMetrics
        if ($LASTEXITCODE -ne 0) { Die 'vcpkg bootstrap failed.' }
    }
}

function Build {
    Push-Location $Src
    try {
        Info 'configuring (windows) ...'
        cmake --preset windows
        if ($LASTEXITCODE -ne 0) { Die 'cmake configure failed.' }
        Info 'building — first build compiles curl/GLFW/ImGui, this takes a while ...'
        cmake --build --preset windows-release
        if ($LASTEXITCODE -ne 0) { Die 'build failed.' }
    }
    finally {
        Pop-Location
    }
    $script:Bin = Join-Path $Src "build\windows\Release\$App.exe"
    if (-not (Test-Path $script:Bin)) { Die "build finished but binary not found at $($script:Bin)" }
}

function Do-Install {
    New-Item -ItemType Directory -Force -Path $Prefix | Out-Null
    Copy-Item -Force $script:Bin (Join-Path $Prefix "$App.exe")
    Ok "installed $App -> $(Join-Path $Prefix "$App.exe")"

    $onPath = ($env:PATH -split ';') -contains $Prefix
    if (-not $onPath) {
        Warn "$Prefix is not on your PATH. Add it with:"
        Write-Host "    [Environment]::SetEnvironmentVariable('PATH', `"$Prefix;`$env:PATH`", 'User')"
    }
    Info "run it with: $App"
}

function Do-Run {
    Ok "launching $App from build tree"
    $script:KeepBuildResolved = $true
    & $script:Bin
}

# --- main -------------------------------------------------------------------
try {
    Info "arvis installer (windows)"
    Check-Tools
    Fetch-Sources
    Bootstrap-Vcpkg
    Build
    if ($Run) { Do-Run } else { Do-Install }
}
finally {
    if (-not $KeepBuild -and -not $script:KeepBuildResolved -and $Src -and (Test-Path $Src)) {
        Remove-Item -Recurse -Force $Src -ErrorAction SilentlyContinue
    }
}
